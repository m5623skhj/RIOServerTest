#pragma once
#include <list>
#include <memory>
#include "EnumType.h"
#include "DefineType.h"
#include <map>
#include <mutex>
#include "LanServerSerializeBuf.h"
#include "RIOTestSession.h"
#include "Protocol.h"
#include <optional>

class RIOTestSession;

class DBJob
{
	friend class BatchedDBJob;

public:
	DBJob() = delete;
	template <typename T, typename = std::enable_if_t<std::is_base_of_v<IGameAndDBPacket, T>>>
	explicit DBJob(RIOTestSession& inOwner, T& packet, T& rollbackPacket)
		: owner(inOwner)
	{
		CSerializationBuf& buffer = *CSerializationBuf::Alloc();
		buffer << packet.GetPacketId();
		jobKeyPosition = buffer.GetWriteBufferPtr();
		buffer.MoveWritePos(sizeof(DBJobKey));
		packet.PacketToBuffer(buffer);

		forRollback = std::make_shared<T>(rollbackPacket);
	}
	virtual ~DBJob();

public:
	virtual void OnCommit() = 0;
	virtual void OnRollback() = 0;

public:
	bool ExecuteJob(DBJobKey batchDBJobKey);
	CSerializationBuf* GetJobBuffer();

private:
	RIOTestSession& owner;

private:
	CSerializationBuf* jobSPBuffer = nullptr;
	char* jobKeyPosition = nullptr;

protected:
	IGameAndDBPacket* GetRollbackItem();

private:
	std::shared_ptr<IGameAndDBPacket> forRollback = nullptr;
};

class BatchedDBJob
{
public:
	BatchedDBJob() = delete;
	explicit BatchedDBJob(RIOTestSession& inOwner);
	virtual ~BatchedDBJob();

public:
	ERROR_CODE AddDBJob(std::shared_ptr<DBJob> job);
	ERROR_CODE ExecuteBatchJob();

	void OnCommit();
	void OnRollback();

private:
	std::list<std::shared_ptr<DBJob>> jobList;
	RIOTestSession& owner;

public:
	DBJobKey GetDBJobKey() { return dbJobKey; }

private:
	DBJobKey dbJobKey = INVALID_DB_JOB_KEY;
};

class GlobalDBJob : public DBJob
{
public:
	GlobalDBJob() = default;
	virtual ~GlobalDBJob() = default;
};

class GlobalBatchedDBJob : public BatchedDBJob
{
public:
	GlobalBatchedDBJob() = default;
	virtual ~GlobalBatchedDBJob() = default;
};

class DBJobManager
{
private:
	DBJobManager() = default;
	~DBJobManager() = default;

	DBJobManager(const DBJobManager&) = delete;
	DBJobManager& operator=(const DBJobManager&) = delete;

public:
	static DBJobManager& GetInst();

public:
	void RegisterDBJob(std::shared_ptr<BatchedDBJob> job);
	std::shared_ptr<BatchedDBJob> GetRegistedDBJob(DBJobKey jobKey);
	void DeregisterDBJob(DBJobKey jobKey);

	DBJobKey GetDBJobKey();

private:
	std::atomic<DBJobKey> jobKey = 1;

	// ������ ���̼��� ���ؼ� �Ϲ� DBJob�� BatchedDBJob�� �־ ����
	std::map<DBJobKey, std::shared_ptr<BatchedDBJob>> jobMap;
	std::mutex jobMapLock;
};