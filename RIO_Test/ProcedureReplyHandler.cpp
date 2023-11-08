#include "PreCompile.h"
#include "ProcedureReplyHandler.h"
#include "LanServerSerializeBuf.h"
#include "NetServerSerializeBuffer.h"
#include "PacketManager.h"

bool CheckPacketSize(OUT IPacket& packet, OUT CSerializationBuf& recvPacket)
{
	return packet.GetPacketSize() > recvPacket.GetUseSize();
}

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
	if (CheckPacketSize(packet, recvPacket) == false)
	{
		return false;
	}

	// 이거 예제가 잘못된듯 한데?
	// DBServer 쪽 패킷 프로시저를 확인해보니 여러개의 결과가 들어오는 형식일텐데,
	// 여기에는 하나로 처리하고 있는 듯
	// 여기에서도 여러개로 처리해야 하는데, 왜 이렇게 되어있는지?
	recvPacket >> packet.no;
	int getLeftSize = recvPacket.GetUseSize();
	recvPacket.ReadBuffer((char*)packet.testString, getLeftSize);

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