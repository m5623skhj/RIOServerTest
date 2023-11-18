#pragma once
#include "DBJob.h"

class DBJob_StartDBJob : public DBJob
{
public:
	DBJob_StartDBJob() = delete;
	explicit DBJob_StartDBJob(RIOTestSession& inOwner, IGameAndDBPacket& packet);
	virtual ~DBJob_StartDBJob() {}

	virtual void OnCommit() override;
	virtual void OnRollback() override;
};