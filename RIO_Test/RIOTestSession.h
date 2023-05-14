#pragma once

#include "Ringbuffer.h"
#include "LockFreeQueue.h"
#include "Queue.h"

enum class IO_MODE : INT8
{
	IO_NONE_SENDING = 0
	, IO_SENDING
};

class NetBuffer;

struct OverlappedForSend : public OVERLAPPED
{
	WORD bufferCount = 0;
	CRingbuffer recvRingBuffer;
};

struct OverlappedForRecv : public OVERLAPPED
{
	IO_MODE nowPostQueuing = IO_MODE::IO_NONE_SENDING;
	WORD bufferCount = 0;
	CLockFreeQueue<NetBuffer*> sendQueue;
};

class RIOTestSession
{
public:
	RIOTestSession() = default;
	virtual ~RIOTestSession() = default;

public:
	void SendPacket(NetBuffer& packet);
	void SendPacketAndDisconnect(NetBuffer& packet);
	void Disconnect();
	void SendPacketAndDisConnect(NetBuffer& pSendPacket);

private:
	SOCKET socket;
	UINT64 sessionID = 0;
	
	bool sendDisconnect = false;
	bool ioCancle = false;

#pragma region IO
private:
	UINT ioCount = 0;

	OverlappedForRecv recvOverlapped;
	OverlappedForSend sendOverlapped;
#pragma endregion IO
};