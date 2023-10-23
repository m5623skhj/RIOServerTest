#include "PreCompile.h"
#include "DBJob.h"
#include "DefineType.h"

ERROR_CODE DBJob::Execute()
{
	// Send to DBServer

	return ERROR_CODE::SUCCESS;
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
	// Send to DBServer start batcheJob

	for (auto& job : jobList)
	{

	}

	// Send to DBServer end batchjob

	return ERROR_CODE::SUCCESS;
}