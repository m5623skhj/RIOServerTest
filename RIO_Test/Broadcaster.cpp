#include "PreCompile.h"
#include "Broadcaster.h"

Broadcaster& Broadcaster::GetInst()
{
	static Broadcaster instance;
	return instance;
}

void Broadcaster::OnSessionEntered(SessionId enteredSessionId)
{
	{
		std::lock_guard<std::mutex> guardLock(sessionSetLock);
		sessionIdSet.insert(enteredSessionId);
	}
}

void Broadcaster::OnSessionLeaved(SessionId enteredSessionId)
{
	{
		std::lock_guard<std::mutex> guardLock(sessionSetLock);
		sessionIdSet.erase(enteredSessionId);
	}
}