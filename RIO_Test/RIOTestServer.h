#pragma once
#include <MSWSock.h>
#include <vector>
#include <thread>
#include <map>
#include "DefineType.h"
#include "EnumType.h"
#include "NetServerSerializeBuffer.h"
#include "RIOTestSession.h"

#pragma comment(lib, "ws2_32.lib")

const DWORD SEND_BUFFER_SIZE = 2048;
const DWORD RIO_PENDING_SEND = 8192;
constexpr DWORD TOTAL_BUFFER_SIZE = SEND_BUFFER_SIZE * RIO_PENDING_SEND;

class RIOTestSession;

class RIOTestServer
{
private:
	RIOTestServer();
	~RIOTestServer() = default;

public:
	static RIOTestServer& GetInst()
	{
		static RIOTestServer instance;
		return instance;
	}

public:
	bool StartServer(const std::wstring& optionFileName);
	void StopServer();

#pragma region thread
public:
	void Accepter();
	void Worker(BYTE inThreadId);
	
private:
	void RunThreads();

	ULONG RIODequeueCompletion(RIO_CQ& rioCQ, RIORESULT* rioResults);
	IOContext* GetIOCompletedContext(RIORESULT& rioResult);

	IO_POST_ERROR IOCompleted(IOContext& context, ULONG transferred, RIOTestSession& session, BYTE threadId);

	IO_POST_ERROR RecvIOCompleted(ULONG transferred, RIOTestSession& session, BYTE threadId);
	IO_POST_ERROR SendIOCompleted(ULONG transferred, RIOTestSession& session, BYTE threadId);
	void InvalidIOCompleted(IOContext& context);
private:
	IO_POST_ERROR RecvCompleted(RIOTestSession& session, DWORD transferred);
	IO_POST_ERROR SendCompleted(RIOTestSession& session);

	WORD GetPayloadLength(OUT NetBuffer& buffer, int restSize);

private:
	std::thread accepterThread;
	std::vector<std::thread> workerThreads;
#pragma endregion thread

#pragma region rio
private:
	bool InitializeRIO();

private:
	GUID functionTableId = WSAID_MULTIPLE_RIO;

	RIO_CQ* rioCQList;
	
	std::shared_ptr<char> rioSendBuffer = nullptr;
	RIO_BUFFERID rioSendBufferId = RIO_INVALID_BUFFERID;

	RIO_NOTIFICATION_COMPLETION rioNotiCompletion;
	RIO_EXTENSION_FUNCTION_TABLE rioFunctionTable;
	OVERLAPPED rioCQOverlapped;

	bool* workerOnList;
#pragma endregion rio

#pragma region io
public:
	void SendPacket(OUT RIOTestSession& session, OUT NetBuffer& packet);

private:
	IO_POST_ERROR RecvPost(OUT RIOTestSession& session);
	IO_POST_ERROR SendPost(OUT RIOTestSession& session);

	ULONG MakeSendStream(OUT RIOTestSession& session, OUT IOContext* context);
private:
	CTLSMemoryPool<IOContext> contextPool;
#pragma endregion io

#pragma region serverOption
private:
	bool ServerOptionParsing(const std::wstring& optionFileName);
	bool SetSocketOption();

private:
	BYTE numOfWorkerThread = 0;
	bool nagleOn = false;
	WORD port = 0;
	UINT maxClientCount = 0;
#pragma endregion serverOption

private:
	SOCKET listenSocket;

#pragma region session
public:
	UINT GetSessionCount() const;

	void Disconnect(UINT64 sessionId);

private:
	std::shared_ptr<RIOTestSession> GetNewSession(SOCKET enteredClientSocket);
	bool MakeNewSession(SOCKET enteredClientSocket, BYTE threadId);
	bool ReleaseSession(OUT RIOTestSession& releaseSession);

	void IOCountDecrement(RIOTestSession& session);

private:
	std::map<SessionId, std::shared_ptr<RIOTestSession>> sessionMap;
	SRWLOCK sessionMapLock;

	UINT64 nextSessionId = INVALID_SESSION_ID + 1;

	UINT sessionCount = 0;
#pragma endregion session
};