#pragma once
#include <MSWSock.h>
#include <vector>
#include <thread>

#pragma comment(lib, "ws2_32.lib")

#define USE_SOCKET_LINGER_OPTION 1

const DWORD SEND_BUFFER_SIZE = 2048;
const DWORD RIO_PENDING_SEND = 8192;
constexpr DWORD TOTAL_BUFFER_SIZE = SEND_BUFFER_SIZE * RIO_PENDING_SEND;

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
	SRWLOCK lock;
	std::vector<std::thread> workerThreads;
#pragma endregion thread

#pragma region rio
private:
	bool InitializeRIO();

private:
	GUID functionTableId = WSAID_MULTIPLE_RIO;

	RIO_CQ rioCQ;
	
	std::shared_ptr<char> rioSendBuffer = nullptr;
	RIO_BUFFERID rioSendBufferId = RIO_INVALID_BUFFERID;

	RIO_EXTENSION_FUNCTION_TABLE rioFunctionTable;
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

private:
	HANDLE iocpHandle;
};