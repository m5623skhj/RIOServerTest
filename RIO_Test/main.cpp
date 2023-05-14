#include "PreCompile.h"
#include "RIOTestServer.h"

int main()
{
	RIOTestServer server;
	server.StartServer(L"OptionFile/ServerOption.txt");

	Sleep(10000);
	server.StopServer();

	return 0;
}