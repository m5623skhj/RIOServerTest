#pragma once

#include <string>
#include <functional>
#include "EnumType.h"
#include "DefineType.h"

using PacketId = unsigned int;

#define SET_PACKET_SIZE() virtual int GetPacketSize() override { return sizeof(*this) - 8; }

#pragma pack(push, 1)
class IPacket
{
public:
	IPacket() = default;
	virtual ~IPacket() = default;

	virtual PacketId GetPacketId() const = 0;
	virtual int GetPacketSize() = 0;
};

class TestStringPacket : public IPacket
{
public:
	TestStringPacket() = default;
	~TestStringPacket() = default;
	virtual PacketId GetPacketId() const override { return static_cast<PacketId>(PACKET_ID::TEST_STRING_PACKET); }
	SET_PACKET_SIZE();

public:
	char testString[20];
};

class EchoStringPacket : public IPacket
{
public:
	EchoStringPacket() = default;
	~EchoStringPacket() = default;
	virtual PacketId GetPacketId() const override { return static_cast<PacketId>(PACKET_ID::ECHO_STRING_PACEKT); }
	SET_PACKET_SIZE();

public:
	char echoString[30];
};

class CallTestProcedurePacket : public IPacket
{
public:
	CallTestProcedurePacket() = default;
	~CallTestProcedurePacket() = default;
	virtual PacketId GetPacketId() const override { return static_cast<PacketId>(PACKET_ID::CALL_TEST_PROCEDURE_PACKET); }
	SET_PACKET_SIZE();

public:
	int id3 = 0;
	WCHAR testString[30];
};

class CallSelectTest2ProcedurePacket : public IPacket
{
public:
	CallSelectTest2ProcedurePacket() = default;
	~CallSelectTest2ProcedurePacket() = default;
	virtual PacketId GetPacketId() const override { return static_cast<PacketId>(PACKET_ID::CALL_SELECT_TEST_2_PROCEDURE_PACKET); }
	SET_PACKET_SIZE();

public:
	long long id = 0;
};

class CallTestProcedurePacketReply : public IPacket
{
public:
	CallTestProcedurePacketReply() = default;
	~CallTestProcedurePacketReply() = default;
	virtual PacketId GetPacketId() const override { return static_cast<PacketId>(PACKET_ID::CALL_TEST_PROCEDURE_PACKET_REPLY); }
	SET_PACKET_SIZE();

public:
};

class CallSelectTest2ProcedurePacketReply : public IPacket
{
public:
	CallSelectTest2ProcedurePacketReply() = default;
	~CallSelectTest2ProcedurePacketReply() = default;
	virtual PacketId GetPacketId() const override { return static_cast<PacketId>(PACKET_ID::CALL_SELECT_TEST_2_PROCEDURE_PACKET_REPLY); }
	SET_PACKET_SIZE();

public:
	int no = 0;
	WCHAR testString[30];
};

#pragma pack(pop)

#define REGISTER_PACKET(PacketType){\
	PacketManager::GetInst().RegisterPacket<PacketType>();\
}

#pragma region PacketHandler

#define REGISTER_HANDLER(PacketType)\
	PacketManager::GetInst().RegisterPacketHandler<PacketType>();

#define DECLARE_HANDLE_PACKET(PacketType)\
	static bool HandlePacket(RIOTestSession& session, PacketType& packet);\

#define REGISTER_ALL_HANDLER()\
	REGISTER_HANDLER(TestStringPacket)\
	REGISTER_HANDLER(EchoStringPacket)\
	REGISTER_HANDLER(CallTestProcedurePacket)\
	REGISTER_HANDLER(CallSelectTest2ProcedurePacket)\
	
#define DECLARE_ALL_HANDLER()\
	DECLARE_HANDLE_PACKET(TestStringPacket)\
	DECLARE_HANDLE_PACKET(EchoStringPacket)\
	DECLARE_HANDLE_PACKET(CallTestProcedurePacket)\
	DECLARE_HANDLE_PACKET(CallSelectTest2ProcedurePacket)\

#pragma endregion PacketHandler

#pragma region ForDB

#define REGISTER_DB_REPLY_HANDLER(PacketType)\
	RegisterPacketHandler<PacketType>();

#define DECLARE_DB_REPLY_HANDLER(PacketType)\
	static bool AssemblePacket(PacketType& packet, OUT CSerializationBuf& recvPacket);\

#define REGISTER_ALL_DB_REPLY_HANDLER()\
	REGISTER_DB_REPLY_HANDLER(CallTestProcedurePacketReply)\
	REGISTER_DB_REPLY_HANDLER(CallSelectTest2ProcedurePacketReply)\

#define DECLARE_ALL_DB_REPLY_HANDLER()\
	DECLARE_DB_REPLY_HANDLER(CallTestProcedurePacketReply)\
	DECLARE_DB_REPLY_HANDLER(CallSelectTest2ProcedurePacketReply)\

#pragma endregion ForDB

#define REGISTER_PACKET_LIST(){\
	REGISTER_PACKET(TestStringPacket)\
	REGISTER_PACKET(EchoStringPacket)\
	REGISTER_PACKET(CallTestProcedurePacket)\
	REGISTER_PACKET(CallSelectTest2ProcedurePacket)\
	REGISTER_PACKET(CallTestProcedurePacketReply)\
	REGISTER_PACKET(CallSelectTest2ProcedurePacketReply)\
}