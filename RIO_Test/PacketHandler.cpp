#include "PreCompile.h"
#include "PacketManager.h"
#include "RIOTestSession.h"
#include "Broadcaster.h"
#include "Protocol.h"
#include "DBClient.h"
#include "../../DBConnector/DBConnector/DBServerProtocol.h"
#include "LanServerSerializeBuf.h"

bool PacketManager::HandlePacket(RIOTestSession& session, TestStringPacket& packet)
{
	UNREFERENCED_PARAMETER(session);
	UNREFERENCED_PARAMETER(packet);

	TestStringPacket sendPacket;
	memcpy(sendPacket.testString, packet.testString, sizeof(packet.testString));

	session.SendPacket(sendPacket);
	//Broadcaster::GetInst().BraodcastToAllSession(sendPacket);

	return true;
}

bool PacketManager::HandlePacket(RIOTestSession& session, EchoStringPacket& packet)
{
	UNREFERENCED_PARAMETER(session);
	UNREFERENCED_PARAMETER(packet);

	return true;
}

bool PacketManager::HandlePacket(RIOTestSession& session, CallTestProcedurePacket& packet)
{
	CSerializationBuf& buffer = *CSerializationBuf::Alloc();
	UINT packetId = DBServerProtocol::PACKET_ID::TEST;
	buffer << packetId << session.GetSessionId() << packet.id3;
	buffer.WriteBuffer((char*)packet.testString, sizeof(packet.testString));

	DBClient::GetInstance().CallProcedure(buffer);

	return true;
}

bool PacketManager::HandlePacket(RIOTestSession& session, CallSelectTest2ProcedurePacket& packet)
{
	CSerializationBuf& buffer = *CSerializationBuf::Alloc();
	UINT packetId = DBServerProtocol::PACKET_ID::SELECT_TEST_2;
	buffer << packetId << session.GetSessionId() << packet.id;

	DBClient::GetInstance().CallProcedure(buffer);

	return true;
}