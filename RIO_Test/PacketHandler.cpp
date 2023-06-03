#include "PreCompile.h"
#include "RIOTestSession.h"

PacketManager& PacketManager::GetInst()
{
	static PacketManager instance;
	return instance;
}

void PacketManager::Init()
{
	REGISTER_ALL_HANDLER()
}

std::shared_ptr<IPacket> PacketManager::MakePacket(PacketId packetId)
{
	auto iter = packetFactoryFunctionMap.find(packetId);
	if (iter == packetFactoryFunctionMap.end())
	{
		return nullptr;
	}

	auto factoryFunc = iter->second;
	return factoryFunc();
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

bool PacketManager::HandlePacket(RIOTestSession& session, TestStringPacket& packet)
{
	UNREFERENCED_PARAMETER(session);
	UNREFERENCED_PARAMETER(packet);

	return true;
}

bool PacketManager::HandlePacket(RIOTestSession& session, EchoStringPacket& packet)
{
	UNREFERENCED_PARAMETER(session);
	UNREFERENCED_PARAMETER(packet);

	return true;
}