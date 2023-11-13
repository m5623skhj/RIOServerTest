#include "PreCompile.h"
#include "ProcedureReplyHandler.h"
#include "LanServerSerializeBuf.h"
#include "NetServerSerializeBuffer.h"
#include "PacketManager.h"
#include "RIOTestServer.h"

void ProcedureReplyHandler::Initialize()
{
	REGISTER_ALL_DB_REPLY_HANDLER();
}

void ProcedureReplyHandler::SPReplyHandle(UINT packetId, OUT CSerializationBuf& recvPacket)
{
	auto packetHandler = ProcedureReplyHandler::GetInst().GetPacketHandler(packetId);
	if(packetHandler == nullptr)
	{
		return;
	}

	auto packet = PacketManager::GetInst().MakePacket(packetId);
	if (packet == nullptr)
	{
		return;
	}

	char* targetPtr = reinterpret_cast<char*>(packet.get()) + sizeof(char*);
	memcpy(targetPtr, recvPacket.GetReadBufferPtr(), recvPacket.GetUseSize());
	std::any anyPacket = std::any(packet.get());
	packetHandler(anyPacket, recvPacket);
}

DBPacketReplyHandler ProcedureReplyHandler::GetPacketHandler(UINT packetId)
{
	auto iter = packetHandlerMap.find(packetId);
	if (iter == packetHandlerMap.end())
	{
		return nullptr;
	}

	return iter->second;
}

bool ProcedureReplyHandler::AssemblePacket(CallTestProcedurePacketReply& packet, OUT CSerializationBuf& recvPacket)
{
	UNREFERENCED_PARAMETER(packet);
	UNREFERENCED_PARAMETER(recvPacket);

	return true;
}

bool ProcedureReplyHandler::AssemblePacket(CallSelectTest2ProcedurePacketReply& packet, OUT CSerializationBuf& recvPacket)
{
	SessionId ownerSessionId;
	recvPacket >> ownerSessionId;
	auto session = RIOTestServer::GetInst().GetSession(ownerSessionId);
	if (session == nullptr)
	{
		return false;
	}
	
	struct listItem
	{
		std::list<int> noList;
		std::list<std::string> testStringList;
	};
	listItem receivedList;

	recvPacket >> receivedList.noList >> receivedList.testStringList;
	auto sendBuffer = NetBuffer::Alloc();
	*sendBuffer << receivedList.noList << receivedList.testStringList;
	session->SendPacket(*sendBuffer);

	return true;
}

bool ProcedureReplyHandler::AssemblePacket(DBJobReply& packet, OUT CSerializationBuf& recvPacket)
{
	SessionId ownerSessionId;
	recvPacket >> ownerSessionId;
	auto session = RIOTestServer::GetInst().GetSession(ownerSessionId);
	if (session == nullptr)
	{
		return false;
	}



	return true;
}
