#include "mem_pool.h"
#include "engine.h"
#include "smtp.h"
#include "mime.h"
#include "path.h"
#include "rcpt-to.h"
#include "command.h"
#include "address.h" 

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
	//psmtp->rcpt_overshoot = 0;

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
smtp_ack_rcpt_parse ( struct smtp_info *psmtp)
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
		goto crlf;
	}
	"251"
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_rcpt_parse: 251\n");
		code = 251;
		cur_token = 3;
		psmtp->mail_state = SMTP_MAIL_STATE_RCPT;
		goto crlf;
	}
	"421"
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_rcpt_parse: 421\n");
		code = 421;
		cur_token = 3;
		smtp_rcpt_reset(psmtp);
		goto crlf;
	}
	"450"
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_rcpt_parse: 450\n");
		code = 450;
		cur_token = 3;
		smtp_rcpt_reset(psmtp);
		goto crlf;
	}
	"451"
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_rcpt_parse: 451\n");
		code = 451;
		cur_token = 3;
		smtp_rcpt_reset(psmtp);
		goto crlf;
	}
	"452"
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_rcpt_parse: 452\n");
		code = 452;
		cur_token = 3;
		smtp_rcpt_reset(psmtp);
		goto crlf;
	}
	"500"
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_rcpt_parse: 500\n");
		code = 500;
		cur_token = 3;
		smtp_rcpt_reset(psmtp);
		goto crlf;
	}
	"501"
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_rcpt_parse: 501\n");
		code = 501;
		cur_token = 3;
		smtp_rcpt_reset(psmtp);
		goto crlf;
	}
	"503"
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_rcpt_parse: 503\n");
		code = 503;
		cur_token = 3;
		smtp_rcpt_reset(psmtp);
		goto crlf;
	}
	"550"
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_rcpt_parse: 550\n");
		code = 550;
		cur_token = 3;
		smtp_rcpt_reset(psmtp);
		goto crlf;
	}
	"551"
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_rcpt_parse: 551\n");
		code = 551;
		cur_token = 3;
		smtp_rcpt_reset(psmtp);
		goto crlf;
	}
	"552"
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_rcpt_parse: 552\n");
		code = 552;
		cur_token = 3;
		smtp_rcpt_reset(psmtp);
		goto crlf;
	}
	"553"
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_rcpt_parse: 553\n");
		code = 553;
		cur_token = 3;
		smtp_rcpt_reset(psmtp);
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
		res = SMTP_ERROR_POLICY;
		goto err;
	}
	else if (psmtp->permit & SMTP_PERMIT_DROP) {
		res = SMTP_ERROR_POLICY;
		goto err;
	}

	DEBUG_SMTP (SMTP_DBG, "len = %d,  cur_token = %lu\n", len, cur_token);
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

#ifdef USE_NEL
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
smtp_cmd_rcpt_parse ( struct smtp_info *psmtp, char *message, size_t length, size_t * index)
{
	struct smtp_cmd_rcpt *rcpt = NULL;
	size_t cur_token = *index;
	int r, res;

	char *recipient;
	char *verp_delims = 0;
	char *p1, *p2, *p3;

	DEBUG_SMTP (SMTP_DBG, "smtp_cmd_rcpt_parse\n");
	r = smtp_wsp_parse (message, length, &cur_token);
	if (r != SMTP_NO_ERROR && r != SMTP_ERROR_PARSE) {
		res = r;
		goto err;
	}

	/* forward-path */
	DEBUG_SMTP (SMTP_DBG, "\n");
	r = smtp_forward_path_parse (message, length, &cur_token, &recipient);
	if (r != SMTP_NO_ERROR) {
		res = r;
		goto err;
	}


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
			DEBUG_SMTP(SMTP_DBG, "length = %lu,  cur_token = %lu\n", length, cur_token);
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
		goto free;
	}

      rcpt_new:
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
		res = SMTP_ERROR_POLICY;
		goto err;

	}
	else if (psmtp->permit & SMTP_PERMIT_DROP) {
		res = SMTP_ERROR_POLICY;
		goto err;
	}

	DEBUG_SMTP (SMTP_DBG, "length = %lu, cur_token = %lu\n", length,
		    cur_token);
	*index = cur_token;
	psmtp->last_cli_event_type = SMTP_CMD_RCPT;

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

