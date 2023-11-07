#include "PreCompile.h"
#include "DBJob.h"
#include "DBClient.h"
#include "RIOTestSession.h"
#include "DBClient.h"

#pragma region DBJob
DBJob::DBJob(std::shared_ptr<RIOTestSession> inOwner, CSerializationBuf* spBuffer)
{
	if (spBuffer == nullptr || inOwner == nullptr)
	{
		printf("Invalid sp buffer");
		g_Dump.Crash();
	}

	owner = inOwner;
	jobSPBuffer = spBuffer;
}

DBJob::~DBJob()
{
	CSerializationBuf::Free(jobSPBuffer);
}

bool DBJob::ExecuteJob()
{
	if (jobSPBuffer == nullptr)
	{
		return false;
	}

	DBClient::GetInstance().CallProcedure(*jobSPBuffer);
	return true;
}

ERROR_CODE BatchedDBJob::AddDBJob(std::shared_ptr<DBJob> job)
{
	if (jobList.size() >= MAXIMUM_BATCHED_DB_JOB_SIZE)
	{
		return ERROR_CODE::BATCHED_DB_JOB_SIZE_OVERFLOWED;
	}
	jobList.push_back(job);

	return ERROR_CODE::SUCCESS;
}

ERROR_CODE BatchedDBJob::ExecuteBatchJob()
{
	int batchedNo = 0;
	for (auto& job : jobList)
	{
		job->batchedNo = batchedNo;
		job->ExecuteJob();

		++batchedNo;
	}

	return ERROR_CODE::SUCCESS;
}

void BatchedDBJob::OnCommit()
{
	for (auto& job : jobList)
	{
		job->OnCommit();
	}
}

void BatchedDBJob::OnRollback()
{
	for (auto& job : jobList)
	{
		job->OnRollback();
	}
}
#pragma endregion DBJob

#pragma region DBJobManager
DBJobManager& DBJobManager::GetInst()
{
	static DBJobManager instance;
	return instance;
}

void DBJobManager::RegisterDBJob(std::shared_ptr<BatchedDBJob> job)
{
	std::lock_guard<std::mutex> guardLock(lock);
	DBJobKey key = jobKey.fetch_add(1);

	jobMap.insert({ key, job });
}

std::shared_ptr<BatchedDBJob> DBJobManager::GetRegistedDBJob(DBJobKey jobKey)
{
	std::lock_guard<std::mutex> guardLock(lock);
	auto iter = jobMap.find(jobKey);
	if (iter == jobMap.end())
	{
		return nullptr;
	}

	return iter->second;
}
#pragma endregion DBJobManager
