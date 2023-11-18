#pragma once
#include <thread>
#include <mutex>
#include <queue>
#include <chrono>
#include <string>

class Logger;

class LogBase
{
	friend Logger;

public:
	LogBase();
	virtual ~LogBase();

private:
	void SetLogTime();

private:
	std::string loggingTime;
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