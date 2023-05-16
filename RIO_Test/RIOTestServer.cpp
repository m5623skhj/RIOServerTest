#include "PreCompile.h"
#include "RIOTestServer.h"
#include "RIOTestSession.h"
#include "ScopeLock.h"
#include "NetServerSerializeBuffer.h"

#include "BuildConfig.h"

using namespace std;

void PrintError(const string& errorFunctionName)
{
	cout << errorFunctionName << "() failed " << GetLastError() << endl;
}

void PrintError(const string& errorFunctionName, DWORD errorCode)
{
	cout << errorFunctionName << "() failed " << errorCode << endl;
}

bool RIOTestServer::StartServer(const std::wstring& optionFileName)
{
	if (ServerOptionParsing(optionFileName) == false)
	{
		PrintError("ServerOptionParsing");
		return false;
	}

	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		PrintError("WSAStartup");
		return false;
	}

	listenSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (listenSocket == INVALID_SOCKET)
	{
		PrintError("socket");
		return false;
	}

	SOCKADDR_IN addr;
	ZeroMemory(&addr, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(port);
	if (bind(listenSocket, (SOCKADDR*)&addr, sizeof(addr)) == SOCKET_ERROR)
	{
		PrintError("bind");
		return false;
	}

	if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR)
	{
		PrintError("listen");
		return false;
	}

	if (SetSocketOption() == false)
	{
		return false;
	}

	iocpHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, numOfWorkerThread);
	if (iocpHandle == NULL)
	{
		PrintError("CreateCompletionPort");
		return false;
	}

	if (InitializeRIO() == false)
	{
		return false;
	}

	RunThreads();

	return true;
}

bool RIOTestServer::SetSocketOption()
{
	if (nagleOn == true)
	{
		if (setsockopt(listenSocket, IPPROTO_TCP, TCP_NODELAY, (const char*)&nagleOn, sizeof(int)) == SOCKET_ERROR)
		{
			PrintError("setsockopt_nagle");
			return false;
		}
	}

#if USE_SOCKET_LINGER_OPTION
	LINGER lingerOption;
	lingerOption.l_onoff = 1;
	lingerOption.l_linger = 0;
	if (setsockopt(listenSocket, SOL_SOCKET, SO_LINGER, (const char*)&lingerOption, sizeof(lingerOption)) == SOCKET_ERROR)
	{
		PrintError("setsockopt_linger");
		return false;
	}
#endif

	return true;
}

void RIOTestServer::StopServer()
{
	closesocket(listenSocket);

	rioFunctionTable.RIOCloseCompletionQueue(rioCQ);

	rioFunctionTable.RIODeregisterBuffer(rioSendBufferId);
}

void RIOTestServer::RunThreads()
{
	accepterThread = std::thread([this]() { this->Accepter(); });
	for (BYTE i = 0; i < numOfWorkerThread; ++i)
	{
		workerThreads.emplace_back([this]() { this->Worker(); });
	}
}

void RIOTestServer::Accepter()
{
	SOCKET enteredClientSocket;
	SOCKADDR_IN enteredClientAddr;
	WCHAR enteredIP[IP_SIZE];
	int addrSize = sizeof(enteredClientAddr);
	DWORD error = 0;

	while (true)
	{
		enteredClientSocket = accept(listenSocket, reinterpret_cast<SOCKADDR*>(&enteredClientAddr), &addrSize);
		if (enteredClientSocket == INVALID_SOCKET)
		{
			error = GetLastError();
			if (error == WSAEINTR)
			{
				break;
			}
			else
			{
				PrintError("Accepter() / accept", error);
				continue;
			}
		}

		inet_ntop(AF_INET, reinterpret_cast<void*>(&enteredClientAddr), reinterpret_cast<PSTR>(enteredIP), sizeof(enteredIP));
		// ���⿡�� �ش� IP�� ���� ó�� ���� �ϸ� �� ��

		if (MakeNewSession(enteredClientSocket) == false)
		{
			continue;
		}
		InterlockedIncrement(&sessionCount);
	}
}

bool RIOTestServer::MakeNewSession(SOCKET enteredClientSocket)
{
	UINT64 newSessionId = InterlockedIncrement(&nextSessionId);
	if (newSessionId == INVALID_SESSION_ID)
	{
		return false;
	}

	auto newSession = make_shared<RIOTestSession>(enteredClientSocket, newSessionId);
	if (newSession == nullptr)
	{
		return false;
	}

	if (newSession->InitSession(iocpHandle, rioFunctionTable, rioNotiCompletion, rioCQ) == false)
	{
		PrintError("RIOTestServer::MakeNewSession.InitSession", GetLastError());
		return false;
	}

	do
	{
		{
			SCOPE_WRITE_LOCK(rioLock);
			if (sessionMap.emplace(newSessionId, newSession).second == false)
			{
				PrintError("RIOTestServer::MakeNewSession.emplace", GetLastError());
				break;
			}
		}

		RecvPost(*newSession);
		if (InterlockedDecrement(&newSession->ioCount) == 0)
		{
			PrintError("RIOTestServer::MakeNewSession.ioCount is zero", 0);
			break;
		}

		newSession->OnClientEntered();

		return true;
	} while (false);

	ReleaseSession(*newSession);
	return false;
}

bool RIOTestServer::ReleaseSession(OUT RIOTestSession& releaseSession)
{
	if (InterlockedCompareExchange64(reinterpret_cast<LONG64*>(releaseSession.ioCount), 0, IO_COUNT_RELEASE_VALUE) != IO_COUNT_RELEASE_VALUE)
	{
		return false;
	}

	{
		SCOPE_WRITE_LOCK(rioLock);
		sessionMap.erase(releaseSession.sessionId);
	}

	int sendBufferRestCount = releaseSession.sendOverlapped.bufferCount;
	int rest = releaseSession.sendOverlapped.sendQueue.GetRestSize();
	
	for (int i = 0; i < sendBufferRestCount; ++i)
	{
		// free buffer
	}

	NetBuffer* deleteBuff;
	while (rest > 0)
	{
		releaseSession.sendOverlapped.sendQueue.Dequeue(&deleteBuff);
		// free buffer

		--rest;
	}

	closesocket(releaseSession.socket);
	InterlockedDecrement(&sessionCount);

	return true;
}

void RIOTestServer::Worker()
{
	printf("worker on");
	while (true)
	{
		Sleep(1000);
	}
}

bool RIOTestServer::InitializeRIO()
{
	GUID functionTableId = WSAID_MULTIPLE_RIO;
	DWORD bytes = 0;
	if (WSAIoctl(listenSocket, SIO_GET_MULTIPLE_EXTENSION_FUNCTION_POINTER, &functionTableId, sizeof(GUID)
		, reinterpret_cast<void**>(&rioFunctionTable), sizeof(rioFunctionTable), &bytes, NULL, NULL) != 0)
	{
		PrintError("WSAIoctl_SIO_GET_MULTIPLE_EXTENSION_FUNCTION_POINTER");
		return false;
	}

	rioNotiCompletion.Type = RIO_IOCP_COMPLETION;
	rioNotiCompletion.Iocp.IocpHandle = iocpHandle;
	rioNotiCompletion.Iocp.CompletionKey = reinterpret_cast<void*>(RIO_COMPLETION_KEY_TYPE::START);
	rioNotiCompletion.Iocp.Overlapped = &rioCQOverlapped;

	rioCQ = rioFunctionTable.RIOCreateCompletionQueue(maxClientCount * DEFAULT_RINGBUFFER_MAX, &rioNotiCompletion);
	if (rioCQ == RIO_INVALID_CQ)
	{
		PrintError("RIOCreateCompletionQueue");
		return false;
	}

	// CQ ���� ���
	rioSendBuffer.reset(new char[TOTAL_BUFFER_SIZE]);
	if (rioSendBuffer == nullptr)
	{
		cout << "rioSendBuff.reset is nullptr " << GetLastError() << endl;
		return false;
	}
	ZeroMemory(rioSendBuffer.get(), TOTAL_BUFFER_SIZE);
	
	rioSendBufferId = rioFunctionTable.RIORegisterBuffer(rioSendBuffer.get(), TOTAL_BUFFER_SIZE);
	if (rioSendBufferId == RIO_INVALID_BUFFERID)
	{
		cout << "rioSendBufferId is ROI_INVALID_BUFFERID " << GetLastError() << endl;
		return false;
	}

	return true;
}

IO_POST_ERROR RIOTestServer::RecvPost(OUT RIOTestSession& session)
{
	if (session.recvOverlapped.recvRingBuffer.IsFull() == true)
	{
		if (InterlockedDecrement(&session.ioCount) == 0)
		{
			ReleaseSession(session);
			return IO_POST_ERROR::IS_DELETED_SESSION;
		}

		return IO_POST_ERROR::FAILED_RECV_POST;
	}

	int brokenSize = session.recvOverlapped.recvRingBuffer.GetNotBrokenGetSize();
	int restSize = session.recvOverlapped.recvRingBuffer.GetFreeSize() - brokenSize;

	RIO_BUF buffer;
	buffer.Length = restSize;
	buffer.Offset = 0;
	{
		SCOPE_WRITE_LOCK(rioLock);

		if (rioFunctionTable.RIOReceiveEx(session.rioRQ, &buffer, 1
			, NULL, NULL, NULL, NULL, NULL, &buffer) == TRUE)
		{
			PrintError("RIOSend", GetLastError());
			return IO_POST_ERROR::FAILED_RECV_POST;
		}
	}

	return IO_POST_ERROR::SUCCESS;
}

bool RIOTestServer::ServerOptionParsing(const std::wstring& optionFileName)
{
	WCHAR buffer[BUFFER_MAX];
	LoadParsingText(buffer, optionFileName.c_str(), BUFFER_MAX);

	if (g_Paser.GetValue_Byte(buffer, L"RIO_SERVER", L"THREAD_COUNT", &numOfWorkerThread) == false)
	{
		return false;
	}
	if (g_Paser.GetValue_Byte(buffer, L"RIO_SERVER", L"NAGLE_ON", (BYTE*)&nagleOn) == false)
	{
		return false;
	}
	if (g_Paser.GetValue_Short(buffer, L"RIO_SERVER", L"PORT", (short*)&port) == false)
	{
		return false;
	}
	if (g_Paser.GetValue_Int(buffer, L"RIO_SERVER", L"MAX_CLIENT_COUNT", (int*)&maxClientCount) == false)
	{
		return false;
	}

	return true;
}
