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

private:

};

class BatchedDBJob
{
public:
	BatchedDBJob() = default;
	virtual ~BatchedDBJob() = default;

public:
	ERROR_CODE AddDBJob(std::shared_ptr<DBJob> job);
	ERROR_CODE ExecuteBatchJob();

private:
	std::list<std::shared_ptr<DBJob>> jobList;
};