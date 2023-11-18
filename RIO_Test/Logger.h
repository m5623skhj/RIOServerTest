#pragma once
#include <thread>
#include <mutex>
#include <queue>

class LogBase
{
public:
	LogBase() {}
	virtual ~LogBase() {}

private:

};

class Logger
{
private:
	Logger();
	~Logger();

public:
	static Logger& GetInstance();

#pragma region Thread
public:
	void RunLoggerThread();
	void Worker();
	void StopLoggerThread();

private:
	std::thread loggerThread;
	HANDLE loggerEventHandle = INVALID_HANDLE_VALUE;
	bool stopThread = false;
#pragma endregion Thread

#pragma region LogWaitingQueue
public:
	void WriteLog(LogBase& logObject);

private:
	void WriteLogToFile(LogBase& logObject);

private:
	std::mutex logQueueLock;
	std::queue<LogBase> logWaitingQueue;
#pragma endregion LogWaitingQueue
};