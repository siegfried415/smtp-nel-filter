#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <elf.h>
#include <setjmp.h>
#include <assert.h>
#include <stdarg.h>
#include <syslog.h>
#include <pcap.h>
#include <config.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
//#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <arpa/inet.h>


#include "mem_pool.h"
#include "tcp.h"
#include "smtp.h"
#include "mime.h"
#include "message.h"
#include "address.h"
#include "auth.h"
#include "helo.h"
#include "ehlo.h"
#include "user.h"
#include "password.h"
#include "mail-from.h"
#include "send-from.h"
#include "soml-from.h"
#include "saml-from.h"
#include "rcpt-to.h"
#include "data.h"
#include "quit.h"
#include "rset.h"
#include "expn.h"
#include "vrfy.h"
#include "turn.h"
#include "from.h"
#include "to.h"
#include "command.h"
#include "body.h" 
#include "disposition.h" 
#include "content-type.h"

#ifdef USE_NEL
#include "engine.h"
#include "analysis.h"

int end_id = 0;
int info_id = 0;
extern struct nel_eng *eng;
#endif

/* for protocol.nel */
int enable_smtp_ack_check = 1;
int max_smtp_ack_multiline_count = 32;
int max_smtp_req_len = 512;
int max_smtp_ack_len = 512;

int var_smtpd_crate_limit;
int var_smtpd_cconn_limit;
int var_smtpd_cmail_limit;
int var_smtpd_crcpt_limit;

int var_disable_turn_cmd;
int var_ipc_timeout;

ObjPool_t smtp_table_pool;
ObjPool_t smtp_ack_pool;


/* FIXME: should define different kind of buffers */
ObjPool_t smtp_string_pool;
ObjPool_t smtp_mailbox_addr_pool;


static int smtp_tbl_size = MAX_SMTPS;
static int max_smtps;
static int smtp_num = 0;
static struct smtp_info *smtp_info_pool;
static struct smtp_info **smtp_info_table;
static struct smtp_info *free_smtps;
static unsigned int app_packet = 0;
static unsigned int app_packet_err = 0;


void
smtp_deny (struct smtp_info *psmtp)
{
	if (psmtp != NULL) {
		psmtp->permit |= SMTP_PERMIT_DENY;
	}
}


/*
 * Function:    smtp_close_connection (struct smtp_info *)
 * Purpose:     close smtp connection
 * Arguments:   psmtp => smtp stream info
 * Return:      void funtion
 */
void
smtp_close_connection (struct smtp_info *psmtp)
{
	DEBUG_SMTP (SMTP_DBG, "smtp_close_connection \n");
	if (psmtp == NULL)
		return;

	if (!(psmtp->connect_state & CLIENT_CLOSED)) {
		psmtp->connect_state |= CLIENT_CLOSED;
		printf ("The Server Side was closed.\n");
	}

	if (!(psmtp->connect_state & SERVER_CLOSED)) {
		psmtp->connect_state |= SERVER_CLOSED;
		printf ("The Client Side was closed.\n");
	}

	free_smtp_info (psmtp);

}



/*
int
reply_to_client (struct smtp_info *psmtp, char *str)
{
	//int len, wrlen;
	//char buffer[256] = "550\r\n";

	//DEBUG_SMTP(SMTP_DBG, "reply_to_client\n");

	//DEBUG_SMTP(SMTP_DBG, "str: %s\n", str);
	//len = strlen(str);

	//wrlen = neti_halfstream_write(ptcp, TCP_CLIENT_SIDE, str, len);
	//if (wrlen < 0) {
	//        DEBUG_SMTP(SMTP_DBG, "wrlen = %d\n", wrlen);
	//        return SMTP_ERROR_TCPAPI_WRITE;
	//}

	//DEBUG_SMTP(SMTP_DBG, "wrlen = %d\n", wrlen);
	return SMTP_NO_ERROR;
}
*/

int
sync_client_data (struct smtp_info *psmtp, int processed_data_len)
{
	int dis, len;

	if (psmtp->cli_data < psmtp->cli_buf
	    || psmtp->cli_data > psmtp->cli_buf + (psmtp->cli_buf_len - 1)
	    || psmtp->cli_data_len < 0
	    || psmtp->cli_data_len > (psmtp->cli_buf_len - 1)
	    || psmtp->cli_data + psmtp->cli_data_len
	    > psmtp->cli_buf + (psmtp->cli_buf_len - 1)) {

		DEBUG_SMTP (SMTP_DBG, "buffer overflow: cli_data_len = %d\n",
			    psmtp->cli_data_len);
		DEBUG_SMTP (SMTP_DBG,
			    "buffer overflow: cli_data - cli_buf = %ld\n",
			    psmtp->cli_data - psmtp->cli_buf);
		return SMTP_ERROR_BUFFER;
	}

	if (processed_data_len > psmtp->cli_data_len) {
		DEBUG_SMTP (SMTP_DBG,
			    "processed_data_len(%d) is larger then cli_data_len(%d)\n",
			    processed_data_len, psmtp->cli_data_len);
		return SMTP_ERROR_BUFFER;
	}

	dis = psmtp->cli_data - psmtp->cli_buf + processed_data_len;
	if (dis == 0) {
		/* no data to sync */
		DEBUG_SMTP (SMTP_DBG, "no data to be sync!\n");
		return SMTP_NO_ERROR;
	}


	/* sync all */
	len = psmtp->cli_data_len - processed_data_len;
	if (len > 0) {
		memmove (psmtp->cli_buf, psmtp->cli_data + processed_data_len,
			 len);
	}
	psmtp->cli_data = psmtp->cli_buf;
	psmtp->cli_data_len = len;


	return SMTP_NO_ERROR;

}


void
reset_client_buf (struct smtp_info *sp, size_t *index)
{
	sp->cli_data = sp->cli_buf;
	sp->cli_data_len = 0;
	*index = 0;
}

int
sync_server_data (struct smtp_info *psmtp, int processed_data_len)
{
	int dis, len;

	if (processed_data_len > psmtp->svr_data_len) {
		DEBUG_SMTP (SMTP_DBG,
			    "processed_data_len(%d) is larger then svr_data_len(%d)\n",
			    processed_data_len, psmtp->cli_data_len);
		return SMTP_ERROR_BUFFER;
	}

	dis = psmtp->svr_data - psmtp->svr_buf + processed_data_len;
	if (dis == 0) {
		/* no data to sync */
		DEBUG_SMTP (SMTP_DBG, "no data to be sync!\n");
		return SMTP_NO_ERROR;
	}


	/* sync all */
	len = psmtp->svr_data_len - processed_data_len;
	if (len > 0) {
		memmove (psmtp->svr_buf, psmtp->svr_data + processed_data_len,
			 len);
	}
	psmtp->svr_data = psmtp->svr_buf;
	psmtp->svr_data_len = len;


	return SMTP_NO_ERROR;
}


void
reset_server_buf (struct smtp_info *sp, size_t *index)
{
	sp->svr_data = sp->svr_buf;
	sp->svr_data_len = 0;
	*index = 0;
}


int
parse_smtp_client (struct smtp_info *psmtp )
{
	if (psmtp->curr_parse_state == SMTP_STATE_UNCERTAIN
	    || psmtp->curr_parse_state == SMTP_STATE_PARSE_COMMAND) {

		DEBUG_SMTP (SMTP_DBG, "Parse command...\n");
		return parse_smtp_command (psmtp);
	}
	else if (psmtp->curr_parse_state == SMTP_STATE_PARSE_MESSAGE) {
		DEBUG_SMTP (SMTP_DBG, "Parse message...\n");
		return parse_smtp_message (psmtp);

	}
	else {
		DEBUG_SMTP (SMTP_DBG, "\n");
		DEBUG_SMTP (SMTP_DBG, "illegal smtp parsing state type \n");
		return SMTP_ERROR_PARSE;
	}

}

int
parse_smtp_server (struct smtp_info *psmtp )
{
	return parse_smtp_ack (psmtp);
}

void
free_smtp_info (struct smtp_info *psmtp)
{
	int hash_index = psmtp->hash_index;
	if (psmtp->cli_buf) {
		free (psmtp->cli_buf);
	}

	if (psmtp->domain) {
		smtp_domain_free (psmtp->domain);
		psmtp->domain = NULL;
	}
	if (psmtp->sender) {
		smtp_mailbox_addr_free (psmtp->sender);
		psmtp->sender = NULL;
	}
	if (psmtp->recipient) {
		smtp_mailbox_addr_free (psmtp->recipient);
		psmtp->recipient = NULL;
	}
	if (psmtp->helo_name) {
		smtp_string_free (psmtp->helo_name);
		psmtp->helo_name = NULL;
	}

	while (psmtp->part_stack_top > 0) {
		pop_part_stack (psmtp);
	}

	free (psmtp->part_stack);
	free (psmtp->auth_allow);
	free (psmtp->cmd_allow);

	if (psmtp->next_node)
		psmtp->next_node->prev_node = psmtp->prev_node;
	if (psmtp->prev_node)
		psmtp->prev_node->next_node = psmtp->next_node;
	else
		smtp_info_table[hash_index] = psmtp->next_node;

#ifdef  USE_NEL
	/* release env last */
	nel_env_cleanup (eng, &(psmtp->env));
#endif

	psmtp->next_free = free_smtps;
	free_smtps = psmtp;
	smtp_num--;
}


struct smtp_info *
new_smtp_info (struct tcp_stream *a_tcp)
{
	struct smtp_info *psmtp;
	struct smtp_info *tolink;
	int hash_index;

	if (smtp_num > max_smtps || !(psmtp = free_smtps)) {
		return (struct smtp_info *) 0;
	}

	free_smtps = psmtp->next_free;
	hash_index = a_tcp->hash_index;
	hash_index %= smtp_tbl_size;

	tolink = smtp_info_table[hash_index];
	memset (psmtp, 0, sizeof (struct smtp_info));
	psmtp->hash_index = hash_index;

	psmtp->saddr = a_tcp->addr.saddr;
	psmtp->sport = a_tcp->addr.source;
	psmtp->daddr = a_tcp->addr.daddr;
	psmtp->dport = a_tcp->addr.dest;

	psmtp->connect_state = 0;
	psmtp->permit = SMTP_PERMIT_FORWARD;
	psmtp->mail_state = SMTP_MAIL_STATE_ORIGN;
	psmtp->encoding = 0;
	psmtp->sender = NULL;
	psmtp->recipient = NULL;


	psmtp->cli_buf_len = SMTP_BUF_LEN + 1;
	psmtp->cli_buf = (unsigned char *) malloc (SMTP_BUF_LEN + 1);
	if (psmtp->cli_buf == NULL) {
		return (struct smtp_info *) 0;
	}

	psmtp->cli_data = psmtp->cli_buf;
	psmtp->cli_data_len = 0;

	psmtp->svr_data = psmtp->svr_buf;
	psmtp->svr_data_len = 0;

	psmtp->part_stack =
		malloc (SMTP_MSG_STACK_MAX_DEPTH * sizeof (struct smtp_part));
	memset ((char *) psmtp->part_stack, '\0',
		SMTP_MSG_STACK_MAX_DEPTH * sizeof (struct smtp_part));
	psmtp->part_stack_top = 0;
	psmtp->new_part_flag = 1;

	psmtp->auth_allow = malloc (SMTP_AUTH_TYPE_NUM);
	memset ((char *) psmtp->auth_allow, '\0', SMTP_AUTH_TYPE_NUM);

	psmtp->cmd_allow = malloc (SMTP_ALLOW_TYPE_NUM * sizeof (int));
	memset ((char *) psmtp->cmd_allow, '\0',
		SMTP_ALLOW_TYPE_NUM * sizeof (int));


#ifdef  USE_NEL
	NEL_REF_INIT (psmtp);
	nel_env_init (eng, &(psmtp->env), info_id, psmtp);
#endif

	psmtp->next_node = tolink;
	psmtp->prev_node = 0;
	if (tolink)
		tolink->prev_node = psmtp;
	smtp_info_table[hash_index] = psmtp;

	smtp_num++;
	return psmtp;
}

struct smtp_info *
search_smtp_info (struct tcp_stream *a_tcp)
{
	struct smtp_info *psmtp = NULL;
	int hash_index = a_tcp->hash_index % smtp_tbl_size;
	psmtp = smtp_info_table[hash_index];

	while (psmtp) {
		if (a_tcp->addr.source == psmtp->sport
		    && a_tcp->addr.dest == psmtp->dport
		    && a_tcp->addr.saddr == psmtp->saddr
		    && a_tcp->addr.daddr == psmtp->daddr) {
			//tp->from_client = 1; 
			break;
		}

		if (a_tcp->addr.source == psmtp->dport
		    && a_tcp->addr.dest == psmtp->dport
		    && a_tcp->addr.saddr == psmtp->daddr
		    && a_tcp->addr.daddr == psmtp->saddr) {
			//tp->from_client = 0; 
			break;
		}
		psmtp = psmtp->next_node;
	}

	return psmtp;
}

int
process_smtp (struct tcp_stream *a_tcp, size_t size, int to_client,
	      int conn_type)
{
	int used_len, left_len;
	int id;
	int ret = -1;
	struct smtp_info *psmtp = NULL;

	app_packet++;
	if (app_packet >= 100000) {
		printf ("process_smtp, app_count =%d, smtp_num=%d\n",
			app_packet, smtp_num);
		app_packet = 0;
	}

	if (conn_type == TCP_CONN_EST) {
		//printf("---> create smtp info(hi=%p)\n", si);
		if (!(psmtp = new_smtp_info (a_tcp))) {
			return -1;
		}
	}

	if (!(psmtp = search_smtp_info (a_tcp))) {
		//if don't find smtp_info, return
		return -1;
	}


	switch (conn_type) {
	case TCP_CONN_EST:
		ret = 0;
		break;

	case TCP_CONN_CLOSE:
		//printf("--->CLOSE Event!\n");
		//type = TOKEN_CLOSE;

		//todo, don't forget call nel_env_analysis with end_id
		//id = end_id;
		smtp_close_connection (psmtp);
		ret = 0;
		break;

	case TCP_CONN_DATA:
		//libnids fires data when half_stream recevied data, but we 'd like to parse 
		//smtp data from point of view of sending half_stream, so here we exchange 
		//client and server. 
		if (size > 0) {
			if (to_client) {
				used_len =
					psmtp->svr_data - psmtp->svr_buf +
					psmtp->svr_data_len;
				left_len = SMTP_BUF_LEN - used_len;

				if (left_len < size) {
					printf ("smtp_info svr_data is not large enough to hold incoming data!\n");
					return -1;
				}
				memcpy (psmtp->svr_data + psmtp->svr_data_len,
					a_tcp->client.data, size);
				psmtp->svr_data_len += size;
				psmtp->svr_data[psmtp->svr_data_len] = '\0';

			}
			else {
				used_len =
					psmtp->cli_data - psmtp->cli_buf +
					psmtp->cli_data_len;
				left_len =
					(psmtp->cli_buf_len - 1) - used_len;

				if (left_len < size) {
					printf ("smtp_info cli_data is not large enough to hold incoming data!\n");
					return -1;
				}
				memcpy (psmtp->cli_data + psmtp->cli_data_len,
					a_tcp->server.data, size);
				psmtp->cli_data_len += size;
				psmtp->cli_data[psmtp->cli_data_len] = '\0';
			}

			//parse smtp command, message or ack. 
			ret = to_client ? parse_smtp_server (psmtp) :
				parse_smtp_client (psmtp);
			if (ret < 0) {
				smtp_close_connection (psmtp);
			}
		}
		break;

	case TCP_CONN_ASST:
	case TCP_CONN_DUP:
		//printf("--->process_smtp(app_packet=%d, len=%d, from_client=%d, dataptr=%s)\n", app_packet, tp->dlen, tp->from_client, tp->dataptr);
		//id = data_id; 
		break;

	case TCP_CONN_LOSS:
		//printf("--->%d,LOSS Event!\n", data_count);
		//type = TOKEN_LOSS_DATA;
		//id = data_id;
		break;

	case TCP_CONN_ERROR:
	default:
		//printf("--->UNKNOWN event!\n");
		ret = -1;
	}

      __end:
	//printf("--->process_smtp end ---------\n\n\n" );
	return ret;
}

void
smtp_cleanup (void)
{
	if (smtp_info_table)
		free (smtp_info_table);

	if (smtp_info_pool)
		free (smtp_info_pool);

	//printf("smtp packet stat: %u err of %u packet", app_packet_err, app_packet);

	return;
}

int
smtp_init (void)
{
	int i;
	int ret = -1;

	smtp_info_table = malloc (smtp_tbl_size * sizeof (char *));
	if (!smtp_info_table) {
		printf ("no memory in smtp init\n");
		return -1;
	}
	memset (smtp_info_table, 0, smtp_tbl_size * sizeof (char *));

	max_smtps = 3 * smtp_tbl_size / 4;
	smtp_info_pool =
		(struct smtp_info *) malloc ((max_smtps + 1) *
					     sizeof (struct smtp_info));
	if (!smtp_info_pool) {
		printf ("no memory in smtp init\n");
		return -1;
	}
	for (i = 0; i < max_smtps; i++)
		smtp_info_pool[i].next_free = &(smtp_info_pool[i + 1]);
	smtp_info_pool[max_smtps].next_free = 0;
	free_smtps = smtp_info_pool;

	//release_smtp_event (NULL);
	//release_smtp_stream_buf (NULL);
	//release_smtp_msg_text (NULL);

#ifdef USE_NEL
	smtp_cmd_helo_init (eng);
	smtp_cmd_ehlo_init (eng);
	smtp_cmd_auth_init (eng);
	smtp_cmd_user_init (eng);
	smtp_cmd_passwd_init (eng);
	smtp_cmd_mail_init (eng);
	smtp_cmd_send_init (eng);
	smtp_cmd_soml_init (eng);
	smtp_cmd_saml_init (eng);
	smtp_cmd_rcpt_init (eng);
	smtp_cmd_vrfy_init (eng);
	smtp_cmd_expn_init (eng);
	smtp_cmd_data_init (eng);
	smtp_cmd_quit_init (eng);
	smtp_cmd_rset_init (eng);

	smtp_msg_from_init (eng);
	smtp_msg_to_init (eng);

	smtp_mime_content_type_init (eng);
	smtp_mime_disposition_init (eng);

	smtp_mime_body_init (eng);
#else
	smtp_cmd_helo_init ();
	smtp_cmd_ehlo_init ();
	smtp_cmd_auth_init ();
	smtp_cmd_user_init ();
	smtp_cmd_passwd_init ();
	smtp_cmd_mail_init ();
	smtp_cmd_send_init ();
	smtp_cmd_soml_init ();
	smtp_cmd_saml_init ();
	smtp_cmd_rcpt_init ();
	smtp_cmd_vrfy_init ();
	smtp_cmd_expn_init ();
	smtp_cmd_data_init ();
	smtp_cmd_quit_init ();
	smtp_cmd_rset_init ();

	smtp_msg_from_init ();
	smtp_msg_to_init ();

	smtp_mime_content_type_init ();
	smtp_mime_disposition_init ();

	smtp_mime_body_init ();

#endif

	/* FIXME: should assign different kind of buffers */
	create_mem_pool (&smtp_string_pool,
			 SMTP_STRING_BUF_LEN * sizeof (char), 512);
	create_mem_pool (&smtp_mailbox_addr_pool,
			 SMTP_MAILBOX_ADDR_BUF_LEN * sizeof (char), 512);
	DEBUG_SMTP (SMTP_DBG, "&smtp_mailbox_addr_pool=%p\n",
		    &smtp_mailbox_addr_pool);

	create_mem_pool (&smtp_table_pool, sizeof (struct smtp_info),
			 SMTP_INFO_STACK_DEPTH);

	//create_mem_pool (&smtp_msg_text_pool, sizeof (struct smtp_msg_text), SMTP_MEM_STACK_DEPTH);
	//create_mem_pool (&smtp_stream_buf_pool, SMTP_CONTENT_BUF_LEN * sizeof(unsigned char), SMTP_MEM_STACK_DEPTH);
	//create_mem_pool (&smtp_boundary_pool, SMTP_BOUNDARY_BUF_LEN * sizeof(char), 512);

	create_mem_pool (&smtp_ack_pool, sizeof (struct smtp_ack),
			 SMTP_MEM_STACK_DEPTH);

	return 0;
}
