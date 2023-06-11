#include "PreCompile.h"
#include "RIOTestServer.h"
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

RIOTestServer::RIOTestServer()
	: contextPool(2, false)
{
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
	if (::bind(listenSocket, (SOCKADDR*)&addr, sizeof(addr)) == SOCKET_ERROR)
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

	for (int i = 0; i < numOfWorkerThread; ++i)
	{
		rioFunctionTable.RIOCloseCompletionQueue(rioSendCQList[i]);
		rioFunctionTable.RIOCloseCompletionQueue(rioRecvCQList[i]);
	}
	rioFunctionTable.RIODeregisterBuffer(rioSendBufferId);

	delete[] rioSendCQList;
	delete[] rioRecvCQList;

	delete[] workerOnList;
}

void RIOTestServer::RunThreads()
{
	workerOnList = new bool[numOfWorkerThread];

	for (BYTE i = 0; i < numOfWorkerThread; ++i)
	{
		workerOnList[i] = false;
		workerThreads.emplace_back([this, i]() { this->Worker(i); });
	}

	do
	{
		bool completed = true;
		for (int i = 0; i < numOfWorkerThread; ++i)
		{
			if (workerOnList[i] == false)
			{
				completed = false;
				break;
			}
		}

		if (completed == true)
		{
			break;
		}

		Sleep(1000);
	} while (true);

	accepterThread = std::thread([this]() { this->Accepter(); });
}

ULONG RIOTestServer::RIODequeueCompletion(RIO_CQ& rioCQ, RIORESULT* rioResults)
{
	ULONG numOfResults = 0;
	{
		numOfResults = rioFunctionTable.RIODequeueCompletion(rioCQ, rioResults, MAX_RIO_RESULT);
		if (numOfResults == 0)
		{
			return 0;
		}
	}

	if (numOfResults == RIO_CORRUPT_CQ)
	{
		g_Dump.Crash();
	}

	return numOfResults;
}

IOContext* RIOTestServer::GetIOCompletedContext(RIORESULT& rioResult)
{
	IOContext* context = reinterpret_cast<IOContext*>(rioResult.RequestContext);
	if (context == nullptr)
	{
		return nullptr;
	}

	RIOTestSession* session = context->ownerSession;
	if (session == nullptr)
	{
		return nullptr;
	}

	if (rioResult.BytesTransferred == 0 || session->ioCancle == true)
	{
		ReleaseSession(*session);
	}

	return context;
}

void RIOTestServer::RecvIOCompleted(RIORESULT* rioResults, ULONG numOfResults, BYTE threadId)
{
	IO_POST_ERROR result = IO_POST_ERROR::SUCCESS;

	for (ULONG i = 0; i < numOfResults; ++i)
	{
		auto context = GetIOCompletedContext(rioResults[i]);
		if (context == nullptr)
		{
			continue;
		}

		RIOTestSession* session = context->ownerSession;
		if (context->ioType == RIO_OPERATION_TYPE::OP_RECV)
		{
			result = RecvCompleted(*session, rioResults[i].BytesTransferred);
		}
		else
		{
			cout << "IO context is invalid " << static_cast<int>(context->ioType) << endl;
			return;
		}

		if (result == IO_POST_ERROR::IS_DELETED_SESSION)
		{
			continue;
		}
	}
}

void RIOTestServer::SendIOCompleted(RIORESULT* rioResults, ULONG numOfResults, BYTE threadId)
{
	IO_POST_ERROR result = IO_POST_ERROR::SUCCESS;

	for (ULONG i = 0; i < numOfResults; ++i)
	{
		auto context = GetIOCompletedContext(rioResults[i]);
		if (context == nullptr)
		{
			continue;
		}

		RIOTestSession* session = context->ownerSession;
		if (session == nullptr)
		{
			continue;
		}

		session->rioSendOffset += rioResults[i].BytesTransferred;
		session->sendOverlapped.sendRingBuffer.RemoveData(rioResults[i].BytesTransferred);
		if (context->ioType == RIO_OPERATION_TYPE::OP_SEND)
		{
			result = SendCompleted(*session);
		}
		else
		{
			cout << "IO context is invalid " << static_cast<int>(context->ioType) << endl;
			return;
		}

		if (result == IO_POST_ERROR::IS_DELETED_SESSION)
		{
			continue;
		}
	}
}

void RIOTestServer::Accepter()
{
	SOCKET enteredClientSocket;
	SOCKADDR_IN enteredClientAddr;
	int addrSize = sizeof(enteredClientAddr);
	DWORD error = 0;
	WCHAR enteredIP[IP_SIZE];
	UNREFERENCED_PARAMETER(enteredIP);

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

		if (MakeNewSession(enteredClientSocket, (nextSessionId % numOfWorkerThread)) == false)
		{
			closesocket(enteredClientSocket);
			continue;
		}
		InterlockedIncrement(&sessionCount);
	}
}

void RIOTestServer::Worker(BYTE inThreadId)
{
	std::shared_ptr<RIOTestSession> session;

	RIORESULT rioRecvResults[MAX_RIO_RESULT];
	RIORESULT rioSendResults[MAX_RIO_RESULT];
	rioRecvCQList[inThreadId] = rioFunctionTable.RIOCreateCompletionQueue(maxClientCount / numOfWorkerThread * MAX_CLIENT_SEND_SIZE, nullptr);
	rioSendCQList[inThreadId] = rioFunctionTable.RIOCreateCompletionQueue(maxClientCount / numOfWorkerThread * MAX_CLIENT_SEND_SIZE, nullptr);

	ULONG numOfResults = 0;

	workerOnList[inThreadId] = true;

	while (true)
	{
		ZeroMemory(rioRecvResults, sizeof(rioRecvResults));
		ZeroMemory(rioSendResults, sizeof(rioSendResults));

		numOfResults = rioFunctionTable.RIODequeueCompletion(rioRecvCQList[inThreadId], rioRecvResults, MAX_RIO_RESULT);
		RecvIOCompleted(rioRecvResults, numOfResults, inThreadId);

		numOfResults = rioFunctionTable.RIODequeueCompletion(rioSendCQList[inThreadId], rioSendResults, MAX_RIO_RESULT);
		SendIOCompleted(rioSendResults, numOfResults, inThreadId);

		Sleep(1000);
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
		NetBuffer& buffer = *NetBuffer::Alloc();
		session.recvOverlapped.recvRingBuffer.Peek((char*)(buffer.m_pSerializeBuffer), df_HEADER_SIZE);
		buffer.m_iRead = 0;

		WORD payloadLength = GetPayloadLength(buffer, restSize);
		if (payloadLength > dfDEFAULTSIZE)
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
		session.OnRecvPacket(buffer);
		NetBuffer::Free(&buffer);
	}

	session.rioRecvOffset += transferred;
	result = RecvPost(session);
	if (packetError == true)
	{
		ReleaseSession(session);
	}

	return result;
}

IO_POST_ERROR RIOTestServer::SendCompleted(RIOTestSession& session)
{
	IO_POST_ERROR result;
	if (session.sendDisconnect == true)
	{
		Disconnect(session.sessionId);
		result = IO_POST_ERROR::SUCCESS;
	}
	else
	{
		result = SendPost(session);
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

bool RIOTestServer::MakeNewSession(SOCKET enteredClientSocket, BYTE threadId)
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

	{
		if (newSession->InitSession(rioFunctionTable, rioNotiCompletion, rioRecvCQList[threadId], rioSendCQList[threadId]) == false)
		{
			PrintError("RIOTestServer::MakeNewSession.InitSession", GetLastError());
			return false;
		}
	}

	do
	{
		{
			SCOPE_WRITE_LOCK(sessionMapLock);
			if (sessionMap.emplace(newSessionId, newSession).second == false)
			{
				PrintError("RIOTestServer::MakeNewSession.emplace", GetLastError());
				break;
			}
		}

		RecvPost(*newSession);

		newSession->OnClientEntered();

		return true;
	} while (false);

	ReleaseSession(*newSession);
	return false;
}

bool RIOTestServer::ReleaseSession(OUT RIOTestSession& releaseSession)
{
	{
		SCOPE_WRITE_LOCK(sessionMapLock);
		sessionMap.erase(releaseSession.sessionId);
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

	rioRecvCQList = new RIO_CQ[numOfWorkerThread];
	if (rioRecvCQList == nullptr)
	{
		return false;
	}

	rioSendCQList = new RIO_CQ[numOfWorkerThread];
	if (rioSendCQList == nullptr)
	{
		return false;
	}

	return true;
}

void RIOTestServer::SendPacket(OUT RIOTestSession& session, OUT NetBuffer& packet)
{
	if (packet.m_bIsEncoded == false)
	{
		packet.m_iWriteLast = packet.m_iWrite;
		packet.m_iWrite = 0;
		packet.m_iRead = 0;
		packet.Encode();
	}

	session.sendOverlapped.sendQueue.Enqueue(&packet);
	SendPost(session);
}

IO_POST_ERROR RIOTestServer::RecvPost(OUT RIOTestSession& session)
{
	int brokenSize = session.recvOverlapped.recvRingBuffer.GetNotBrokenGetSize();
	int restSize = session.recvOverlapped.recvRingBuffer.GetNotBrokenPutSize() - brokenSize;

	auto context = contextPool.Alloc();
	context->InitContext(&session, RIO_OPERATION_TYPE::OP_RECV);
	context->BufferId = session.recvOverlapped.recvBufferId;
	context->Length = restSize;
	context->Offset = session.rioRecvOffset % DEFAULT_RINGBUFFER_MAX;
	{
		SCOPE_MUTEX(session.rioRQLock);
		if (rioFunctionTable.RIOReceive(session.rioRQ, (PRIO_BUF)context, 1, NULL, context) == FALSE)
		{
			PrintError("RIOReceive", GetLastError());
			return IO_POST_ERROR::FAILED_RECV_POST;
		}
	}

	return IO_POST_ERROR::SUCCESS;
}

IO_POST_ERROR RIOTestServer::SendPost(OUT RIOTestSession& session)
{
	while (1)
	{
		int bufferCount = session.sendOverlapped.sendQueue.GetRestSize();
		if (bufferCount == 0)
		{
			return IO_POST_ERROR::SUCCESS;
		}

		int contextCount = 1;
		IOContext* context = contextPool.Alloc();
		context->InitContext(&session, RIO_OPERATION_TYPE::OP_SEND);
		context->BufferId = session.sendOverlapped.sendBufferId;
		context->Offset = session.rioSendOffset % DEFAULT_RINGBUFFER_MAX;
		context->ioType = RIO_OPERATION_TYPE::OP_SEND;

		int restBufferSize = session.sendOverlapped.sendRingBuffer.GetNotBrokenPutSize();
		NetBuffer* bufferPtr;
		for (int i = 0; i < bufferCount; ++i)
		{
			session.sendOverlapped.sendQueue.Dequeue(&bufferPtr);
			if (bufferPtr->GetAllUseSize() > restBufferSize)
			{
				// TODO :
				// 다시 넣을 수 있는 방법이 없으므로 백업을 해야할 듯
				// 백업 버퍼는 가장 먼저 확인하고 먼저 집어어야 함
				break;
			}

			session.sendOverlapped.sendRingBuffer.Enqueue(bufferPtr->GetBufferPtr(), bufferPtr->GetAllUseSize());
		}
		context->Length = session.sendOverlapped.sendRingBuffer.GetUseSize();

		{
			SCOPE_MUTEX(session.rioRQLock);
			if (rioFunctionTable.RIOSend(session.rioRQ, (PRIO_BUF)context, contextCount, NULL, context) == FALSE)
			{
				PrintError("RIOSend", GetLastError());
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
	if (g_Paser.GetValue_Byte(buffer, L"SERIALIZEBUF", L"PACKET_CODE", &NetBuffer::m_byHeaderCode) == false)
	{
		return false;
	}
	if (g_Paser.GetValue_Byte(buffer, L"SERIALIZEBUF", L"PACKET_KEY", &NetBuffer::m_byXORCode) == false)
	{
		return false;
	}

	return true;
}
