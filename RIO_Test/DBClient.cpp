#include "PreCompile.h"
#include "DBClient.h"
#include "RIOTestServer.h"
#include "LanServerSerializeBuf.h"
#include "NetServerSerializeBuffer.h"
#include "ProcedureReplyHandler.h"
#include <memory>

DBClient::DBClient()
{
	ProcedureReplyHandler::GetInst().Initialize();
}

DBClient::~DBClient()
{
	Stop();
}

DBClient& DBClient::GetInstance()
{
	static DBClient instance;
	return instance;
}

void DBClient::Start(const std::wstring& optionFile)
{
	if (CMultiLanClient::Start(optionFile.c_str()) == false)
	{
		g_Dump.Crash();
	}
}

void DBClient::OnConnectionComplete()
{

}

void DBClient::OnRecv(CSerializationBuf* OutReadBuf)
{
	CSerializationBuf& buffer = *OutReadBuf;
	UINT recvPacketId = 0;
	UINT64 sessionId;
	buffer >> recvPacketId >> sessionId;

	auto session = RIOTestServer::GetInst().GetSession(sessionId);
	if (session == nullptr)
	{
		return;
	}
	std::shared_ptr<IPacket> packet = ProcedureReplyHandler::GetInst().MakeProcedureResponse(recvPacketId, buffer);
	if (packet == nullptr)
	{
		return;
	}

	session->SendPacket(*packet);
}

void DBClient::OnSend()
{

}

void DBClient::OnWorkerThreadBegin()
{

}

void DBClient::OnWorkerThreadEnd()
{

}

void DBClient::OnError(st_Error* OutError)
{

}

void DBClient::OnDisconnect()
{

}

void DBClient::CallProcedure(CSerializationBuf& packet)
{
	CMultiLanClient::SendPacket(&packet);
}

void DBClient::SendPacket(CSerializationBuf& packet)
{
	CMultiLanClient::SendPacket(&packet);
}

void DBClient::SendPacketToFixedChannel(CSerializationBuf& packet, UINT64 sessionId)
{
	CMultiLanClient::SendPacket(&packet, sessionId);
}