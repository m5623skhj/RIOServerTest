#include "PreCompile.h"
#include "TestDBJob.h"

DBJob_test::DBJob_test(RIOTestSession& inOwner, IGameAndDBPacket& packet)
	: DBJob(inOwner, packet)
{

}

void DBJob_test::OnCommit()
{

}

void DBJob_test::OnRollback()
{

}
