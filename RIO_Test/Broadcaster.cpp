#include "PreCompile.h"
#include "Broadcaster.h"
#include "RIOTestServer.h"

Broadcaster& Broadcaster::GetInst()
{
	static Broadcaster instance;
	return instance;
}

void Broadcaster::BraodcastToAllSession(IGameAndClientPacket& packet)
{
	NetBuffer* buffer = NetBuffer::Alloc();
	NetBuffer::AddRefCount(buffer);
	if (buffer == nullptr)
	{
		std::cout << "buffer is nullptr" << std::endl;
		return;
	}

	*buffer << packet.GetPacketId();
	packet.PacketToBuffer(*buffer);
	{
		std::lock_guard<std::mutex> guardLock(sessionSetLock);
		for (const auto& sessionId : sessionIdSet)
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
	}
	NetBuffer::Free(buffer);
}

// unuse
/*
void Broadcaster::BraodcastToAllSession(NetBuffer& packet)
{
	NetBuffer::AddRefCount(&packet);
	{
		std::lock_guard<std::mutex> guardLock(sessionSetLock);
		for (const auto& sessionId : sessionIdSet)
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

			session->SendPacket(packet);
		}
	}
	NetBuffer::Free(&packet);
}
*/

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