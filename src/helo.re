
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "mem_pool.h"
#include "smtp.h"
#include "mime.h"
#include "address.h"
#include "helo.h"
#include "command.h"
#include "rcpt-to.h"
#include "mail-from.h"

#ifdef USE_NEL
#include "engine.h"
extern struct nel_eng *eng;
int helo_id;
extern int ack_id;
#endif

int var_disable_helo_cmd = 0;
int var_helo_required;
ObjPool_t smtp_cmd_helo_pool;

void
smtp_cmd_helo_init (
#ifdef USE_NEL
	struct nel_eng *eng
#endif
)
{
	create_mem_pool (&smtp_cmd_helo_pool,
			 sizeof (struct smtp_cmd_helo), SMTP_MEM_STACK_DEPTH);
}


void
smtp_helo_reset (struct smtp_info *psmtp)
{
	if (psmtp->helo_name) {
		smtp_string_free (psmtp->helo_name);
		psmtp->helo_name = NULL;
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
smtp_ack_helo_parse ( struct smtp_info *psmtp)
{
	struct smtp_ack *ack;
	int r, res;
	int code;
	size_t cur_token;

	char *buf = psmtp->svr_data;
	int len = psmtp->svr_data_len;

	char *p1, *p2, *p3;

	DEBUG_SMTP (SMTP_DBG, "smtp_ack_helo_parse... \n");

	p1 = buf;
	p2 = buf + len;

	/* *INDENT-OFF* */
	/*!re2c
	"250"
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_helo_parse: 250\n");
		code = 250;

		cur_token = 3;
		if (psmtp->mail_state != SMTP_MAIL_STATE_HELO
			/*&& psmtp->mail_state != SMTP_MAIL_STATE_RSET*/)
		{
			smtp_rcpt_reset(psmtp);
			smtp_mail_reset(psmtp);
		}
		psmtp->mail_state = SMTP_MAIL_STATE_HELO;

		//goto ack_new;
		goto crlf;
	}
	"421"
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_helo_parse: 421\n");
		code = 421;
		cur_token = 3;
		smtp_helo_reset(psmtp);
		goto crlf;
	}
	"451"
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_helo_parse: 451\n");
		code = 451;
		cur_token = 3;
		smtp_helo_reset(psmtp);
		goto crlf;
	}

	"500"
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_helo_parse: 500\n");
		code = 500;
		cur_token = 3;
		smtp_helo_reset(psmtp);
		goto crlf;
	}
	"501"
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_helo_parse: 501\n");
		code = 501;
		cur_token = 3;
		smtp_helo_reset(psmtp);
		goto crlf;
	}
	"504"
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_helo_parse: 504\n");
		code = 504;
		cur_token = 3;
		smtp_helo_reset(psmtp);
		goto crlf;
	}
	"550"
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_helo_parse: 550\n");
		code = 550;
		cur_token = 3;
		smtp_helo_reset(psmtp);
		goto crlf;
	}

	digit{3,3}
	{
		code = atoi(p1-6);
		cur_token = 3;
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_helo_parse: %d\n", code);

		if (code >= 300) {
			smtp_helo_reset(psmtp);
		}
		goto crlf;
	}

	any 
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_helo_parse: any\n");
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
		goto free;
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
			    "smtp_ack_helo_parse: nel_env_analysis error\n");
		res = SMTP_ERROR_ENGINE;
		goto err;
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
	DEBUG_SMTP (SMTP_DBG, "smtp_ack_helo_parse: Free\n");
	if (psmtp->domain) {
		smtp_domain_free (psmtp->domain);
		psmtp->domain = NULL;
	}
	if (ack)
		smtp_ack_free (ack);
      err:
	DEBUG_SMTP (SMTP_DBG, "smtp_ack_helo_parse: Error\n");
	reset_server_buf (psmtp, &cur_token);
	return res;

}

void
smtp_cmd_helo_free (struct smtp_cmd_helo *helo)
{
	if (helo->host_name) {
		smtp_atom_free (helo->host_name);
	}
	free_mem (&smtp_cmd_helo_pool, (void *) helo);
	DEBUG_SMTP (SMTP_MEM, "smtp_cmd_helo_free: pointer=%p, elm=%p\n",
		    &smtp_cmd_helo_pool, (void *) helo);
}

struct smtp_cmd_helo *
smtp_cmd_helo_new (int len, char *host_name)
{
	struct smtp_cmd_helo *helo;
	helo = (struct smtp_cmd_helo *) alloc_mem (&smtp_cmd_helo_pool);
	if (helo == NULL) {
		return NULL;
	}
	DEBUG_SMTP (SMTP_MEM, "smtp_cmd_helo_new: pointer=%p, elm=%p\n",
		    &smtp_cmd_helo_pool, (void *) helo);

#ifdef USE_NEL
	NEL_REF_INIT (helo);
#endif
	helo->len = len;

	if (host_name) {
		helo->host_name = host_name;
		helo->host_name_len = strlen (host_name);
	}
	else {
		helo->host_name = NULL;
		helo->host_name_len = 0;
	}

	return helo;

}


int
smtp_cmd_helo_parse ( struct smtp_info *psmtp, char *message, size_t length, size_t * index)
{
	struct smtp_cmd_helo *helo = NULL;
	char *host_name;
	size_t cur_token = *index;
	int r, res;

	DEBUG_SMTP (SMTP_DBG, "smtp_cmd_helo_parse\n");

	/* if nel configureable variable var_disable_helo_cmd 
	   is set to 1, don't allow the command pass through. */
	if (var_disable_helo_cmd == 1) {
		DEBUG_SMTP (SMTP_DBG,
			    "smtp_cmd_helo_parse:var_disable_helo_cmd\n");
		res = SMTP_NO_ERROR;
		goto err;
	}

	r = smtp_wsp_parse (message, length, &cur_token);
	if (r != SMTP_NO_ERROR) {
		res = r;
		goto err;
	}

	DEBUG_SMTP (SMTP_DBG, "\n");
	r = smtp_wsp_atom_parse (message, length, &cur_token, &host_name);
	if (r != SMTP_NO_ERROR) {
		res = r;
		goto err;
	}

	DEBUG_SMTP (SMTP_DBG, "\n");
	r = smtp_wsp_unstrict_crlf_parse (message, length, &cur_token);
	if (r != SMTP_NO_ERROR) {
		DEBUG_SMTP (SMTP_DBG, "\n");
		res = r;
		goto free;
	}

	DEBUG_SMTP (SMTP_DBG, "\n");
	psmtp->helo_name = smtp_string_new (host_name, strlen (host_name));
	if (psmtp->helo_name == NULL) {
		res = SMTP_ERROR_MEMORY;
		goto free;
	}
	helo = smtp_cmd_helo_new (length, host_name);
	if (helo == NULL) {
		DEBUG_SMTP (SMTP_DBG,
			    "smtp_cmd_helo_parse:smtp_cmd_helo_new error\n");
		res = SMTP_ERROR_MEMORY;
		goto free;
	}

#ifdef USE_NEL
	if ((r = nel_env_analysis (eng, &(psmtp->env), helo_id,
				   (struct smtp_simple_event *) helo)) < 0) {
		DEBUG_SMTP (SMTP_DBG,
			    "smtp_cmd_helo_parse: nel_env_analysis error\n");
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

	*index = cur_token;
	psmtp->last_cli_event_type = SMTP_CMD_HELO;
	res = sync_client_data (psmtp, cur_token);
	if (res < 0) {
		goto err;
	}

	return SMTP_NO_ERROR;

      free:
	DEBUG_SMTP (SMTP_DBG, "smtp_cmd_ehlo_parse: Free\n");
	if (host_name) {
		smtp_atom_free (host_name);
	}
	if (psmtp->helo_name) {
		smtp_string_free (psmtp->helo_name);
		psmtp->helo_name = NULL;
	}
	if (helo)
		smtp_cmd_helo_free (helo);

      err:
	DEBUG_SMTP (SMTP_DBG, "smtp_cmd_ehlo_parse: Error\n");
	reset_client_buf (psmtp, index);

	return res;

}
