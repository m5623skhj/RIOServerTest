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
public:
	RIOTestServer();
	~RIOTestServer() = default;

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
	void RecvIOCompleted(RIORESULT* rioResults);
	void SendIOCompleted(RIORESULT* rioResults);
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

	RIO_CQ rioRecvCQ;
	RIO_CQ rioSendCQ;
	
	std::shared_ptr<char> rioSendBuffer = nullptr;
	RIO_BUFFERID rioSendBufferId = RIO_INVALID_BUFFERID;

	RIO_NOTIFICATION_COMPLETION rioNotiCompletion;
	RIO_EXTENSION_FUNCTION_TABLE rioFunctionTable;
	OVERLAPPED rioCQOverlapped;

	SRWLOCK rioLock;
#pragma endregion rio

#pragma region io
private:
	IO_POST_ERROR RecvPost(OUT RIOTestSession& session);
	IO_POST_ERROR SendPost(OUT RIOTestSession& session);

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
	HANDLE iocpHandle;

#pragma region session
public:
	UINT GetSessionCount() const;

	void Disconnect(UINT64 sessionId);

private:
	bool MakeNewSession(SOCKET enteredClientSocket, BYTE threadId);
	bool ReleaseSession(OUT RIOTestSession& releaseSession);

private:
	std::map<SessionId, std::shared_ptr<RIOTestSession>> sessionMap;
	SRWLOCK sessionMapLock;

	UINT64 nextSessionId = INVALID_SESSION_ID + 1;

	UINT sessionCount = 0;
#pragma endregion session
};