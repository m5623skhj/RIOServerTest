#pragma once
#include <list>
#include <memory>
#include "EnumType.h"
#include "DefineType.h"
#include <map>
#include <mutex>
#include "LanServerSerializeBuf.h"

class RIOTestSession;

class DBJob
{
public:
	DBJob() = delete;
	explicit DBJob(std::shared_ptr<RIOTestSession> inOwner, CSerializationBuf* spBuffer);
	virtual ~DBJob();

public:
	virtual void OnCommit() = 0;
	virtual void OnRollback() = 0;

public:
	bool ExecuteJob();

private:
	std::shared_ptr<RIOTestSession> owner = nullptr;

private:
	CSerializationBuf* jobSPBuffer = nullptr;
};

class BatchedDBJob
{
public:
	BatchedDBJob() = default;
	virtual ~BatchedDBJob() = default;

public:
	ERROR_CODE AddDBJob(std::shared_ptr<DBJob> job);
	ERROR_CODE ExecuteBatchJob();

	void OnCommit();
	void OnRollback();

private:
	std::list<std::shared_ptr<DBJob>> jobList;
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

private:
	std::atomic<DBJobKey> jobKey;

	// 관리의 용이성을 위해서 일반 DBJob도 BatchedDBJob에 넣어서 보냄
	std::map<DBJobKey, std::shared_ptr<BatchedDBJob>> jobMap;
	std::mutex lock;
};
