#include "PreCompile.h"
#include "Broadcaster.h"
#include "RIOTestServer.h"

Broadcaster& Broadcaster::GetInst()
{
	static Broadcaster instance;
	return instance;
}

void Broadcaster::BroadcastToAllSession(IGameAndClientPacket&& packet)
{
	NetBuffer* buffer = NetBuffer::Alloc();
	NetBuffer::AddRefCount(buffer);
	if (buffer == nullptr)
	{
		std::cout << "buffer is nullptr" << std::endl;
		return;
	}

	*buffer << packet.GetPacketId();
	std::list<SessionId> sessionIdList;
	packet.PacketToBuffer(*buffer);
	{
		std::lock_guard guardLock(sessionSetLock);
		for (const auto& sessionId : sessionIdSet)
		{
			sessionIdList.push_back(sessionId);
		}
	}

	for(const auto& sessionId : sessionIdList)
	{
		auto session = RIOTestServer::GetInst().GetSession(sessionId);
		if (session == nullptr)
		{
			continue;
		}
		else if (session->IsReleasedSession() == true)
		{
			continue;
		}

		session->SendPacket(*buffer);
	}
	NetBuffer::Free(buffer);
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