#pragma once
#include <memory>

class CSerializationBuf;
class CNetServerSerializationBuf;
class IPacket;

namespace ProcedureHelper
{
	std::shared_ptr<IPacket> MakeProcedureResponse(OUT CSerializationBuf& recvPacket);
}