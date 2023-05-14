#include "PreCompile.h"
#include "RIOTestServer.h"

int main()
{
	RIOTestServer server;
	server.StartServer(L"OptionFile/ServerOption.txt");

	return 0;
}