/*
 * $Id: send-from.re,v 1.22 2005/12/20 08:30:43 xiay Exp $
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "mem_pool.h"

#include "smtp.h"
#include "send-from.h"
#include "command.h"

#ifdef USE_NEL
#include "engine.h"
extern struct nel_eng *eng;

int send_id;
extern int ack_id;
#endif

extern int var_helo_required;
//extern int var_smtpd_cmail_limit;

ObjPool_t smtp_cmd_send_pool;

void
smtp_cmd_send_init (
#ifdef USE_NEL
			   struct nel_eng *eng
#endif
	)
{
	create_mem_pool (&smtp_cmd_send_pool,
			 sizeof (struct smtp_cmd_send), SMTP_MEM_STACK_DEPTH);
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
space	= [ ];
*/

int
smtp_ack_send_parse ( /*struct neti_tcp_stream *ptcp, */ struct smtp_info
		     *psmtp)
{
	struct smtp_ack *ack;
	int r, res;
	int code;
	size_t cur_token = 0;

	char *buf = psmtp->svr_data;
	int len = psmtp->svr_data_len;

	char *p1, *p2, *p3;

	DEBUG_SMTP (SMTP_DBG, "smtp_ack_send_parse... \n");

	p1 = buf;
	p2 = buf + len;

	/* *INDENT-OFF* */
	/*!re2c
	"250"
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_send_parse: 250\n");
		code = 250;
		cur_token = 3;
		//goto ack_new;
		goto crlf;
	}
	"355"
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_send_parse: 355\n");
		//Fixme octet-offset need to be parsed here
		code = 355;
		cur_token = 3;
		//goto ack_new;
		goto crlf;
	}
	"421"
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_send_parse: 421\n");
		code = 421;
		cur_token = 3;
		smtp_mailbox_addr_free(psmtp->sender);
		psmtp->sender = NULL;
		//goto ack_new;
		goto crlf;
	}
	"451"
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_send_parse: 451\n");
		code = 451;
		cur_token = 3;
		smtp_mailbox_addr_free(psmtp->sender);
		psmtp->sender = NULL;
		//goto ack_new;
		goto crlf;
	}
	"452"
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_send_parse: 452\n");
		code = 452;
		cur_token = 3;
		smtp_mailbox_addr_free(psmtp->sender);
		psmtp->sender = NULL;
		//goto ack_new;
		goto crlf;
	}
	"500"
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_send_parse: 500\n");
		code = 500;
		cur_token = 3;
		smtp_mailbox_addr_free(psmtp->sender);
		psmtp->sender = NULL;
		//goto ack_new;
		goto crlf;
	}
	"501"
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_send_parse: 501\n");
		code = 501;
		cur_token = 3;
		smtp_mailbox_addr_free(psmtp->sender);
		psmtp->sender = NULL;
		//goto ack_new;
		goto crlf;
	}
	"502"
	{
		//xiayu added this 502 parsing
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_send_parse: 502\n");
		code = 502;
		cur_token = 3;
		smtp_mailbox_addr_free(psmtp->sender);
		psmtp->sender = NULL;
		//goto ack_new;
		goto crlf;
	}
	"503"
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_send_parse: 503\n");
		code = 503;
		cur_token = 3;
		smtp_mailbox_addr_free(psmtp->sender);
		psmtp->sender = NULL;
		//goto ack_new;
		goto crlf;
	}
	"550"
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_send_parse: 550\n");
		code = 550;
		cur_token = 3;
		smtp_mailbox_addr_free(psmtp->sender);
		psmtp->sender = NULL;
		//goto ack_new;
		goto crlf;
	}
	"552"
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_send_parse: 552\n");
		code = 552;
		cur_token = 3;
		smtp_mailbox_addr_free(psmtp->sender);
		psmtp->sender = NULL;
		//goto ack_new;
		goto crlf;
	}
	"553"
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_send_parse: 553\n");
		code = 553;
		cur_token = 3;
		smtp_mailbox_addr_free(psmtp->sender);
		psmtp->sender = NULL;
		//goto ack_new;
		goto crlf;
	}
	digit{3,3}
	{
		cur_token = 3;
		code = atoi(p1-6);
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_send_parse: %d\n", code);

		if (code >= 300) {
			smtp_mailbox_addr_free(psmtp->sender);
			psmtp->sender = NULL;
		}

		//goto ack_new;
		goto crlf;
	}
	any 
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_send_parse: ANY\n");
		DEBUG_SMTP(SMTP_DBG, "server response unrecognizable data %s\n", psmtp->cli_data);
		res = SMTP_ERROR_PARSE;
		goto err;
	}
	*/
	/* *INDENT-ON* */

      crlf:
	DEBUG_SMTP (SMTP_DBG, "p1 : %s\n", p1);
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
			    "smtp_ack_mail_parse: nel_env_analysis error\n");
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
	smtp_ack_free (ack);

      err:
	reset_server_buf (psmtp, &cur_token);
	return res;

}

void
smtp_cmd_send_free (struct smtp_cmd_send *send)
{
	if (send->addr)
		smtp_mailbox_addr_free (send->addr);
	free_mem (&smtp_cmd_send_pool, (void *) send);
	DEBUG_SMTP (SMTP_MEM, "smtp_cmd_send_free: pointer=%p, elm=%p\n",
		    &smtp_cmd_send_pool, (void *) send);
}

struct smtp_cmd_send *
smtp_cmd_send_new (int len, char *addr)
{
	struct smtp_cmd_send *send;

	DEBUG_SMTP (SMTP_DBG, "addr = %s\n", addr);

	send = (struct smtp_cmd_send *) alloc_mem (&smtp_cmd_send_pool);
	if (send == NULL) {
		return NULL;
	}
	DEBUG_SMTP (SMTP_DBG, "\n");
	DEBUG_SMTP (SMTP_MEM, "smtp_cmd_send_new: pointer=%p, elm=%p\n",
		    &smtp_cmd_send_pool, (void *) send);

	DEBUG_SMTP (SMTP_DBG, "\n");
	//send->event_type = SMTP_CMD_SEND;
	//send->nel_id = send_id;

#ifdef USE_NEL
	//send->count = 0;
	NEL_REF_INIT (send);
#endif
	send->len = len;

	if (addr) {
		DEBUG_SMTP (SMTP_DBG, "\n");
		send->addr_len = strlen (addr);
		send->addr = addr;
	}
	else {
		DEBUG_SMTP (SMTP_DBG, "\n");
		send->addr_len = 0;
		send->addr = NULL;
	}
	DEBUG_SMTP (SMTP_DBG, "send->addr = %s\n", send->addr);

	return send;
}

int
smtp_cmd_send_parse (struct smtp_info *psmtp, char *message, size_t length,
		     size_t * index)
{
	struct smtp_cmd_send *send = NULL;
	size_t cur_token = *index;
	char *sender;
	int r, res;

	//char          *verp_delims = 0;
	int key = 0;

	DEBUG_SMTP (SMTP_DBG, "smtp_cmd_send_parse\n");

#if 0				//xiayu 2005.11.22 let engine do the checking
	/*
	 * Sanity checks.
	 * 
	 * XXX 2821 pedantism: Section 4.1.2 says that SMTP servers that 
	 * receive a command in which invalid character codes have been 
	 * employed, and for which there are no other reasons for rejection, 
	 * MUST reject that command with a 501 response. So much for the 
	 * principle of "be liberal in what you accept, be strict in what 
	 * you send".
	 */
//      var_helo_required = 1;
//      if (var_helo_required && psmtp->helo_name == NULL ) {
//              reply_to_client(ptcp, "503 Error: send HELO/EHLO first\r\n");
//              DEBUG_SMTP(SMTP_DBG, "smtp_cmd_send_parse: 503 Error, send HELO/EHLO first\n");
//              res = SMTP_ERROR_POLICY;
//              goto err;
//      }

	if (psmtp->sender) {
		r = reply_to_client (ptcp,
				     "503 Error: nested SEND command\r\n");
		if (r != SMTP_NO_ERROR) {
			res = r;
			goto err;
		}
		res = SMTP_ERROR_POLICY;
		goto err;
	}

	/*
	 * xiayu 2005.10.19
	 * fixme: maybe we'll need this "count/rate control" stuff in the future
	 *
	 * XXX The client event count/rate control must be consistent in its 
	 * use of client address information in connect and disconnect 
	 * events. For now we exclude xclient authorized hosts from event 
	 * count/rate control.
	 if(anvil_clnt
	 && var_smtpd_cmail_limit > 0
	 && anvil_clnt_mail(anvil_clnt, sp->service, sp->addr, &rate) == ANVIL_STAT_OK
	 && rate > var_smtpd_cmail_limit) {

	 //smtpd_chat_reply(state, "421 %s Error: too much mail from %s",
	 //              var_myhostname, state->addr);
	 //msg_warn("Message delivery request rate limit exceeded: %d from %s for service %s",
	 //      rate, state->namaddr, state->service);
	 res = SMTP_ERROR_POLICY;
	 goto free;
	 }
	 */


	/*
	 * "SEND FROM:" ("<>" / Reverse-Path) CRLF
	 */
	if (length < 13) {	/* no parameters at all */
		r = reply_to_client (ptcp,
				     "501 Syntax: SEND FROM: <address>\r\n");
		if (r != SMTP_NO_ERROR) {
			res = r;
			goto err;
		}
		res = SMTP_ERROR_PARSE;
		goto err;
	}

	if (length > 512) {
		r = reply_to_client (ptcp,
				     "503 Protocol: Length of SEND FROM command line is too long\r\n");
		if (r != SMTP_NO_ERROR) {
			res = r;
			goto err;
		}
		res = SMTP_ERROR_PROTO;
		goto err;
	}
#endif

	r = smtp_wsp_parse (message, length, &cur_token);
	//xiayu 2005.11.22
	if (r != SMTP_NO_ERROR && r != SMTP_ERROR_PARSE) {
		res = r;
		//if (res == SMTP_ERROR_CONTINUE) {
		//      r = reply_to_client(ptcp, "501 SEND FROM Syntax error.\r\n");
		//      if (r != SMTP_NO_ERROR) {
		//              res = r;
		//              goto err;
		//      }
		//      res = SMTP_ERROR_PARSE;
		//}
		goto err;
	}

	/* reverse-path */
	r = smtp_reverse_path_parse (message, length, &cur_token, &sender);
	if (r != SMTP_NO_ERROR) {
		res = r;
		//if (res == SMTP_ERROR_PARSE || res == SMTP_ERROR_CONTINUE) {
		//      r = reply_to_client(ptcp, "501 SEND Syntax: reverse-path error.\r\n");
		//      if (r != SMTP_NO_ERROR) {
		//              res = r;
		//              goto err;
		//      }
		//      res = SMTP_ERROR_PARSE;
		//}
		goto err;
	}

	/* NOTE,NOTE,NOTE, code for preparing the list , wyong, 2005.9.19 
	   if (namadr_list_match(mail_from_black_list, psmtp->name, psmtp->addr) != 0 ){
	   DEBUG_SMTP(SMTP_DBG, "smtp_cmd_send_parse: 503 Error, acess denied for black list \n");
	   res = SMTP_ERROR_POLICY;
	   goto free;
	   }
	 */

	r = smtp_wsp_unstrict_crlf_parse (message, length, &cur_token);
	if (r != SMTP_NO_ERROR) {
		res = r;
		//if (res == SMTP_ERROR_PARSE || res == SMTP_ERROR_CONTINUE) {
		//      r = reply_to_client(ptcp, "501 Syntax: \"SEND FROM:\" \"<\"Reverse-Path\">\" CRLF\r\n");
		//      if (r != SMTP_NO_ERROR) {
		//              res = r;
		//              goto free;
		//      }
		//      res = SMTP_ERROR_PARSE;
		//}
		goto free;
	}

#if 0				//xiayu 2005.11.22 let engine do the checking
	psmtp->sender = mailbox_addr_dup (sender);
#endif

	DEBUG_SMTP (SMTP_DBG, "\n");
	send = smtp_cmd_send_new (length, sender);
	if (send == NULL) {
		DEBUG_SMTP (SMTP_DBG,
			    "smtp_cmd_send_parse: smtp_cmd_send_new error\n");
		res = SMTP_ERROR_MEMORY;
		goto free;
	}
	sender = NULL;

#ifdef USE_NEL
	DEBUG_SMTP (SMTP_DBG, "\n");
	/* NOTE,NOTE,NOTE, call the engine here, wyong, 2005.9.15 */
	if ((r = nel_env_analysis (eng, &(psmtp->env), send_id,
				   (struct smtp_simple_event *) send)) < 0) {
		DEBUG_SMTP (SMTP_DBG,
			    "smtp_cmd_send_parse: nel_env_analysis error\n");
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
		//r = reply_to_client(ptcp, "550 SEND cannot be implemented.\r\n");
		//if (r != SMTP_NO_ERROR) {
		//      res = r;
		//      goto err;
		//}
		res = SMTP_ERROR_POLICY;
		goto err;
	}

	DEBUG_SMTP (SMTP_DBG, "length = %d, cur_token = %d\n", length,
		    cur_token);
	*index = cur_token;

	psmtp->last_cli_event_type = SMTP_CMD_SEND;

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
	DEBUG_SMTP (SMTP_DBG, "smtp_cmd_send_parse: at free\n");
	if (psmtp->sender) {
		smtp_mailbox_addr_free (psmtp->sender);
		psmtp->sender = NULL;
	}
	if (sender)
		smtp_mailbox_addr_free (sender);
	if (send)
		smtp_cmd_send_free (send);

      err:
	DEBUG_SMTP (SMTP_DBG, "smtp_cmd_send_parse: at err\n");
	reset_client_buf (psmtp, index);
	return res;

}
