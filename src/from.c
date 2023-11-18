/*
 * $Id: from.c,v 1.15 2005/12/20 08:30:43 xiay Exp $
 * RFC 2045, RFC 2046, RFC 2047, RFC 2048, RFC 2049, RFC 2231, RFC 2387
 * RFC 2424, RFC 2557, RFC 2183 Content-Disposition, RFC 1766  Language
 */

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "mmapstring.h"
#include "mem_pool.h"

#include "smtp.h"
#include "address.h"
#include "from.h"

#ifdef USE_NEL
#include "engine.h"
#include "analysis.h"

extern struct nel_eng *eng;
int from_id;

#endif

//extern struct nel_eng *eng;
ObjPool_t smtp_msg_from_pool;


void
smtp_msg_from_init (
#ifdef USE_NEL
			   struct nel_eng *eng
#endif
	)
{
	create_mem_pool (&smtp_msg_from_pool,
			 sizeof (struct smtp_from), SMTP_MEM_STACK_DEPTH);
}


void
smtp_msg_from_free (struct smtp_from *from)
{
	if (from->frm_mb_list != NULL)
		smtp_mailbox_list_free (from->frm_mb_list);
	free_mem (&smtp_msg_from_pool, (void *) from);
	DEBUG_SMTP (SMTP_MEM, "smtp_msg_from_free: pointer=%p, elm=%p\n",
		    &smtp_msg_from_pool, (void *) from);
}

struct smtp_from *
smtp_msg_from_new (struct smtp_mailbox_list *frm_mb_list)
{
	struct smtp_from *from;
	//from = malloc(sizeof(* from));
	from = (struct smtp_from *) alloc_mem (&smtp_msg_from_pool);
	if (from == NULL)
		return NULL;
	DEBUG_SMTP (SMTP_MEM, "smtp_msg_from_new: pointer=%p, elm=%p\n",
		    &smtp_msg_from_pool, (void *) from);
	//from->event_type = SMTP_FIELD_FROM;
	//from->nel_id = from_id;
	//from->len = len;

#ifdef USE_NEL
	//from->count = 0;
	NEL_REF_INIT (from);
#endif

	from->frm_mb_list = frm_mb_list;
	from->err_cnt = 0;
	return from;
}


/*
 * from = "From:" mailbox-list CRLF
 */

int
smtp_msg_from_parse ( /* struct neti_tcp_stream *ptcp, */ struct smtp_info
		     *psmtp,
		     const char *message, size_t length, size_t * index,
		     struct smtp_from **result)
{
	struct smtp_mailbox_list *mb_list;
	struct smtp_from *from;
	size_t cur_token;
	int r, res;

	cur_token = *index;

	//r = smtp_token_case_insensitive_parse(message, length,
	//                                         &cur_token, "From");
	//if (r != SMTP_NO_ERROR) {
	//      res = r;
	//      goto err;
	//}

	//r = smtp_colon_parse(message, length, &cur_token);
	//if (r != SMTP_NO_ERROR) {
	//      res = r;
	//      goto err;
	//}

	DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__, __FUNCTION__,
		    __LINE__);
	r = smtp_mailbox_list_parse (message, length, &cur_token, &mb_list);
	if (r != SMTP_NO_ERROR) {
		res = r;
		goto err;
	}

	DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__, __FUNCTION__,
		    __LINE__);
	r = smtp_unstrict_crlf_parse (message, length, &cur_token);
	if (r != SMTP_NO_ERROR) {
		res = r;
		goto free_mb_list;
	}

	DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__, __FUNCTION__,
		    __LINE__);
	from = smtp_msg_from_new (mb_list);
	if (from == NULL) {
		res = SMTP_ERROR_MEMORY;
		goto free_mb_list;
	}

#ifdef USE_NEL
	if ((r = nel_env_analysis (eng, &(psmtp->env), from_id,
				   (struct smtp_simple_event *) from)) < 0) {
		DEBUG_SMTP (SMTP_DBG,
			    "smtp_from_parse: nel_env_analysis error\n");
		smtp_msg_from_free (from);
		return SMTP_ERROR_ENGINE;
	}
#endif

	if (psmtp->permit & SMTP_PERMIT_DENY) {
		//fprintf(stderr, "found a deny event\n");
		//smtp_close_connection(psmtp);
		return SMTP_ERROR_POLICY;

	}
	else if (psmtp->permit & SMTP_PERMIT_DROP) {
		fprintf (stderr,
			 "found a drop event, no drop for message, denied.\n");
		//smtp_close_connection(psmtp);
		return SMTP_ERROR_POLICY;
	}

	DEBUG_SMTP (SMTP_DBG, "\n");
	*result = from;
	*index = cur_token;

	return SMTP_NO_ERROR;

      free_mb_list:
	smtp_mailbox_list_free (mb_list);
      err:
	return res;

}
