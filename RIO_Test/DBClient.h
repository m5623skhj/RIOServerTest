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

	// ������ Connect �� �Ϸ� �� ��
	virtual void OnConnectionComplete(UINT64 sessionId);

	// ��Ŷ ���� �Ϸ� ��
	virtual void OnRecv(UINT64 sessionId, CSerializationBuf* OutReadBuf);
	// ��Ŷ �۽� �Ϸ� ��
	virtual void OnSend(UINT64 sessionId);

	// ��Ŀ������ GQCS �ٷ� �ϴܿ��� ȣ��
	virtual void OnWorkerThreadBegin();
	// ��Ŀ������ 1���� ���� ��
	virtual void OnWorkerThreadEnd();
	// ����� ���� ó�� �Լ�
	virtual void OnError(st_Error* OutError);
	// �� ������ �������� ����� ȣ��
	virtual void OnDisconnect(UINT64 sessionId);

private:

};