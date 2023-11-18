/*
 * rcpt-to.re
 * $Id: rcpt-to.re,v 1.23 2005/11/29 06:29:26 xiay Exp $
 */


#include "mem_pool.h"
#include "engine.h"

#include "smtp.h"
#include "rcpt-to.h"
#include "command.h"

#ifdef USE_NEL
#include "engine.h"
extern struct nel_eng *eng;
int rcpt_id;
extern int ack_id;
#endif


ObjPool_t smtp_cmd_rcpt_pool;


void
smtp_cmd_rcpt_init (
#ifdef USE_NEL
			   struct nel_eng *eng
#endif
	)
{
	create_mem_pool (&smtp_cmd_rcpt_pool,
			 sizeof (struct smtp_cmd_rcpt), SMTP_MEM_STACK_DEPTH);
#ifdef USE_NEL
	//nel_func_name_call(eng, (char *)&rcpt_id, "nel_id_of", "rcpt_req");
#endif
}


/* reset RCPT stuff */
void
smtp_rcpt_reset (struct smtp_info *psmtp)
{
	if (psmtp->recipient) {
		smtp_mailbox_addr_free (psmtp->recipient);
		psmtp->recipient = NULL;
	}
	psmtp->rcpt_count = 0;

	/* XXX Must flush the command history. */
//    psmtp->rcpt_overshoot = 0;

}


#if 0
/* check_mail_access - OK/FAIL based on mail address lookup */
static int
check_mail_access (SMTPD_STATE * state, const char *table,
		   const char *addr, int *found,
		   const char *reply_name,
		   const char *reply_class, const char *def_acl)
{
	char *myname = "check_mail_access";
	const RESOLVE_REPLY *reply;
	const char *domain;
	int status;
	char *local_at;
	char *bare_addr;
	char *bare_at;

	if (msg_verbose)
		msg_info ("%s: %s", myname, addr);

	/*
	 * Resolve the address.
	 */
	reply = (const RESOLVE_REPLY *) ctable_locate (smtpd_resolve_cache,
						       addr);
	if (reply->flags & RESOLVE_FLAG_FAIL)
		reject_dict_retry (state, addr);

	/*
	 * Garbage in, garbage out. Every address from rewrite_clnt_internal()
	 * and from resolve_clnt_query() must be fully qualified.
	 */
	if ((domain = strrchr (CONST_STR (reply->recipient), '@')) == 0) {
		msg_warn ("%s: no @domain in address: %s", myname,
			  CONST_STR (reply->recipient));
		return (0);
	}
	domain += 1;

	/*
	 * In case of address extensions.
	 */
	if (*var_rcpt_delim == 0) {
		bare_addr = 0;
	}
	else {
		bare_addr = strip_addr (addr, (char **) 0, *var_rcpt_delim);
	}

#define CHECK_MAIL_ACCESS_RETURN(x) \
	{ if (bare_addr) myfree(bare_addr); return(x); }

	/*
	 * Source-routed (non-local or virtual) recipient addresses are too
	 * suspicious for returning an "OK" result. The complicated expression
	 * below was brought to you by the keyboard of Victor Duchovni, Morgan
	 * Stanley and hacked up a bit by Wietse.
	 */
#define SUSPICIOUS(reply, reply_class) \
	(var_allow_untrust_route == 0 \
	&& (reply->flags & RESOLVE_FLAG_ROUTED) \
	&& strcmp(reply_class, SMTPD_NAME_RECIPIENT) == 0)

	/*
	 * Look up user+foo@domain if the address has an extension, user@domain
	 * otherwise.
	 */
	if ((status =
	     check_access (state, table, CONST_STR (reply->recipient), FULL,
			   found, reply_name, reply_class, def_acl)) != 0
	    || *found)
		CHECK_MAIL_ACCESS_RETURN (status == SMTPD_CHECK_OK
					  && SUSPICIOUS (reply,
							 reply_class) ?
					  SMTPD_CHECK_DUNNO : status);

	/*
	 * Try user@domain if the address has an extension.
	 */
	if (bare_addr)
		if ((status = check_access (state, table, bare_addr, PARTIAL,
					    found, reply_name, reply_class,
					    def_acl)) != 0 || *found)
			CHECK_MAIL_ACCESS_RETURN (status == SMTPD_CHECK_OK
						  && SUSPICIOUS (reply,
								 reply_class)
						  ? SMTPD_CHECK_DUNNO :
						  status);

	/*
	 * Look up the domain name, or parent domains thereof.
	 */
	if ((status = check_domain_access (state, table, domain, PARTIAL,
					   found, reply_name, reply_class,
					   def_acl)) != 0 || *found)
		CHECK_MAIL_ACCESS_RETURN (status == SMTPD_CHECK_OK
					  && SUSPICIOUS (reply,
							 reply_class) ?
					  SMTPD_CHECK_DUNNO : status);

	/*
	 * Look up user+foo@ if the address has an extension, user@ otherwise.
	 * XXX This leaks a little memory if map lookup is aborted.
	 */
	local_at = mystrndup (CONST_STR (reply->recipient),
			      domain - CONST_STR (reply->recipient));
	status = check_access (state, table, local_at, PARTIAL, found,
			       reply_name, reply_class, def_acl);
	myfree (local_at);
	if (status != 0 || *found)
		CHECK_MAIL_ACCESS_RETURN (status == SMTPD_CHECK_OK
					  && SUSPICIOUS (reply, reply_class) ?
					  SMTPD_CHECK_DUNNO : status);

	/*
	 * Look up user@ if the address has an extension. XXX Same problem here.
	 */
	if (bare_addr) {
		bare_at = strrchr (bare_addr, '@');
		local_at =
			(bare_at ?
			 mystrndup (bare_addr,
				    bare_at + 1 -
				    bare_addr) : mystrdup (bare_addr));
		status = check_access (state, table, local_at, PARTIAL, found,
				       reply_name, reply_class, def_acl);
		myfree (local_at);
		if (status != 0 || *found)
			CHECK_MAIL_ACCESS_RETURN (status == SMTPD_CHECK_OK
						  && SUSPICIOUS (reply,
								 reply_class)
						  ? SMTPD_CHECK_DUNNO :
						  status);
	}

	/*
	 * Undecided when no match found.
	 */
	CHECK_MAIL_ACCESS_RETURN (SMTPD_CHECK_DUNNO);
}
#endif



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
smtp_ack_rcpt_parse ( /* struct neti_tcp_stream *ptcp, */ struct smtp_info
		     *psmtp)
{
	struct smtp_ack *ack;
	int r, res;
	int code;
	size_t cur_token = 0;

	char *buf = psmtp->svr_data;
	int len = psmtp->svr_data_len;

	char *p1, *p2, *p3;

	DEBUG_SMTP (SMTP_DBG, "smtp_ack_rcpt_parse... \n");

	p1 = buf;
	p2 = buf + len;

	/* *INDENT-OFF* */
	/*!re2c
	"250"
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_rcpt_parse: 250\n");
		code = 250;
		cur_token = 3;
		psmtp->mail_state = SMTP_MAIL_STATE_RCPT;
		//goto ack_new;
		goto crlf;
	}
	"251"
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_rcpt_parse: 251\n");
		code = 251;
		cur_token = 3;
		psmtp->mail_state = SMTP_MAIL_STATE_RCPT;
		//goto ack_new;
		goto crlf;
	}
	"421"
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_rcpt_parse: 421\n");
		code = 421;
		cur_token = 3;
		smtp_rcpt_reset(psmtp);
		//goto ack_new;
		goto crlf;
	}
	"450"
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_rcpt_parse: 450\n");
		code = 450;
		cur_token = 3;
		smtp_rcpt_reset(psmtp);
		//goto ack_new;
		goto crlf;
	}
	"451"
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_rcpt_parse: 451\n");
		code = 451;
		cur_token = 3;
		smtp_rcpt_reset(psmtp);
		//goto ack_new;
		goto crlf;
	}
	"452"
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_rcpt_parse: 452\n");
		code = 452;
		cur_token = 3;
		smtp_rcpt_reset(psmtp);
		//goto ack_new;
		goto crlf;
	}
	"500"
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_rcpt_parse: 500\n");
		code = 500;
		cur_token = 3;
		smtp_rcpt_reset(psmtp);
		//goto ack_new;
		goto crlf;
	}
	"501"
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_rcpt_parse: 501\n");
		code = 501;
		cur_token = 3;
		smtp_rcpt_reset(psmtp);
		//goto ack_new;
		goto crlf;
	}
	"503"
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_rcpt_parse: 503\n");
		code = 503;
		cur_token = 3;
		smtp_rcpt_reset(psmtp);
		//goto ack_new;
		goto crlf;
	}
	"550"
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_rcpt_parse: 550\n");
		code = 550;
		cur_token = 3;
		smtp_rcpt_reset(psmtp);
		//goto ack_new;
		goto crlf;
	}
	"551"
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_rcpt_parse: 551\n");
		code = 551;
		cur_token = 3;
		smtp_rcpt_reset(psmtp);
		//goto ack_new;
		goto crlf;
	}
	"552"
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_rcpt_parse: 552\n");
		code = 552;
		cur_token = 3;
		smtp_rcpt_reset(psmtp);
		//goto ack_new;
		goto crlf;
	}
	"553"
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_rcpt_parse: 553\n");
		code = 553;
		cur_token = 3;
		smtp_rcpt_reset(psmtp);
		//goto ack_new;
		goto crlf;
	}
	digit{3,3}
	{
		cur_token = 3;
		code = atoi(p1-6);
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_rcpt_parse: %d\n", code);
		if (code >= 300) {
			smtp_rcpt_reset(psmtp);
		}
		//goto ack_new;
		goto crlf;
	}
	any 
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_rcpt_parse: any\n");
		DEBUG_SMTP(SMTP_DBG, "server response unrecognizable data %s\n", psmtp->svr_data);
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
	//psmtp->svr_data += cur_token;
	//psmtp->svr_data_len -= cur_token;
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


struct smtp_cmd_rcpt *
smtp_cmd_rcpt_new (int len, char *addr)
{
	struct smtp_cmd_rcpt *rcpt;
	rcpt = (struct smtp_cmd_rcpt *) alloc_mem (&smtp_cmd_rcpt_pool);
	if (rcpt == NULL) {
		return NULL;
	}
	DEBUG_SMTP (SMTP_DBG, "\n");
	DEBUG_SMTP (SMTP_MEM, "smtp_cmd_rcpt_new: pointer=%p, elm=%p\n",
		    &smtp_cmd_rcpt_pool, (void *) rcpt);

	DEBUG_SMTP (SMTP_DBG, "\n");
	//rcpt->event_type = SMTP_CMD_RCPT;
	//rcpt->nel_id = rcpt_id;

#ifdef USE_NEL
	//rcpt->count = 0;
	NEL_REF_INIT (rcpt);
#endif

	rcpt->len = len;

	if (addr) {
		rcpt->addr_len = strlen (addr);
		rcpt->addr = addr;
	}
	else {
		rcpt->addr_len = 0;
		rcpt->addr = NULL;
	}

	DEBUG_SMTP (SMTP_DBG, "rcpt->addr = %s\n", rcpt->addr);
	DEBUG_SMTP (SMTP_DBG, "\n");
	return rcpt;
}

void
smtp_cmd_rcpt_free (struct smtp_cmd_rcpt *rcpt)
{
	if (rcpt->addr)
		smtp_mailbox_addr_free (rcpt->addr);
	free_mem (&smtp_cmd_rcpt_pool, (void *) rcpt);
	DEBUG_SMTP (SMTP_MEM, "smtp_cmd_rcpt_free: pointer=%p, elm=%p\n",
		    &smtp_cmd_rcpt_pool, (void *) rcpt);
}


int
smtp_cmd_rcpt_parse ( /* struct neti_tcp_stream *ptcp, */ struct smtp_info
		     *psmtp, char *message, size_t length, size_t * index)
{
	struct smtp_cmd_rcpt *rcpt = NULL;
	size_t cur_token = *index;
	int r, res;

	char *recipient;
	char *verp_delims = 0;
	char *p1, *p2, *p3;

	DEBUG_SMTP (SMTP_DBG, "smtp_cmd_rcpt_parse\n");

	/*
	 * Sanity checks.
	 * 
	 * XXX 2821 pedantism: Section 4.1.2 says that SMTP servers that receive a
	 * command in which invalid character codes have been employed, and for
	 * which there are no other reasons for rejection, MUST reject that
	 * command with a 501 response. So much for the principle of "be liberal
	 * in what you accept, be strict in what you send".
	 */
#if 0				//xiayu 2005.11.22 let engine do the checking

	if (psmtp->sender == NULL) {
		//psmtp->error_mask |= RCPT_ERROR_PROTOCOL;
		DEBUG_SMTP (SMTP_DBG,
			    "smtp_cmd_rcpt_parse: 503 Error, send MAIL command first\n");
		r = reply_to_client (ptcp,
				     "503 RCPT Error: send MAIL command first\r\n");
		if (r != SMTP_NO_ERROR) {
			res = r;
			goto err;
		}
		res = SMTP_ERROR_POLICY;
		goto err;
	}

#if 0
	if (argc < 3 || strcasecmp (argv[1].strval, "to:") != 0) {
		state->error_mask |= RCPT_ERROR_PROTOCOL;
		smtpd_chat_reply (state, "501 Syntax: RCPT TO: <address>");
		return (-1);
	}

	/*
	 * XXX The client event count/rate control must be consistent in its use
	 * of client address information in connect and disconnect events. For
	 * now we exclude xclient authorized hosts from event count/rate control.
	 */
	if (SMTPD_STAND_ALONE (state) == 0
	    && !xclient_allowed
	    && anvil_clnt
	    && var_smtpd_crcpt_limit > 0
	    && !namadr_list_match (hogger_list, state->name, state->addr)
	    && anvil_clnt_rcpt (anvil_clnt, state->service, state->addr,
				&rate) == ANVIL_STAT_OK
	    && rate > var_smtpd_crcpt_limit) {
		smtpd_chat_reply (state,
				  "421 %s Error: too many recipients from %s",
				  var_myhostname, state->addr);
		msg_warn ("Recipient address rate limit exceeded: %d from %s for service %s", rate, state->namaddr, state->service);
		return (-1);
	}

	if (argv[2].tokval == SMTPD_TOK_ERROR) {
		state->error_mask |= RCPT_ERROR_PROTOCOL;
		smtpd_chat_reply (state, "501 Bad recipient address syntax");
		return (-1);
	}
#endif

	/*
	 * "RCPT TO:" ("<Postmaster@" domain ">" / "<Postmaster>"
	 *               / Forward-Path)  [SP Rcpt-parameters] CRLF
	 */
	DEBUG_SMTP (SMTP_DBG, "\n");
	if (length < 11) {	/* no parameters at all */
		DEBUG_SMTP (SMTP_DBG,
			    "smtp_cmd_rcpt_parse: 501 Syntax: RCPT TO: <address>\n");
		r = reply_to_client (ptcp,
				     "501 Syntax: RCPT TO: <address>\r\n");
		if (r != SMTP_NO_ERROR) {
			res = r;
			goto err;
		}
		res = SMTP_ERROR_PARSE;
		goto err;
	}

	DEBUG_SMTP (SMTP_DBG, "\n");
	if (length > 512) {
		DEBUG_SMTP (SMTP_DBG,
			    "smtp_cmd_rcpt_parse: 503 Protocol: Length of RCPT TO command line is too long\n");
		r = reply_to_client (ptcp,
				     "503 Protocol: Length of RCPT TO command line is too long\r\n");
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
		//      r = reply_to_client(ptcp, "501 RCPT Syntax Error.\r\n");
		//      if (r != SMTP_NO_ERROR) {
		//              res = r;
		//              goto err;
		//      }
		//      res = SMTP_ERROR_PARSE;
		//}
		goto err;
	}

	/* forward-path */
	DEBUG_SMTP (SMTP_DBG, "\n");
	r = smtp_forward_path_parse (message, length, &cur_token, &recipient);
	if (r != SMTP_NO_ERROR) {
		res = r;
		//if (res == SMTP_ERROR_PARSE || res == SMTP_ERROR_CONTINUE) {
		//      r = reply_to_client(ptcp, "501 RCPT Syntax: forword-path error.\r\n");
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
	   DEBUG_SMTP(SMTP_DBG, "smtp_cmd_rcpt_parse: 503 Error, acess denied for black list \n");
	   res = SMTP_ERROR_POLICY;
	   goto free;
	   }
	 */

	DEBUG_SMTP (SMTP_DBG, "p1: %s\n", p1);
	p1 = message + cur_token;
	DEBUG_SMTP (SMTP_DBG, "smtp_cmd_rcpt_parse: p1: %s\n", p1);
	p2 = message + length;


	while (1) {

		DEBUG_SMTP (SMTP_DBG, "p1 = %s\n", p1);

		/* *INDENT-OFF* */
		/*!re2c
		"NOTIFY=NEVER"
		{
			DEBUG_SMTP(SMTP_DBG, "smtp_cmd_rcpt_parse: NOTIFY=NEVER\n");
			goto crlf;
		}
		"NOTIFY=SUCCESS"
		{
			DEBUG_SMTP(SMTP_DBG, "smtp_cmd_rcpt_parse: NOTIFY=SUCCESS\n");
			goto crlf;
		}
		"NOTIFY=FAILURE"
		{
			DEBUG_SMTP(SMTP_DBG, "smtp_cmd_rcpt_parse: NOTIFY=FAILURE\n");
			goto crlf;
		}
		"NOTIFY=DELAY"
		{
			DEBUG_SMTP(SMTP_DBG, "smtp_cmd_rcpt_parse: NOTIFY=DELAY\n");
			goto crlf;
		}
		"ORCPT="
		{
			DEBUG_SMTP(SMTP_DBG, "smtp_cmd_rcpt_parse: ORCPT\n");
			goto crlf;
		}


		space +
		{
			DEBUG_SMTP(SMTP_DBG, "space\n");
			continue;
		}

		"\r\n"
		{
			DEBUG_SMTP(SMTP_DBG, "smtp_cmd_rcpt_parse: CRLF\n");
			cur_token = p1 - message;
			DEBUG_SMTP(SMTP_DBG, "length = %d,  cur_token = %d\n", length, cur_token);
			goto rcpt_new;
		}
		any
		{
			DEBUG_SMTP(SMTP_DBG, "smtp_cmd_rcpt_parse: ANY\n");
			DEBUG_SMTP(SMTP_DBG, "server response unrecognizable data %s\n", psmtp->cli_data);
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
		//      r = reply_to_client(ptcp, "501 RCPT Syntax: no CRLF.\r\n");
		//      if (r != SMTP_NO_ERROR) {
		//              res = r;
		//              goto free;
		//      }
		//      res = SMTP_ERROR_PARSE;
		//}
		goto free;
	}

      rcpt_new:

#if 0				//xiayu 2005.11.22 let engine do the checking
	/*
	 * Store the recipient. Remember the first one.
	 * 
	 * Flush recipients to maintain a stiffer coupling with the next stage and
	 * to better utilize parallelism.
	 */
	DEBUG_SMTP (SMTP_DBG, "\n");
	psmtp->rcpt_count++;
	if (psmtp->recipient == NULL) {
		psmtp->recipient = mailbox_addr_dup (recipient);
		if (psmtp->recipient == NULL) {
			res = SMTP_ERROR_MEMORY;
			goto free;
		}
	}
#endif

	DEBUG_SMTP (SMTP_DBG, "\n");
	rcpt = smtp_cmd_rcpt_new (length, recipient);
	if (rcpt == NULL) {
		DEBUG_SMTP (SMTP_DBG,
			    "smtp_cmd_rcpt_parse: smtp_cmd_rcpt_new error\n");
		res = SMTP_ERROR_MEMORY;
		goto free;
	}
	recipient = NULL;

#ifdef USE_NEL
	DEBUG_SMTP (SMTP_DBG, "\n");
	if ((r = nel_env_analysis (eng, &(psmtp->env), rcpt_id,
				   (struct smtp_simple_event *) rcpt)) < 0) {
		DEBUG_SMTP (SMTP_DBG,
			    "smtp_cmd_rcpt_parse: nel_env_analysis error\n");
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
		//r = reply_to_client(ptcp, "550 RCPT cannot be implemented.\r\n");
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
	psmtp->last_cli_event_type = SMTP_CMD_RCPT;

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
	DEBUG_SMTP (SMTP_DBG, "smtp_cmd_rcpt_parse: at free\n");
	if (psmtp->recipient)
		smtp_rcpt_reset (psmtp);
	if (recipient)
		smtp_mailbox_addr_free (recipient);
	if (rcpt)
		smtp_cmd_rcpt_free (rcpt);

      err:
	DEBUG_SMTP (SMTP_DBG, "smtp_cmd_rcpt_parse: at err\n");
	reset_client_buf (psmtp, index);
	return res;

}



#if 0
int
mailesmtp_rcpt (mailsmtp * session,
		const char *to, int notify, const char *orcpt)
{
	int r;
	char command[SMTP_STRING_SIZE];
	char notify_str[30] = "";
	char notify_info_str[30] = "";

	if (notify != 0 && session->esmtp & MAILSMTP_ESMTP_DSN) {
		if (notify & MAILSMTP_DSN_NOTIFY_SUCCESS)
			strcat (notify_info_str, ",SUCCESS");
		if (notify & MAILSMTP_DSN_NOTIFY_FAILURE)
			strcat (notify_info_str, ",FAILURE");
		if (notify & MAILSMTP_DSN_NOTIFY_DELAY)
			strcat (notify_info_str, ",DELAY");

		if (notify & MAILSMTP_DSN_NOTIFY_NEVER)
			strcpy (notify_info_str, ",NEVER");

		notify_info_str[0] = '=';

		strcpy (notify_str, " NOTIFY");
		strcat (notify_str, notify_info_str);
	}

	if (orcpt && session->esmtp & MAILSMTP_ESMTP_DSN)
		snprintf (command, SMTP_STRING_SIZE,
			  "RCPT TO:<%s>%s ORCPT=%s\r\n", to, notify_str,
			  orcpt);
	else
		snprintf (command, SMTP_STRING_SIZE, "RCPT TO:<%s>%s\r\n", to,
			  notify_str);

	r = send_command (session, command);
	if (r == -1)
		return MAILSMTP_ERROR_STREAM;
	r = read_response (session);

	switch (r) {
	case 250:
		return MAILSMTP_NO_ERROR;

	case 251:		/* not local user, will be forwarded */
		return MAILSMTP_NO_ERROR;

	case 550:
	case 450:
		return MAILSMTP_ERROR_MAILBOX_UNAVAILABLE;

	case 551:
		return MAILSMTP_ERROR_USER_NOT_LOCAL;

	case 552:
		return MAILSMTP_ERROR_EXCEED_STORAGE_ALLOCATION;

	case 553:
		return MAILSMTP_ERROR_MAILBOX_NAME_NOT_ALLOWED;

	case 451:
		return MAILSMTP_ERROR_IN_PROCESSING;

	case 452:
		return MAILSMTP_ERROR_INSUFFICIENT_SYSTEM_STORAGE;

	case 503:
		return MAILSMTP_ERROR_BAD_SEQUENCE_OF_COMMAND;

	case 0:
		return MAILSMTP_ERROR_STREAM;

	default:
		return MAILSMTP_ERROR_UNEXPECTED_CODE;
	}
}


int
mailsmtp_rcpt (mailsmtp * session, const char *to)
{
	return mailesmtp_rcpt (session, to, 0, NULL);
}

#endif
