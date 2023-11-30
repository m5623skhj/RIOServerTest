#pragma once

#include <string>
#include <functional>
#include "EnumType.h"
#include "DefineType.h"
#include "NetServerSerializeBuffer.h"
#include "LanServerSerializeBuf.h"
#include <list>

using PacketId = unsigned int;

#define GET_PACKET_ID(packetId) virtual PacketId GetPacketId() const override { return static_cast<PacketId>(packetId); }

#pragma region ForGameServerPacket
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

#define SET_BUFFER_TO_PARAMETERS(...)\
virtual void BufferToPacket(OUT NetBuffer& buffer) override { SetBufferToParameters(buffer, __VA_ARGS__); }

#define SET_PARAMETERS_TO_BUFFER(...)\
virtual void PacketToBuffer(OUT NetBuffer& buffer) override { SetParametersToBuffer(buffer, __VA_ARGS__); }

// This function assembles the packet based on the order of the defined parameters
#define SET_PARAMETERS(...)\
	SET_BUFFER_TO_PARAMETERS(__VA_ARGS__)\
	SET_PARAMETERS_TO_BUFFER(__VA_ARGS__)

#pragma endregion ForGameServerPacket

#pragma region ForDBServerPacket
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

#define SET_PARAMETERS_TO_BUFFER_DB_PACKET(...)\
virtual void PacketToBuffer(OUT CSerializationBuf& buffer) override { SetParametersToBuffer(buffer, __VA_ARGS__); }

#pragma endregion ForDBServerPacket

////////////////////////////////////////////////////////////////////////////////////
// Packet
////////////////////////////////////////////////////////////////////////////////////

#pragma pack(push, 1)

#pragma region GameServer
class IPacket
{
public:
	IPacket() = default;
	virtual ~IPacket() = default;

	virtual PacketId GetPacketId() const = 0;
};

class IGameAndClientPacket : public IPacket
{
public:
	virtual void BufferToPacket(OUT NetBuffer& buffer) { UNREFERENCED_PARAMETER(buffer); }
	virtual void PacketToBuffer(OUT NetBuffer& buffer) { UNREFERENCED_PARAMETER(buffer); }
};

class TestStringPacket : public IGameAndClientPacket
{
public:
	TestStringPacket() = default;
	~TestStringPacket() = default;
	GET_PACKET_ID(PACKET_ID::TEST_STRING_PACKET);
	SET_PARAMETERS(testString);

public:
	std::string testString;
};

class EchoStringPacket : public IGameAndClientPacket
{
public:
	EchoStringPacket() = default;
	~EchoStringPacket() = default;
	GET_PACKET_ID(PACKET_ID::ECHO_STRING_PACEKT);
	SET_PARAMETERS(echoString);

public:
	char echoString[30];
};

class TestProcedurePacket : public IGameAndClientPacket
{
public:
	GET_PACKET_ID(PACKET_ID::CALL_TEST_PROCEDURE_PACKET);
	SET_PARAMETERS(id3, testString);

public:
	int id3 = 0;
	std::wstring testString;
};

class SelectTest2ProcedurePacket : public IGameAndClientPacket
{
public:
	GET_PACKET_ID(PACKET_ID::CALL_SELECT_TEST_2_PROCEDURE_PACKET);
	SET_PARAMETERS(id);

public:
	long long id = 0;
};

class Ping : public IGameAndClientPacket
{
public:
	GET_PACKET_ID(PACKET_ID::PING);
};

class Pong : public IGameAndClientPacket
{
public:
	GET_PACKET_ID(PACKET_ID::PONG);
};

class RequestFileStream : public IGameAndClientPacket
{
public:
	GET_PACKET_ID(PACKET_ID::REQUEST_FILE_STREAM);
};

class ResponseFileStream : public IGameAndClientPacket
{
public:
	GET_PACKET_ID(PACKET_ID::RESPONSE_FILE_STREAM);
	SET_PARAMETERS(fileStream);

public:
	char fileStream[4096];
};

#pragma endregion GameServer

#pragma region DBServer
class IGameAndDBPacket : public IPacket
{
public:
	virtual void PacketToBuffer(OUT CSerializationBuf& buffer) { UNREFERENCED_PARAMETER(buffer); }
};

class DBJobStart : public IGameAndDBPacket
{
public:
	GET_PACKET_ID(PACKET_ID::GAME2DB_BATCHED_DB_JOB);

public:
	//DBJobKey jobKey = INVALID_DB_JOB_KEY;
	SessionId sessionId = INVALID_SESSION_ID;
	UINT batchSize = 0;
};

class DBJobReply : public IGameAndDBPacket
{
public:
	GET_PACKET_ID(PACKET_ID::DB2GAME_BATCHED_DB_JOB);

public:
	DBJobKey jobKey = INVALID_DB_JOB_KEY;
	bool isSuccessed = false;
};

class CallSelectTest2ProcedurePacket : public IGameAndDBPacket
{
public:
	GET_PACKET_ID(PACKET_ID::GAME2DB_SELECT_TEST_2);
	SET_PARAMETERS_TO_BUFFER_DB_PACKET(ownerSessionId, id);

public:
	SessionId ownerSessionId = INVALID_SESSION_ID;
	long long id = 0;
};

class CallSelectTest2ProcedurePacketReply : public IGameAndDBPacket
{
public:
	CallSelectTest2ProcedurePacketReply() = default;
	~CallSelectTest2ProcedurePacketReply() = default;
	GET_PACKET_ID(PACKET_ID::DB2GAME_SELECT_TEST_2);
	SET_PARAMETERS_TO_BUFFER_DB_PACKET(ownerSessionId, no, testString);

public:
	SessionId ownerSessionId = INVALID_SESSION_ID;
	std::list<int> no;
	std::list<std::string> testString;
};

class test : public IGameAndDBPacket
{
public:
	GET_PACKET_ID(PACKET_ID::GAME2DB_TEST);
	SET_PARAMETERS_TO_BUFFER_DB_PACKET(id3, teststring);

public:
	int id3 = 0;
	std::wstring teststring;
};

#pragma endregion DBServer

#pragma pack(pop)

////////////////////////////////////////////////////////////////////////////////////
// Packet Register
////////////////////////////////////////////////////////////////////////////////////

#pragma region PacketHandler
#define REGISTER_PACKET(PacketType){\
	RegisterPacket<PacketType>();\
}

#define REGISTER_HANDLER(PacketType)\
	RegisterPacketHandler<PacketType>();

#define DECLARE_HANDLE_PACKET(PacketType)\
	static bool HandlePacket(RIOTestSession& session, PacketType& packet);\

#define REGISTER_ALL_HANDLER()\
	REGISTER_HANDLER(TestStringPacket)\
	REGISTER_HANDLER(EchoStringPacket)\
	REGISTER_HANDLER(TestProcedurePacket)\
	REGISTER_HANDLER(SelectTest2ProcedurePacket)\
	REGISTER_HANDLER(Ping)\
	REGISTER_HANDLER(RequestFileStream)\
	
#define DECLARE_ALL_HANDLER()\
	DECLARE_HANDLE_PACKET(TestStringPacket)\
	DECLARE_HANDLE_PACKET(EchoStringPacket)\
	DECLARE_HANDLE_PACKET(TestProcedurePacket)\
	DECLARE_HANDLE_PACKET(SelectTest2ProcedurePacket)\
	DECLARE_HANDLE_PACKET(Ping)\
	DECLARE_HANDLE_PACKET(RequestFileStream)\

#define REGISTER_PACKET_LIST(){\
	REGISTER_PACKET(TestStringPacket)\
	REGISTER_PACKET(EchoStringPacket)\
	REGISTER_PACKET(TestProcedurePacket)\
	REGISTER_PACKET(SelectTest2ProcedurePacket)\
	REGISTER_PACKET(Ping)\
	REGISTER_PACKET(RequestFileStream)\
}

#pragma endregion PacketHandler

#pragma region ForDB
#define REGISTER_DB_REPLY_HANDLER(PacketType)\
	RegisterPacketHandler<PacketType>();

#define DECLARE_DB_REPLY_HANDLER(PacketType)\
	static bool AssemblePacket(PacketType& packet, OUT CSerializationBuf& recvPacket);\

#define REGISTER_ALL_DB_REPLY_HANDLER()\
	REGISTER_DB_REPLY_HANDLER(CallSelectTest2ProcedurePacketReply)\
	REGISTER_DB_REPLY_HANDLER(DBJobReply)\

#define DECLARE_ALL_DB_REPLY_HANDLER()\
	DECLARE_DB_REPLY_HANDLER(CallSelectTest2ProcedurePacketReply)\
	DECLARE_DB_REPLY_HANDLER(DBJobReply)\

#pragma endregion ForDB