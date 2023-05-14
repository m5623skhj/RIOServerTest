#pragma once

#include <synchapi.h>

class ScopeSRWLock
{
public:
	ScopeSRWLock() = delete;
	explicit ScopeSRWLock(OUT SRWLOCK& lock, bool exclusive);
	~ScopeSRWLock();

private:
	SRWLOCK& lock;
	bool exclusive;
};

#define SCOPE_READ_LOCK(lock) ScopeSRWLock scopeLock_##__LINE__(lock, false)
#define SCOPE_WRITE_LOCK(lock) ScopeSRWLock scopeLock_##__LINE__(lock, true)