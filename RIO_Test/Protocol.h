#pragma once

#include <string>
#include <functional>
#include "EnumType.h"
#include "DefineType.h"

using PacketId = unsigned int;

class IPacket
{
public:
	IPacket() = default;
	virtual ~IPacket() = default;

	virtual PacketId GetPacketId() const = 0;
	
};

class TestStringPacket : public IPacket
{
public:
	TestStringPacket() = default;
	~TestStringPacket() = default;
	virtual PacketId GetPacketId() const override { return static_cast<PacketId>(PACKET_ID::TEST_STRING_PACKET); }
};

class EchoStringPacket : public IPacket
{
public:
	EchoStringPacket() = default;
	~EchoStringPacket() = default;
	virtual PacketId GetPacketId() const override { return static_cast<PacketId>(PACKET_ID::ECHO_STRING_PACEKT); }

public:
	char echoString[30];
};

#define INPUT_TO_MAP(InputMap, InputItem){\
		InputMap.emplace(0, std::make_shared<InputItem>());\
	}

#define REGISTER_PACKET(...){\
	INPUT_TO_MAP(PacketManager::GetInst().packetMap, __VA_ARGS__);\
}

#define REGISTER_PACKET_HANDLER(...){\
	INPUT_TO_MAP(PacketManager::GetInst().packetHandlerMap, __VA_ARGS__);\
}

#define PACKET_LIST\
	TestStringPacket\
	, EchoStringPacket

#define REGISTER_PACKET_LIST(){\
	REGISTER_PACKET(PACKET_LIST);\
}
/*REGISTER_PACKET_HANDLER(PACKET_LIST);\*/
