#pragma once
#include <MSWSock.h>
#include <vector>
#include <thread>
#include <map>
#include "DefineType.h"

#pragma comment(lib, "ws2_32.lib")

const DWORD SEND_BUFFER_SIZE = 2048;
const DWORD RIO_PENDING_SEND = 8192;
constexpr DWORD TOTAL_BUFFER_SIZE = SEND_BUFFER_SIZE * RIO_PENDING_SEND;

class RIOTestSession;

class RIOTestServer
{
public:
	RIOTestServer() = default;
	~RIOTestServer() = default;

public:
	bool StartServer(const std::wstring& optionFileName);
	void StopServer();

#pragma region thread
public:
	void Accepter();
	void Worker();
	
private:
	void RunThreads();

private:
	SRWLOCK lock;
	std::thread accepterThread;
	std::vector<std::thread> workerThreads;
#pragma endregion thread

#pragma region rio
private:
	bool InitializeRIO();

private:
	GUID functionTableId = WSAID_MULTIPLE_RIO;

	RIO_CQ rioCQ = RIO_INVALID_CQ;
	
	std::shared_ptr<char> rioSendBuffer = nullptr;
	RIO_BUFFERID rioSendBufferId = RIO_INVALID_BUFFERID;

	RIO_NOTIFICATION_COMPLETION rioNotiCompletion;
	RIO_EXTENSION_FUNCTION_TABLE rioFunctionTable;
	OVERLAPPED rioCQOverlapped;
#pragma endregion rio

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
private:
	bool MakeNewSession(SOCKET enteredClientSocket);
	bool ReleaseSession(std::shared_ptr<RIOTestSession> releaseSession);

private:
	std::map<UINT_PTR, std::shared_ptr<RIOTestSession>> sessionMap;
	SRWLOCK sessionMapLock;

	UINT64 nextSessionId = INVALID_SESSION_ID + 1;

	UINT sessionCount = 0;
#pragma endregion session
};