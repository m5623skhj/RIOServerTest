#pragma once

#define USE_SOCKET_LINGER_OPTION 1

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
#pragma endregion thread

#pragma region RIO
private:
	bool InitializeRIO();
#pragma endregion RIO

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
	HANDLE IOCPHandle;
};