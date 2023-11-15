#pragma once
#include <list>
#include <memory>
#include "EnumType.h"
#include "DefineType.h"
#include <map>
#include <mutex>
#include "LanServerSerializeBuf.h"
#include "RIOTestSession.h"

class RIOTestSession;
class IGameAndDBPacket;

class DBJob
{
	friend class BatchedDBJob;

public:
	DBJob() = delete;
	explicit DBJob(RIOTestSession& inOwner, IGameAndDBPacket& packet, DBJobKey dbJobKey);
	virtual ~DBJob();

public:
	virtual void OnCommit() = 0;
	virtual void OnRollback() = 0;

public:
	bool ExecuteJob();
	CSerializationBuf* GetJobBuffer();

private:
	RIOTestSession& owner;

private:
	CSerializationBuf* jobSPBuffer = nullptr;
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

	// 관리의 용이성을 위해서 일반 DBJob도 BatchedDBJob에 넣어서 보냄
	std::map<DBJobKey, std::shared_ptr<BatchedDBJob>> jobMap;
	std::mutex jobMapLock;
};