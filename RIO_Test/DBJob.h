#pragma once
#include <list>
#include <memory>
#include "EnumType.h"

class DBJob
{
public:
	DBJob() = default;
	virtual ~DBJob() = default;

public:
	virtual void OnCommit() {}
	virtual void OnRollback() {}

public:
	ERROR_CODE Execute();

private:
	// session을 가지고 있는게 맞는가?
	// 아니면 DBJob을 상속 받은 곳에서 session을 따로 갖도록 하는 것이 맞는가?
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

// job이나, batched job을 보내는 것 까지는 좋은데,
// 이후에 어떻게 받아야 하지?
// 어딘가에 보관하고 있다가 DBServer에서 Job이 완료되었다고 신호가 오면 꺼내다 써야할까?
// ex) map<jobKey, job? batchedJob?> jobMap