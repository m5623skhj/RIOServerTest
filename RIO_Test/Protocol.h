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

#define REGISTER_PACKET(PacketType){\
	PacketManager::GetInst().RegisterPacket<PacketType>();\
}
#define REGISTER_PACKET_HANDLER(PacketType)\
	PacketManager::GetInst().RegisterPacketHandler<PacketType>();

#define DECLARE_HANDLE_PACKET(PacketType)\
	static bool HandlePacket(RIOTestSession& session, PacketType& packet);\

#define REGISTER_HANDLER()\
	REGISTER_PACKET_HANDLER(TestStringPacket)\
	REGISTER_PACKET_HANDLER(EchoStringPacket)\
	
#define DECLARE_ALL_HANDLER()\
	DECLARE_HANDLE_PACKET(TestStringPacket)\
	DECLARE_HANDLE_PACKET(EchoStringPacket)\

#define REGISTER_PACKET_LIST(){\
	REGISTER_PACKET(TestStringPacket)\
	REGISTER_PACKET(EchoStringPacket)\
}