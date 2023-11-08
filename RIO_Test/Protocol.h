#pragma once

#include <string>
#include <functional>
#include "EnumType.h"
#include "DefineType.h"

using PacketId = unsigned int;

#define GET_PACKET_SIZE() virtual int GetPacketSize() override { return sizeof(*this) - 8; }
#define GET_PACKET_ID(packetId) virtual PacketId GetPacketId() const override { return static_cast<PacketId>(packetId); }

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
	GET_PACKET_ID(PACKET_ID::TEST_STRING_PACKET);
	GET_PACKET_SIZE();

public:
	char testString[20];
};

class EchoStringPacket : public IPacket
{
public:
	EchoStringPacket() = default;
	~EchoStringPacket() = default;
	GET_PACKET_ID(PACKET_ID::ECHO_STRING_PACEKT);
	GET_PACKET_SIZE();

public:
	char echoString[30];
};

class CallTestProcedurePacket : public IPacket
{
public:
	CallTestProcedurePacket() = default;
	~CallTestProcedurePacket() = default;
	GET_PACKET_ID(PACKET_ID::CALL_TEST_PROCEDURE_PACKET);
	GET_PACKET_SIZE();

public:
	int id3 = 0;
	WCHAR testString[30];
};

class CallSelectTest2ProcedurePacket : public IPacket
{
public:
	CallSelectTest2ProcedurePacket() = default;
	~CallSelectTest2ProcedurePacket() = default;
	GET_PACKET_ID(PACKET_ID::CALL_SELECT_TEST_2_PROCEDURE_PACKET);
	GET_PACKET_SIZE();

public:
	long long id = 0;
};

class CallTestProcedurePacketReply : public IPacket
{
public:
	CallTestProcedurePacketReply() = default;
	~CallTestProcedurePacketReply() = default;
	GET_PACKET_ID(PACKET_ID::CALL_TEST_PROCEDURE_PACKET_REPLY);
	GET_PACKET_SIZE();

public:
};

class CallSelectTest2ProcedurePacketReply : public IPacket
{
public:
	CallSelectTest2ProcedurePacketReply() = default;
	~CallSelectTest2ProcedurePacketReply() = default;
	GET_PACKET_ID(PACKET_ID::CALL_SELECT_TEST_2_PROCEDURE_PACKET_REPLY);
	GET_PACKET_SIZE();

public:
	int no = 0;
	WCHAR testString[30];
};

class Ping : public IPacket
{
public:
	GET_PACKET_ID(PACKET_ID::PING);
	GET_PACKET_SIZE();
};

class Pong : public IPacket
{
public:
	GET_PACKET_ID(PACKET_ID::PONG);
	GET_PACKET_SIZE();
};

class RequestFileStream : public IPacket
{
public:
	GET_PACKET_ID(PACKET_ID::REQUEST_FILE_STREAM);
	GET_PACKET_SIZE();
};

class ResponseFileStream : public IPacket
{
public:
	GET_PACKET_ID(PACKET_ID::RESPONSE_FILE_STREAM);
	GET_PACKET_SIZE();

public:
	char fileStream[4096];
};

#pragma pack(pop)

#define REGISTER_PACKET(PacketType){\
	RegisterPacket<PacketType>();\
}

#pragma region PacketHandler

#define REGISTER_HANDLER(PacketType)\
	RegisterPacketHandler<PacketType>();

#define DECLARE_HANDLE_PACKET(PacketType)\
	static bool HandlePacket(RIOTestSession& session, PacketType& packet);\

#define REGISTER_ALL_HANDLER()\
	REGISTER_HANDLER(TestStringPacket)\
	REGISTER_HANDLER(EchoStringPacket)\
	REGISTER_HANDLER(CallTestProcedurePacket)\
	REGISTER_HANDLER(CallSelectTest2ProcedurePacket)\
	REGISTER_HANDLER(Ping)\
	REGISTER_HANDLER(RequestFileStream)\
	
#define DECLARE_ALL_HANDLER()\
	DECLARE_HANDLE_PACKET(TestStringPacket)\
	DECLARE_HANDLE_PACKET(EchoStringPacket)\
	DECLARE_HANDLE_PACKET(CallTestProcedurePacket)\
	DECLARE_HANDLE_PACKET(CallSelectTest2ProcedurePacket)\
	DECLARE_HANDLE_PACKET(Ping)\
	DECLARE_HANDLE_PACKET(RequestFileStream)\

#pragma endregion PacketHandler

#pragma region ForDB

#define REGISTER_DB_REPLY_HANDLER(PacketType)\
	RegisterPacketHandler<PacketType>();

// 이거 뭔가 이상한데
// 왜 packet하고 recvPacket하고 둘 다 주는건지?
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