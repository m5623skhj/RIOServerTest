#include "PreCompile.h"
#include "Logger.h"
#include "RIOTestServer.h"
#include <list>

Logger::Logger()
{

}

Logger::~Logger()
{

}

Logger& Logger::GetInstance()
{
	static Logger inst;
	return inst;
}

void Logger::RunLoggerThread()
{
	loggerEventHandle = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (loggerEventHandle == NULL)
	{
		std::cout << "Logger event handle is invalid" << std::endl;
		g_Dump.Crash();
	}

	loggerThread = std::thread([this]() { this->Worker(); });
}

void Logger::Worker()
{
	std::list<LogBase> waitingLogList;
	while (true)
	{
		if (WaitForSingleObject(loggerEventHandle, INFINITE) == WAIT_FAILED)
		{
			std::cout << "WaitForSingleObject wait failed in logger thread" << std::endl;
			g_Dump.Crash();
		}

		{
			std::lock_guard<std::mutex> guardLock(logQueueLock);

			size_t restSize = logWaitingQueue.size();
			while (restSize > 0)
			{
				waitingLogList.push_back(std::move(logWaitingQueue.front()));
			}
		}

		size_t logSize = waitingLogList.size();
		for (auto& logObject : waitingLogList)
		{
			WriteLogToFile(logObject);
		}

		if (stopThread == true)
		{
			break;
		}
	}
}

void Logger::StopLoggerThread()
{
	stopThread = true;
	SetEvent(loggerEventHandle);

	loggerThread.join();
}

void Logger::WriteLog(LogBase& logObject)
{
	std::lock_guard<std::mutex> guardLock(logQueueLock);
	logWaitingQueue.push(logObject);
	
	SetEvent(loggerEventHandle);
}

void Logger::WriteLogToFile(LogBase& logObject)
{
	// Write to file like json object
}