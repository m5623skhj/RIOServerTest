#pragma once
#include <unordered_map>
#include "Ringbuffer.h"
//#include "Stack.h"
#include "LockFreeStack.h"
#include "LockFreeQueue.h"
#include "ServerCommon.h"
#include <list>

// �ش� ������ �۽��߿� �ִ��� �ƴ���
#define NONSENDING	0
#define SENDING		1
// Recv / Send Post ��ȯ ��
#define POST_RETVAL_ERR_SESSION_DELETED		0
#define POST_RETVAL_ERR						1
#define POST_RETVAL_COMPLETE				2

#define IOCP_ONE_SEND_WSABUF_MAX			200

#define df_RELEASE_VALUE					0x100000000

struct st_Error;

class CSerializationBuf;

class CMultiLanClient
{
public:
	CMultiLanClient();
	virtual ~CMultiLanClient();

	bool Start(const WCHAR* szOptionFileName);
	void Stop();

public:
	bool SendPacket(CSerializationBuf* pSerializeBuf);
	bool SendPacket(CSerializationBuf* pSerializeBuf, UINT64 fixedId);

private:
	bool SendPacketImpl(CSerializationBuf* pSerializeBuf, UINT64 channelId);

public:
	// ������ Connect �� �Ϸ� �� ��
	virtual void OnConnectionComplete() = 0;

	// ��Ŷ ���� �Ϸ� ��
	virtual void OnRecv(CSerializationBuf* OutReadBuf) = 0;
	// ��Ŷ �۽� �Ϸ� ��
	virtual void OnSend() = 0;

	// ��Ŀ������ GQCS �ٷ� �ϴܿ��� ȣ��
	virtual void OnWorkerThreadBegin() = 0;
	// ��Ŀ������ 1���� ���� ��
	virtual void OnWorkerThreadEnd() = 0;
	// ����� ���� ó�� �Լ�
	virtual void OnError(st_Error* OutError) = 0;
	// �� ������ �������� ����� ȣ��
	virtual void OnDisconnect() {}

private:
	struct OVERLAPPEDIODATA
	{
		WORD		wBufferCount;
		OVERLAPPED  Overlapped;
		CRingbuffer RingBuffer;
	};

	struct OVERLAPPED_SEND_IO_DATA
	{
		LONG										lBufferCount;
		UINT										IOMode;
		OVERLAPPED									Overlapped;
		CLockFreeQueue<CSerializationBuf*>			SendQ;
	};

	struct Session
	{
		bool						bIsConnect = false;
		UINT						m_IOCount = 0;
		UINT64						m_SessionId = 0;
		OVERLAPPEDIODATA			m_RecvIOData;
		OVERLAPPED_SEND_IO_DATA		m_SendIOData;
		SOCKET						m_sock;
		CSerializationBuf* pSeirializeBufStore[IOCP_ONE_SEND_WSABUF_MAX];
	};

private:
	void WriteError(int WindowErr, int UserErr);
	
	char RecvPost(Session& session);
	char SendPost(Session& session);

	UINT Worker();
	static UINT __stdcall WorkerThread(LPVOID pLanClient);

	UINT Reconnecter();
	static UINT __stdcall ReconnecterThread(LPVOID pLanClient);

	bool LanClientOptionParsing(const WCHAR* szOptionFileName);

	bool ReleaseSession(Session& session);

private:
	UINT64 GetChannelId();
	UINT64 GetFixedChannelId(UINT64 fixedId);

private:
	BYTE		m_byNumOfWorkerThread;
	BYTE		m_byNumOfUsingWorkerThread;
	bool		m_bIsNagleOn;
	bool		m_bIsReconnect;
	UINT		m_numOfReconnect;
	WORD		m_wPort;
	BYTE		m_byNumOfSession;

	WCHAR		m_IP[16];

	HANDLE		*m_pWorkerThreadHandle;
	HANDLE		m_hWorkerIOCP;
	HANDLE		m_hReconnecterHandle;

	std::atomic<UINT64> m_sessionIdGenerator = 0;
	std::atomic<UINT64> m_channelSelector = 0;

	// sessionList��� ���������, ������ ä���� �� �´� ǥ���ε� �ϴ�
	// �ϴ� sessionList�� Ʋ�� ǥ���� �ƴϹǷ� �̴�� ����
	std::vector<Session*> sessionList;

	CRITICAL_SECTION reconnectListLock;
	std::list<UINT64> sessionReconnectIdList;

	bool stopClient = false;
};
