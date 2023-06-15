#pragma once
#include <set>
#include <mutex>
#include "DefineType.h"

class RIOTestServer;

class Broadcaster
{
	friend RIOTestServer;

private:
	Broadcaster() = default;
	~Broadcaster() = default;

	Broadcaster(const Broadcaster&) = delete;
	Broadcaster& operator=(const Broadcaster&) = delete;

public:
	static Broadcaster& GetInst();

private:
	void OnSessionEntered();
	void OnSessionLeaved();

private:
	std::mutex sessionSetLock;
	std::set<SessionId> sessionIdSet;
};