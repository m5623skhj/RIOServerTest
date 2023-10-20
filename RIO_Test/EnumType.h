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
	, OP_SEND_REQUEST
};

enum class IO_MODE : LONG
{
	IO_NONE_SENDING = 0
	, IO_SENDING
};

enum class IO_POST_ERROR : INT8
{
	SUCCESS = 0
	, IS_DELETED_SESSION
	, FAILED_RECV_POST
	, FAILED_SEND_POST
	, INVALID_OPERATION_TYPE
};

enum class PACKET_ID : unsigned int
{
	INVALID_PACKET = 0
	, TEST_STRING_PACKET
	, ECHO_STRING_PACEKT
	, CALL_TEST_PROCEDURE_PACKET
	, CALL_SELECT_TEST_2_PROCEDURE_PACKET
	, CALL_TEST_PROCEDURE_PACKET_REPLY
	, CALL_SELECT_TEST_2_PROCEDURE_PACKET_REPLY
	, PING
	, PONG
};