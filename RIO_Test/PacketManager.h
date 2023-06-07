#pragma once

#include "Protocol.h"
#include <functional>
#include <any>
#include <map>

class RIOTestSession;

using PacketHandler = std::function<bool(RIOTestSession&, std::any&)>;

class PacketManager
{
private:
	PacketManager() = default;
	~PacketManager() = default;

	PacketManager(const PacketManager&) = delete;
	PacketManager& operator=(const PacketManager&) = delete;

public:
	static PacketManager& GetInst();
	std::shared_ptr<IPacket> MakePacket(PacketId packetId);
	PacketHandler GetPacketHandler(PacketId packetId);

	void Init();

public:
	using PacketFactoryFunction = std::function<std::shared_ptr<IPacket>()>;

	template <typename PacketType>
	void RegisterPacket()
	{
		static_assert(std::is_base_of<IPacket, PacketType>::value, "RegisterPacket() : PacketType must inherit from IPacket");
		PacketFactoryFunction factoryFunc = []()
		{
			return std::make_shared<PacketType>();
		};

		PacketType packetType;
		packetFactoryFunctionMap[packetType.GetPacketId()] = factoryFunc;
	}

	template <typename PacketType>
	void RegisterPacketHandler()
	{
		static_assert(std::is_base_of<IPacket, PacketType>::value, "RegisterPacketHandler() : PacketType must inherit from IPacket");
		auto handler = [](RIOTestSession& session, std::any& packet)
		{
			auto realPacket = static_cast<PacketType*>(std::any_cast<IPacket*>(packet));
			return HandlePacket(session, *realPacket);
		};

		PacketType packetType;
		packetHandlerMap[packetType.GetPacketId()] = handler;
	}

	std::map<PacketId, PacketFactoryFunction> packetFactoryFunctionMap;
	std::map<PacketId, PacketHandler> packetHandlerMap;

#pragma region PacketHandler
public:
	DECLARE_ALL_HANDLER();
#pragma endregion PacketHandler
};