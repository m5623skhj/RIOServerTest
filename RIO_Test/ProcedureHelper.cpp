#include "PreCompile.h"
#include "ProcedureHelper.h"
#include "LanServerSerializeBuf.h"
#include "NetServerSerializeBuffer.h"
#include "PacketManager.h"
#include "Protocol.h"

namespace ProcedureHelper
{
	bool CheckPacketSize(OUT IPacket& packet, OUT CSerializationBuf& recvPacket)
	{
		return packet.GetPacketSize() > recvPacket.GetUseSize();
	}

	bool AssemblePacket(OUT IPacket& packet, OUT CSerializationBuf& recvPacket)
	{
		return false;
	}

	bool AssemblePacket(OUT CallTestProcedurePacketReply& packet, OUT CSerializationBuf& recvPacket)
	{
		return true;
	}

	bool AssemblePacket(OUT CallSelectTest2ProcedurePacketReply& packet, OUT CSerializationBuf& recvPacket)
	{
		if (CheckPacketSize(packet, recvPacket) == false)
		{
			return false;
		}

		recvPacket >> packet.no;
		recvPacket.ReadBuffer((char*)packet.testString, sizeof(packet.testString));

		return true;
	}

	std::shared_ptr<IPacket> MakeProcedureResponse(OUT CSerializationBuf& recvPacket)
	{
		PacketId packetId;
		recvPacket >> packetId;

		auto packet = PacketManager::GetInst().MakePacket(packetId);
		if (packet == nullptr)
		{
			return nullptr;
		}

		if (ProcedureHelper::AssemblePacket(*packet, recvPacket) == false)
		{
			return nullptr;
		}

		return packet;
	}
}