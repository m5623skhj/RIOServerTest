#pragma once

#include <MSWSock.h>
#include "Ringbuffer.h"
#include "LockFreeQueue.h"
#include "Queue.h"
#include "EnumType.h"
#include "DefineType.h"
#include "NetServerSerializeBuffer.h"
#include <map>
#include <functional>
#include <any>
#include "Protocol.h"

class RIOTestServer;
class RIOTestSession;
using PacketHandler = std::function<bool(RIOTestSession&, std::any&)>;

struct IOContext : RIO_BUF
{
	IOContext() = default;

	void InitContext(RIOTestSession* inOwnerSession, RIO_OPERATION_TYPE inIOType);

	RIOTestSession* ownerSession = nullptr;
	RIO_OPERATION_TYPE ioType = RIO_OPERATION_TYPE::OP_ERROR;
};

struct OverlappedForRecv : public OVERLAPPED
{
	WORD bufferCount = 0;
	CRingbuffer recvRingBuffer;
};

struct OverlappedForSend : public OVERLAPPED
{
	IO_MODE ioMode = IO_MODE::IO_NONE_SENDING;
	IO_MODE nowPostQueuing = IO_MODE::IO_NONE_SENDING;
	WORD bufferCount = 0;
	NetBuffer* storedBuffer[ONE_SEND_WSABUF_MAX];
	RIO_BUF rioBuffer[ONE_SEND_WSABUF_MAX];
	CLockFreeQueue<NetBuffer*> sendQueue;
};

class RIOTestSession
{
	friend RIOTestServer;

public:
	RIOTestSession() = delete;
	explicit RIOTestSession(SOCKET socket, UINT64 sessionId);
	virtual ~RIOTestSession() = default;

private:
	bool InitSession(HANDLE iocpHandle, const RIO_EXTENSION_FUNCTION_TABLE& rioFunctionTable, RIO_NOTIFICATION_COMPLETION& rioNotiCompletion, RIO_CQ& rioRecvCQ, RIO_CQ& rioSendCQ);

public:
	virtual void OnClientEntered() {}
	virtual void OnClientLeaved() {}

public:
	void SendPacket(NetBuffer& packet);
	void SendPacketAndDisconnect(NetBuffer& packet);
	void Disconnect();
	void SendPacketAndDisConnect(NetBuffer& packet);

	void OnRecvPacket(NetBuffer& packet);

private:
	SOCKET socket;
	SessionId sessionId = INVALID_SESSION_ID;
	
	bool sendDisconnect = false;
	bool ioCancle = false;

#pragma region IO
private:
	LONG ioCount = 0;
	UINT nowPostQueuing = 0;

	OverlappedForRecv recvOverlapped;
	OverlappedForSend sendOverlapped;
	OVERLAPPED postQueueOverlapped;

	RIO_RQ rioRQ = RIO_INVALID_RQ;

	RIO_BUFFERID bufferId;
#pragma endregion IO
};

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