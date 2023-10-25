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
	// session�� ������ �ִ°� �´°�?
	// �ƴϸ� DBJob�� ��� ���� ������ session�� ���� ������ �ϴ� ���� �´°�?
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

// job�̳�, batched job�� ������ �� ������ ������,
// ���Ŀ� ��� �޾ƾ� ����?
// ��򰡿� �����ϰ� �ִٰ� DBServer���� Job�� �Ϸ�Ǿ��ٰ� ��ȣ�� ���� ������ ����ұ�?
// ex) map<jobKey, job? batchedJob?> jobMap