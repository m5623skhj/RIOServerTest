#pragma once
#include <thread>
#include <mutex>
#include <queue>
#include <chrono>
#include <string>
#include <list>

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
	void WriteLogImpl(std::list<LogBase>& waitingLogList);

private:
	std::thread loggerThread;
	// 0. LogginHandle
	// 1. StopHandle
	HANDLE loggerEventHandles[2];
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