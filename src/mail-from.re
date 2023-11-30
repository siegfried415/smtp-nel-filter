#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "mem_pool.h"
#include "smtp.h"
#include "mime.h"
#include "path.h"
#include "mail-from.h"
#include "command.h"
#include "address.h" 


#ifdef USE_NEL
#include "engine.h"

extern struct nel_eng *eng;
int mail_id;
extern int ack_id;
#endif

ObjPool_t smtp_cmd_mail_pool;
extern int var_helo_required;



void
smtp_cmd_mail_init (
#ifdef USE_NEL
			   struct nel_eng *eng
#endif
	)
{
	create_mem_pool (&smtp_cmd_mail_pool,
			 sizeof (struct smtp_cmd_mail), SMTP_MEM_STACK_DEPTH);
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

	if (psmtp->sender) {
		smtp_mailbox_addr_free (psmtp->sender);
		psmtp->sender = NULL;
	}
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
smtp_ack_mail_parse ( struct smtp_info *psmtp)
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
		goto crlf;
	}
	"355"
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_mail_parse: 355\n");
		//Fixme octet-offset need to be parsed here
		code = 355;
		cur_token = 3;
		goto crlf;
	}
	"421"
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_mail_parse: 421\n");
		code = 421;
		cur_token = 3;
		smtp_mail_reset(psmtp);
		goto crlf;
	}
	"451"
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_mail_parse: 451\n");
		code = 451;
		cur_token = 3;
		smtp_mail_reset(psmtp);
		goto crlf;
	}
	"452"
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_mail_parse: 452\n");
		code = 452;
		cur_token = 3;
		smtp_mail_reset(psmtp);
		goto crlf;
	}
	"500"
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_mail_parse: 500\n");
		code = 500;
		cur_token = 3;
		smtp_mail_reset(psmtp);
		goto crlf;
	}
	"501"
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_mail_parse: 501\n");
		code = 501;
		cur_token = 3;
		smtp_mail_reset(psmtp);
		goto crlf;
	}
	"503"
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_mail_parse: 503\n");
		code = 503;
		cur_token = 3;
		smtp_mail_reset(psmtp);
		goto crlf;
	}
	"550"
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_mail_parse: 550\n");
		code = 550;
		cur_token = 3;
		smtp_mail_reset(psmtp);
		goto crlf;
	}
	"552"
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_mail_parse: 552\n");
		code = 552;
		cur_token = 3;
		smtp_mail_reset(psmtp);
		goto crlf;
	}
	"553"
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_mail_parse: 553\n");
		code = 553;
		cur_token = 3;
		smtp_mail_reset(psmtp);
		goto crlf;
	}
	digit{3,3}
	{
		cur_token = 3;
		code = atoi(p1-6);
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_mail_parse: %d\n", code);
		if (code >= 300) {
			smtp_mail_reset(psmtp);
		}
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

#ifdef USE_NEL
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
smtp_cmd_mail_parse ( struct smtp_info *psmtp, char *message, size_t length, size_t * index)
{
	struct smtp_cmd_mail *mail = NULL;
	size_t cur_token = *index;
	char *sender;
	int r, res;

	char *verp_delims = 0;
	char *p1, *p2, *p3;
	int key = 0;

	DEBUG_SMTP (SMTP_DBG, "smtp_cmd_mail_parse\n");
	r = smtp_wsp_parse (message, length, &cur_token);
	if (r != SMTP_NO_ERROR && r != SMTP_ERROR_PARSE) {
		res = r;
		goto err;
	}

	/* reverse-path */
	r = smtp_reverse_path_parse (message, length, &cur_token, &sender);
	if (r != SMTP_NO_ERROR) {
		res = r;
		goto err;
	}

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

			key = SMTP_MAIL_AUTH;
			goto crlf;
		}

		"BODY=BINARYMIME"
		{
			DEBUG_SMTP(SMTP_DBG, "smtp_cmd_mail_parse: BODY=BINARYMIME\n");
			psmtp->encoding = SMTP_ENCODE_BINARY;
			key = SMTP_MAIL_BODY;
			continue;
		}
		"BODY=8BITMIME"
		{
			DEBUG_SMTP(SMTP_DBG, "smtp_cmd_mail_parse: BODY=8BITMIME\n");
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

			// Reject non-numeric size. 
			r = smtp_mail_size_parse(p1, p2-p1, &size_len, &size);
			if (r == SMTP_ERROR_PARSE) {
				res = SMTP_ERROR_PARSE;
				goto free;
			}

			DEBUG_SMTP(SMTP_DBG, "\n");
			// Reject size overflow. 
			if (size < 0) {
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
			key = SMTP_MAIL_TRANSID;
			goto crlf;
		}

		"RET="
		{
			DEBUG_SMTP(SMTP_DBG, "smtp_cmd_mail_parse: RET\n");
			key = SMTP_MAIL_RET;
			goto crlf;
		}

		"ENVID="
		{
			DEBUG_SMTP(SMTP_DBG, "smtp_cmd_mail_parse: ENVID\n");
			key = SMTP_MAIL_ENVID;
			goto crlf;
		}

		"BY="
		{
			DEBUG_SMTP(SMTP_DBG, "smtp_cmd_mail_parse: BY\n");
			key = SMTP_MAIL_BY;
			goto crlf;
		}

		"SOLICIT="
		{
			DEBUG_SMTP(SMTP_DBG, "smtp_cmd_mail_parse: SOLICIT\n");
			key = SMTP_MAIL_SOLICIT;
			goto crlf;
		}

		"MTRK="
		{
			DEBUG_SMTP(SMTP_DBG, "smtp_cmd_mail_parse: MTRK\n");
			key = SMTP_MAIL_MTRK;
			goto crlf;
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
			DEBUG_SMTP(SMTP_DBG, "length = %lu,  cur_token = %lu\n", length, cur_token);
			goto mail_new;
		}

		any
		{
			DEBUG_SMTP(SMTP_DBG, "smtp_cmd_mail_parse: ANY\n");
			DEBUG_SMTP(SMTP_DBG, "Server response unrecognizable data %s\n", psmtp->cli_data);
			res = SMTP_ERROR_PARSE;
			goto free;
		}
		*/
		/* *INDENT-ON* */
	}

      crlf:
	r = smtp_str_crlf_parse (message, length, &cur_token);
	if (r != SMTP_NO_ERROR) {
		res = r;
		goto free;
	}

      mail_new:
	mail = smtp_cmd_mail_new (length, sender, key);
	if (mail == NULL) {
		DEBUG_SMTP (SMTP_DBG,
			    "smtp_cmd_mail_parse: smtp_cmd_mail_new error\n");
		res = SMTP_ERROR_MEMORY;
		goto free;
	}
	sender = NULL;

#ifdef USE_NEL
	if ((r = nel_env_analysis (eng, &(psmtp->env), mail_id,
				   (struct smtp_simple_event *) mail)) < 0) {
		DEBUG_SMTP (SMTP_DBG,
			    "smtp_cmd_mail_parse: nel_env_analysis error\n");
		res = SMTP_ERROR_ENGINE;
		goto free;
	}
#endif

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
	psmtp->last_cli_event_type = SMTP_CMD_MAIL;

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

