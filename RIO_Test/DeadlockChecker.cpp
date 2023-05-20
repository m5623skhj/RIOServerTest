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
		auto iter = checkerMap.insert({ threadId, LockStateChecker(0, 0, countGetterFunctor) });
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
	Sleep(checkSleepTime);
	{
		std::lock_guard<std::mutex> guardLock(lock);
		
		for (auto& iter : checkerMap)
		{
			WORD thisTickCount = std::get<2>(iter.second)();
			if (thisTickCount != std::get<1>(iter.second))
			{
				++(std::get<0>(iter.second));

				if (std::get<0>(iter.second) > maxCheckCount)
				{
					g_Dump.Crash();
				}
			}
			else
			{
				std::get<0>(iter.second) = 0;
				std::get<1>(iter.second) = thisTickCount;
			}
		}
	}
}

