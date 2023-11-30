#include "PreCompile.h"
#include "PacketManager.h"
#include "RIOTestSession.h"
#include "Broadcaster.h"
#include "Protocol.h"
#include "DBClient.h"
#include "LanServerSerializeBuf.h"
#include "TestDBJob.h"
#include "DBJobUtil.h"
#include "DBHelper.h"

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

bool PacketManager::HandlePacket(RIOTestSession& session, TestProcedurePacket& packet)
{
	test t;
	t.id3 = packet.id3;
	t.teststring = packet.testString;

	test t_rollback;
	t_rollback.id3 = -1;
	t_rollback.teststring = L"rollbackString";

	auto batchJob = MakeBatchedDBJob(session);
	auto job = MakeDBJob<DBJob_test, test>(session, t, t_rollback);
	batchJob->AddDBJob(job);
	batchJob->ExecuteBatchJob();

	return true;
}

bool PacketManager::HandlePacket(RIOTestSession& session, SelectTest2ProcedurePacket& packet)
{
	CallSelectTest2ProcedurePacket dbPacket;
	dbPacket.id = packet.id;
	dbPacket.ownerSessionId = session.GetSessionId();

	DBClient::GetInstance().SendPacketToFixedChannel(dbPacket, session.GetSessionId());

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