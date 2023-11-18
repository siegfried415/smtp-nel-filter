#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>

//#include <netinet/ip.h>
//#include <netinet/tcp.h>
//#include <netinet/ip_icmp.h>
//#include <time.h>

//#include "checksum.h"
//#include "util.h"
//#include "main.h"
//#include "hash.h"
//#include "nerrno.h"
//#include "tcptrack.h"

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
	//char buf[1024];
	struct half_stream *hlf;
	int to_client;

	//strcpy (buf, adres (a_tcp->addr));

	switch (a_tcp->nids_state) {
	case NIDS_JUST_EST:
		if (a_tcp->addr.dest == 25) {
			a_tcp->server.collect++;
			a_tcp->client.collect++;

			//print_msg("%s established\n", buf);

			//wyong, 20231025 
			stat = process_smtp (a_tcp, 0, 0, TCP_CONN_EST);
		}
		return;

	case NIDS_CLOSE:
		// connection has been closed normally
		stat = process_smtp (a_tcp, 0, 0, TCP_CONN_CLOSE);

		//print_msg("%s closing\n", buf);
		return;

	case NIDS_RESET:
		// connection has been closed by RST
		stat = process_smtp (a_tcp, 0, 0, TCP_CONN_CLOSE);

		//print_msg("%s reset\n", buf);
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
			// which will point to client side of conn
			//strcat (buf, "(<-)"); // symbolic direction of data
			//printf("### tcp_ptoro=%d\n", a_tcp->proto_type);

			/* wyong, 20230912 
			   if (a_tcp->proto_type == TCP_PROTO_NONE) {
			   if(a_tcp->client.count_new < PROTO_MINLEN)
			   return;
			   a_tcp->proto_type = readkey(a_tcp->client.data, a_tcp->client.count_new);
			   //printf("### tcp_ptoro=%d\n", a_tcp->proto_type);
			   if(a_tcp->proto_type == TCP_PROTO_NONE) {
			   a_tcp->client.collect--;
			   a_tcp->server.collect--;
			   }
			   }
			 */
		}

		if (a_tcp->server.count_new) {
			to_client = 0;
			hlf = &a_tcp->server;	// analogical

			/* wyong, 20230912 
			   //strcat (buf, "(->)");
			   if (a_tcp->proto_type == TCP_PROTO_NONE) {
			   if(a_tcp->server.count_new < PROTO_MINLEN)
			   return;
			   stat = readkey(a_tcp->server.data, a_tcp->server.count_new);
			   if (!stat)
			   return;
			   else {
			   if(a_tcp->proto_type == TCP_PROTO_NONE) {
			   a_tcp->client.collect--;
			   a_tcp->server.collect--;
			   }
			   }
			   }
			 */
		}

		//wyong, 20231113 
		stat = process_smtp (a_tcp, hlf->count_new, to_client,
				     TCP_CONN_DATA);

		//if(stat) 
		//      print_msg("%s output failed.\n", buf);
		//else
		//      print_msg("%s output success.\n", buf);
		return;


	default:
		//print_msg("%s process_tcpdata, unknown nids state.\n");
		return;
	}

	return;
}
