#include "PreCompile.h"
#include "Logger.h"
#include "RIOTestServer.h"
#include <list>

LogBase::LogBase()
{

}

LogBase::~LogBase()
{

}

void LogBase::SetLogTime()
{
	const auto now = std::chrono::system_clock::now();
	std::time_t currentTime = std::chrono::system_clock::to_time_t(now);
	std::tm utcTime;

	auto error = gmtime_s(&utcTime, &currentTime);
	if (error != 0)
	{
		std::cout << "Error in gmtime_s() : " << error << std::endl;
		g_Dump.Crash();
	}
	auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() % 1000;

	std::stringstream stream;
	stream << std::put_time(&utcTime, "%Y-%m-%d %H:%M:%S.") << std::setfill('0') << std::setw(3) << milliseconds;

	loggingTime = stream.str();
}

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
	logObject.SetLogTime();

	std::lock_guard<std::mutex> guardLock(logQueueLock);
	logWaitingQueue.push(logObject);
	
	SetEvent(loggerEventHandle);
}

void Logger::WriteLogToFile(LogBase& logObject)
{
	// Write to file like json object
}