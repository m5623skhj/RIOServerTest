#pragma once
#include "MultiLanClient.h"

class DBClient : public CMultiLanClient
{
private:
	DBClient();
	virtual ~DBClient();

public:
	static DBClient& GetInstance();
	void Start(const std::wstring& optionFile);

public:
	// ������ Connect �� �Ϸ� �� ��
	virtual void OnConnectionComplete();

	// ��Ŷ ���� �Ϸ� ��
	virtual void OnRecv(CSerializationBuf* OutReadBuf);
	// ��Ŷ �۽� �Ϸ� ��
	virtual void OnSend();

	// ��Ŀ������ GQCS �ٷ� �ϴܿ��� ȣ��
	virtual void OnWorkerThreadBegin();
	// ��Ŀ������ 1���� ���� ��
	virtual void OnWorkerThreadEnd();
	// ����� ���� ó�� �Լ�
	virtual void OnError(st_Error* OutError);
	// �� ������ �������� ����� ȣ��
	virtual void OnDisconnect();

public:
	void CallProcedure(CSerializationBuf& packet);
	void SendPacket(CSerializationBuf& packet);
	void SendPacketToFixedChannel(CSerializationBuf& packet, UINT64 sessionId);

private:

};