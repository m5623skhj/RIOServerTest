// 참고 : https://gist.github.com/ujentus/5997058#file-rioserver_sm9-cpp

#include <iostream>
#include <WinSock2.h>
#include <MSWSock.h>
#include <cmath>
#include <process.h>

#pragma comment(lib, "ws2_32.lib")

using namespace std;

#define PORT 10001
#define STOP_THREAD 0

enum OPERATION_TYPE
{
	OP_NONE = 0
	, OP_RECV = 1
	, OP_SEND = 2
};

struct EXTENDED_RIO_BUF : public RIO_BUF
{
	OPERATION_TYPE operationType;
};

const DWORD RIO_PENDING_RECVS = 100000;
const DWORD RIO_PENDING_SENDS = 10000;
const DWORD RECV_BUFFER_SIZE = 1024;
const DWORD SEND_BUFFER_SIZE = 1024;
const DWORD ADDR_BUFFER_SIZE = 64;

HANDLE g_IOCPHandle = NULL;

RIO_CQ g_RIOCQ = 0;
RIO_RQ g_RIORQ = 0;
RIO_EXTENSION_FUNCTION_TABLE g_RIOFunctionTable;

char* g_sendBufferPointer = nullptr;
char* g_recvBufferPointer = nullptr;
char* g_addrBufferPointer = nullptr;

RIO_BUFFERID g_sendBufferId;
RIO_BUFFERID g_recvBufferId;
RIO_BUFFERID g_addrBufferId;

EXTENDED_RIO_BUF* g_sendRIOBufs = nullptr;
DWORD g_sendRIOBufTotalCount = 0;
INT64 g_sendRIOBufIndex = 0;

EXTENDED_RIO_BUF* g_addrRIOBufs = nullptr;
DWORD g_addrRIOBufTotalCount = 0;
INT64 g_addrRIOBufIndex = 0;

CRITICAL_SECTION g_lock;

static const DWORD NUM_IOCP_THREADS = 4;

UINT WorkerThread(LPVOID arg);

int main()
{
	WSAData wsaData;

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		cout << "WSAStartup() failed " << GetLastError() << endl;
		return 0;
	}

	auto sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET)
	{
		cout << "socket() failed " << GetLastError() << endl;
		return 0;
	}

	SOCKADDR_IN addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(PORT);
	addr.sin_addr.s_addr = INADDR_ANY;
	if (bind(sock, reinterpret_cast<SOCKADDR*>(&addr), sizeof(addr)) == SOCKET_ERROR)
	{
		cout << "bind() falied " << GetLastError() << endl;
		return 0;
	}

	if (listen(sock, SOMAXCONN) == SOCKET_ERROR)
	{
		cout << "listen() failed " << GetLastError() << endl;
		return 0;
	}

	InitializeCriticalSectionAndSpinCount(&g_lock, 4096);

	GUID functionTableId = WSAID_MULTIPLE_RIO;
	DWORD bytes = 0;
	if (WSAIoctl(sock, SIO_GET_MULTIPLE_EXTENSION_FUNCTION_POINTER, &functionTableId, sizeof(GUID)
		, reinterpret_cast<void**>(&g_RIOFunctionTable), sizeof(g_RIOFunctionTable), &bytes, NULL, NULL) == NULL)
	{
		cout << "WSAloctl() failed " << GetLastError() << endl;
		return 0;
	}

	g_IOCPHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (g_IOCPHandle == NULL)
	{
		cout << "CreateIoCompletionPort() failed " << GetLastError() << endl;
		return 0;
	}

	OVERLAPPED overlapped;
	RIO_NOTIFICATION_COMPLETION rioNotificationCompletion;
	rioNotificationCompletion.Type = RIO_IOCP_COMPLETION;
	rioNotificationCompletion.Iocp.IocpHandle = g_IOCPHandle;
	rioNotificationCompletion.Iocp.CompletionKey = (void*)1;
	rioNotificationCompletion.Iocp.Overlapped = &overlapped;

	// Create RIO CQ
	g_RIOCQ = g_RIOFunctionTable.RIOCreateCompletionQueue(RIO_PENDING_RECVS + RIO_PENDING_SENDS, &rioNotificationCompletion);
	if (g_RIOCQ == RIO_INVALID_CQ)
	{
		cout << "RIOCreateCompletionQueue() failed " << GetLastError() << endl;
		return 0;
	}

	SYSTEM_INFO systemInfo;
	GetSystemInfo(&systemInfo);
	auto granularity = systemInfo.dwAllocationGranularity;

	// register send rio buffer
	{
		DWORD totalBufferCount = SEND_BUFFER_SIZE;
		DWORD totalBufferSize = SEND_BUFFER_SIZE * RIO_PENDING_SENDS / granularity;

		g_sendBufferPointer = new char[totalBufferSize];
		ZeroMemory(g_sendBufferPointer, totalBufferSize);
		g_sendBufferId = g_RIOFunctionTable.RIORegisterBuffer(g_sendBufferPointer, totalBufferSize);

		if (g_sendBufferId == RIO_INVALID_BUFFERID)
		{
			cout << "RIORegisterBuffer() failed " << GetLastError() << endl;
			return 0;
		}

		DWORD offset = 0;
		g_sendRIOBufs = new EXTENDED_RIO_BUF[totalBufferCount];
		g_sendRIOBufTotalCount = totalBufferCount;

		for (DWORD i = 0; i < g_sendRIOBufTotalCount; ++i)
		{
			EXTENDED_RIO_BUF* buffer = g_sendRIOBufs + i;

			buffer->BufferId = g_sendBufferId;
			buffer->Offset = offset;
			buffer->Length = SEND_BUFFER_SIZE;

			offset += SEND_BUFFER_SIZE;
		}
	}

	// register addr RIO buffer
	{
		DWORD totalBufferCount = ADDR_BUFFER_SIZE;
		DWORD totalBufferSize = ADDR_BUFFER_SIZE * RIO_PENDING_RECVS / granularity;

		g_addrBufferPointer = new char[totalBufferSize];
		ZeroMemory(g_addrBufferPointer, totalBufferSize);
		g_addrBufferId = g_RIOFunctionTable.RIORegisterBuffer(g_addrBufferPointer, totalBufferSize);

		if (g_addrBufferId == RIO_INVALID_BUFFERID)
		{
			cout << "RIORegisterBuffer() failed " << GetLastError() << endl;
			return 0;
		}

		DWORD offset = 0;
		g_addrRIOBufs = new EXTENDED_RIO_BUF[totalBufferCount];
		g_addrRIOBufTotalCount = totalBufferCount;

		for (DWORD i = 0; i < g_addrRIOBufTotalCount; ++i)
		{
			EXTENDED_RIO_BUF* buffer = g_addrRIOBufs + i;

			buffer->BufferId = g_addrBufferId;
			buffer->Offset = offset;
			buffer->Length = ADDR_BUFFER_SIZE;

			offset += ADDR_BUFFER_SIZE;
		}
	}

	// register recv RIO buffer
	{
		DWORD totalBufferCount = RECV_BUFFER_SIZE;
		DWORD totalBufferSize = RECV_BUFFER_SIZE * RIO_PENDING_RECVS / granularity;

		g_recvBufferPointer = new char[totalBufferSize];
		ZeroMemory(g_recvBufferPointer, totalBufferSize);
		g_recvBufferId = g_RIOFunctionTable.RIORegisterBuffer(g_recvBufferPointer, totalBufferSize);

		if (g_recvBufferId == RIO_INVALID_BUFFERID)
		{
			cout << "RIORegisterBuffer() failed " << GetLastError() << endl;
			return 0;
		}

		DWORD offset = 0;
		EXTENDED_RIO_BUF* buffers = new EXTENDED_RIO_BUF[totalBufferCount];

		for (DWORD i = 0; i < totalBufferCount; ++i)
		{
			EXTENDED_RIO_BUF* buffer = buffers + i;

			buffer->BufferId = g_recvBufferId;
			buffer->Offset = offset;
			buffer->Length = RECV_BUFFER_SIZE;

			offset += RECV_BUFFER_SIZE;

			// 미리 recv를 걸어놓음
			if (g_RIOFunctionTable.RIOReceiveEx(g_RIORQ, buffer, 1, NULL, &g_addrRIOBufs[g_addrRIOBufIndex++],
				NULL, 0, 0, buffer) == false)
			{
				cout << "RIOReceive() failed " << GetLastError() << endl;
				return 0;
			}
		}

		cout << totalBufferCount << " total receives pending" << endl;
	}

	// Create IO thread
	for (DWORD i = 0; i < NUM_IOCP_THREADS; ++i)
	{
		auto result = _beginthreadex(NULL, 0, WorkerThread, NULL, 0, NULL);
		if (result == 0)
		{
			cout << "_beginthreadex() failed " << GetLastError() << endl;
			return 0;
		}
	}

	// Completion ready
	{
		int result = g_RIOFunctionTable.RIONotify(g_RIOCQ);
		if (result != ERROR_SUCCESS)
		{
			cout << "RIONotify() failed " << GetLastError() << endl;
			return 0;
		}
	}

	{
		cout << "Press any key to stop" << endl;
		auto temp = getchar();
	}

	// exit code

	closesocket(sock);

	g_RIOFunctionTable.RIOCloseCompletionQueue(g_RIOCQ);

	g_RIOFunctionTable.RIODeregisterBuffer(g_sendBufferId);
	g_RIOFunctionTable.RIODeregisterBuffer(g_recvBufferId);
	g_RIOFunctionTable.RIODeregisterBuffer(g_addrBufferId);

	return 0;
}

#define RIO_MAX_RESULTS 1024

UINT WorkerThread(LPVOID arg)
{
	DWORD transferred;
	ULONG_PTR completionKey;
	LPOVERLAPPED overlapped;
	RIORESULT* rioResults = new RIORESULT[RIO_MAX_RESULTS];

	while (true)
	{
		transferred = -1;
		completionKey = 0;
		overlapped = nullptr;

		if (GetQueuedCompletionStatus(g_IOCPHandle, &transferred, &completionKey, &overlapped, INFINITE) == false)
		{
			cout << "GetQueuedCompletionStatus() failed " << GetLastError() << endl;
			return 0;
		}

		if (completionKey == STOP_THREAD)
		{
			break;
		}

		memset(rioResults, 0, sizeof(rioResults));

		ULONG numResults = g_RIOFunctionTable.RIODequeueCompletion(g_RIOCQ, rioResults, RIO_MAX_RESULTS);
		if (numResults == 0 || RIO_CORRUPT_CQ == numResults)
		{
			cout << "RIODequeueCompletion() failed " << GetLastError() << endl;
			return 0;
		}

		if (g_RIOFunctionTable.RIONotify(g_RIOCQ) != ERROR_SUCCESS)
		{
			cout << "RIONotify() failed " << GetLastError() << endl;
			return 0;
		}

		for (DWORD i = 0; i < numResults; ++i)
		{
			EXTENDED_RIO_BUF* buffer = (EXTENDED_RIO_BUF*)rioResults[i].RequestContext;
			if (buffer == nullptr)
			{
				cout << "WorkerThread() buffer is nullptr" << endl;
				continue;
			}

			if (buffer->operationType == OP_RECV)
			{
				EnterCriticalSection(&g_lock);

				const char* offset = g_recvBufferPointer + buffer->Offset;
				EXTENDED_RIO_BUF* sendBuff = &(g_sendRIOBufs[g_sendRIOBufIndex++ % g_sendRIOBufTotalCount]);
				if (sendBuff == nullptr)
				{
					cout << "WorkerThread() sendBuff is nullptr" << endl;
					continue;
				}
				char* sendOffset = g_sendBufferPointer + sendBuff->Offset;
				memcpy_s(sendOffset, RECV_BUFFER_SIZE, offset, buffer->Length);

				cout << strlen(sendOffset) << " " << endl;

				if (g_RIOFunctionTable.RIOSendEx(g_RIORQ, sendBuff, 1, NULL, &g_addrRIOBufs[g_addrRIOBufIndex % g_addrRIOBufTotalCount], NULL, NULL, 0, sendBuff) == false)
				{
					cout << "RIOSend() failed " << GetLastError() << endl;
					continue;
				}

				LeaveCriticalSection(&g_lock);
			}
			else if (buffer->operationType == OP_SEND)
			{
				EnterCriticalSection(&g_lock);

				if (g_RIOFunctionTable.RIOReceiveEx(g_RIORQ, buffer, 1, NULL, &g_addrRIOBufs[g_addrRIOBufIndex % g_addrRIOBufTotalCount], NULL, 0, 0, buffer) == false)
				{
					cout << "RIORecv() failed " << GetLastError() << endl;
					continue;
				}

				++g_addrRIOBufIndex;

				LeaveCriticalSection(&g_lock);
			}
			else
			{
				break;
			}
		}
	}

	delete[] rioResults;

	return 0;
}
