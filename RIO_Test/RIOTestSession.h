#pragma once

#include <MSWSock.h>
#include "Ringbuffer.h"
#include "LockFreeQueue.h"
#include "Queue.h"
#include "EnumType.h"

class NetBuffer;
class RIOTestServer;

struct OverlappedForRecv : public OVERLAPPED
{
	WORD bufferCount = 0;
	CRingbuffer recvRingBuffer;
};

struct OverlappedForSend : public OVERLAPPED
{
	IO_MODE nowPostQueuing = IO_MODE::IO_NONE_SENDING;
	WORD bufferCount = 0;
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
	bool InitSession(HANDLE iocpHandle, const RIO_EXTENSION_FUNCTION_TABLE& rioFunctionTable, RIO_NOTIFICATION_COMPLETION& rioNotiCompletion, RIO_CQ& rioCQ);

public:
	void SendPacket(NetBuffer& packet);
	void SendPacketAndDisconnect(NetBuffer& packet);
	void Disconnect();
	void SendPacketAndDisConnect(NetBuffer& pSendPacket);

private:
	SOCKET socket;
	UINT64 sessionId = 0;
	
	bool sendDisconnect = false;
	bool ioCancle = false;

#pragma region IO
private:
	UINT ioCount = 0;

	OverlappedForRecv recvOverlapped;
	OverlappedForSend sendOverlapped;
	OVERLAPPED postQueueOverlapped;

	RIO_RQ rioRQ = RIO_INVALID_RQ;
#pragma endregion IO
};