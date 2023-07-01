#include "PreCompile.h"
#include "PacketManager.h"
#include "RIOTestSession.h"
#include "Broadcaster.h"
#include "Protocol.h"
#include "DBClient.h"

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
	UNREFERENCED_PARAMETER(session);
	UNREFERENCED_PARAMETER(packet);

	return true;
}

bool PacketManager::HandlePacket(RIOTestSession& session, CallSelectTest2ProcedurePacket& packet)
{
	UNREFERENCED_PARAMETER(session);
	UNREFERENCED_PARAMETER(packet);

	return true;
}