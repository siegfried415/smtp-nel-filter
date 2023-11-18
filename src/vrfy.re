
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "mem_pool.h"

#include "smtp.h"
#include "address.h"
#include "vrfy.h"
#include "command.h"

#ifdef USE_NEL
#include "engine.h"
extern struct nel_eng *eng;
int vrfy_id;
int ack_vrfy_id;
#endif

extern max_smtp_ack_len;
ObjPool_t smtp_cmd_vrfy_pool;
ObjPool_t smtp_ack_vrfy_pool;

int var_disable_vrfy_cmd = 0;

void
smtp_cmd_vrfy_init (
#ifdef USE_NEL
			   struct nel_eng *eng
#endif
	)
{
	create_mem_pool (&smtp_ack_vrfy_pool,
			 sizeof (struct smtp_ack_vrfy), SMTP_MEM_STACK_DEPTH);
	create_mem_pool (&smtp_cmd_vrfy_pool,
			 sizeof (struct smtp_cmd_vrfy), SMTP_MEM_STACK_DEPTH);
#ifdef USE_NEL
	//nel_func_name_call(eng, (char *)&ack_vrfy_id, "nel_id_of", "vrfy_ack");
	//nel_func_name_call(eng, (char *)&vrfy_id, "nel_id_of", "vrfy_req");
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


void
smtp_ack_vrfy_free (struct smtp_ack_vrfy *ack)
{
	if (ack->mailbox)
		smtp_mailbox_free (ack->mailbox);
	free_mem (&smtp_ack_vrfy_pool, (void *) ack);
	DEBUG_SMTP (SMTP_MEM, "smtp_ack_vrfy_free: pointer=%p, elm=%p\n",
		    &smtp_ack_vrfy_pool, (void *) ack);
}

struct smtp_ack_vrfy *
smtp_ack_vrfy_new (int len, int code, struct smtp_mailbox *mailbox)
{
	struct smtp_ack_vrfy *ack;

	ack = (struct smtp_ack_vrfy *) alloc_mem (&smtp_ack_vrfy_pool);
	if (!ack) {
		return NULL;
	}
	DEBUG_SMTP (SMTP_MEM, "smtp_ack_vrfy_new: pointer=%p, elm=%p\n",
		    &smtp_ack_vrfy_pool, (void *) ack);

	/* the total length of this ack line */

	DEBUG_SMTP (SMTP_DBG, "\n");
	//ack->event_type = SMTP_ACK_VRFY;
	//ack->nel_id = ack_vrfy_id;


#ifdef USE_NEL
	//ack->count = 0;
	NEL_REF_INIT (ack);
#endif

	ack->len = len;
	ack->code = code;

	ack->mailbox = mailbox;

	if (mailbox) {
		DEBUG_SMTP (SMTP_DBG,
			    "ack_vrfy->mailbox->mb_display_name = %s\n",
			    ack->mailbox->mb_display_name);
		DEBUG_SMTP (SMTP_DBG,
			    "ack_vrfy->mailbox->mb_addr_spec = %s\n",
			    ack->mailbox->mb_addr_spec);

	}
	return ack;
}


int
smtp_ack_vrfy_parse ( /*struct neti_tcp_stream *ptcp, */ struct smtp_info
		     *psmtp)
{
	struct smtp_ack_vrfy *ack = NULL;
	struct smtp_mailbox *mailbox = NULL;
	int code;
	int r, res;

	char *buf = psmtp->svr_data;
	int len = psmtp->svr_data_len;

	char *p1, *p2, *p3;
	size_t cur_token = 0;

	DEBUG_SMTP (SMTP_DBG, "smtp_ack_vrfy_parse... \n");

#if 0				//xiayu 2005.11.22 let engine do the checking
	if (len > max_smtp_ack_len) {
		DEBUG_SMTP (SMTP_DBG, "smtp server ack line is too long\n");
		res = SMTP_ERROR_PROTO;
		goto err;
	}
#endif
	p1 = buf;
	p2 = buf + len;

	/* *INDENT-OFF* */
	/*!re2c
	[2] digit{2,2} space
	{
		code = atoi(buf);
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_vrfy_parse: %d\n", code);

		cur_token += 4;
		r = smtp_mailbox_parse(buf, len, &cur_token, &mailbox);
		if (r != SMTP_NO_ERROR) {
			res = r;
			goto err;
		}

		r = smtp_wsp_unstrict_crlf_parse(buf, len, &cur_token);
		if (r != SMTP_NO_ERROR) {
			res = SMTP_ERROR_PARSE;
			goto free;
		}

		goto ack_new;
	}

	digit{3,3} space
	{
		cur_token = 4;

		code = atoi(buf);
		DEBUG_SMTP(SMTP_DBG, "smtp_ack_vrfy_parse: %d\n", code);

		mailbox = NULL;
		r = smtp_str_crlf_parse(buf, len, &cur_token);
		if (r != SMTP_NO_ERROR) {
			res = SMTP_ERROR_PARSE;
			goto free;
		}

		goto ack_new;
	}

	any 
	{
		DEBUG_SMTP(SMTP_DBG, "server response unrecognizable data %s\n", psmtp->svr_data);
		res = SMTP_ERROR_PARSE;
		goto free;
	}
	*/
	/* *INDENT-ON* */

      ack_new:
	ack = smtp_ack_vrfy_new (len, code, mailbox);
	if (!ack) {
		res = SMTP_ERROR_MEMORY;
		goto err;
	}
	mailbox = NULL;

#ifdef USE_NEL
	if ((r = nel_env_analysis (eng, &(psmtp->env), ack_vrfy_id,
				   (struct smtp_simple_event *) ack)) < 0) {
		DEBUG_SMTP (SMTP_DBG,
			    "smtp_ack_vrfy_parse: nel_env_analysis error\n");
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
	if (mailbox)
		smtp_mailbox_free (mailbox);
	if (ack)
		smtp_ack_vrfy_free (ack);
      err:
	reset_server_buf (psmtp, &cur_token);
	return res;
}


void
smtp_cmd_vrfy_free (struct smtp_cmd_vrfy *vrfy)
{
	if (vrfy->user_name)
		smtp_display_name_free (vrfy->user_name);
	if (vrfy->mail_box)
		smtp_angle_addr_free (vrfy->mail_box);

	free_mem (&smtp_cmd_vrfy_pool, (void *) vrfy);
	DEBUG_SMTP (SMTP_MEM, "smtp_cmd_vrfy_free: pointer=%p, elm=%p\n",
		    &smtp_cmd_vrfy_pool, (void *) vrfy);
}


struct smtp_cmd_vrfy *
smtp_cmd_vrfy_new (int len, char *user_name, char *mail_box)
{
	struct smtp_cmd_vrfy *vrfy;
	vrfy = (struct smtp_cmd_vrfy *) alloc_mem (&smtp_cmd_vrfy_pool);
	if (vrfy == NULL) {
		return NULL;
	}
	DEBUG_SMTP (SMTP_DBG, "\n");
	DEBUG_SMTP (SMTP_MEM, "smtp_cmd_vrfy_new: pointer=%p, elm=%p\n",
		    &smtp_cmd_vrfy_pool, (void *) vrfy);

	//vrfy->event_type = SMTP_CMD_VRFY;
	//vrfy->nel_id = vrfy_id;

#ifdef USE_NEL
	//vrfy->count = 0;
	NEL_REF_INIT (vrfy);
#endif

	vrfy->len = len;
	if (user_name) {	//bugfix, wyong, 2005.11.8 
		vrfy->user_name = user_name;
		vrfy->user_name_len = strlen (user_name);
	}
	else {
		vrfy->user_name = NULL;
		vrfy->user_name_len = 0;
	}
	if (mail_box) {		//bugfix, wyong, 2005.11.8
		vrfy->mail_box = mail_box;
		vrfy->mail_box_len = strlen (mail_box);
	}
	else {
		vrfy->mail_box = NULL;
		vrfy->mail_box_len = 0;
	}

	return vrfy;
}


int
smtp_cmd_vrfy_parse ( /*struct neti_tcp_stream *ptcp, */ struct smtp_info
		     *psmtp, char *message, size_t length, size_t * index)
{
	struct smtp_cmd_vrfy *vrfy = NULL;
	char *display_name = NULL, *angle_addr = NULL;
	size_t cur_token = *index;
	int r, res;

	DEBUG_SMTP (SMTP_DBG, "smtp_cmd_vrfy_parse\n");

	/* if nel configureable variable var_disable_vrfy_cmd is set to 1, 
	   don't allow the command pass through, wyong, 2005.9.26  */
	//if (var_disable_vrfy_cmd == 1) {
	//      r = reply_to_client(ptcp, "550 VRFY not implemented.\r\n");
	//      if (r != SMTP_NO_ERROR) {
	//              res = r;
	//              goto err;
	//      }
	//      res = SMTP_ERROR_POLICY;
	//      goto err;
	//}

	r = smtp_wsp_parse (message, length, &cur_token);
	if (r != SMTP_NO_ERROR) {
		res = r;
		//if (res == SMTP_ERROR_PARSE || res == SMTP_ERROR_CONTINUE) {
		//      r = reply_to_client(ptcp, "501 VRFY Syntax error.\r\n");
		//      if (r != SMTP_NO_ERROR) {
		//              res = r;
		//              goto err;
		//      }
		//      res = SMTP_ERROR_PARSE;
		//}
		goto err;
	}



	/*
	 * name-addr = [display-name] angle-addr
	 */

	r = smtp_wsp_name_addr_parse (message, length, &cur_token,
				      &display_name, &angle_addr);
	switch (r) {
	case SMTP_NO_ERROR:
		/* do nothing */
		DEBUG_SMTP (SMTP_DBG, "cur_token = %d\n", cur_token);
		break;

	case SMTP_ERROR_CONTINUE:
		res = SMTP_ERROR_PARSE;
		goto err;

	case SMTP_ERROR_PARSE:
		r = smtp_wsp_display_name_parse (message, length, &cur_token,
						 &display_name);
		if (r != SMTP_NO_ERROR) {
			res = r;
			//if (res == SMTP_ERROR_PARSE || res == SMTP_ERROR_CONTINUE) {
			//      r = reply_to_client(ptcp, "501 VRFY Syntax: parameters error.\r\n");
			//      if (r != SMTP_NO_ERROR) {
			//              res = r;
			//              goto err;
			//      }
			//      res = SMTP_ERROR_PARSE;
			////}
			goto err;
		}
		DEBUG_SMTP (SMTP_DBG, "cur_token = %d\n", cur_token);
		break;
	default:
		DEBUG_SMTP (SMTP_DBG, "\n");
		res = r;
		goto err;
	}
	DEBUG_SMTP (SMTP_DBG, "display_name = %s\n", display_name);
	DEBUG_SMTP (SMTP_DBG, "angle_addr = %s\n", angle_addr);

	r = smtp_wsp_unstrict_crlf_parse (message, length, &cur_token);
	if (r != SMTP_NO_ERROR) {
		res = r;
		//if (res == SMTP_ERROR_PARSE || res == SMTP_ERROR_CONTINUE) {
		//      r = reply_to_client(ptcp, "501 VRFY Syntax: no CRLF.\r\n");
		//      if (r != SMTP_NO_ERROR) {
		//              res = r;
		//              goto free;
		//      }
		//      res = SMTP_ERROR_PARSE;
		//}
		goto free;
	}

	vrfy = smtp_cmd_vrfy_new (length, display_name, angle_addr);
	if (vrfy == NULL) {
		res = SMTP_ERROR_MEMORY;
		goto free;
	}
	display_name = angle_addr = NULL;

#ifdef USE_NEL
	DEBUG_SMTP (SMTP_DBG, "\n");
	if ((r = nel_env_analysis (eng, &(psmtp->env), vrfy_id,
				   (struct smtp_simple_event *) vrfy)) < 0) {
		DEBUG_SMTP (SMTP_DBG,
			    "smtp_cmd_vrfy_parse: nel_env_analysis error\n");
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
		//r = reply_to_client(ptcp, "550 VRFY cannot be implemented.\r\n");
		//if (r != SMTP_NO_ERROR) {
		//      res = r;
		//      goto err;
		//}
		res = SMTP_ERROR_POLICY;
		goto err;
	}


	DEBUG_SMTP (SMTP_DBG, "\n");
	DEBUG_SMTP (SMTP_DBG, "cur_token = %d\n", cur_token);
	*index = cur_token;
	psmtp->last_cli_event_type = SMTP_CMD_VRFY;


	//wyong, 20231003 
	//psmtp->cli_data += cur_token;
	//psmtp->cli_data_len -= cur_token;

	res = sync_client_data (psmtp, cur_token);
	if (res < 0) {
		goto err;
	}

	DEBUG_SMTP (SMTP_DBG, "smtp_cmd_vrfy_parse: return SMTP_NO_ERROR\n");
	return SMTP_NO_ERROR;

      free:
	DEBUG_SMTP (SMTP_DBG, "Free\n");
	if (display_name)
		smtp_display_name_free (display_name);
	if (angle_addr)
		smtp_angle_addr_free (angle_addr);
	if (vrfy)
		smtp_cmd_vrfy_free (vrfy);

      err:
	DEBUG_SMTP (SMTP_DBG, "Error\n");
	reset_client_buf (psmtp, index);
	return res;
}
