/*
 * quit.c
 * $Id: quit.re,v 1.16 2005/12/20 08:30:43 xiay Exp $
 */


#include "mem_pool.h"

#include "smtp.h"
#include "quit.h"
#include "command.h"

#ifdef USE_NEL
#include "engine.h"

extern struct nel_eng *eng;
int quit_id;
extern int ack_id;
#endif


ObjPool_t smtp_cmd_quit_pool;
int var_disable_quit_cmd;


void
smtp_cmd_quit_init (
#ifdef USE_NEL
			   struct nel_eng *eng
#endif
	)
{
	create_mem_pool (&smtp_cmd_quit_pool,
			 sizeof (struct smtp_cmd_quit), SMTP_MEM_STACK_DEPTH);
}


#define YYCURSOR  p1
#define YYCTYPE   char
#define YYLIMIT   p2
#define YYMARKER  p3
#define YYRESCAN  yy2
#define YYFILL(n)


/*!re2c
any     = [\000-\377];
print	= [\040-\176];
digit   = [0-9];
letter  = [a-zA-Z];
space	= [\040];
*/


int
smtp_ack_quit_parse (struct smtp_info *psmtp)
{
	struct smtp_ack *ack;
	int r, res;
	int code;
	size_t cur_token = 0;

	char *p1, *p2, *p3;

	char *buf = psmtp->svr_data;
	int len = psmtp->svr_data_len;

	DEBUG_SMTP (SMTP_DBG, "smtp_ack_quit_parse... \n");
	DEBUG_SMTP (SMTP_DBG, "server buf: %s\n", psmtp->svr_data);

	p1 = buf;
	p2 = buf + len;

	/* *INDENT-OFF* */
	/*!re2c
	"221"
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_data_parse: 221\n");
		code = 221;
		cur_token = 3;
		//goto ack_new;
		goto crlf;
	}
	digit{3,3}
	{
		cur_token = 3;
		code = atoi(p1-6);
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_rset_parse: %d\n", code);
		goto crlf;
	}
	any 
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_rset_parse: ANY\n");
		DEBUG_SMTP(SMTP_DBG, "server response unrecognizable data %s\n", psmtp->svr_data);
		res = SMTP_ERROR_PARSE;
		goto err;
	}
	*/
	/* *INDENT-ON* */

      crlf:
	DEBUG_SMTP (SMTP_DBG, "\n");
	r = smtp_str_crlf_parse (buf, len, &cur_token);
	if (r != SMTP_NO_ERROR) {
		res = SMTP_ERROR_PARSE;
		goto err;
	}
	DEBUG_SMTP (SMTP_DBG, "length = %d, cur_token = %d\n", len,
		    cur_token);

      ack_new:
	ack = smtp_ack_new (len, code);
	if (!ack) {
		DEBUG_SMTP (SMTP_DBG, "\n");
		res = SMTP_ERROR_MEMORY;
		goto err;
	}
	DEBUG_SMTP (SMTP_DBG, "ack->code = %d\n", ack->code);

#ifdef USE_NEL
	if ((r = nel_env_analysis (eng, &(psmtp->env), ack_id,
				   (struct smtp_simple_event *) ack)) < 0) {
		DEBUG_SMTP (SMTP_DBG,
			    "smtp_ack_quit_parse: nel_env_analysis error\n");
		res = SMTP_ERROR_ENGINE;
		goto free;
	}
#endif

	/* Fixme: how to deal with the client? */
	if (psmtp->permit & SMTP_PERMIT_DENY) {
		//smtp_close_connection(ptcp, psmtp);
		res = SMTP_ERROR_POLICY;
		goto err;
	}
	else if (psmtp->permit & SMTP_PERMIT_DROP) {
		res = SMTP_ERROR_POLICY;
		goto err;
	}

	DEBUG_SMTP (SMTP_DBG, "len = %d,  cur_token = %d\n", len, cur_token);
	DEBUG_SMTP (SMTP_DBG, "data = %s,  data_len = %d\n", psmtp->svr_data,
		    psmtp->svr_data_len);
	//psmtp->svr_data += cur_token;
	//psmtp->svr_data_len -= cur_token;
	//DEBUG_SMTP(SMTP_DBG, "data-2 = %s\n", psmtp->svr_data -2);
	res = sync_server_data (psmtp, cur_token);
	if (res < 0) {
		goto err;
	}

	return SMTP_NO_ERROR;

      free:
	DEBUG_SMTP (SMTP_DBG, "smtp_ack_quit_parse: Free\n");
	smtp_ack_free (ack);

      err:
	DEBUG_SMTP (SMTP_DBG, "smtp_ack_quit_parse: Error\n");
	reset_server_buf (psmtp, &cur_token);
	return res;

}


void
smtp_cmd_quit_free (struct smtp_cmd_quit *quit)
{
	free_mem (&smtp_cmd_quit_pool, (void *) quit);
	DEBUG_SMTP (SMTP_MEM, "smtp_cmd_quit_free: pointer=%p, elm=%p\n",
		    &smtp_cmd_quit_pool, (void *) quit);
}

struct smtp_cmd_quit *
smtp_cmd_quit_new (int len)
{
	struct smtp_cmd_quit *quit;
	quit = (struct smtp_cmd_quit *) alloc_mem (&smtp_cmd_quit_pool);
	if (quit == NULL) {
		return NULL;
	}
	DEBUG_SMTP (SMTP_MEM, "smtp_cmd_quit_new: pointer=%p, elm=%p\n",
		    &smtp_cmd_quit_pool, (void *) quit);
	//quit->event_type = SMTP_CMD_QUIT;
	//quit->nel_id = quit_id;

#ifdef USE_NEL
	//quit->count = 0;
	NEL_REF_INIT (quit);
#endif
	quit->len = len;
	return quit;

}


int
smtp_cmd_quit_parse (struct smtp_info *psmtp, char *message, size_t length,
		     size_t * index)
{
	struct smtp_cmd_quit *quit = NULL;
	size_t cur_token = *index;
	int r, res;

	DEBUG_SMTP (SMTP_DBG, "smtp_cmd_quit_parse\n");

	r = smtp_wsp_unstrict_crlf_parse (message, length, &cur_token);
	if (r != SMTP_NO_ERROR) {
		res = r;
		//if (res == SMTP_ERROR_PARSE || res == SMTP_ERROR_CONTINUE) {
		//      r = reply_to_client(ptcp, "501 QUIT Syntax: no CRLF.\r\n");
		//      if (r != SMTP_NO_ERROR) {
		//              res = r;
		//              goto err;
		//      }
		//      res = SMTP_ERROR_PARSE;
		//}
		goto err;
	}

	DEBUG_SMTP (SMTP_DBG, "\n");
	quit = smtp_cmd_quit_new (length);
	if (quit == NULL) {
		DEBUG_SMTP (SMTP_DBG,
			    "smtp_cmd_quit_parse:smtp_cmd_quit_new error\n");
		res = SMTP_ERROR_MEMORY;
		goto free;
	}

#ifdef USE_NEL
	if ((r = nel_env_analysis (eng, &(psmtp->env), quit_id,
				   (struct smtp_simple_event *) quit)) < 0) {
		DEBUG_SMTP (SMTP_DBG,
			    "smtp_cmd_quit_parse: nel_env_analysis error\n");
		res = SMTP_ERROR_ENGINE;
		goto free;
	}
#endif

	if (psmtp->permit & SMTP_PERMIT_DENY) {
		//fprintf(stderr, "found a deny event\n");
		//smtp_close_connection(ptcp, psmtp);
		res = SMTP_ERROR_POLICY;
		goto err;

	}
	else if (psmtp->permit & SMTP_PERMIT_DROP) {
		//r = reply_to_client(ptcp, "550 QUIT cannot be implemented.\r\n");
		//if (r != SMTP_NO_ERROR) {
		//      res = r;
		//      goto err;
		//}
		res = SMTP_ERROR_POLICY;
		goto err;
	}

	*index = cur_token;
	psmtp->last_cli_event_type = SMTP_CMD_QUIT;

	//wyong, 20231003
	//psmtp->cli_data += cur_token;
	//psmtp->cli_data_len -= cur_token;
	//r = write_to_server(ptcp, psmtp);
	res = sync_client_data (psmtp, cur_token);
	if (res < 0) {
		goto err;
	}

	return SMTP_NO_ERROR;

      free:
	DEBUG_SMTP (SMTP_DBG, "smtp_cmd_quit_parse: Free\n");
	smtp_cmd_quit_free (quit);

      err:
	DEBUG_SMTP (SMTP_DBG, "smtp_cmd_quit_parse: Error\n");
	reset_client_buf (psmtp, index);
	return res;

}


#if 0
#define SMTP_STRING_SIZE 513

int
mailsmtp_quit (mailsmtp * session)
{
	char command[SMTP_STRING_SIZE];
	int r;

	snprintf (command, SMTP_STRING_SIZE, "QUIT\r\n");
	r = send_command (session, command);
	if (r == -1)
		return MAILSMTP_ERROR_STREAM;
	r = read_response (session);
	if (r == 0)
		return MAILSMTP_ERROR_STREAM;
	mailstream_close (session->stream);
	session->stream = NULL;

	return MAILSMTP_NO_ERROR;
}
#endif
