#pragma once

#include <MSWSock.h>
#include "Ringbuffer.h"
#include "LockFreeQueue.h"
#include "Queue.h"
#include "EnumType.h"
#include "DefineType.h"
#include "NetServerSerializeBuffer.h"
#include <mutex>

class RIOTestServer;
class RIOTestSession;
class IPacket;

struct IOContext : RIO_BUF
{
	IOContext() = default;
	~IOContext() = default;

	//void InitContext(RIOTestSession* inOwnerSession, RIO_OPERATION_TYPE inIOType);
	void InitContext(UINT64 inOwnerSessionId, RIO_OPERATION_TYPE inIOType);

	//RIOTestSession* ownerSession = nullptr;
	UINT64 ownerSessionId = 0;
	RIO_OPERATION_TYPE ioType = RIO_OPERATION_TYPE::OP_ERROR;
};

struct RecvItem
{
	WORD bufferCount = 0;
	CRingbuffer recvRingBuffer;
	RIO_BUFFERID recvBufferId;
};

struct SendItem
{
	WORD bufferCount = 0;
	//NetBuffer* storedBuffer[ONE_SEND_WSABUF_MAX];
	CLockFreeQueue<NetBuffer*> sendQueue;
	//CRingbuffer sendRingBuffer;
	char rioSendBuffer[MAX_SEND_BUFFER_SIZE];
	NetBuffer* reservedBuffer = nullptr;
	RIO_BUFFERID sendBufferId;
	IO_MODE ioMode = IO_MODE::IO_NONE_SENDING;
};

class RIOTestSession
{
	friend RIOTestServer;

public:
	RIOTestSession() = delete;
	explicit RIOTestSession(SOCKET socket, UINT64 sessionId);
	virtual ~RIOTestSession() = default;

private:
	bool InitSession(const RIO_EXTENSION_FUNCTION_TABLE& rioFunctionTable, RIO_NOTIFICATION_COMPLETION& rioNotiCompletion, RIO_CQ& rioRecvCQ, RIO_CQ& rioSendCQ);

public:
	virtual void OnClientEntered() {}
	virtual void OnClientLeaved() {}

public:
	void SendPacket(NetBuffer& packet);
	void SendPacket(IPacket& packet);
	void SendPacketAndDisconnect(NetBuffer& packet);
	void SendPacketAndDisconnect(IPacket& packet);
	void Disconnect();

	void OnRecvPacket(NetBuffer& packet);

private:
	void OnSessionReleased(const RIO_EXTENSION_FUNCTION_TABLE& rioFunctionTable);

private:
	SOCKET socket;
	SessionId sessionId = INVALID_SESSION_ID;
	
	bool sendDisconnect = false;
	bool ioCancle = false;

	bool isReleasedSession = false;

#pragma region IO
private:
	LONG ioCount = 0;

	RecvItem recvItem;
	SendItem sendItem;
	OVERLAPPED postQueueOverlapped;

	RIO_RQ rioRQ = RIO_INVALID_RQ;
	std::mutex rioRQLock;

	ULONG rioRecvOffset = 0;
#pragma endregion IO
};