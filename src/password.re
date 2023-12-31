#include "mem_pool.h"
#include "smtp.h"
#include "mime.h"
#include "command.h"
#include "password.h"


#ifdef USE_NEL
#include "engine.h"
extern struct nel_eng *eng;
int pass_id;
extern int ack_id;
#endif

extern int max_smtp_ack_len;
int var_disable_passwd_cmd;
ObjPool_t smtp_cmd_passwd_pool;


void
smtp_cmd_passwd_init (
#ifdef USE_NEL
			     struct nel_eng *eng
#endif
	)
{
	create_mem_pool (&smtp_cmd_passwd_pool,
			 sizeof (struct smtp_cmd_passwd),
			 SMTP_MEM_STACK_DEPTH);
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
smtp_ack_passwd_parse (struct smtp_info *psmtp)
{
	struct smtp_ack *ack;
	int r, res;
	int code;
	size_t cur_token = 0;

	char *buf = psmtp->svr_data;
	int len = psmtp->svr_data_len;

	char *p1, *p2, *p3;

	DEBUG_SMTP (SMTP_DBG, "smtp_ack_passwd_parse... \n");

	p1 = buf;
	p2 = buf + len;

	/* *INDENT-OFF* */
	/*!re2c
	"334"
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_passwd_parse: 334\n");
		code = 334;
		cur_token = 3;
		goto crlf;
	}

	"235"
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_passwd_parse: 235\n");
		code = 235;
		cur_token = 3;
		goto crlf;
	}

	[45] digit{2,2}
	{
		cur_token = 3;
		code = atoi(p1-6);
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_passwd_parse: %d\n", code);

		psmtp->last_cli_event_type = SMTP_EVENT_UNCERTAIN;
		goto crlf;
	}

	digit{3,3}
	{
		cur_token = 3;
		code = atoi(p1-6);
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_passwd_parse: %d\n", code);

		if (code >= 300) {
			psmtp->last_cli_event_type = SMTP_EVENT_UNCERTAIN;
		}

		goto crlf;
	}

	any 
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_passwd_parse: any\n");
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
			    "smtp_ack_passwd_parse: nel_env_analysis error\n");
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
	DEBUG_SMTP (SMTP_DBG, "smtp_ack_passwd_parse: Free\n");
	smtp_ack_free (ack);

      err:
	DEBUG_SMTP (SMTP_DBG, "smtp_ack_passwd_parse: Error\n");
	reset_server_buf (psmtp, &cur_token);
	return res;

}


void
smtp_cmd_passwd_free (struct smtp_cmd_passwd *passwd)
{
	if (passwd->pass)
		smtp_string_free (passwd->pass);
	free_mem (&smtp_cmd_passwd_pool, (void *) passwd);
	DEBUG_SMTP (SMTP_MEM, "smtp_cmd_passwd_free: pointer=%p, elm=%p\n",
		    &smtp_cmd_passwd_pool, (void *) passwd);
}

struct smtp_cmd_passwd *
smtp_cmd_passwd_new (int len, char *pass)
{
	struct smtp_cmd_passwd *passwd;
	passwd = (void *) alloc_mem (&smtp_cmd_passwd_pool);
	if (passwd == NULL) {
		return NULL;
	}
	DEBUG_SMTP (SMTP_MEM, "smtp_cmd_passwd_new: pointer=%p, elm=%p\n",
		    &smtp_cmd_passwd_pool, (void *) passwd);

#ifdef USE_NEL
	NEL_REF_INIT (passwd);
#endif

	passwd->len = len;
	passwd->pass = pass;
	if (pass != NULL)
		passwd->pass_len = strlen (pass);
	else
		passwd->pass_len = 0;

	return passwd;

}


int
smtp_cmd_passwd_parse ( struct smtp_info *psmtp, char *message, size_t length, size_t * index)
{
	struct smtp_cmd_passwd *passwd = NULL;
	char *pass, *crlf;
	size_t cur_token = *index;
	int r, res;

	DEBUG_SMTP (SMTP_DBG, "smtp_cmd_passwd_parse\n");
	DEBUG_SMTP (SMTP_DBG, "message = %s\n", message);
	crlf = strstr (message, "\r\n");
	if (crlf == NULL) {
		res = SMTP_ERROR_PARSE;
		goto err;
	}

	if (crlf > message) {
		pass = smtp_string_new (message, crlf - message);
		if (pass == NULL) {
			res = SMTP_ERROR_MEMORY;
			goto err;
		}
	}
	else {
		pass = NULL;
	}

	passwd = smtp_cmd_passwd_new (length, pass);
	if (passwd == NULL) {
		DEBUG_SMTP (SMTP_DBG,
			    "smtp_cmd_passwd_parse:smtp_cmd_passwd_new error\n");
		res = SMTP_ERROR_MEMORY;
		goto free;
	}
	pass = NULL;

	cur_token = crlf + 2 - message;
	*index = cur_token;


#ifdef USE_NEL
	if ((r = nel_env_analysis (eng, &(psmtp->env), pass_id,
			(struct smtp_simple_event *) passwd)) < 0) {
		DEBUG_SMTP (SMTP_DBG,
			    "smtp_cmd_passwd_parse: nel_env_analysis error\n");
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

	psmtp->last_cli_event_type = SMTP_CMD_PASSWD;
	res = sync_client_data (psmtp, cur_token);
	if (res < 0) {
		goto err;
	}

	return SMTP_NO_ERROR;

      free:
	DEBUG_SMTP (SMTP_DBG, "smtp_cmd_passwd_parse: Free\n");
	if (pass)
		smtp_string_free (pass);
	if (passwd)
		smtp_cmd_passwd_free (passwd);

      err:
	DEBUG_SMTP (SMTP_DBG, "Error\n");
	reset_client_buf (psmtp, index);
	return res;
}
