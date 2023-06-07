#include "PreCompile.h"
#include "RIOTestSession.h"
#include "PacketManager.h"
#include "RIOTestServer.h"

#include "BuildConfig.h"
#include "DefineType.h"
#include "EnumType.h"

void IOContext::InitContext(RIOTestSession* inOwnerSession, RIO_OPERATION_TYPE inIOType)
{
	ownerSession = inOwnerSession;
	ioType = inIOType;
}

RIOTestSession::RIOTestSession(SOCKET inSocket, UINT64 inSessionId)
	: socket(inSocket)
	, sessionId(inSessionId)
{

}

bool RIOTestSession::InitSession(HANDLE iocpHandle, const RIO_EXTENSION_FUNCTION_TABLE& rioFunctionTable, RIO_NOTIFICATION_COMPLETION& rioNotiCompletion, RIO_CQ& rioRecvCQ, RIO_CQ& rioSendCQ)
{
	InterlockedIncrement(&ioCount);

	u_long arg = 1;
	ioctlsocket(socket, FIONBIO, &arg);

	ZeroMemory(static_cast<OVERLAPPED*>(&recvOverlapped), sizeof(OVERLAPPED));
	ZeroMemory(static_cast<OVERLAPPED*>(&sendOverlapped), sizeof(OVERLAPPED));
	ZeroMemory(&postQueueOverlapped, sizeof(postQueueOverlapped));

	recvOverlapped.recvRingBuffer.InitPointer();

	bufferId = rioFunctionTable.RIORegisterBuffer(recvOverlapped.recvRingBuffer.GetBufferPtr(), DEFAULT_RINGBUFFER_MAX);
	if (bufferId == RIO_INVALID_BUFFERID)
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

void RIOTestSession::SendPacket(IPacket& packet)
{
	NetBuffer* buffer = NetBuffer::Alloc();
	if (buffer == nullptr)
	{
		std::cout << "buffer is nullptr" << std::endl;
		return;
	}

	buffer->WriteBuffer((char*)&packet, packet.GetPacketSize());
	SendPacket(*buffer);
}

void RIOTestSession::SendPacket(NetBuffer& packet)
{
	RIOTestServer::GetInst().SendPacket(*this, packet);
}

void RIOTestSession::SendPacketAndDisconnect(IPacket& packet)
{
	NetBuffer* buffer = NetBuffer::Alloc();
	if (buffer == nullptr)
	{
		std::cout << "buffer is nullptr" << std::endl;
		return;
	}

	buffer->WriteBuffer((char*)&packet, packet.GetPacketSize());
	SendPacket(*buffer);
}

void RIOTestSession::SendPacketAndDisconnect(NetBuffer& packet)
{
	sendDisconnect = true;
	RIOTestServer::GetInst().SendPacket(*this, packet);
}

void RIOTestSession::Disconnect()
{

}

void RIOTestSession::OnRecvPacket(NetBuffer& recvPacket)
{
	// packet�� ���̷ε带 IPacket�� ����?
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

	if (packet->GetPacketSize() <= recvPacket.GetUseSize())
	{
		return;
	}

	char* targetPtr = reinterpret_cast<char*>(packet.get()) + sizeof(char*);
	memcpy(targetPtr, recvPacket.GetReadBufferPtr(), recvPacket.GetUseSize());
	std::any anyPacket = std::any(packet.get());
	packetHandler(*this, anyPacket);
}
