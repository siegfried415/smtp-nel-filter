/*
 * $Id: mail-from.re,v 1.32 2005/12/16 10:05:25 xiay Exp $
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "engine.h"
#include "mem_pool.h"

#include "smtp.h"
#include "mail-from.h"
#include "command.h"


#ifdef USE_NEL
#include "engine.h"

extern struct nel_eng *eng;
int mail_id;
extern int ack_id;
#endif

//#include "anvil_clnt.h"
//#include "namadr_list.h"

//extern ANVIL_CLNT *anvil_clnt;
//extern NAMADR_LIST *hogger_list;
//extern NAMADR_LIST *mail_from_black_list;

ObjPool_t smtp_cmd_mail_pool;

extern int var_helo_required;
//extern int var_smtpd_cmail_limit;



void
smtp_cmd_mail_init (
#ifdef USE_NEL
			   struct nel_eng *eng
#endif
	)
{
	create_mem_pool (&smtp_cmd_mail_pool,
			 sizeof (struct smtp_cmd_mail), SMTP_MEM_STACK_DEPTH);
#ifdef USE_NEL
	//nel_func_name_call(eng, (char *)&mail_id, "nel_id_of", "mail_req");
#endif
}


int
smtp_mail_size_parse (char *message, size_t length, int *digit_len,
		      int *digit_val)
{
	int i;
	for (i = 0; i < length; i++) {
		if (!isdigit (message[i])) {
			if (message[i] != ' '
			    && memcmp (message + i, "\r\n", 2) != 0) {
				return SMTP_ERROR_PARSE;
			}
			break;
		}
	}
	DEBUG_SMTP (SMTP_DBG, "\n");
	*digit_len = i;
	*digit_val = atoi (message);	//fixme: not efficient enough
	DEBUG_SMTP (SMTP_DBG, "digit_val = %d\n", *digit_val);

	return SMTP_NO_ERROR;
}




/* reset MAIL command stuff */
void
smtp_mail_reset (struct smtp_info *psmtp)
{
	psmtp->msg_size = 0;
	psmtp->act_size = 0;

	/*
	 * Unceremoniously close the pipe to the cleanup service. The cleanup
	 * service will delete the queue file when it detects a premature
	 * end-of-file condition on input.
	 */
//    if (psmtp->cleanup != 0) {
//      mail_stream_cleanup(psmtp->dest);
//      psmtp->dest = 0;
//      psmtp->cleanup = 0;
//    }

//    psmtp->err = 0;
//    if (psmtp->queue_id != 0) {
//      myfree(psmtp->queue_id);
//      psmtp->queue_id = 0;
//    }


	if (psmtp->sender) {
		smtp_mailbox_addr_free (psmtp->sender);
		psmtp->sender = NULL;
	}


//    if (psmtp->verp_delims) {
//      myfree(psmtp->verp_delims);
//      psmtp->verp_delims = 0;
//    }
//    if (psmtp->proxy_mail) {
//      myfree(psmtp->proxy_mail);
//      psmtp->proxy_mail = 0;
//    }
//    if (psmtp->saved_filter) {
//      myfree(psmtp->saved_filter);
//      psmtp->saved_filter = 0;
//    }
//    if (psmtp->saved_redirect) {
//      myfree(psmtp->saved_redirect);
//      psmtp->saved_redirect = 0;
//    }
//    psmtp->saved_flags = 0;
//#ifdef USE_SASL_AUTH
//    if (var_smtpd_sasl_enable)
//      smtpd_sasl_mail_reset(psmtp);
//#endif
//    psmtp->discard = 0;
//    VSTRING_RESET(psmtp->instance);
//    VSTRING_TERMINATE(psmtp->instance);

	/*
	 * Try to be nice. Don't bother when we lost the connection. Don't bother
	 * waiting for a reply, it just increases latency.
	 */
//    if (psmtp->proxy) {
//      (void) smtpd_proxy_cmd(psmtp, SMTPD_PROX_WANT_NONE, "RSET");
//      smtpd_proxy_close(psmtp);
//    }
//    if (psmtp->xforward.flags)
//      smtpd_xforward_reset(psmtp);
//    if (psmtp->prepend)
//      psmtp->prepend = argv_free(psmtp->prepend);

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
smtp_ack_mail_parse ( /*struct neti_tcp_stream *ptcp, */ struct smtp_info
		     *psmtp)
{
	struct smtp_ack *ack = NULL;
	int r, res;
	int code;
	size_t cur_token = 0;

	char *buf = psmtp->svr_data;
	int len = psmtp->svr_data_len;

	char *p1, *p2, *p3;

	DEBUG_SMTP (SMTP_DBG, "smtp_ack_mail_parse... \n");

	p1 = buf;
	p2 = buf + len;

	/* *INDENT-OFF* */
	/*!re2c
	"250"
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_mail_parse: 250\n");
		code = 250;
		cur_token = 3;
		psmtp->mail_state = SMTP_MAIL_STATE_MAIL;
		//goto ack_new;
		goto crlf;
	}
	"355"
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_mail_parse: 355\n");
		//Fixme octet-offset need to be parsed here
		code = 355;
		cur_token = 3;

		//Fixme the code contrling here

		//xiayu 2005.12.3 command state dfa
		//psmtp->mail_state = SMTP_MAIL_STATE_HELO;

		//goto ack_new;
		goto crlf;
	}
	"421"
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_mail_parse: 421\n");
		code = 421;
		cur_token = 3;
		smtp_mail_reset(psmtp);
		//goto ack_new;
		goto crlf;
	}
	"451"
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_mail_parse: 451\n");
		code = 451;
		cur_token = 3;
		smtp_mail_reset(psmtp);
		//goto ack_new;
		goto crlf;
	}
	"452"
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_mail_parse: 452\n");
		code = 452;
		cur_token = 3;
		smtp_mail_reset(psmtp);
		//goto ack_new;
		goto crlf;
	}
	"500"
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_mail_parse: 500\n");
		code = 500;
		cur_token = 3;
		smtp_mail_reset(psmtp);
		//goto ack_new;
		goto crlf;
	}
	"501"
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_mail_parse: 501\n");
		code = 501;
		cur_token = 3;
		smtp_mail_reset(psmtp);
		//goto ack_new;
		goto crlf;
	}
	"503"
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_mail_parse: 503\n");
		code = 503;
		cur_token = 3;
		smtp_mail_reset(psmtp);
		//goto ack_new;
		goto crlf;
	}
	"550"
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_mail_parse: 550\n");
		code = 550;
		cur_token = 3;
		smtp_mail_reset(psmtp);
		//goto ack_new;
		goto crlf;
	}
	"552"
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_mail_parse: 552\n");
		code = 552;
		cur_token = 3;
		smtp_mail_reset(psmtp);
		//goto ack_new;
		goto crlf;
	}
	"553"
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_mail_parse: 553\n");
		code = 553;
		cur_token = 3;
		smtp_mail_reset(psmtp);
		//goto ack_new;
		goto crlf;
	}
	digit{3,3}
	{
		cur_token = 3;
		code = atoi(p1-6);
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_mail_parse: %d\n", code);
		//Fixme the code contrling here
		if (code >= 300) {
			smtp_mail_reset(psmtp);
		}
		//goto ack_new;
		goto crlf;
	}

	any 
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_mail_parse: ANY\n");
		DEBUG_SMTP(SMTP_DBG, "Response unrecognizable: %s\n", psmtp->cli_data);
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

#ifdef USE_NEL
	DEBUG_SMTP (SMTP_DBG, "ack->code = %d\n", ack->code);
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
	DEBUG_SMTP (SMTP_DBG, "Free\n");
	smtp_ack_free (ack);

      err:
	DEBUG_SMTP (SMTP_DBG, "Error\n");
	reset_server_buf (psmtp, &cur_token);
	return res;

}


void
smtp_cmd_mail_free (struct smtp_cmd_mail *mail)
{
	if (mail->addr)
		smtp_mailbox_addr_free (mail->addr);
	free_mem (&smtp_cmd_mail_pool, (void *) mail);
	DEBUG_SMTP (SMTP_MEM, "smtp_cmd_mail_free: pointer=%p, elm=%p\n",
		    &smtp_cmd_mail_pool, (void *) mail);
}

struct smtp_cmd_mail *
smtp_cmd_mail_new (int len, char *addr, int key)
{
	struct smtp_cmd_mail *mail;
	mail = (struct smtp_cmd_mail *) alloc_mem (&smtp_cmd_mail_pool);
	if (mail == NULL) {
		return NULL;
	}
	DEBUG_SMTP (SMTP_MEM, "smtp_cmd_mail_new: pointer=%p, elm=%p\n",
		    &smtp_cmd_mail_pool, (void *) mail);

	DEBUG_SMTP (SMTP_DBG, "\n");
	//mail->event_type = SMTP_CMD_MAIL;
	//mail->nel_id = mail_id;

#ifdef USE_NEL
	//mail->count = 0;
	NEL_REF_INIT (mail);
#endif

	mail->len = len;

	if (addr) {
		mail->addr_len = strlen (addr);
		mail->addr = addr;
	}
	else {
		mail->addr_len = 0;
		mail->addr = NULL;
	}
	mail->key = key;
	DEBUG_SMTP (SMTP_DBG, "mail->addr = %s\n", mail->addr);

	return mail;
}


int
smtp_cmd_mail_parse ( /* struct neti_tcp_stream *ptcp, */ struct smtp_info
		     *psmtp, char *message, size_t length, size_t * index)
{
	struct smtp_cmd_mail *mail = NULL;
	size_t cur_token = *index;
	char *sender;
	int r, res;

	char *verp_delims = 0;
	char *p1, *p2, *p3;
	int key = 0;

	DEBUG_SMTP (SMTP_DBG, "smtp_cmd_mail_parse\n");

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
	var_helo_required = 1;
	if (var_helo_required && psmtp->helo_name == NULL) {
		r = reply_to_client (ptcp,
				     "503 MAIL Error: send HELO/EHLO first\r\n");
		if (r != SMTP_NO_ERROR) {
			res = r;
			goto err;
		}
		res = SMTP_ERROR_POLICY;
		goto err;
	}

	if (psmtp->sender) {
		r = reply_to_client (ptcp,
				     "503 Error: nested MAIL command\r\n");
		if (r != SMTP_NO_ERROR) {
			res = r;
			goto err;
		}
		res = SMTP_ERROR_PROTO;
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
	 goto err;
	 }
	 */


	/*
	 * "MAIL FROM:" ("<>" / Reverse-Path) [SP Mail-parameters] CRLF
	 */
	if (length < 13) {	/* no parameters at all */
		//r = reply_to_client(ptcp, "501 Syntax: MAIL FROM: <address>\r\n");
		//if (r != SMTP_NO_ERROR) {
		//      res = r;
		//      goto err;
		//}
		//res = SMTP_ERROR_PARSE;
		goto err;
	}

	if (length > 512) {
		//r = reply_to_client(ptcp, "501 Protocol: Length of MAIL FROM command line is too long\r\n");
		//if (r != SMTP_NO_ERROR) {
		//      res = r;
		//      goto err;
		//}
		res = SMTP_ERROR_PROTO;
		goto err;
	}
#endif

	r = smtp_wsp_parse (message, length, &cur_token);
	//xiayu 2005.11.22
	if (r != SMTP_NO_ERROR && r != SMTP_ERROR_PARSE) {
		res = r;
		//if (res == SMTP_ERROR_CONTINUE) {
		//      r = reply_to_client(ptcp, "501 MAIL Syntax Error.\r\n");
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
		//      r = reply_to_client(ptcp, "501 MAIL Syntax: reverse-path error.\r\n");
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
	   DEBUG_SMTP(SMTP_DBG, "smtp_cmd_mail_parse: 503 Error, acess denied for black list \n");
	   res = SMTP_ERROR_POLICY;
	   goto free;
	   }
	 */

	key = SMTP_MAIL_NO_PARAM;

	p1 = message + cur_token;
	p2 = message + length;
	DEBUG_SMTP (SMTP_DBG, "smtp_cmd_mail_parse: p1: %s\n", p1);

	while (1) {

		DEBUG_SMTP (SMTP_DBG, "p1 = %s\n", p1);

		/* *INDENT-OFF* */
		/*!re2c
		"AUTH="
		{
			DEBUG_SMTP(SMTP_DBG, "smtp_cmd_mail_parse: AUTH \n");
			//if ((err = smtpd_sasl_mail_opt(psmtp,  p1 )) != 0) {
			//	//smtpd_chat_reply(state, "%s", err);
			//	res = SMTP_ERROR_PARSE;
			//	goto free;
			//}

			key = SMTP_MAIL_AUTH;
			goto crlf;
			//reply_to_client(ptcp, "503 Syntax: MAIL FROM: <reverse-path> \"AUTH=<>\" CRLF not supported\r\n");
			//res = SMTP_ERROR_PARSE;
			//goto free;
		}

		"BODY=BINARYMIME"
		{
			DEBUG_SMTP(SMTP_DBG, "smtp_cmd_mail_parse: BODY=BINARYMIME\n");
#if 0	//xiayu bugfix 2005.12.15 let engine do the checking
			if (psmtp->cmd_allow[SMTP_ALLOW_CHUNKING] != 1) {
				//r = reply_to_client(ptcp, "503 MAIL Error: Server doesn't support CHUNKING\r\n");
				//if (r != SMTP_NO_ERROR) {
				//	res = r;
				//	goto free;
				//}
				res = SMTP_ERROR_POLICY;
				goto free;
			}
			if (psmtp->cmd_allow[SMTP_ALLOW_BINARYMIME] != 1) {
				//r = reply_to_client(ptcp, "503 MAIL Error: Server doesn't support BINARYMIME\r\n");
				//if (r != SMTP_NO_ERROR) {
				//	res = r;
				//	goto free;
				//}
				res = SMTP_ERROR_POLICY;
				goto free;
			}
#endif
			psmtp->encoding = SMTP_ENCODE_BINARY;
			key = SMTP_MAIL_BODY;
			continue;
		}
		"BODY=8BITMIME"
		{
			DEBUG_SMTP(SMTP_DBG, "smtp_cmd_mail_parse: BODY=8BITMIME\n");
#if 0	//xiayu bugfix 2005.12.15 let engine do the checking
			if (psmtp->cmd_allow[SMTP_ALLOW_8BITMIME] != 1) {
				//r = reply_to_client(ptcp, "503 MAIL Error: Server doesn't support 8BITMIME\r\n");
				//if (r != SMTP_NO_ERROR) {
				//	res = r;
				//	goto free;
				//}
				res = SMTP_ERROR_POLICY;
				goto free;
			}
#endif
			psmtp->encoding = SMTP_ENCODE_8BIT;
			key = SMTP_MAIL_BODY;
			continue;
		}
		"BODY=7BIT"
		{
			DEBUG_SMTP(SMTP_DBG, "smtp_cmd_mail_parse: BODY=7BIT\n");
			psmtp->encoding = SMTP_ENCODE_7BIT;
			key = SMTP_MAIL_BODY;
			continue;
		}

		"SIZE="
		{
			int size = 0;
			int size_len = 0;
			DEBUG_SMTP(SMTP_DBG, "smtp_cmd_mail_parse: SIZE=\n");

#if 0	//xiayu bugfix 2005.12.15 let engine do the checking
			if (psmtp->cmd_allow[SMTP_ALLOW_SIZE] != 1) {
				//r = reply_to_client(ptcp, "503 MAIL Error: Server doesn't support SIZE\r\n");
				//if (r != SMTP_NO_ERROR) {
				//	res = r;
				//	goto free;
				//}
				res = SMTP_ERROR_POLICY;
				goto free;
			}
#endif

			DEBUG_SMTP(SMTP_DBG, "\n");
			// Reject non-numeric size. 
			r = smtp_mail_size_parse(p1, p2-p1, &size_len, &size);
			if (r == SMTP_ERROR_PARSE) {
				//r = reply_to_client(ptcp, "501 MAIL Syntax: Bad message size syntax\r\n");
				//if (r != SMTP_NO_ERROR) {
				//	res = r;
				//	goto free;
				//}
				res = SMTP_ERROR_PARSE;
				goto free;
			}

			DEBUG_SMTP(SMTP_DBG, "\n");
			// Reject size overflow. 
			if (size < 0) {
				//state->error_mask |= MAIL_ERROR_POLICY;
				//r = reply_to_client(ptcp, "552 MAIL Error: Message size exceeds file system imposed limit\r\n");
				//if (r != SMTP_NO_ERROR) {
				//	res = r;
				//	goto free;
				//}
				res = SMTP_ERROR_PARSE;
				goto free;
			}

			DEBUG_SMTP(SMTP_DBG, "\n");
			psmtp->msg_size = size;
			key = SMTP_MAIL_SIZE;
			p1 += size_len;

			continue;
		}

		"TRANSID="
		{
			DEBUG_SMTP(SMTP_DBG, "smtp_cmd_mail_parse: TRANSID\n");
#if 0	//xiayu bugfix 2005.12.15 let engine do the checking
			if (psmtp->cmd_allow[SMTP_ALLOW_CHECKPOINT] != 1) {
				//r = reply_to_client(ptcp, "503 MAIL Error: Server doesn't support CHECKPOINT\r\n");
				//if (r != SMTP_NO_ERROR) {
				//	res = r;
				//	goto free;
				//}
				res = SMTP_ERROR_POLICY;
				goto free;
			}
#endif			
			key = SMTP_MAIL_TRANSID;
			goto crlf;

			//reply_to_client(ptcp, "503 Syntax: MAIL FROM: <reverse-path> \"TRANSID=\"transid-value CRLF not supported\r\n");
			//res = SMTP_ERROR_PARSE;
			//goto free;
		}

		"RET="
		{
			DEBUG_SMTP(SMTP_DBG, "smtp_cmd_mail_parse: RET\n");
#if 0	//xiayu bugfix 2005.12.15 let engine do the checking
			if (psmtp->cmd_allow[SMTP_ALLOW_DSN] != 1) {
				//r = reply_to_client(ptcp, "503 MAIL Error: Server doesn't support DSN\r\n");
				//if (r != SMTP_NO_ERROR) {
				//	res = r;
				//	goto free;
				//}
				res = SMTP_ERROR_POLICY;
				goto free;
			}
#endif
			key = SMTP_MAIL_RET;
			goto crlf;

			//reply_to_client(ptcp, "503 Syntax: MAIL FROM: <reverse-path> \"RET=\"ret-value CRLF not supported\r\n");
			//res = SMTP_ERROR_PARSE;
			//goto free;
		}

		"ENVID="
		{
			DEBUG_SMTP(SMTP_DBG, "smtp_cmd_mail_parse: ENVID\n");
#if 0	//xiayu bugfix 2005.12.15 let engine do the checking
			if (psmtp->cmd_allow[SMTP_ALLOW_DSN] != 1) {
				//r = reply_to_client(ptcp, "MAIL 503 Error: Server doesn't support DSN\r\n");
				//if (r != SMTP_NO_ERROR) {
				//	res = r;
				//	goto free;
				//}
				res = SMTP_ERROR_POLICY;
				goto free;
			}
#endif
			key = SMTP_MAIL_ENVID;
			goto crlf;
			//reply_to_client(ptcp, "503 Syntax: MAIL FROM: <reverse-path> \"ENVID\"=envid-value CRLF not supported\r\n");
			//res = SMTP_ERROR_PARSE;
			//goto free;
		}

		"BY="
		{
			DEBUG_SMTP(SMTP_DBG, "smtp_cmd_mail_parse: BY\n");
#if 0	//xiayu bugfix 2005.12.15 let engine do the checking
			if (psmtp->cmd_allow[SMTP_ALLOW_DELIVERBY] != 1) {
				//r = reply_to_client(ptcp, "503 MAIL Error: Server doesn't support DELIVERBY\r\n");
				//if (r != SMTP_NO_ERROR) {
				//	res = r;
				//	goto free;
				//}
				res = SMTP_ERROR_POLICY;
				goto free;
			}
#endif
			key = SMTP_MAIL_BY;
			goto crlf;
			//reply_to_client(ptcp, "503 Syntax: MAIL FROM: <reverse-path> \"BY=\"by-value CRLF not supported\r\n");
			//res = SMTP_ERROR_PARSE;
			//goto free;
		}

		"SOLICIT="
		{
			DEBUG_SMTP(SMTP_DBG, "smtp_cmd_mail_parse: SOLICIT\n");
			key = SMTP_MAIL_SOLICIT;
			goto crlf;
			//reply_to_client(ptcp, "503 Syntax: MAIL FROM: <reverse-path> \"SOLICIT=\" Solicitation-keywords CRLF not supported\r\n");
			//res = SMTP_ERROR_PARSE;
			//goto free;
		}

		"MTRK="
		{
			DEBUG_SMTP(SMTP_DBG, "smtp_cmd_mail_parse: MTRK\n");
			key = SMTP_MAIL_MTRK;
			goto crlf;
			//reply_to_client(ptcp, "503 Syntax: MAIL FROM: <reverse-path> \"MTRK=\" mtrk-value CRLF not supported\r\n");
			//res = SMTP_ERROR_PARSE;
			//goto free;
		}

		space +
		{
			DEBUG_SMTP(SMTP_DBG, "space\n");
			continue;
		}

		"\r\n"
		{
			DEBUG_SMTP(SMTP_DBG, "smtp_cmd_mail_parse: CRLF\n");
			cur_token = p1 - message;
			DEBUG_SMTP(SMTP_DBG, "length = %d,  cur_token = %d\n", length, cur_token);
			goto mail_new;
		}

		any
		{
			DEBUG_SMTP(SMTP_DBG, "smtp_cmd_mail_parse: ANY\n");
			//state->error_mask |= MAIL_ERROR_PROTOCOL;
			//reply_to_client(ptcp, "555 Unsupported option: %s", p1);
			//DEBUG_SMTP(SMTP_DBG, "smtp_cmd_mail_parse: 555, Unsupported option\n");
			//r = smtp_cfws_parse(message, length, &cur_token);
			//if (r != SMTP_NO_ERROR) { 
			//	res = r;
			//	goto free;
			//}

			DEBUG_SMTP(SMTP_DBG, "smtp_cmd_mail_parse: ANY\n");
			DEBUG_SMTP(SMTP_DBG, "Server response unrecognizable data %s\n", psmtp->cli_data);
			//r = reply_to_client(ptcp, "501 MAIL Syntax: parameter unrecognizable.\r\n");
			//if (r != SMTP_NO_ERROR) {
			//	res = r;
			//	goto free;
			//}
			res = SMTP_ERROR_PARSE;
			goto free;
		}
		*/
		/* *INDENT-ON* */
	}

      crlf:
	DEBUG_SMTP (SMTP_DBG, "p1 : %s\n", p1);
	r = smtp_str_crlf_parse (message, length, &cur_token);
	if (r != SMTP_NO_ERROR) {
		res = r;
		//if (res == SMTP_ERROR_PARSE || res == SMTP_ERROR_CONTINUE) {
		//      r = reply_to_client(ptcp, "501 MAIL Syntax: no CRLF.\r\n");
		//      if (r != SMTP_NO_ERROR) {
		//              res = r;
		//              goto free;
		//      }
		//      res = SMTP_ERROR_PARSE;
		//}
		goto free;
	}

      mail_new:

#if 0				//xiayu 2005.11.22 let engine do the checking
	psmtp->sender = mailbox_addr_dup (sender);
	if (psmtp->sender == NULL) {
		res = SMTP_ERROR_MEMORY;
		goto free;
	}
#endif

	DEBUG_SMTP (SMTP_DBG, "\n");
	/* now we can create the mail event, wyong, 2005.9.15 */
	mail = smtp_cmd_mail_new (length, sender, key);
	if (mail == NULL) {
		DEBUG_SMTP (SMTP_DBG,
			    "smtp_cmd_mail_parse: smtp_cmd_mail_new error\n");
		res = SMTP_ERROR_MEMORY;
		goto free;
	}
	sender = NULL;

#ifdef USE_NEL
	DEBUG_SMTP (SMTP_DBG, "\n");
	if ((r = nel_env_analysis (eng, &(psmtp->env), mail_id,
				   (struct smtp_simple_event *) mail)) < 0) {
		DEBUG_SMTP (SMTP_DBG,
			    "smtp_cmd_mail_parse: nel_env_analysis error\n");
		res = SMTP_ERROR_ENGINE;
		goto free;
	}
#endif

	DEBUG_SMTP (SMTP_DBG, "\n");
	if (psmtp->permit & SMTP_PERMIT_DENY) {
		//smtp_close_connection(ptcp, psmtp);
		res = SMTP_ERROR_POLICY;
		goto err;

	}
	else if (psmtp->permit & SMTP_PERMIT_DROP) {
		//r = reply_to_client(ptcp, "550 MAIL cannot be implemented.\r\n");
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
	psmtp->last_cli_event_type = SMTP_CMD_MAIL;

	//wyong, 20231003
	//psmtp->cli_data += cur_token;
	//psmtp->cli_data_len -= cur_token;
	//DEBUG_SMTP(SMTP_DBG, "RAIN RAIN RAIN cli_data_len = %d\n", psmtp->cli_data_len);
	//r = write_to_server(ptcp, psmtp);

	res = sync_client_data (psmtp, cur_token);
	if (res < 0) {
		goto err;
	}

	return SMTP_NO_ERROR;

      free:
	DEBUG_SMTP (SMTP_DBG, "smtp_cmd_mail_parse: at free\n");
	if (psmtp->sender)
		smtp_mail_reset (psmtp);
	if (sender)
		smtp_mailbox_addr_free (sender);
	if (mail)
		smtp_cmd_mail_free (mail);

      err:
	DEBUG_SMTP (SMTP_DBG, "smtp_cmd_mail_parse: at err\n");
	reset_client_buf (psmtp, index);
	return res;

}


#if 0
int
mailesmtp_mail (mailsmtp * session,
		const char *from, int return_full, const char *envid)
{
	int r;
	char command[SMTP_STRING_SIZE];
	char *body = "";

#if notyet
	/* TODO: figure out a way for the user to explicity enable this or not */
	if (session->esmtp & MAILSMTP_ESMTP_8BITMIME)
		body = " BODY=8BITMIME";
#endif

	if (session->esmtp & MAILSMTP_ESMTP_DSN) {
		if (envid)
			snprintf (command, SMTP_STRING_SIZE,
				  "MAIL FROM:<%s> RET=%s ENVID=%s%s\r\n",
				  from, return_full ? "FULL" : "HDRS", envid,
				  body);
		else
			snprintf (command, SMTP_STRING_SIZE,
				  "MAIL FROM:<%s> RET=%s%s\r\n", from,
				  return_full ? "FULL" : "HDRS", body);
	}
	else
		snprintf (command, SMTP_STRING_SIZE, "MAIL FROM:<%s>%s\r\n",
			  from, body);

	r = send_command (session, command);
	if (r == -1)
		return MAILSMTP_ERROR_STREAM;
	r = read_response (session);

	switch (r) {
	case 250:
		return MAILSMTP_NO_ERROR;

	case 552:
		return MAILSMTP_ERROR_EXCEED_STORAGE_ALLOCATION;

	case 451:
		return MAILSMTP_ERROR_IN_PROCESSING;

	case 452:
		return MAILSMTP_ERROR_INSUFFICIENT_SYSTEM_STORAGE;

	case 550:
		return MAILSMTP_ERROR_MAILBOX_UNAVAILABLE;

	case 553:
		return MAILSMTP_ERROR_MAILBOX_NAME_NOT_ALLOWED;

	case 503:
		return MAILSMTP_ERROR_BAD_SEQUENCE_OF_COMMAND;

	case 0:
		return MAILSMTP_ERROR_STREAM;

	default:
		return MAILSMTP_ERROR_UNEXPECTED_CODE;
	}
}

int
mailsmtp_mail (mailsmtp * session, const char *from)
{
	int r;
	char command[SMTP_STRING_SIZE];

	snprintf (command, SMTP_STRING_SIZE, "MAIL FROM:<%s>\r\n", from);
	r = send_command (session, command);
	if (r == -1)
		return MAILSMTP_ERROR_STREAM;
	r = read_response (session);

	switch (r) {
	case 250:
		return MAILSMTP_NO_ERROR;

	case 552:
		return MAILSMTP_ERROR_EXCEED_STORAGE_ALLOCATION;

	case 451:
		return MAILSMTP_ERROR_IN_PROCESSING;

	case 452:
		return MAILSMTP_ERROR_INSUFFICIENT_SYSTEM_STORAGE;

	case 550:
		return MAILSMTP_ERROR_MAILBOX_UNAVAILABLE;

	case 553:
		return MAILSMTP_ERROR_MAILBOX_NAME_NOT_ALLOWED;

	case 503:
		return MAILSMTP_ERROR_BAD_SEQUENCE_OF_COMMAND;

	case 0:
		return MAILSMTP_ERROR_STREAM;

	default:
		return MAILSMTP_ERROR_UNEXPECTED_CODE;
	}
}
#endif
