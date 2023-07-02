#include "PreCompile.h"
#include "DBClient.h"
#include "RIOTestServer.h"
#include "LanServerSerializeBuf.h"
#include "NetServerSerializeBuffer.h"
#include "ProcedureHelper.h"
#include <memory>

DBClient::DBClient()
{
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
	UINT64 sessionId;
	buffer >> sessionId;

	auto session = RIOTestServer::GetInst().GetSession(sessionId);
	if (session == nullptr)
	{
		return;
	}
	std::shared_ptr<IPacket> packet = ProcedureHelper::MakeProcedureResponse(buffer);

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