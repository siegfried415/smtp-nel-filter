/*
 * rset.re
 * $Id: rset.re,v 1.19 2005/12/20 08:30:43 xiay Exp $
 */


#include "mem_pool.h"

#include "smtp.h"
#include "rset.h"
#include "command.h"

#ifdef USE_NEL
#include "engine.h"
extern struct nel_eng *eng;
int rset_id;
extern int ack_id;
#endif


int var_disable_rset_cmd;
ObjPool_t smtp_cmd_rset_pool;


void
smtp_cmd_rset_init (
#ifdef USE_NEL
			   struct nel_eng *eng
#endif
	)
{
	create_mem_pool (&smtp_cmd_rset_pool,
			 sizeof (struct smtp_cmd_rset), SMTP_MEM_STACK_DEPTH);
#ifdef USE_NEL
	//nel_func_name_call(eng, (char *)&rset_id, "nel_id_of", "rset_req");
#endif
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
smtp_ack_rset_parse ( /* struct neti_tcp_stream *ptcp, */ struct smtp_info
		     *psmtp)
{
	struct smtp_ack *ack;
	int r, res;
	int code;
	size_t cur_token = 0;

	char *p1, *p2, *p3;

	char *buf = psmtp->svr_data;
	int len = psmtp->svr_data_len;

	DEBUG_SMTP (SMTP_DBG, "smtp_ack_rset_parse... \n");

	p1 = buf;
	p2 = buf + len;

	/* *INDENT-OFF* */
	/*!re2c
	"250"
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_rset_parse: 250\n");
		code = 250;
		cur_token = 3;

		//xiayu 2005.12.3 command state dfa
		if (psmtp->mail_state != SMTP_MAIL_STATE_HELO
			/*&& psmtp->mail_state != SMTP_MAIL_STATE_RSET*/)
		{
			smtp_rcpt_reset(psmtp);
			smtp_mail_reset(psmtp);
		}
		if (psmtp->mail_state >= SMTP_MAIL_STATE_HELO) {
			psmtp->mail_state = SMTP_MAIL_STATE_HELO;
		} else {
			psmtp->mail_state = SMTP_MAIL_STATE_ORIGN;
		}

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
			    "smtp_ack_rset_parse: nel_env_analysis error\n");
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
	//psmtp->svr_data += len;
	//psmtp->svr_data_len = 0;
	res = sync_server_data (psmtp, cur_token);
	if (res < 0) {
		goto err;
	}

	return SMTP_NO_ERROR;

      free:
	DEBUG_SMTP (SMTP_DBG, "smtp_ack_rset_parse: Free\n");
	smtp_ack_free (ack);

      err:
	DEBUG_SMTP (SMTP_DBG, "smtp_ack_rset_parse: Error\n");
	reset_server_buf (psmtp, &cur_token);
	return res;
}


void
smtp_cmd_rset_free (struct smtp_cmd_rset *rset)
{
	free_mem (&smtp_cmd_rset_pool, (void *) rset);
	DEBUG_SMTP (SMTP_MEM, "smtp_cmd_rset_free: pointer=%p, elm=%p\n",
		    &smtp_cmd_rset_pool, (void *) rset);
}

struct smtp_cmd_rset *
smtp_cmd_rset_new (int len)
{
	struct smtp_cmd_rset *rset;
	rset = (struct smtp_cmd_rset *) alloc_mem (&smtp_cmd_rset_pool);
	if (rset == NULL) {
		return NULL;
	}
	DEBUG_SMTP (SMTP_MEM, "smtp_cmd_rset_new: pointer=%p, elm=%p\n",
		    &smtp_cmd_rset_pool, (void *) rset);
	//rset->event_type = SMTP_CMD_RSET;
	//rset->nel_id = rset_id;

#ifdef USE_NEL
	//rset->count = 0;
	NEL_REF_INIT (rset);
#endif
	rset->len = len;
	return rset;
}


int
smtp_cmd_rset_parse ( /* struct neti_tcp_stream *ptcp, */ struct smtp_info
		     *psmtp, char *message, size_t length, size_t * index)
{
	struct smtp_cmd_rset *rset = NULL;
	size_t cur_token = *index;
	int r, res;

	DEBUG_SMTP (SMTP_DBG, "smtp_cmd_rset_parse\n");

	r = smtp_wsp_unstrict_crlf_parse (message, length, &cur_token);
	if (r != SMTP_NO_ERROR) {
		DEBUG_SMTP (SMTP_DBG, "\n");
		res = r;
		//if (res == SMTP_ERROR_PARSE || res == SMTP_ERROR_CONTINUE) {
		//      r = reply_to_client(ptcp, "501 RSET Syntax: no CRLF.\r\n");
		//      if (r != SMTP_NO_ERROR) {
		//              res = r;
		//              goto err;
		//      }
		//      res = SMTP_ERROR_PARSE;
		//}
		goto err;
	}

	DEBUG_SMTP (SMTP_DBG, "\n");
	rset = smtp_cmd_rset_new (length);
	if (rset == NULL) {
		DEBUG_SMTP (SMTP_DBG,
			    "smtp_cmd_rset_parse:smtp_cmd_rset_new error\n");
		res = SMTP_ERROR_MEMORY;
		goto err;
	}

#ifdef USE_NEL
	if ((r = nel_env_analysis (eng, &(psmtp->env), rset_id,
				   (struct smtp_simple_event *) rset)) < 0) {
		DEBUG_SMTP (SMTP_DBG,
			    "smtp_cmd_rset_parse: nel_env_analysis error\n");
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
		//r = reply_to_client(ptcp, "550 RSET cannot be implemented.\r\n");
		//if (r != SMTP_NO_ERROR) {
		//      res = r;
		//      goto err;
		//}
		res = SMTP_ERROR_POLICY;
		goto err;
	}

	*index = cur_token;

	psmtp->last_cli_event_type = SMTP_CMD_RSET;

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
	DEBUG_SMTP (SMTP_DBG, "smtp_cmd_rset_parse: Free\n");
	if (rset)
		smtp_cmd_rset_free (rset);

      err:
	DEBUG_SMTP (SMTP_DBG, "smtp_cmd_rset_parse: Error\n");
	reset_client_buf (psmtp, index);
	return res;

}
