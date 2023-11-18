
#ifndef TCP_CALLBACK_H
#define TCP_CALLBACK_H

#include "nids.h"
enum
{
	TCP_CONN_NULL = 0,

	//wyong, 20231025 
	TCP_CONN_EST,

	TCP_CONN_DATA,
	TCP_CONN_ASST,
	TCP_CONN_CLOSE,
	TCP_CONN_LOSS,
	TCP_CONN_DUP,
	TCP_CONN_ERROR,
	TCP_CONN_MAX,
	TCP_CONN_TMO
};


void process_tcpdata (struct tcp_stream *a_tcp, void **this_time_not_needed);

#endif
