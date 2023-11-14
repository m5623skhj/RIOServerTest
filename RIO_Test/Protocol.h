#pragma once

#include <string>
#include <functional>
#include "EnumType.h"
#include "DefineType.h"
#include "NetServerSerializeBuffer.h"
#include "LanServerSerializeBuf.h"
#include <list>

using PacketId = unsigned int;

#define GET_PACKET_SIZE() virtual int GetPacketSize() override { return sizeof(*this) - 8; }
#define GET_PACKET_ID(packetId) virtual PacketId GetPacketId() const override { return static_cast<PacketId>(packetId); }

template<typename T>
void SetBufferToParameters(OUT NetBuffer& buffer, T& param)
{
	buffer >> param;
}

template<typename T, typename... Args>
void SetBufferToParameters(OUT NetBuffer& buffer, T& param, Args&... argList)
{
	buffer >> param;
	SetBufferToParameters(buffer, argList...);
}

#define SET_NO_BUFFER_TO_PARAMETER()\
virtual void BufferToPacket(OUT NetBuffer& buffer) override { UNREFERENCED_PARAMETER(buffer); }

#define SET_BUFFER_TO_PARAMETERS(...)\
virtual void BufferToPacket(OUT NetBuffer& buffer) override { SetBufferToParameters(buffer, __VA_ARGS__); }

template<typename T>
void SetParametersToBuffer(OUT NetBuffer& buffer, T& param)
{
	buffer << param;
}

template<typename T, typename... Args>
void SetParametersToBuffer(OUT NetBuffer& buffer, T& param, Args&... argList)
{
	buffer << param;
	SetParametersToBuffer(buffer, argList...);
}

template<typename T>
void SetParametersToBuffer(OUT CSerializationBuf& buffer, T& param)
{
	buffer << param;
}

template<typename T, typename... Args>
void SetParametersToBuffer(OUT CSerializationBuf& buffer, T& param, Args&... argList)
{
	buffer << param;
	SetParametersToBuffer(buffer, argList...);
}

#define SET_NO_PARAMETERS_TO_BUFFER()\
virtual void PacketToBuffer(OUT NetBuffer& buffer) override { UNREFERENCED_PARAMETER(buffer); }

#define SET_PARAMETERS_TO_BUFFER(...)\
virtual void PacketToBuffer(OUT NetBuffer& buffer) override { SetParametersToBuffer(buffer, __VA_ARGS__); }

#define SET_NO_PARAMETERS_TO_BUFFER_FOR_DBSERVER()\
virtual void PacketToBuffer(OUT NetBuffer& buffer) override { UNREFERENCED_PARAMETER(buffer); }

#define SET_PARAMETERS_TO_BUFFER_FOR_DBSERVER(...)\
virtual void PacketToBuffer(OUT CSerializationBuf& buffer) override { SetParametersToBuffer(buffer, __VA_ARGS__); }

#define SET_NO_PARAMETER()\
	SET_NO_BUFFER_TO_PARAMETER()\
	SET_NO_PARAMETERS_TO_BUFFER()

// This function assembles the packet based on the order of the defined parameters
#define SET_PARAMETERS(...)\
	SET_BUFFER_TO_PARAMETERS(__VA_ARGS__)\
	SET_PARAMETERS_TO_BUFFER(__VA_ARGS__)

#pragma pack(push, 1)
// IPacket�� �ֻ����� �ø��� GameClientPacket, GameDBPacket �̷� ������ �����°� ������?
class IPacket
{
public:
	IPacket() = default;
	virtual ~IPacket() = default;

	virtual PacketId GetPacketId() const = 0;
	virtual int GetPacketSize() = 0;
	virtual void BufferToPacket(OUT NetBuffer& buffer) = 0;
	virtual void PacketToBuffer(OUT NetBuffer& buffer) = 0;
};

class IDBSendPacket : IPacket
{
public:
	virtual void PacketToBuffer(OUT CSerializationBuf& buffer) = 0;
};

class TestStringPacket : public IPacket
{
public:
	TestStringPacket() = default;
	~TestStringPacket() = default;
	GET_PACKET_ID(PACKET_ID::TEST_STRING_PACKET);
	GET_PACKET_SIZE();
	SET_PARAMETERS(testString);

public:
	std::string testString;
};

class EchoStringPacket : public IPacket
{
public:
	EchoStringPacket() = default;
	~EchoStringPacket() = default;
	GET_PACKET_ID(PACKET_ID::ECHO_STRING_PACEKT);
	GET_PACKET_SIZE();
	SET_PARAMETERS(echoString);

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
	SET_PARAMETERS(id3, testString);

public:
	int id3 = 0;
	std::wstring testString;
};

class CallSelectTest2ProcedurePacket : public IPacket // public IDBSendPacket
{
public:
	CallSelectTest2ProcedurePacket() = default;
	~CallSelectTest2ProcedurePacket() = default;
	GET_PACKET_ID(PACKET_ID::CALL_SELECT_TEST_2_PROCEDURE_PACKET);
	GET_PACKET_SIZE();
	SET_PARAMETERS(id);
	//SET_PARAMETERS_TO_BUFFER_FOR_DBSERVER(id);

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
	SET_PARAMETERS(ownerSessionId);

public:
	SessionId ownerSessionId = INVALID_SESSION_ID;
};

class CallSelectTest2ProcedurePacketReply : public IPacket
{
public:
	CallSelectTest2ProcedurePacketReply() = default;
	~CallSelectTest2ProcedurePacketReply() = default;
	GET_PACKET_ID(PACKET_ID::CALL_SELECT_TEST_2_PROCEDURE_PACKET_REPLY);
	GET_PACKET_SIZE();
	SET_PARAMETERS(ownerSessionId, no, testString);

public:
	SessionId ownerSessionId = INVALID_SESSION_ID;
	std::list<int> no;
	std::list<std::string> testString;
};

class Ping : public IPacket
{
public:
	GET_PACKET_ID(PACKET_ID::PING);
	GET_PACKET_SIZE();
	SET_NO_PARAMETER();
};

class Pong : public IPacket
{
public:
	GET_PACKET_ID(PACKET_ID::PONG);
	GET_PACKET_SIZE();
	SET_NO_PARAMETER();
};

class RequestFileStream : public IPacket
{
public:
	GET_PACKET_ID(PACKET_ID::REQUEST_FILE_STREAM);
	GET_PACKET_SIZE();
	SET_NO_PARAMETER();
};

class ResponseFileStream : public IPacket
{
public:
	GET_PACKET_ID(PACKET_ID::RESPONSE_FILE_STREAM);
	GET_PACKET_SIZE();
	SET_PARAMETERS(fileStream);

public:
	char fileStream[4096];
};

class DBJobStart : public IPacket
{
public:
	GET_PACKET_ID(PACKET_ID::BATCHED_DB_JOB);
	GET_PACKET_SIZE();
	SET_PARAMETERS(batchSize, jobKey);

public:
	SessionId sessionId = INVALID_SESSION_ID;
	UINT batchSize = 0;
	DBJobKey jobKey = INVALID_DB_JOB_KEY;
};

class DBJobReply : public IPacket
{
public:
	GET_PACKET_ID(PACKET_ID::BATCHED_DB_JOB_RES);
	GET_PACKET_SIZE();
	SET_PARAMETERS(jobKey, isSuccessed);

public:
	DBJobKey jobKey = UINT64_MAX;
	bool isSuccessed = false;
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

#define DECLARE_DB_REPLY_HANDLER(PacketType)\
	static bool AssemblePacket(PacketType& packet, OUT CSerializationBuf& recvPacket);\

#define REGISTER_ALL_DB_REPLY_HANDLER()\
	REGISTER_DB_REPLY_HANDLER(CallTestProcedurePacketReply)\
	REGISTER_DB_REPLY_HANDLER(CallSelectTest2ProcedurePacketReply)\
	REGISTER_DB_REPLY_HANDLER(DBJobReply)\

#define DECLARE_ALL_DB_REPLY_HANDLER()\
	DECLARE_DB_REPLY_HANDLER(CallTestProcedurePacketReply)\
	DECLARE_DB_REPLY_HANDLER(CallSelectTest2ProcedurePacketReply)\
	DECLARE_DB_REPLY_HANDLER(DBJobReply)\

#pragma endregion ForDB

#define REGISTER_PACKET_LIST(){\
	REGISTER_PACKET(TestStringPacket)\
	REGISTER_PACKET(EchoStringPacket)\
	REGISTER_PACKET(CallTestProcedurePacket)\
	REGISTER_PACKET(CallSelectTest2ProcedurePacket)\
	REGISTER_PACKET(CallTestProcedurePacketReply)\
	REGISTER_PACKET(CallSelectTest2ProcedurePacketReply)\
	REGISTER_PACKET(Ping)\
	REGISTER_PACKET(RequestFileStream)\
	REGISTER_PACKET(DBJobReply)\
}