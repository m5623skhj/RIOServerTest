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

	listenSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_REGISTERED_IO);
	//listenSocket = socket(AF_INET, SOCK_STREAM, 0);
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

		// 여기에서 해당 IP에 대한 처리 등을 하면 될 듯

		if (MakeNewSession(enteredClientSocket) == false)
		{
			closesocket(enteredClientSocket);
			continue;
		}
		InterlockedIncrement(&sessionCount);
	}
}

void RIOTestServer::Worker()
{
	DWORD transferred;
	ULONG_PTR completionKey;
	LPOVERLAPPED overlapped;
	std::shared_ptr<RIOTestSession> session;
	IO_POST_ERROR result;

	while (true)
	{
		transferred = -1;
		completionKey = 0;
		overlapped = nullptr;
		result = IO_POST_ERROR::SUCCESS;

		if (GetQueuedCompletionStatus(iocpHandle, &transferred, &completionKey, &overlapped, INFINITE) == false)
		{
			if (overlapped == nullptr)
			{
				cout << "overlapped is nullptr" << endl;
				PrintError("Worker/overlapped");
				g_Dump.Crash();
			}

			int errorCode = GetLastError();
			if (transferred == 0 || session->ioCancle)
			{
				if (InterlockedDecrement(&session->ioCount) == 0)
				{
					ReleaseSession(*session);
				}
			}

			// log to GQCSFailed() with errorCode
		}

		{
			SCOPE_READ_LOCK(sessionMapLock);
			auto iter = sessionMap.find(completionKey);
			if (iter == sessionMap.end())
			{
				cout << "iter == sessionMap.end. completionKey : " << completionKey  << endl;
				PrintError("Worker/session is nullptr", GetLastError());
				continue;
			}

			session = iter->second;
		}

		if (transferred == 0 || session->ioCancle == true)
		{
			if (InterlockedDecrement(&session->ioCount) == 0)
			{
				ReleaseSession(*session);
				continue;
			}
		}

		if (&session->recvOverlapped == overlapped)
		{
			result = RecvCompleted(*session, transferred);
		}
		else if (&session->sendOverlapped == overlapped)
		{
			result = SendCompleted(*session);
		}
		else if(&session->postQueueOverlapped == overlapped)
		{
			result = SendPost(*session);
			InterlockedExchange((LONG*)(&session->sendOverlapped.nowPostQueuing), (LONG)(IO_MODE::IO_NONE_SENDING));
		}

		if (result == IO_POST_ERROR::IS_DELETED_SESSION)
		{
			continue;
		}

		if (InterlockedDecrement(&session->ioCount) == 0)
		{
			ReleaseSession(*session);
		}
	}
}

IO_POST_ERROR RIOTestServer::RecvCompleted(RIOTestSession& session, DWORD transferred)
{
	session.recvOverlapped.recvRingBuffer.MoveWritePos(transferred);
	int restSize = session.recvOverlapped.recvRingBuffer.GetUseSize();
	bool packetError = false;
	IO_POST_ERROR result = IO_POST_ERROR::SUCCESS;

	while (restSize > df_HEADER_SIZE)
	{
		NetBuffer buffer = *NetBuffer::Alloc();
		session.recvOverlapped.recvRingBuffer.Peek((char*)(buffer.m_pSerializeBuffer), df_HEADER_SIZE);
		buffer.m_iRead = 0;

		WORD payloadLength = GetPayloadLength(buffer, restSize);
		if (payloadLength)
		{
			packetError = true;
			break;
		}
		session.recvOverlapped.recvRingBuffer.RemoveData(df_HEADER_SIZE);

		int dequeuedSize = session.recvOverlapped.recvRingBuffer.Dequeue(&buffer.m_pSerializeBuffer[buffer.m_iWrite], payloadLength);
		buffer.m_iWrite += dequeuedSize;
		if (buffer.Decode() == false)
		{
			packetError = true;
			NetBuffer::Free(&buffer);
			break;
		}

		restSize -= (dequeuedSize + df_HEADER_SIZE);
		// OnRecv(session);
		NetBuffer::Free(&buffer);
	}

	result = RecvPost(session);
	if (packetError == true)
	{
		Disconnect(session.sessionId);
	}

	return result;
}

IO_POST_ERROR RIOTestServer::SendCompleted(RIOTestSession& session)
{
	int bufferCount = session.sendOverlapped.bufferCount;
	for (int i = 0; i < bufferCount; ++i)
	{
		NetBuffer::Free(session.sendOverlapped.storedBuffer[i]);
	}

	// session.OnSend();
	IO_POST_ERROR result;
	if (session.sendDisconnect == true)
	{
		Disconnect(session.sessionId);
		result = IO_POST_ERROR::SUCCESS;
	}
	else
	{
		InterlockedExchange((LONG*)(&session.sendOverlapped.nowPostQueuing), (LONG)(IO_MODE::IO_NONE_SENDING));
		result = SendPost(session);
		InterlockedExchange((LONG*)(&session.sendOverlapped.ioMode), (LONG)(IO_MODE::IO_NONE_SENDING));
	}

	return result;
}

WORD RIOTestServer::GetPayloadLength(OUT NetBuffer& buffer, int restSize)
{
	BYTE code;
	WORD payloadLength;
	buffer >> code >> payloadLength;

	if (code != NetBuffer::m_byHeaderCode)
	{
		PrintError("GetPayloadLength/code error", 0);
		NetBuffer::Free(&buffer);

		return 0;
	}
	if (restSize < payloadLength + df_HEADER_SIZE)
	{
		if (payloadLength > dfDEFAULTSIZE)
		{
			PrintError("GetPayloadLength/payloadLength error", 0);
		}
		NetBuffer::Free(&buffer);
		
		return 0;
	}

	return payloadLength;
}

UINT RIOTestServer::GetSessionCount() const
{
	return sessionCount;
}

void RIOTestServer::Disconnect(UINT64 sessionId)
{
	SCOPE_READ_LOCK(sessionMapLock);
	auto session = sessionMap.find(sessionId);
	if (session == sessionMap.end())
	{
		return;
	}

	shutdown(session->second->socket, SD_BOTH);
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
	if (InterlockedCompareExchange(&releaseSession.ioCount, 0, IO_COUNT_RELEASE_VALUE) != IO_COUNT_RELEASE_VALUE)
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
		NetBuffer::Free(releaseSession.sendOverlapped.storedBuffer[i]);
	}

	NetBuffer* deleteBuffer;
	while (rest > 0)
	{
		releaseSession.sendOverlapped.sendQueue.Dequeue(&deleteBuffer);
		NetBuffer::Free(deleteBuffer);

		--rest;
	}

	closesocket(releaseSession.socket);
	InterlockedDecrement(&sessionCount);

	return true;
}

bool RIOTestServer::InitializeRIO()
{
	GUID functionTableId = WSAID_MULTIPLE_RIO;
	DWORD bytes = 0;
	if (WSAIoctl(listenSocket, SIO_GET_MULTIPLE_EXTENSION_FUNCTION_POINTER, &functionTableId, sizeof(GUID)
		, reinterpret_cast<void**>(&rioFunctionTable), sizeof(rioFunctionTable), &bytes, NULL, NULL))
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

	// CQ 버퍼 등록
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
			PrintError("RIOReceiveEx", GetLastError());
			return IO_POST_ERROR::FAILED_RECV_POST;
		}
	}

	return IO_POST_ERROR::SUCCESS;
}

IO_POST_ERROR RIOTestServer::SendPost(OUT RIOTestSession& session)
{
	while (1)
	{
		if (InterlockedExchange((LONG*)(&session.sendOverlapped.ioMode), (LONG)(IO_MODE::IO_SENDING)
			!= (LONG)IO_MODE::IO_NONE_SENDING))
		{
			return IO_POST_ERROR::SUCCESS;
		}

		int bufferCount = session.sendOverlapped.sendQueue.GetRestSize();
		if (bufferCount == 0)
		{
			InterlockedExchange((LONG*)(&session.sendOverlapped.ioMode), (LONG)(IO_MODE::IO_SENDING));
			if (session.sendOverlapped.sendQueue.GetRestSize() > 0)
			{
				continue;
			}

			return IO_POST_ERROR::SUCCESS;
		}

		RIO_BUF buffer[ONE_SEND_WSABUF_MAX];
		if (ONE_SEND_WSABUF_MAX < bufferCount)
		{
			bufferCount = ONE_SEND_WSABUF_MAX;
		}

		for (int i = 0; i < bufferCount; ++i)
		{
			session.sendOverlapped.sendQueue.Dequeue(&session.sendOverlapped.storedBuffer[i]);
			int bufferUseSize = session.sendOverlapped.storedBuffer[i]->GetAllUseSize();
			buffer[i].Length = bufferUseSize;
			memcpy_s(&buffer[i], bufferUseSize, session.sendOverlapped.storedBuffer[i], bufferUseSize);
		}

		{
			SCOPE_WRITE_LOCK(rioLock);
			if (rioFunctionTable.RIOSendEx(session.rioRQ, buffer, bufferCount
				, NULL, NULL, NULL, NULL, NULL, buffer) == TRUE)
			{
				PrintError("RIOSendEx", GetLastError());
				return IO_POST_ERROR::FAILED_SEND_POST;
			}
		}

		return IO_POST_ERROR::SUCCESS;

	}
	return IO_POST_ERROR::IS_DELETED_SESSION;
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
