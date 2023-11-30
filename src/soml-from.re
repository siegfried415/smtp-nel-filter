#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "mem_pool.h"
#include "smtp.h"
#include "mime.h"
#include "soml-from.h"
#include "command.h"
#include "path.h"
#include "address.h"

#ifdef USE_NEL
#include "engine.h"
extern struct nel_eng *eng;
int soml_id;
extern int ack_id;
#endif

extern int var_helo_required;
ObjPool_t smtp_cmd_soml_pool;

void
smtp_cmd_soml_init (
#ifdef USE_NEL
			   struct nel_eng *eng
#endif
	)
{
	create_mem_pool (&smtp_cmd_soml_pool,
			 sizeof (struct smtp_cmd_soml), SMTP_MEM_STACK_DEPTH);
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
smtp_ack_soml_parse ( struct smtp_info *psmtp)
{
	struct smtp_ack *ack;
	int r, res;
	int code;
	size_t cur_token = 0;

	char *buf = psmtp->svr_data;
	int len = psmtp->svr_data_len;

	char *p1, *p2, *p3;

	DEBUG_SMTP (SMTP_DBG, "smtp_ack_soml_parse... \n");

	p1 = buf;
	p2 = buf + len;

	/* *INDENT-OFF* */
	/*!re2c
	"250"
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_soml_parse: 250\n");
		code = 250;
		cur_token = 3;
		goto crlf;
	}
	"355"
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_soml_parse: 355\n");
		//Fixme octet-offset need to be parsed here
		code = 355;
		cur_token = 3;
		goto crlf;
	}
	"421"
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_soml_parse: 421\n");
		code = 421;
		cur_token = 3;
		smtp_mailbox_addr_free(psmtp->sender);
		psmtp->sender = NULL;
		goto crlf;
	}
	"451"
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_soml_parse: 451\n");
		code = 451;
		cur_token = 3;
		smtp_mailbox_addr_free(psmtp->sender);
		psmtp->sender = NULL;
		goto crlf;
	}
	"452"
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_soml_parse: 452\n");
		code = 452;
		cur_token = 3;
		smtp_mailbox_addr_free(psmtp->sender);
		psmtp->sender = NULL;
		goto crlf;
	}
	"500"
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_soml_parse: 500\n");
		code = 500;
		cur_token = 3;
		smtp_mailbox_addr_free(psmtp->sender);
		psmtp->sender = NULL;
		goto crlf;
	}
	"501"
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_soml_parse: 501\n");
		code = 501;
		cur_token = 3;
		smtp_mailbox_addr_free(psmtp->sender);
		psmtp->sender = NULL;
		goto crlf;
	}
	"502"
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_soml_parse: 502\n");
		code = 502;
		cur_token = 3;
		smtp_mailbox_addr_free(psmtp->sender);
		psmtp->sender = NULL;
		goto crlf;
	}
	"503"
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_soml_parse: 503\n");
		code = 503;
		cur_token = 3;
		smtp_mailbox_addr_free(psmtp->sender);
		psmtp->sender = NULL;
		goto crlf;
	}
	"550"
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_soml_parse: 550\n");
		code = 550;
		cur_token = 3;
		smtp_mailbox_addr_free(psmtp->sender);
		psmtp->sender = NULL;
		goto crlf;
	}
	"552"
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_soml_parse: 552\n");
		code = 552;
		cur_token = 3;
		smtp_mailbox_addr_free(psmtp->sender);
		psmtp->sender = NULL;
		goto crlf;
	}
	"553"
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_soml_parse: 553\n");
		code = 553;
		cur_token = 3;
		smtp_mailbox_addr_free(psmtp->sender);
		psmtp->sender = NULL;
		goto crlf;
	}

	digit{3,3}
	{
		cur_token = 3;
		code = atoi(p1-6);
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_soml_parse: %d\n", code);

		if (code >= 300) {
			smtp_mailbox_addr_free(psmtp->sender);
			psmtp->sender = NULL;
		}

		goto crlf;
	}
	any 
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_soml_parse: ANY\n");
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
	smtp_ack_free (ack);

      err:
	reset_server_buf (psmtp, &cur_token);
	return res;

}


void
smtp_cmd_soml_free (struct smtp_cmd_soml *soml)
{
	if (soml->addr)
		smtp_mailbox_addr_free (soml->addr);
	free_mem (&smtp_cmd_soml_pool, (void *) soml);
	DEBUG_SMTP (SMTP_MEM, "smtp_cmd_soml_free: pointer=%p, elm=%p\n",
		    &smtp_cmd_soml_pool, (void *) soml);
}

struct smtp_cmd_soml *
smtp_cmd_soml_new (int len, char *addr)
{
	struct smtp_cmd_soml *soml;

	DEBUG_SMTP (SMTP_DBG, "addr = %s\n", addr);

	soml = (struct smtp_cmd_soml *) alloc_mem (&smtp_cmd_soml_pool);
	if (soml == NULL) {
		return NULL;
	}
	DEBUG_SMTP (SMTP_MEM, "smtp_cmd_soml_free: pointer=%p, elm=%p\n",
		    &smtp_cmd_soml_pool, (void *) soml);

#ifdef USE_NEL
	NEL_REF_INIT (soml);
#endif

	soml->len = len;
	if (addr) {
		soml->addr_len = strlen (addr);
		soml->addr = addr;
	}
	else {
		soml->addr_len = 0;
		soml->addr = NULL;
	}
	DEBUG_SMTP (SMTP_DBG, "soml->addr = %s\n", soml->addr);

	return soml;
}

int
smtp_cmd_soml_parse ( struct smtp_info *psmtp, char *message, size_t length, size_t * index)
{
	struct smtp_cmd_soml *soml = NULL;
	size_t cur_token = *index;
	char *sender;
	int r, res;

	int key = 0;

	DEBUG_SMTP (SMTP_DBG, "smtp_cmd_soml_parse\n");

	psmtp->encoding = 0;

	r = smtp_wsp_parse (message, length, &cur_token);
	if (r != SMTP_NO_ERROR && r != SMTP_ERROR_PARSE) {
		res = r;
		goto err;
	}

	/* reverse-path */
	r = smtp_reverse_path_parse ( message, length, &cur_token, &sender);
	if (r != SMTP_NO_ERROR) {
		res = r;
		goto err;
	}

	r = smtp_wsp_unstrict_crlf_parse (message, length, &cur_token);
	if (r != SMTP_NO_ERROR) {
		res = r;
		goto free;
	}


	soml = smtp_cmd_soml_new (length, sender);
	if (soml == NULL) {
		DEBUG_SMTP (SMTP_DBG,
			    "smtp_cmd_soml_parse: smtp_cmd_soml_new error\n");
		res = SMTP_ERROR_MEMORY;
		goto free;
	}

#ifdef USE_NEL
	if ((r = nel_env_analysis (eng, &(psmtp->env), soml_id,
				   (struct smtp_simple_event *) soml)) < 0) {
		DEBUG_SMTP (SMTP_DBG,
			    "smtp_cmd_soml_parse: nel_env_analysis error\n");
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

	psmtp->last_cli_event_type = SMTP_CMD_SOML;
	res = sync_client_data (psmtp, cur_token);
	if (res < 0) {
		goto err;
	}

	return SMTP_NO_ERROR;

      free:
	DEBUG_SMTP (SMTP_DBG, "smtp_cmd_soml_parse: at free\n");
	if (psmtp->sender) {
		smtp_mailbox_addr_free (psmtp->sender);
		psmtp->sender = NULL;
	}
	if (sender)
		smtp_mailbox_addr_free (sender);
	if (soml)
		smtp_cmd_soml_free (soml);

      err:
	DEBUG_SMTP (SMTP_DBG, "smtp_cmd_soml_parse: at err\n");
	reset_client_buf (psmtp, index);
	return res;

}
