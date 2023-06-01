#include "PreCompile.h"
#include "RIOTestSession.h"

PacketManager& PacketManager::GetInst()
{
	static PacketManager instance;
	return instance;
}

std::shared_ptr<IPacket> PacketManager::MakePacket(PacketId packetId)
{
	auto iter = packetMap.find(packetId);
	if (iter == packetMap.end())
	{
		return nullptr;
	}

	return nullptr;
	//return std::make_shared<IPacket>();
}

PacketHandler PacketManager::GetPacketHandler(PacketId packetId)
{
	auto iter = packetHandlerMap.find(packetId);
	if (iter == packetHandlerMap.end())
	{
		return nullptr;
	}

	return iter->second;
}

void PacketManager::RegisterPacket(PacketId packetId, std::shared_ptr<IPacket> packet)
{

}

void PacketManager::RegisterPacketHandler(PacketId packetId, PacketHandler& packetHandler)
{
	
}

template<>
bool RIOTestSession::PacketHanedler(RIOTestSession& session, TestStringPacket& packet)
{
	UNREFERENCED_PARAMETER(session);
	UNREFERENCED_PARAMETER(packet);

	return true;
}

template<>
bool RIOTestSession::PacketHanedler(RIOTestSession& session, EchoStringPacket& packet)
{
	UNREFERENCED_PARAMETER(session);
	UNREFERENCED_PARAMETER(packet);

	return true;
}