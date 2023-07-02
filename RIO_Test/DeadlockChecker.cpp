#include "PreCompile.h"
#include "DeadlockChecker.h"
#include <thread>

DeadlockChecker::DeadlockChecker()
	: checkerThread([this]() { UpdateThreadState(); })
{

}

DeadlockChecker::~DeadlockChecker()
{

}

DeadlockChecker& DeadlockChecker::GetInstance()
{
	static DeadlockChecker instance;
	return instance;
}

bool DeadlockChecker::RegisterCheckThread(const std::thread::id& threadId, const UpdateCountGetter& countGetterFunctor)
{
	{
		std::lock_guard<std::mutex> guardLock(lock);
		auto iter = checkerMap.insert({ threadId, countGetterFunctor });
		if (iter.second == false)
		{
			return false;
		}
	}

	return true;
}

void DeadlockChecker::DeregisteredCheckThread(const std::thread::id& threadId)
{
	std::lock_guard<std::mutex> guardLock(lock);
	checkerMap.erase(threadId);
}

void DeadlockChecker::UpdateThreadState()
{
	while (true)
	{
		Sleep(checkSleepTime);

		UINT64 now = GetTickCount64();
		{
			std::lock_guard<std::mutex> guardLock(lock);

			for (auto& iter : checkerMap)
			{
				UINT64 threadUpdatedTick = iter.second();
				if (threadUpdatedTick >= now - maxCheckCount)
				{
					continue;
				}
			
				g_Dump.Crash();
			}
		}
	}
}

