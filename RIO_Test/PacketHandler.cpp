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
	// 매번 MakeDBJob() 할 때 GetDBJobKey()를 호출해줘야 하니까 불편한거 같음
	// 어차피 batchJob에 항상 종속된 값이니까, CSerializationBuffer를 상속 받던가 해서
	// DBJobKey 자리를 비워두고 먼저 패킷을 쌓아놓은 다음
	// 이후에 DBJobKey를 채우는 방향으로 하면 어떨까 싶음
	auto job = MakeDBJob<DBJob_test>(session, t);
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