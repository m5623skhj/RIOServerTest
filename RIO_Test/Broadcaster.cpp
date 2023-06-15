#include "PreCompile.h"
#include "Broadcaster.h"

Broadcaster& Broadcaster::GetInst()
{
	static Broadcaster instance;
	return instance;
}

void Broadcaster::OnSessionEntered()
{

}

void Broadcaster::OnSessionLeaved()
{

}