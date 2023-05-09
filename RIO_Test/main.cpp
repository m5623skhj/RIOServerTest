// Âü°í : https://gist.github.com/ujentus/5997058#file-rioserver_sm9-cpp

#include <iostream>
#include <WinSock2.h>
#include <MSWSock.h>
#include <cmath>

#pragma comment(lib, "ws2_32.lib")

using namespace std;

#define PORT 10001
const DWORD RIO_PENDING_RECVS = 100000;
const DWORD RIO_PENDING_SENDS = 10000;
const DWORD RECV_BUFFER_SIZE = 1024;
const DWORD SEND_BUFFER_SIZE = 1024;

HANDLE g_IOCPHandle = NULL;

RIO_CQ g_RIOCQ = 0;
RIO_RQ g_RIORQ = 0;
RIO_EXTENSION_FUNCTION_TABLE g_RIOFunctionTable;

char* g_sendBufferPointer = nullptr;
char* g_recvBufferPointer = nullptr;
char* g_addBufferPointer = nullptr;

RIO_BUFFERID g_sendBufferId;
RIO_BUFFERID g_recvBufferId;
RIO_BUFFERID g_addrBufferId;

RIO_BUF* g_sendRIOBufs = NULL;
DWORD g_sendRIOBufTotalCount = 0;
__int64 g_sendRIOBufIndex = 0;

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

	return 0;
}

/*
#define RIO_MAX_RESULTS 1000

UINT WorkerThread()
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

		if(GetQueuedCompletionStatus())

	}

	delete rioResults;

	return 0;
}
*/
