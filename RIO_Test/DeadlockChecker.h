#pragma once

#include <map>
#include <mutex>
#include <functional>

using CheckSameCount = WORD;
using BeforeChecked = ULONG64;
using UpdateCountGetter = std::function<WORD()>;
using LockStateChecker = std::tuple<CheckSameCount, BeforeChecked, UpdateCountGetter>;

class DeadlockChecker
{
private:
	DeadlockChecker();
	~DeadlockChecker();

	DeadlockChecker(const DeadlockChecker&) = delete;
	DeadlockChecker& operator=(const DeadlockChecker&) = delete;

public:
	static DeadlockChecker& GetInstance();

#pragma region ThreadSatateCheck
public:
	bool RegisterCheckThread(const std::thread::id& threadId, const UpdateCountGetter& countGetterFunctor);
	void DeregisteredCheckThread(const std::thread::id& threadId);

private:
	void UpdateThreadState();
	
private:
	std::thread checkerThread;

	std::map<std::thread::id, LockStateChecker> checkerMap;
	mutable std::mutex lock;
		
	// 20 seconds
	CheckSameCount maxCheckCount = 20;
	const DWORD checkSleepTime = 1000;
#pragma endregion ThreadSatateCheck
};