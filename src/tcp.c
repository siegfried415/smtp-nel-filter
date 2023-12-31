#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <arpa/inet.h>

#include "nids.h"
#include "tcp.h"
#include "smtp.h"

void
process_tcpdata (struct tcp_stream *a_tcp, void **this_time_not_needed)
{
	int stat = 0, dest;
	struct half_stream *hlf;
	int to_client;

	switch (a_tcp->nids_state) {
	case NIDS_JUST_EST:
		if (a_tcp->addr.dest == 25) {
			a_tcp->server.collect++;
			a_tcp->client.collect++;

			stat = process_smtp (a_tcp, 0, 0, TCP_CONN_EST);
		}
		return;

	case NIDS_CLOSE:
		// connection has been closed normally
		stat = process_smtp (a_tcp, 0, 0, TCP_CONN_CLOSE);
		return;

	case NIDS_RESET:
		// connection has been closed by RST
		stat = process_smtp (a_tcp, 0, 0, TCP_CONN_CLOSE);
		return;

	case NIDS_DATA:
		// new data has arrived; gotta determine in what direction
		// and if it's urgent or not
		hlf = (struct half_stream *) 0;
		to_client = 0;

		// We don't have to check if urgent data to client has arrived,
		// because we haven't increased a_tcp->client.collect_urg variable.
		// So, we have some normal data to take care of.
		if (a_tcp->client.count_new) {
			// new data for client
			to_client = 1;
			hlf = &a_tcp->client;	// from now on, we will deal with hlf var,
		}

		if (a_tcp->server.count_new) {
			to_client = 0;
			hlf = &a_tcp->server;	// analogical
		}

		stat = process_smtp (a_tcp, hlf->count_new, to_client,
				     TCP_CONN_DATA);
		return;


	default:
		return;
	}

	return;
}
