#pragma once
#include <memory>
#include "DBJob.h"

template<typename T, typename = std::enable_if_t<std::is_base_of_v<DBJob, T>>>
std::shared_ptr<T> MakeDBJob(RIOTestSession& inOwner, IGameAndDBPacket& packet, DBJobKey dbJobKey)
{
	return std::make_shared<T>(inOwner, packet, dbJobKey);
}

std::shared_ptr<BatchedDBJob> MakeBatchedDBJob(RIOTestSession& inOwner);