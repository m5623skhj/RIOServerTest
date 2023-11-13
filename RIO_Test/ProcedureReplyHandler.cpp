#include "PreCompile.h"
#include "ProcedureReplyHandler.h"
#include "LanServerSerializeBuf.h"
#include "NetServerSerializeBuffer.h"
#include "PacketManager.h"

void ProcedureReplyHandler::Initialize()
{
	REGISTER_ALL_DB_REPLY_HANDLER();
}

bool ProcedureReplyHandler::AssemblePacket(CallTestProcedurePacketReply& packet, OUT CSerializationBuf& recvPacket)
{
	UNREFERENCED_PARAMETER(packet);
	UNREFERENCED_PARAMETER(recvPacket);

	return true;
}

bool ProcedureReplyHandler::AssemblePacket(CallSelectTest2ProcedurePacketReply& packet, OUT CSerializationBuf& recvPacket)
{
	struct listItem
	{
		std::list<int> noList;
		std::list<std::string> testStringList;
	};
	listItem receivedList;

	recvPacket >> receivedList.noList >> receivedList.testStringList;
	return true;
}

std::shared_ptr<IPacket> ProcedureReplyHandler::MakeProcedureResponse(UINT packetId, OUT CSerializationBuf& recvPacket)
{
	auto packetHandler = ProcedureReplyHandler::GetInst().GetPacketHandler(packetId);
	if(packetHandler == nullptr)
	{
		return nullptr;
	}

	auto packet = PacketManager::GetInst().MakePacket(packetId);
	if (packet == nullptr)
	{
		return nullptr;
	}

	char* targetPtr = reinterpret_cast<char*>(packet.get()) + sizeof(char*);
	memcpy(targetPtr, recvPacket.GetReadBufferPtr(), recvPacket.GetUseSize());
	std::any anyPacket = std::any(packet.get());
	if (packetHandler(anyPacket, recvPacket) == false)
	{
		return nullptr;
	}

	return packet;
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