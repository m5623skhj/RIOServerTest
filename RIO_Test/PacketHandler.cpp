#include "PreCompile.h"
#include "PacketManager.h"
#include "RIOTestSession.h"
#include "Protocol.h"

bool PacketManager::HandlePacket(RIOTestSession& session, TestStringPacket& packet)
{
	UNREFERENCED_PARAMETER(session);
	UNREFERENCED_PARAMETER(packet);

	std::cout << packet.testString << std::endl;

	return true;
}

bool PacketManager::HandlePacket(RIOTestSession& session, EchoStringPacket& packet)
{
	UNREFERENCED_PARAMETER(session);
	UNREFERENCED_PARAMETER(packet);

	return true;
}