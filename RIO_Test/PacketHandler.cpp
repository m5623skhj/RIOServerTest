#include "PreCompile.h"
#include "PacketManager.h"
#include "RIOTestSession.h"
#include "Broadcaster.h"
#include "Protocol.h"
#include "DBClient.h"
#include "LanServerSerializeBuf.h"
#include "TestDBJob.h"
#include "DBJobUtil.h"

bool PacketManager::HandlePacket(RIOTestSession& session, TestStringPacket& packet)
{
	UNREFERENCED_PARAMETER(session);
	UNREFERENCED_PARAMETER(packet);

	TestStringPacket sendPacket;
	sendPacket.testString = packet.testString;

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
	test t;
	t.id3 = packet.id3;
	t.teststring = packet.testString;

	auto batchJob = MakeBatchedDBJob(session);
	auto job = MakeDBJob<DBJob_test>(session, t, batchJob->GetDBJobKey());
	batchJob->AddDBJob(job);
	batchJob->ExecuteBatchJob();

	return true;
}

bool PacketManager::HandlePacket(RIOTestSession& session, CallSelectTest2ProcedurePacket& packet)
{
	CSerializationBuf& buffer = *CSerializationBuf::Alloc();
	PACKET_ID packetId = PACKET_ID::SELECT_TEST_2;
	buffer << packetId << session.GetSessionId() << packet.id;

	//DBClient::GetInstance().CallProcedure(buffer);

	return true;
}

bool PacketManager::HandlePacket(RIOTestSession& session, Ping& packet)
{
	Pong pong;
	session.SendPacket(pong);

	return true;
}

bool PacketManager::HandlePacket(RIOTestSession& session, RequestFileStream& packet)
{
	UNREFERENCED_PARAMETER(packet);

	return true;
}