#include "PreCompile.h"
#include "RIOTestSession.h"

#include "BuildConfig.h"
#include "DefineType.h"
#include "EnumType.h"

RIOTestSession::RIOTestSession(SOCKET inSocket, UINT64 inSessionId)
	: socket(inSocket)
	, sessionId(inSessionId)
{

}

bool RIOTestSession::InitSession(HANDLE iocpHandle, const RIO_EXTENSION_FUNCTION_TABLE& rioFunctionTable, RIO_NOTIFICATION_COMPLETION& rioNotiCompletion, RIO_CQ& rioCQ)
{
	InterlockedIncrement(&ioCount);

	ZeroMemory(static_cast<OVERLAPPED*>(&recvOverlapped), sizeof(OVERLAPPED));
	ZeroMemory(static_cast<OVERLAPPED*>(&sendOverlapped), sizeof(OVERLAPPED));
	ZeroMemory(&postQueueOverlapped, sizeof(postQueueOverlapped));

	recvOverlapped.recvRingBuffer.InitPointer();

	rioRQ = rioFunctionTable.RIOCreateRequestQueue(socket, 1, 1, 1, 1, rioCQ, rioCQ, nullptr);
	if (rioRQ == RIO_INVALID_RQ)
	{
		return false;
	}

	return InterlockedDecrement(&ioCount) != 0;
}

void RIOTestSession::SendPacket(NetBuffer& packet)
{

}

void RIOTestSession::SendPacketAndDisconnect(NetBuffer& packet)
{

}

void RIOTestSession::Disconnect()
{

}

void RIOTestSession::SendPacketAndDisConnect(NetBuffer& pSendPacket)
{

}