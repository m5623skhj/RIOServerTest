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

struct TickSet
{
	UINT64 nowTick = 0;
	UINT64 beforeTick = 0;
};

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
	FORCEINLINE void SleepRemainingFrameTime(OUT TickSet& tickSet);

	ULONG RIODequeueCompletion(RIO_CQ& rioCQ, RIORESULT* rioResults);
	IOContext* GetIOCompletedContext(RIORESULT& rioResult);

	IO_POST_ERROR IOCompleted(IOContext& context, ULONG transferred, RIOTestSession& session, BYTE threadId);

	IO_POST_ERROR RecvIOCompleted(ULONG transferred, RIOTestSession& session, BYTE threadId);
	IO_POST_ERROR SendIOCompleted(ULONG transferred, RIOTestSession& session, BYTE threadId);
	void InvalidIOCompleted(IOContext& context);
private:
	IO_POST_ERROR RecvCompleted(RIOTestSession& session, DWORD transferred);
	IO_POST_ERROR SendCompleted(RIOTestSession& session);

	FORCEINLINE WORD GetPayloadLength(OUT NetBuffer& buffer, int restSize);

private:
	BYTE GetMinimumSessionThreadId() const;

private:
	float GetSessionRationInThisThread(BYTE threadId);
	bool CheckRebalancingSession(BYTE threadId);
	void RebalanceSessionToThread();

private:
	std::thread accepterThread;
	std::vector<std::thread> workerThreads;
	
	const float sessionRatioInThread = 50.0f;
#pragma endregion thread

#pragma region rio
private:
	bool InitializeRIO();

private:
	GUID functionTableId = WSAID_MULTIPLE_RIO;

	RIO_CQ* rioCQList;
	
	std::shared_ptr<char> rioSendBuffer = nullptr;

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
	bool IsValidPacketSize(int bufferSize);
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
	std::shared_ptr<RIOTestSession> GetSession(SessionId sessionId);

	void Disconnect(SessionId sessionId);

private:
	std::shared_ptr<RIOTestSession> GetNewSession(SOCKET enteredClientSocket, BYTE threadId);
	bool MakeNewSession(SOCKET enteredClientSocket, const std::wstring_view& enteredClientIP);
	FORCEINLINE bool ReleaseSession(OUT RIOTestSession& releaseSession);

	FORCEINLINE void IOCountDecrement(RIOTestSession& session);

private:
	std::map<SessionId, std::shared_ptr<RIOTestSession>> sessionMap;
	SRWLOCK sessionMapLock;

	SessionId nextSessionId = INVALID_SESSION_ID + 1;

	UINT sessionCount = 0;

private:
	std::vector<UINT> numOfSessionInWorkerThread;
#pragma endregion session
};