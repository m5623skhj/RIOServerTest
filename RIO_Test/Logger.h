#pragma once
#include <thread>
#include <mutex>
#include <queue>
#include <chrono>
#include <string>
#include <list>
#include "nlohmann/json.hpp"
#include <memory>
#include <fstream>

class Logger;

#define OBJECT_TO_JSON_LOG(...)\
    virtual nlohmann::json ObjectToJson() override {\
        nlohmann::json jsonObject;\
        { __VA_ARGS__ }\
        return jsonObject;\
    }\

#define SET_LOG_ITEM(logObject){\
    jsonObject[#logObject] = logObject;\
}

class LogBase
{
	friend Logger;

public:
	LogBase() = default;
	virtual ~LogBase() = default;

public:
	virtual nlohmann::json ObjectToJson() = 0;

private:
	nlohmann::json ObjectToJsonImpl();
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
	void WriteLogImpl(std::list<std::shared_ptr<LogBase>>& waitingLogList);

private:
	std::thread loggerThread;
	// 0. LogginHandle
	// 1. StopHandle
	HANDLE loggerEventHandles[2];
#pragma endregion Thread

#pragma region LogWaitingQueue
public:
	void WriteLog(std::shared_ptr<LogBase> logObject);

private:
	void WriteLogToFile(std::shared_ptr<LogBase> logObject);

private:
	std::mutex logQueueLock;
	std::queue<std::shared_ptr<LogBase>> logWaitingQueue;

	std::ofstream logFileStream;
#pragma endregion LogWaitingQueue
};