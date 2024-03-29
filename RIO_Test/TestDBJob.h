#pragma once
#include "DBJob.h"

class DBJob_test : public DBJob
{
public:
	DBJob_test() = delete;
	explicit DBJob_test(RIOTestSession& inOwner, test& packet, test& rollbackPacket);
	virtual ~DBJob_test() {}

	virtual void OnCommit() override;
	virtual void OnRollback() override;
};