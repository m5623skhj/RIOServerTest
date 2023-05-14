#include "PreCompile.h"
#include "ScopeLock.h"

ScopeSRWLock::ScopeSRWLock(OUT SRWLOCK& inLock, bool inExclusive)
	: lock(inLock)
	, exclusive(inExclusive)
{
	if (exclusive == true)
	{
		AcquireSRWLockExclusive(&lock);
	}
	else
	{
		AcquireSRWLockShared(&lock);
	}
}

ScopeSRWLock::~ScopeSRWLock()
{
	if (exclusive == true)
	{
		ReleaseSRWLockExclusive(&lock);
	}
	else
	{
		ReleaseSRWLockShared(&lock);
	}
}