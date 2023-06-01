#include "PreCompile.h"
#include "RIOTestServer.h"
#include "DeadlockChecker.h"
#include "Protocol.h"

int main()
{
	RIOTestServer server;
	server.StartServer(L"OptionFile/ServerOption.txt");

	REGISTER_PACKET_LIST();

	auto& deadlockChecker = DeadlockChecker::GetInstance();

	while (true)
	{
		Sleep(1000);

		if (GetAsyncKeyState(VK_ESCAPE) & 0x8000)
		{
			server.StopServer();
			break;
		}
	}
	server.StopServer();

	return 0;
}