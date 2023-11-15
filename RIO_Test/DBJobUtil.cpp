#include "PreCompile.h"
#include "DBJobUtil.h"

std::shared_ptr<BatchedDBJob> MakeBatchedDBJob(RIOTestSession& inOwner)
{
	auto returnObject = std::make_shared<BatchedDBJob>(inOwner);
	DBJobManager::GetInst().RegisterDBJob(returnObject);

	return returnObject;
}