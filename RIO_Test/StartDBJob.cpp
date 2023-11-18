#include "PreCompile.h"
#include "StartDBJob.h"

DBJob_StartDBJob::DBJob_StartDBJob(RIOTestSession& inOwner, IGameAndDBPacket& packet)
	: DBJob(inOwner, packet)
{

}

void DBJob_StartDBJob::OnCommit()
{

}

void DBJob_StartDBJob::OnRollback()
{

}
