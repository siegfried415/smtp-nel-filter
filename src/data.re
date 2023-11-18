/*
 * data.re
 * $Id: data.re,v 1.23 2005/12/20 08:30:43 xiay Exp $
 */

#include <stdio.h>
#include <stdlib.h>

#include "mem_pool.h"

#include "smtp.h"
#include "data.h"
#include "command.h"

#ifdef USE_NEL
#include <engine.h>

extern struct nel_eng *eng;
int data_id;
extern int ack_id;
#endif

extern max_smtp_ack_len;
ObjPool_t smtp_cmd_data_pool;

void
smtp_cmd_data_init (
#ifdef USE_NEL
			   struct nel_eng *eng
#endif
	)
{
	create_mem_pool (&smtp_cmd_data_pool,
			 sizeof (struct smtp_cmd_data), SMTP_MEM_STACK_DEPTH);
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
smtp_ack_data_parse (struct smtp_info *psmtp)
{
	struct smtp_ack *ack;
	int r, res;
	int code;
	size_t cur_token = 0;

	char *buf = psmtp->svr_data;
	int len = psmtp->svr_data_len;

	char *p1, *p2, *p3;

	DEBUG_SMTP (SMTP_DBG, "smtp_ack_data_parse... \n");
	DEBUG_SMTP (SMTP_DBG, "smtp_ack_data_parse,buf=%s \n", buf);

	p1 = buf;
	p2 = buf + len;

	cur_token = 0;

	/* *INDENT-OFF* */
	/*!re2c
	"354"
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_data_parse: 354\n");
		code = 354;
		cur_token = 3;
		psmtp->mail_state = SMTP_MAIL_STATE_DATA;
		psmtp->curr_parse_state = SMTP_STATE_PARSE_MESSAGE;
		//goto ack_new;
		goto crlf;
	}
	"421"
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_data_parse: 421\n");
		code = 421;
		cur_token = 3;
		//psmtp->curr_parse_state = SMTP_STATE_PARSE_COMMAND;
		//psmtp->sender = NULL;
		//goto ack_new;
		goto crlf;
	}

	"451"
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_data_parse: 451\n");
		code = 451;
		cur_token = 3;
		//psmtp->curr_parse_state = SMTP_STATE_PARSE_COMMAND;
		//psmtp->sender = NULL;
		//goto ack_new;
		goto crlf;
	}

	"500"
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_data_parse: 500\n");
		code = 500;
		cur_token = 3;
		//psmtp->curr_parse_state = SMTP_STATE_PARSE_COMMAND;
		//psmtp->sender = NULL;
		//goto ack_new;
		goto crlf;
	}

	"501"
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_data_parse: 501\n");
		code = 501;
		cur_token = 3;
		//psmtp->curr_parse_state = SMTP_STATE_PARSE_COMMAND;
		//psmtp->sender = NULL;
		//goto ack_new;
		goto crlf;
	}

	"503"
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_data_parse: 503\n");
		code = 503;
		cur_token = 3;
		//psmtp->curr_parse_state = SMTP_STATE_PARSE_COMMAND;
		//psmtp->sender = NULL;
		//goto ack_new;
		goto crlf;
	}

	"554"
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_data_parse: 554\n");
		code = 554;
		cur_token = 3;
		//psmtp->curr_parse_state = SMTP_STATE_PARSE_COMMAND;
		//psmtp->sender = NULL;
		//goto ack_new;
		goto crlf;
	}


	digit{3,3}
	{
		code = atoi(p1-6);
		cur_token = 3;
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_data_parse: %d\n", code);

		//if (code >= 300) {
		//	psmtp->curr_parse_state = SMTP_STATE_PARSE_COMMAND;
		//	psmtp->sender = NULL;
		//}

		//goto ack_new;
		goto crlf;
	}

	any 
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_data_parse: any\n");
		DEBUG_SMTP(SMTP_DBG, "server response unrecognizable data %s\n", psmtp->svr_data);
		res = SMTP_ERROR_PARSE;
		goto err;
	}
	*/

	/* *INDENT-ON* */
      crlf:
	r = smtp_str_crlf_parse (buf, len, &cur_token);
	if (r != SMTP_NO_ERROR) {
		res = SMTP_ERROR_PARSE;
		goto err;
	}

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
			    "smtp_ack_data_parse: nel_env_analysis error\n");
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

	//psmtp->svr_data += cur_token;
	//psmtp->svr_data_len -= cur_token;
	r = sync_server_data (psmtp, cur_token);
	if (r < 0) {
		//      res = SMTP_ERROR_TCPAPI_WRITE;
		goto err;
	}

	return SMTP_NO_ERROR;

      free:
	smtp_ack_free (ack);

      err:
	reset_server_buf (psmtp, &cur_token);
	return res;

}


void
smtp_cmd_data_free (struct smtp_cmd_data *data)
{
	DEBUG_SMTP (SMTP_MEM, "smtp_cmd_data_free\n");
	free_mem (&smtp_cmd_data_pool, (void *) data);
	DEBUG_SMTP (SMTP_MEM, "smtp_cmd_data_free: pointer=%p, elm=%p\n",
		    &smtp_cmd_data_pool, data);
}

struct smtp_cmd_data *
smtp_cmd_data_new (int len)
{
	struct smtp_cmd_data *data;
	data = (struct smtp_cmd_data *) alloc_mem (&smtp_cmd_data_pool);
	if (data == NULL) {
		return NULL;
	}
	DEBUG_SMTP (SMTP_MEM, "smtp_cmd_data_new: pointer=%p, elm=%p\n",
		    &smtp_cmd_data_pool, data);
	//data->event_type = SMTP_CMD_DATA;
	//data->nel_id = data_id;

	//data->count = 0;
	NEL_REF_INIT (data);

	data->len = len;
	return data;
}

int
smtp_cmd_data_parse (struct smtp_info *psmtp, char *message, size_t length,
		     size_t * index)
{
	struct smtp_cmd_data *data = NULL;
	size_t cur_token = *index;
	int r, res;

	DEBUG_SMTP (SMTP_DBG, "smtp_cmd_data_parse\n");

#if 0
	/* Let NEL do the dirty thing, wyong, 2005.11.8  */
	/*
	 * Sanity checks. With ESMTP command pipelining the client can send DATA
	 * before all recipients are rejected, so don't report that as a protocol
	 * error.
	 */
	if (psmtp->rcpt_count == 0) {
		if (psmtp->sender == NULL) {
			r = reply_to_client (ptcp,
					     "503 Error: need RCPT command\r\n");
			if (r != SMTP_NO_ERROR) {
				res = r;
				goto err;
			}
		}
		else {
			r = reply_to_client (ptcp,
					     "554 Error: no valid recipients\r\n");
			if (r != SMTP_NO_ERROR) {
				res = r;
				goto err;
			}
		}
		res = SMTP_ERROR_PROTO;
		goto err;
	}
#endif

	r = smtp_wsp_unstrict_crlf_parse (message, length, &cur_token);
	if (r != SMTP_NO_ERROR) {
		res = r;
		//if (res == SMTP_ERROR_PARSE || res == SMTP_ERROR_CONTINUE) {
		//      r = reply_to_client(ptcp, "501 DATA Syntax: no CRLF.\r\n");
		//      if (r != SMTP_NO_ERROR) {
		//              res = r;
		//              goto err;
		//      }
		//      res = SMTP_ERROR_PARSE;
		//}
		goto err;
	}

	DEBUG_SMTP (SMTP_DBG, "\n");
	data = smtp_cmd_data_new (length);
	if (data == NULL) {
		DEBUG_SMTP (SMTP_DBG,
			    "smtp_cmd_data_parse:smtp_cmd_data_new error\n");
		res = SMTP_ERROR_MEMORY;
		goto err;
	}

#ifdef USE_NEL
	DEBUG_SMTP (SMTP_DBG, "\n");
	if ((r = nel_env_analysis (eng, &(psmtp->env), data_id,
				   (struct smtp_simple_event *) data)) < 0) {
		DEBUG_SMTP (SMTP_DBG,
			    "smtp_cmd_data_parse: nel_env_analysis error\n");
		res = SMTP_ERROR_ENGINE;
		goto free;
	}
#endif

	DEBUG_SMTP (SMTP_DBG, "\n");
	if (psmtp->permit & SMTP_PERMIT_DENY) {
		//fprintf(stderr, "found a deny event\n");
		//smtp_close_connection(ptcp, psmtp);
		res = SMTP_ERROR_POLICY;
		goto err;

	}
	else if (psmtp->permit & SMTP_PERMIT_DROP) {
		//r = reply_to_client(ptcp, "550 DATA cannot be implemented.\r\n");
		//if (r != SMTP_NO_ERROR) {
		//      res = r;
		//      goto err;
		//}
		res = SMTP_ERROR_POLICY;
		goto err;
	}

	*index = cur_token;
	DEBUG_SMTP (SMTP_DBG, "\n");

	psmtp->last_cli_event_type = SMTP_CMD_DATA;

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
	DEBUG_SMTP (SMTP_DBG, "smtp_cmd_data_parse: Free\n");
	smtp_cmd_data_free (data);

      err:
	DEBUG_SMTP (SMTP_DBG, "smtp_cmd_data_parse: Error\n");
	reset_client_buf (psmtp, index);

	return res;
}
