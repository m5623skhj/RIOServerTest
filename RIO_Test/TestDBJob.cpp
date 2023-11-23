#include "PreCompile.h"
#include "TestDBJob.h"

DBJob_test::DBJob_test(RIOTestSession& inOwner, test& packet, test& rollbackPacket)
	: DBJob(inOwner, packet, rollbackPacket)
{
	
}

void DBJob_test::OnCommit()
{

}

void DBJob_test::OnRollback()
{
	auto rollbackItem = GetRollbackItem();
	if (rollbackItem == nullptr)
	{
		return;
	}

	test rollback = static_cast<test&>(*rollbackItem);
	// Do rollback to server memory
}
