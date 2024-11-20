#include "PreCompile.h"
#include "RIOTestSession.h"
#include "PacketManager.h"
#include "RIOTestServer.h"

#include "BuildConfig.h"
#include "DefineType.h"
#include "EnumType.h"

void IOContext::InitContext(SessionId inOwnerSessionId, RIO_OPERATION_TYPE inIOType)
{
	ownerSessionId = inOwnerSessionId;
	ioType = inIOType;
}

RIOTestSession::RIOTestSession(SOCKET inSocket, SessionId inSessionId, BYTE inThreadId)
	: socket(inSocket)
	, sessionId(inSessionId)
	, threadId(inThreadId)
{
	sendItem.reservedBuffer = nullptr;
}

bool RIOTestSession::InitSession(const RIO_EXTENSION_FUNCTION_TABLE& rioFunctionTable, RIO_CQ& rioRecvCQ, RIO_CQ& rioSendCQ)
{
	u_long arg = 1;
	ioctlsocket(socket, FIONBIO, &arg);

	recvItem.recvRingBuffer.InitPointer();

	recvItem.recvBufferId = rioFunctionTable.RIORegisterBuffer(recvItem.recvRingBuffer.GetBufferPtr(), DEFAULT_RINGBUFFER_MAX);
	if (recvItem.recvBufferId == RIO_INVALID_BUFFERID)
	{
		return false;
	}

	sendItem.sendBufferId = rioFunctionTable.RIORegisterBuffer(sendItem.rioSendBuffer, MAX_SEND_BUFFER_SIZE);
	if (sendItem.sendBufferId == RIO_INVALID_BUFFERID)
	{
		return false;
	}

	rioRQ = rioFunctionTable.RIOCreateRequestQueue(socket, 32, 1, 32, 1, rioRecvCQ, rioSendCQ, &sessionId);
	if (rioRQ == RIO_INVALID_RQ)
	{
		return false;
	}

	return true;
}

void RIOTestSession::SendPacket(IGameAndClientPacket& packet)
{
	NetBuffer* buffer = NetBuffer::Alloc();
	if (buffer == nullptr)
	{
		std::cout << "buffer is nullptr" << std::endl;
		return;
	}

	*buffer << packet.GetPacketId();
	packet.PacketToBuffer(*buffer);

	SendPacket(*buffer);
}

void RIOTestSession::SendPacket(NetBuffer& packet)
{
	RIOTestServer::GetInst().SendPacket(*this, packet);
}

void RIOTestSession::SendPacketAndDisconnect(IGameAndClientPacket& packet)
{
	sendDisconnect = true;
	NetBuffer* buffer = NetBuffer::Alloc();
	if (buffer == nullptr)
	{
		std::cout << "buffer is nullptr" << std::endl;
		return;
	}

	*buffer << packet.GetPacketId();
	packet.PacketToBuffer(*buffer);

	SendPacket(*buffer);
}

void RIOTestSession::SendPacketAndDisconnect(NetBuffer& packet)
{
	sendDisconnect = true;
	RIOTestServer::GetInst().SendPacket(*this, packet);
}

void RIOTestSession::Disconnect()
{
	RIOTestServer::GetInst().Disconnect(sessionId);
}

void RIOTestSession::OnRecvPacket(NetBuffer& recvPacket)
{
	PacketId packetId;
	recvPacket >> packetId;

	auto packetHandler = PacketManager::GetInst().GetPacketHandler(packetId);
	if (packetHandler == nullptr)
	{
		return;
	}
	
	auto packet = PacketManager::GetInst().MakePacket(packetId);
	if (packet == nullptr)
	{
		return;
	}

	std::any anyPacket = std::any(packet.get());
	packetHandler(*this, recvPacket, anyPacket);
}

void RIOTestSession::OnSessionReleased(const RIO_EXTENSION_FUNCTION_TABLE& rioFunctionTable)
{
	rioFunctionTable.RIODeregisterBuffer(recvItem.recvBufferId);
	rioFunctionTable.RIODeregisterBuffer(sendItem.sendBufferId);
}