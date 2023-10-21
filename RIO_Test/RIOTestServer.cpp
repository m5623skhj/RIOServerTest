#include "PreCompile.h"
#include "RIOTestServer.h"
#include "ScopeLock.h"
#include "NetServerSerializeBuffer.h"
#include "Broadcaster.h"
#include "DeadlockChecker.h"

#include "BuildConfig.h"

using namespace std;

void PrintError(const string_view errorFunctionName)
{
	cout << errorFunctionName << "() failed " << GetLastError() << endl;
}

void PrintError(const string_view errorFunctionName, DWORD errorCode)
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
		rioFunctionTable.RIOCloseCompletionQueue(rioCQList[i]);
	}
	rioFunctionTable.RIODeregisterBuffer(rioSendBufferId);

	delete[] rioCQList;

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

	std::shared_ptr<RIOTestSession> session;
	SessionId sessionId = context->ownerSessionId;
	{
		SCOPE_READ_LOCK(sessionMapLock);
		auto iter = sessionMap.find(sessionId);
		 if (iter == sessionMap.end())
		 {
			 return nullptr;
		 }

		 session = iter->second;
	}

	if (rioResult.BytesTransferred == 0 || session->ioCancle == true)
	{
		IOCountDecrement(*session);
		return nullptr;
	}

	return context;
}

IO_POST_ERROR RIOTestServer::IOCompleted(IOContext& context, ULONG transferred, RIOTestSession& session, BYTE threadId)
{
	if (context.ioType == RIO_OPERATION_TYPE::OP_RECV)
	{
		return RecvIOCompleted(transferred, session, threadId);
	}
	else if (context.ioType == RIO_OPERATION_TYPE::OP_SEND)
	{
		return SendIOCompleted(transferred, session, threadId);
	}
	else
	{
		InvalidIOCompleted(context);
		return IO_POST_ERROR::INVALID_OPERATION_TYPE;
	}
}

IO_POST_ERROR RIOTestServer::RecvIOCompleted(ULONG transferred, RIOTestSession& session, BYTE threadId)
{
	return RecvCompleted(session, transferred);
}

IO_POST_ERROR RIOTestServer::SendIOCompleted(ULONG transferred, RIOTestSession& session, BYTE threadId)
{
	InterlockedExchange((UINT*)&session.sendItem.ioMode, (UINT)IO_MODE::IO_NONE_SENDING);
	return SendCompleted(session);
}

void RIOTestServer::InvalidIOCompleted(IOContext& context)
{
	cout << "IO context is invalid " << static_cast<int>(context.ioType) << endl;

	auto session = GetSession(context.ownerSessionId);
	if (session != nullptr)
	{
		IOCountDecrement(*session);
	}

	contextPool.Free(&context);
}

void RIOTestServer::Accepter()
{
	SOCKET enteredClientSocket;
	SOCKADDR_IN enteredClientAddr;
	int addrSize = sizeof(enteredClientAddr);
	DWORD error = 0;
	WCHAR enteredIP[INET_ADDRSTRLEN];

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

		if (InetNtop(AF_INET, &(enteredClientAddr.sin_addr), enteredIP, INET_ADDRSTRLEN) == NULL)
		{
			error = GetLastError();
			PrintError("Accepter() / InetNtop() return NULL", error);
		}

		if (MakeNewSession(enteredClientSocket, enteredIP) == false)
		{
			closesocket(enteredClientSocket);
			continue;
		}

		InterlockedIncrement(&sessionCount);
	}
}

void RIOTestServer::Worker(BYTE inThreadId)
{
	RIORESULT rioResults[MAX_RIO_RESULT];
	rioCQList[inThreadId] = rioFunctionTable.RIOCreateCompletionQueue(maxClientCount / numOfWorkerThread * MAX_SEND_BUFFER_SIZE, nullptr);
	ULONG numOfResults = 0;
	workerOnList[inThreadId] = true;

	UINT64 updatedTick = GetTickCount64();
	std::function<UINT64()> getUpdatedTickFunc = [&updatedTick]()
	{
		return updatedTick;
	};

	DeadlockChecker::GetInstance().RegisterCheckThread(std::this_thread::get_id(), getUpdatedTickFunc);

	while (true)
	{
		ZeroMemory(rioResults, sizeof(rioResults));
		updatedTick = GetTickCount64();

		numOfResults = rioFunctionTable.RIODequeueCompletion(rioCQList[inThreadId], rioResults, MAX_RIO_RESULT);

		for (ULONG i = 0; i < numOfResults; ++i)
		{
			IO_POST_ERROR result = IO_POST_ERROR::SUCCESS;
			auto context = GetIOCompletedContext(rioResults[i]);
			if (context == nullptr)
			{
				continue;
			}

			auto session = GetSession(context->ownerSessionId);
			if (session == nullptr)
			{
				contextPool.Free(context);
				continue;
			}

			result = IOCompleted(*context, rioResults[i].BytesTransferred, *session, inThreadId);
			if (result == IO_POST_ERROR::INVALID_OPERATION_TYPE)
			{
				continue;
			}
			contextPool.Free(context);

			if (result == IO_POST_ERROR::IS_DELETED_SESSION)
			{
				continue;
			}
			IOCountDecrement(*session);
		}

		Sleep(33);
	}

	DeadlockChecker::GetInstance().DeregisteredCheckThread(std::this_thread::get_id());
}

IO_POST_ERROR RIOTestServer::RecvCompleted(RIOTestSession& session, DWORD transferred)
{
	session.recvItem.recvRingBuffer.MoveWritePos(transferred);
	int restSize = session.recvItem.recvRingBuffer.GetUseSize();
	bool packetError = false;
	IO_POST_ERROR result = IO_POST_ERROR::SUCCESS;

	while (restSize > df_HEADER_SIZE)
	{
		NetBuffer& buffer = *NetBuffer::Alloc();
		session.recvItem.recvRingBuffer.Peek((char*)(buffer.m_pSerializeBuffer), df_HEADER_SIZE);
		buffer.m_iRead = 0;

		WORD payloadLength = GetPayloadLength(buffer, restSize);
		if (payloadLength == 0)
		{
			NetBuffer::Free(&buffer);
			break;
		}
		else if (payloadLength > dfDEFAULTSIZE)
		{
			packetError = true;
			NetBuffer::Free(&buffer);
			break;
		}
		session.recvItem.recvRingBuffer.RemoveData(df_HEADER_SIZE);

		int dequeuedSize = session.recvItem.recvRingBuffer.Dequeue(&buffer.m_pSerializeBuffer[buffer.m_iWrite], payloadLength);
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
		cout << "code : " << code << endl;
		PrintError("GetPayloadLength/code error", 0);

		return 0;
	}
	if (restSize < payloadLength + df_HEADER_SIZE)
	{
		return 0;
	}

	return payloadLength;
}


BYTE RIOTestServer::GetMinimumSessionThreadId() const
{
	BYTE targetThreadId = 0;
	UINT minimumSessionCount = maxClientCount;

	for (BYTE threadId = 0; threadId < numOfSessionInWorkerThread.size(); ++threadId)
	{
		if (numOfSessionInWorkerThread[threadId] == 0)
		{
			return threadId;
		}
		else if (numOfSessionInWorkerThread[threadId] < minimumSessionCount)
		{
			minimumSessionCount = numOfSessionInWorkerThread[threadId];
			targetThreadId = threadId;
		}
	}

	return targetThreadId;
}

UINT RIOTestServer::GetSessionCount() const
{
	return sessionCount;
}

void RIOTestServer::Disconnect(SessionId sessionId)
{
	SCOPE_READ_LOCK(sessionMapLock);
	auto session = sessionMap.find(sessionId);
	if (session == sessionMap.end())
	{
		return;
	}

	shutdown(session->second->socket, SD_BOTH);
}

std::shared_ptr<RIOTestSession> RIOTestServer::GetNewSession(SOCKET enteredClientSocket, BYTE threadId)
{
	SessionId newSessionId = InterlockedIncrement(&nextSessionId);
	if (newSessionId == INVALID_SESSION_ID)
	{
		return nullptr;
	}

	return make_shared<RIOTestSession>(enteredClientSocket, newSessionId, threadId);
}


bool RIOTestServer::MakeNewSession(SOCKET enteredClientSocket, const std::wstring_view& enteredClientIP)
{
	BYTE threadId = GetMinimumSessionThreadId();
	auto newSession = GetNewSession(enteredClientSocket, threadId);
	if (newSession == nullptr)
	{
		return false;
	}
	InterlockedIncrement(&newSession->ioCount);

	if (newSession->InitSession(rioFunctionTable, rioNotiCompletion, rioCQList[threadId], rioCQList[threadId]) == false)
	{
		PrintError("RIOTestServer::MakeNewSession.InitSession", GetLastError());
		return false;
	}

	do
	{
		InterlockedIncrement(&numOfSessionInWorkerThread[threadId]);
		{
			SCOPE_WRITE_LOCK(sessionMapLock);
			if (sessionMap.emplace(newSession->sessionId, newSession).second == false)
			{
				IOCountDecrement(*newSession);

				PrintError("RIOTestServer::MakeNewSession.emplace", GetLastError());
				break;
			}
		}

		RecvPost(*newSession);
		IOCountDecrement(*newSession);

		Broadcaster::GetInst().OnSessionEntered(newSession->sessionId);
		newSession->OnClientEntered(enteredClientIP);

		return true;
	} while (false);

	ReleaseSession(*newSession);
	return false;
}

bool RIOTestServer::ReleaseSession(OUT RIOTestSession& releaseSession)
{
	closesocket(releaseSession.socket);
	InterlockedDecrement(&numOfSessionInWorkerThread[releaseSession.threadId]);
	InterlockedDecrement(&sessionCount);

	{
		SCOPE_WRITE_LOCK(sessionMapLock);
		sessionMap.erase(releaseSession.sessionId);
	}

	releaseSession.OnSessionReleased(rioFunctionTable);

	return true;
}

void RIOTestServer::IOCountDecrement(RIOTestSession& session)
{
	if (InterlockedDecrement(&session.ioCount) == 0)
	{
		Broadcaster::GetInst().OnSessionLeaved(session.sessionId);
		ReleaseSession(session);
	}
}

std::shared_ptr<RIOTestSession> RIOTestServer::GetSession(SessionId sessionId)
{
	SCOPE_READ_LOCK(sessionMapLock);
	auto iter = sessionMap.find(sessionId);
	if (iter == sessionMap.end())
	{
		return nullptr;
	}
	
	return iter->second;
}

bool RIOTestServer::InitializeRIO()
{
	for (int i = 0; i < numOfWorkerThread; ++i)
	{
		numOfSessionInWorkerThread.emplace_back(0);
	}

	GUID functionTableId = WSAID_MULTIPLE_RIO;
	DWORD bytes = 0;
	if (WSAIoctl(listenSocket, SIO_GET_MULTIPLE_EXTENSION_FUNCTION_POINTER, &functionTableId, sizeof(GUID)
		, reinterpret_cast<void**>(&rioFunctionTable), sizeof(rioFunctionTable), &bytes, NULL, NULL))
	{
		PrintError("WSAIoctl_SIO_GET_MULTIPLE_EXTENSION_FUNCTION_POINTER");
		return false;
	}

	rioCQList = new RIO_CQ[numOfWorkerThread];
	if (rioCQList == nullptr)
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

	session.sendItem.sendQueue.Enqueue(&packet);
	SendPost(session);
}

IO_POST_ERROR RIOTestServer::RecvPost(OUT RIOTestSession& session)
{
	int brokenSize = session.recvItem.recvRingBuffer.GetNotBrokenGetSize();
	int restSize = session.recvItem.recvRingBuffer.GetNotBrokenPutSize() - brokenSize;

	auto context = contextPool.Alloc();
	context->InitContext(session.sessionId, RIO_OPERATION_TYPE::OP_RECV);
	context->BufferId = session.recvItem.recvBufferId;
	context->Length = restSize;
	context->Offset = session.rioRecvOffset % DEFAULT_RINGBUFFER_MAX;
	{
		if (rioFunctionTable.RIOReceive(session.rioRQ, (PRIO_BUF)context, 1, NULL, context) == FALSE)
		{
			PrintError("RIOReceive", GetLastError());
			return IO_POST_ERROR::FAILED_RECV_POST;
		}
	}
	
	InterlockedIncrement(&session.ioCount);
	return IO_POST_ERROR::SUCCESS;
}

IO_POST_ERROR RIOTestServer::SendPost(OUT RIOTestSession& session)
{
	while (1)
	{
		if (InterlockedCompareExchange((UINT*)&session.sendItem.ioMode, (UINT)IO_MODE::IO_SENDING, (UINT)IO_MODE::IO_NONE_SENDING))
		{
			return IO_POST_ERROR::SUCCESS;
		}

		if (session.sendItem.sendQueue.GetRestSize() == 0 &&
			session.sendItem.reservedBuffer == nullptr)
		{
			InterlockedExchange((UINT*)&session.sendItem.ioMode, (UINT)IO_MODE::IO_NONE_SENDING);
			if (session.sendItem.sendQueue.GetRestSize() > 0)
			{
				continue;
			}
			return IO_POST_ERROR::SUCCESS;
		}

		int contextCount = 1;
		IOContext* context = contextPool.Alloc();
		context->InitContext(session.sessionId, RIO_OPERATION_TYPE::OP_SEND);
		context->BufferId = session.sendItem.sendBufferId;
		context->Offset = 0;
		context->ioType = RIO_OPERATION_TYPE::OP_SEND;
		context->Length = MakeSendStream(session, context);

		InterlockedIncrement(&session.ioCount);
		{
			if (rioFunctionTable.RIOSend(session.rioRQ, (PRIO_BUF)context, contextCount, NULL, context) == FALSE)
			{
				PrintError("RIOSend", GetLastError());
				IOCountDecrement(session);

				return IO_POST_ERROR::FAILED_SEND_POST;
			}
		}
	}

	return IO_POST_ERROR::SUCCESS;
}

ULONG RIOTestServer::MakeSendStream(OUT RIOTestSession& session, OUT IOContext* context)
{
	int totalSendSize = 0;
	int bufferCount = session.sendItem.sendQueue.GetRestSize();
	char* bufferPositionPointer = session.sendItem.rioSendBuffer;
	
	if (session.sendItem.reservedBuffer != nullptr)
	{
		int useSize = session.sendItem.reservedBuffer->GetAllUseSize();
		if (IsValidPacketSize(useSize) == false)
		{
			return 0;
		}

		memcpy_s(bufferPositionPointer, MAX_SEND_BUFFER_SIZE
			, session.sendItem.reservedBuffer->GetBufferPtr(), useSize);
		
		totalSendSize += useSize;
		bufferPositionPointer += totalSendSize;
		session.sendItem.reservedBuffer = nullptr;
	}

	NetBuffer* netBufferPtr;
	for (int i = 0; i < bufferCount; ++i)
	{
		session.sendItem.sendQueue.Dequeue(&netBufferPtr);
		
		int useSize = netBufferPtr->GetAllUseSize();
		if (IsValidPacketSize(useSize) == false)
		{
			return 0;
		}

		totalSendSize += useSize;
		if (totalSendSize >= MAX_SEND_BUFFER_SIZE)
		{
			session.sendItem.reservedBuffer = netBufferPtr;
			break;
		}

		memcpy_s(&session.sendItem.rioSendBuffer[totalSendSize - useSize], MAX_SEND_BUFFER_SIZE - totalSendSize - useSize
			, netBufferPtr->GetBufferPtr(), useSize);
	}

	return totalSendSize;
}

bool RIOTestServer::IsValidPacketSize(int bufferSize)
{
	if (bufferSize < MAX_SEND_BUFFER_SIZE)
	{
		return true;
	}

	// �������� Send�� �Ұǵ�,
	// MAX_SEND_BUFFER_SIZE ���� ũ�ٸ�, �ϴ� ��Ŷ ���� ���������� ������
	// TODO : 
	// Bunch �� ����
	g_Dump.Crash();

	return false;
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
