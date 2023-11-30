
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "clist.h"
#include "mem_pool.h"
#include "smtp.h"
#include "mime.h" 
#include "address.h"
#include "expn.h"
#include "command.h"

#ifdef USE_NEL
#include "engine.h"

extern struct nel_eng *eng;
int expn_id;
int ack_expn_id;
#endif

extern int max_smtp_ack_multiline_count;
extern int max_smtp_ack_len;


int var_disable_expn_cmd;
ObjPool_t smtp_ack_expn_pool;
ObjPool_t smtp_cmd_expn_pool;


void
smtp_cmd_expn_init (
#ifdef USE_NEL
			   struct nel_eng *eng
#endif
	)
{
	create_mem_pool (&smtp_ack_expn_pool,
			 sizeof (struct smtp_ack_expn), SMTP_MEM_STACK_DEPTH);
	create_mem_pool (&smtp_cmd_expn_pool,
			 sizeof (struct smtp_cmd_expn), SMTP_MEM_STACK_DEPTH);
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
smtp_ack_expn_free (struct smtp_ack_expn *ack)
{
	if (ack->mb_list) {
		clist_foreach (ack->mb_list, (clist_func) smtp_mailbox_free,
			       NULL);
		clist_free (ack->mb_list);
	}
	free_mem (&smtp_ack_expn_pool, (void *) ack);
	DEBUG_SMTP (SMTP_MEM, "smtp_ack_expn_free: pointer=%p, elm=%p\n",
		    &smtp_ack_expn_pool, (void *) ack);
}

struct smtp_ack_expn *
smtp_ack_expn_new (int len, int code, clist * mb_list)
{
	struct smtp_ack_expn *ack = NULL;

	ack = (struct smtp_ack_expn *) alloc_mem (&smtp_ack_expn_pool);
	if (!ack) {
		return NULL;
	}
	DEBUG_SMTP (SMTP_MEM, "smtp_ack_expn_new: pointer=%p, elm=%p\n",
		    &smtp_ack_expn_pool, (void *) ack);

#ifdef USE_NEL
	NEL_REF_INIT (ack);
#endif
	ack->len = len;		/* the total length of this ack line */
	ack->code = code;

	ack->mb_list = mb_list;

	if (mb_list) {
		DEBUG_SMTP (SMTP_DBG,
			    "ack_expn->mb_list->first->data->mb_display_name = %s\n",
			    ((struct smtp_mailbox *) (ack->mb_list->first->
						      data))->
			    mb_display_name);
		DEBUG_SMTP (SMTP_DBG,
			    "ack_expn->mb_list->first->data->mb_addr_spec = %s\n",
			    ((struct smtp_mailbox *) (ack->mb_list->first->
						      data))->mb_addr_spec);
	}

	return ack;
}


int
smtp_ack_expn_parse ( struct smtp_info *psmtp)
{
	struct smtp_ack_expn *ack = NULL;
	struct smtp_mailbox *mailbox = NULL;
	clist *mailbox_list = NULL;
	int multiline_count;
	int code;
	int r, res;

	char *buf = psmtp->svr_data;
	int len = psmtp->svr_data_len;

	char *p1, *p2, *p3;
	size_t cur_token;

	DEBUG_SMTP (SMTP_DBG, "smtp_ack_expn_parse... \n");

	p1 = buf;
	p2 = buf + len;
	code = 0;
	multiline_count = 0;

	cur_token = 0;
	while (1) {

		p1 = buf + cur_token;

		/* *INDENT-OFF* */
		/*!re2c
		"250-"
		{
			DEBUG_SMTP(SMTP_DBG, "smtp_ack_expn_parse: 250-\n");
			multiline_count ++;

			cur_token += strlen("250-");
			
			if (0 == code) {
				code = 250;

			} else if (250 != code) {
				DEBUG_SMTP(SMTP_DBG, "Multi-line ack of EXPN does not have consistent codes: %s\n", psmtp->svr_data);
				res = SMTP_ERROR_PARSE;
				goto free;
			}

			if (multiline_count > max_smtp_ack_multiline_count) {
				DEBUG_SMTP(SMTP_DBG, "smtp multi-line count of ack are too many\n");
				res = SMTP_ERROR_PROTO;
				goto free;
			}

			r = smtp_mailbox_parse(buf, len, &cur_token, &mailbox);
			if (r != SMTP_NO_ERROR) {
				res = r;
				goto free;
			}

			r = smtp_wsp_unstrict_crlf_parse(buf, len, &cur_token);
			if (r != SMTP_NO_ERROR) {
				res = SMTP_ERROR_PARSE;
				goto free;
			}

			if (mailbox_list == NULL) {
				mailbox_list = clist_new();
				if (mailbox_list == NULL) {
					res = SMTP_ERROR_MEMORY;
					goto free;
				}
			}

			r = clist_append(mailbox_list, mailbox);
			if (r < 0) {
				res = SMTP_ERROR_MEMORY;
				goto free;
			}
			mailbox = NULL;

			continue;
		}

		"250 "
		{
			DEBUG_SMTP(SMTP_DBG, "smtp_ack_expn_parse: 250 space\n");
			multiline_count ++;
			cur_token += strlen("250 ");

			if (0 == code) {
				code = 250;

			} else if (250 != code ) {
				DEBUG_SMTP(SMTP_DBG, "Multi-line ack of EXPN does not has consistent codes: %s\n", psmtp->svr_data);
				res = SMTP_ERROR_PARSE;
				goto free;
			}

			if (multiline_count > max_smtp_ack_multiline_count) {
				DEBUG_SMTP(SMTP_DBG, "smtp multi-line count of ack are too many\n");
				res = SMTP_ERROR_PROTO;
				goto free;
			}

			r = smtp_mailbox_parse(buf, len, &cur_token, &mailbox);
			if (r != SMTP_NO_ERROR) {
				res = r;
				goto free;
			}

			r = smtp_wsp_unstrict_crlf_parse(buf, len, &cur_token);
			if (r != SMTP_NO_ERROR) {
				res = SMTP_ERROR_PARSE;
				goto free;
			}

			if (mailbox_list == NULL) {
				mailbox_list = clist_new();
				if (mailbox_list == NULL) {
					res = SMTP_ERROR_MEMORY;
					goto free;
				}
			}

			r = clist_append(mailbox_list, mailbox);
			if (r < 0) {
				res = SMTP_ERROR_MEMORY;
				goto free;
			}
			mailbox = NULL;

			goto ack_new;
		}
		digit{3,3}
		{
			code = atoi(p1-6);
			DEBUG_SMTP(SMTP_DBG, "smtp_ack_expn_parse: %d\n", code);
			cur_token = 3;

			r = smtp_str_crlf_parse(buf, len, &cur_token);
			if (r != SMTP_NO_ERROR) {
				res = SMTP_ERROR_PARSE;
				goto free; 
			}

			goto ack_new;
		}

		any 
		{
			DEBUG_SMTP(SMTP_DBG, "smtp_ack_expn_parse: any\n");
			DEBUG_SMTP(SMTP_DBG, "p1: %s\n", p1);
			DEBUG_SMTP(SMTP_DBG, "server response unrecognizable data %s\n", psmtp->svr_data);
			res = SMTP_ERROR_PARSE;
			goto free;
		}
		*/

		/* *INDENT-ON* */
	}

      ack_new:
	ack = smtp_ack_expn_new (len, code, mailbox_list);
	if (!ack) {
		res = SMTP_ERROR_MEMORY;
		goto free;
	}
	mailbox_list = NULL;

#ifdef USE_NEL
	if ((r = nel_env_analysis (eng, &(psmtp->env), ack_expn_id,
				   (struct smtp_simple_event *) ack)) < 0) {
		DEBUG_SMTP (SMTP_DBG,
			    "smtp_ack_expn_parse: nel_env_analysis error\n");
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

	res = sync_server_data (psmtp, cur_token);
	if (res < 0) {
		goto err;
	}

	return SMTP_NO_ERROR;

      free:
	DEBUG_SMTP (SMTP_DBG, "smtp_ack_expn_parse: Free\n");
	if (mailbox)
		smtp_mailbox_free (mailbox);
	if (mailbox_list) {
		clist_foreach (mailbox_list, (clist_func) smtp_mailbox_free,
			       NULL);
		clist_free (mailbox_list);
	}
	if (ack)
		smtp_ack_expn_free (ack);
      err:
	DEBUG_SMTP (SMTP_DBG, "smtp_ack_expn_parse: Error\n");
	reset_server_buf (psmtp, &cur_token);
	return res;

}


void
smtp_cmd_expn_free (struct smtp_cmd_expn *expn)
{
	if (expn->user_name)
		smtp_display_name_free (expn->user_name);
	if (expn->mail_box)
		smtp_angle_addr_free (expn->mail_box);

	free_mem (&smtp_cmd_expn_pool, (void *) expn);
	DEBUG_SMTP (SMTP_MEM, "smtp_cmd_expn_free: pointer=%p, elm=%p\n",
		    &smtp_cmd_expn_pool, (void *) expn);
}


struct smtp_cmd_expn *
smtp_cmd_expn_new (int len, char *user_name, char *mail_box)
{
	struct smtp_cmd_expn *expn;
	expn = (struct smtp_cmd_expn *) alloc_mem (&smtp_cmd_expn_pool);
	if (expn == NULL) {
		return NULL;
	}
	DEBUG_SMTP (SMTP_MEM, "smtp_cmd_expn_new: pointer=%p, elm=%p\n",
		    &smtp_cmd_expn_pool, (void *) expn);

#ifdef USE_NEL
	NEL_REF_INIT (expn);
#endif
	expn->len = len;

	if (expn->user_name) {
		expn->user_name = user_name;
		expn->user_name_len = strlen (user_name);
	}
	else {
		expn->user_name = NULL;
		expn->user_name_len = 0;
	}
	if (expn->mail_box) {
		expn->mail_box = mail_box;
		expn->mail_box_len = strlen (mail_box);
	}
	else {
		expn->mail_box = NULL;
		expn->mail_box_len = 0;
	}

	return expn;
}


int
smtp_cmd_expn_parse ( struct smtp_info *psmtp, char *message, size_t length, size_t * index)
{
	struct smtp_cmd_expn *expn;
	char *display_name = NULL, *angle_addr = NULL;
	size_t cur_token = *index;
	int r, res;

	DEBUG_SMTP (SMTP_DBG, "smtp_cmd_expn_parse\n");

	/* if nel configureable variable var_disable_expn_cmd is set to 0, 
	   don't allow the command pass through */
	//var_disable_expn_cmd = 1;
	if (var_disable_expn_cmd == 1) {
		DEBUG_SMTP (SMTP_DBG,
			    "smtp_cmd_expn_parse: var_disable_expn_cmd\n");
		res = SMTP_ERROR_POLICY;
		goto err;
	}

	r = smtp_wsp_parse (message, length, &cur_token);
	if (r != SMTP_NO_ERROR) {
		res = r;
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
		DEBUG_SMTP (SMTP_DBG, "cur_token = %lu\n", cur_token);
		break;

	case SMTP_ERROR_CONTINUE:
		res = SMTP_ERROR_PARSE;
		goto err;

	case SMTP_ERROR_PARSE:
		r = smtp_wsp_display_name_parse (message, length, &cur_token,
						 &display_name);
		if (r != SMTP_NO_ERROR) {
			res = r;
			goto err;
		}
		DEBUG_SMTP (SMTP_DBG, "cur_token = %lu\n", cur_token);
		break;
	default:
		DEBUG_SMTP (SMTP_DBG, "\n");
		res = r;
		goto err;
	}
	DEBUG_SMTP (SMTP_DBG, "display_name = %s\n", display_name);

	r = smtp_wsp_unstrict_crlf_parse (message, length, &cur_token);
	if (r != SMTP_NO_ERROR) {
		res = r;
		goto free;
	}

	expn = smtp_cmd_expn_new (length, display_name, angle_addr);
	if (expn == NULL) {
		res = SMTP_ERROR_MEMORY;
		goto free;
	}
	display_name = angle_addr = NULL;

#ifdef USE_NEL
	DEBUG_SMTP (SMTP_DBG, "\n");
	if ((r = nel_env_analysis (eng, &(psmtp->env), expn_id,
				   (struct smtp_simple_event *) expn)) < 0) {
		DEBUG_SMTP (SMTP_DBG,
			    "smtp_cmd_expn_parse: nel_env_analysis error\n");
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

	DEBUG_SMTP (SMTP_DBG, "cur_token = %lu\n", cur_token);
	*index = cur_token;

	DEBUG_SMTP (SMTP_DBG, "\n");
	psmtp->last_cli_event_type = SMTP_CMD_EXPN;

	res = sync_client_data (psmtp, cur_token);
	if (res < 0) {
		goto err;
	}

	return SMTP_NO_ERROR;

      free:
	DEBUG_SMTP (SMTP_DBG, "Free\n");
	if (display_name)
		smtp_display_name_free (display_name);
	if (angle_addr)
		smtp_angle_addr_free (angle_addr);
	if (expn)
		smtp_cmd_expn_free (expn);

      err:
	DEBUG_SMTP (SMTP_DBG, "Error\n");
	reset_client_buf (psmtp, index);

	return res;

}
