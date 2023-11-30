#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "mmapstring.h"
#include "mem_pool.h"
#include "smtp.h"
#include "mime.h" 
#include "address.h"
#include "from.h"

#ifdef USE_NEL
#include "engine.h"
#include "analysis.h"

extern struct nel_eng *eng;
int from_id;

#endif

ObjPool_t smtp_msg_from_pool;


void
smtp_msg_from_init (
#ifdef USE_NEL
			   struct nel_eng *eng
#endif
	)
{
	create_mem_pool (&smtp_msg_from_pool,
			 sizeof (struct smtp_msg_from), SMTP_MEM_STACK_DEPTH);
}


void
smtp_msg_from_free (struct smtp_msg_from *from)
{
	if (from->frm_mb_list != NULL)
		smtp_mailbox_list_free (from->frm_mb_list);
	free_mem (&smtp_msg_from_pool, (void *) from);
	DEBUG_SMTP (SMTP_MEM, "smtp_msg_from_free: pointer=%p, elm=%p\n",
		    &smtp_msg_from_pool, (void *) from);
}

struct smtp_msg_from *
smtp_msg_from_new (struct smtp_mailbox_list *frm_mb_list)
{
	struct smtp_msg_from *from;
	from = (struct smtp_msg_from *) alloc_mem (&smtp_msg_from_pool);
	if (from == NULL)
		return NULL;
	DEBUG_SMTP (SMTP_MEM, "smtp_msg_from_new: pointer=%p, elm=%p\n",
		    &smtp_msg_from_pool, (void *) from);

#ifdef USE_NEL
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
smtp_msg_from_parse ( struct smtp_info *psmtp,
		     const char *message, 
			size_t length, 
			size_t * index,
		     struct smtp_msg_from **result)
{
	struct smtp_mailbox_list *mb_list;
	struct smtp_msg_from *from;
	size_t cur_token;
	int r, res;

	cur_token = *index;
	r = smtp_mailbox_list_parse (message, length, &cur_token, &mb_list);
	if (r != SMTP_NO_ERROR) {
		res = r;
		goto err;
	}

	r = smtp_unstrict_crlf_parse (message, length, &cur_token);
	if (r != SMTP_NO_ERROR) {
		res = r;
		goto free_mb_list;
	}

	from = smtp_msg_from_new (mb_list);
	if (from == NULL) {
		res = SMTP_ERROR_MEMORY;
		goto free_mb_list;
	}

#ifdef USE_NEL
	if ((r = nel_env_analysis (eng, &(psmtp->env), from_id,
				   (struct smtp_simple_event *) from)) < 0) {
		DEBUG_SMTP (SMTP_DBG,
			    "smtp_msg_from_parse: nel_env_analysis error\n");
		smtp_msg_from_free (from);
		return SMTP_ERROR_ENGINE;
	}
#endif

	if (psmtp->permit & SMTP_PERMIT_DENY) {
		return SMTP_ERROR_POLICY;

	}
	else if (psmtp->permit & SMTP_PERMIT_DROP) {
		fprintf (stderr,
			 "found a drop event, no drop for message, denied.\n");
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
