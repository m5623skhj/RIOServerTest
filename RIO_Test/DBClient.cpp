#include "PreCompile.h"
#include "DBClient.h"

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