#pragma once

#include "Ringbuffer.h"
#include "Queue.h"

class CNetServerSerializationBuf;

class RIOTestSession
{
public:
	RIOTestSession() = default;
	virtual ~RIOTestSession() = default;

public:
	void SendPacket(CNetServerSerializationBuf& packet);
	void SendPacketAndDisconnect(CNetServerSerializationBuf& packet);
	void Disconnect();


private:
	SOCKET socket;
	UINT64 sessionID = 0;
	bool sendDisconnect = false;
};