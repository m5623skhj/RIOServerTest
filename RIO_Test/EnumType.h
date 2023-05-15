#pragma once

enum class RIO_COMPLETION_KEY_TYPE : INT8
{
	STOP = 0
	, START
};

enum class RIO_OPERATION_TYPE : INT8
{
	OP_ERROR = 0
	, OP_RECV
	, OP_SEND
};

enum class IO_MODE : INT8
{
	IO_NONE_SENDING = 0
	, IO_SENDING
};
