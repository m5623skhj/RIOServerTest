// Âü°í : https://github.com/zeliard/RIOTcpServer

#include "PreCompile.h"
#include "RIOTestServer.h"
#include "PacketManager.h"
#include "DeadlockChecker.h"
#include "Protocol.h"

int main()
{
	{
		PacketManager::GetInst().Init();
	}

	RIOTestServer& server = RIOTestServer::GetInst();
	server.StartServer(L"OptionFile/ServerOption.txt");

	REGISTER_PACKET_LIST();

	auto& deadlockChecker = DeadlockChecker::GetInstance();

	while (true)
	{
		Sleep(1000);

		std::cout << server.GetSessionCount() << std::endl;

		if (GetAsyncKeyState(VK_ESCAPE) & 0x8000)
		{
			server.StopServer();
			break;
		}
	}
	server.StopServer();

	return 0;
}