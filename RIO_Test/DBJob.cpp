#include "PreCompile.h"
#include "DBJob.h"
#include "DefineType.h"

ERROR_CODE DBJob::Execute()
{
	// Send to DB?

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
	for (auto& job : jobList)
	{

	}

	return ERROR_CODE::SUCCESS;
}