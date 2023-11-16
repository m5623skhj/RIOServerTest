#include "PreCompile.h"
#include "DBJob.h"
#include "DBClient.h"
#include "RIOTestSession.h"
#include "DBClient.h"
#include "EnumType.h"
#include "Protocol.h"
#include "DBJobUtil.h"
#include "StartDBJob.h"

#pragma region DBJob
DBJob::DBJob(RIOTestSession& inOwner, IGameAndDBPacket& packet, DBJobKey dbJobKey)
	: owner(inOwner)
{
	if (dbJobKey == INVALID_DB_JOB_KEY)
	{
		printf("Invalid db job parameter");
		g_Dump.Crash();
	}

	CSerializationBuf& buffer = *CSerializationBuf::Alloc();
	buffer << dbJobKey;
	packet.PacketToBuffer(buffer);
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

	DBClient::GetInstance().SendPacketToFixedChannel(*this);
	return true;
}

CSerializationBuf* DBJob::GetJobBuffer()
{
	return jobSPBuffer;
}

BatchedDBJob::BatchedDBJob(RIOTestSession& inOwner)
	: owner(inOwner)
{
	dbJobKey = DBJobManager::GetInst().GetDBJobKey();
}

BatchedDBJob::~BatchedDBJob()
{
	DBJobManager::GetInst().DeregisterDBJob(dbJobKey);
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
	DBJobStart dbJobStartPacket;
	dbJobStartPacket.batchSize = static_cast<UINT>(jobList.size());
	dbJobStartPacket.sessionId = owner.GetSessionId();

	auto batchStartJob = MakeDBJob<DBJob_StartDBJob>(owner, dbJobStartPacket, dbJobKey);
	DBClient::GetInstance().SendPacketToFixedChannel(*batchStartJob);

	for (auto& job : jobList)
	{
		job->ExecuteJob();
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
	std::lock_guard<std::mutex> guardLock(jobMapLock);
	DBJobKey key = jobKey.fetch_add(1);

	jobMap.insert({ key, job });
}

std::shared_ptr<BatchedDBJob> DBJobManager::GetRegistedDBJob(DBJobKey jobKey)
{
	std::lock_guard<std::mutex> guardLock(jobMapLock);
	auto iter = jobMap.find(jobKey);
	if (iter == jobMap.end())
	{
		return nullptr;
	}

	return iter->second;
}

void DBJobManager::DeregisterDBJob(DBJobKey jobKey)
{
	std::lock_guard<std::mutex> guardLock(jobMapLock);
	jobMap.erase(jobKey);
}

DBJobKey DBJobManager::GetDBJobKey()
{
	return jobKey.fetch_add(1);
}
#pragma endregion DBJobManager
