#include "PreCompile.h"
#include "TestDBJob.h"

DBJob_test::DBJob_test(RIOTestSession& inOwner, IGameAndDBPacket& packet, DBJobKey dbJobKey)
	: DBJob(inOwner, packet, dbJobKey)
{

}

void DBJob_test::OnCommit()
{

}

void DBJob_test::OnRollback()
{

}
