#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "mmapstring.h"
#include "mem_pool.h"
#include "smtp.h"
#include "address.h"
#include "to.h"
#include "mime.h" 

#ifdef USE_NEL
#include "engine.h"
extern struct nel_eng *eng;
int to_id;
#endif

ObjPool_t smtp_msg_to_pool;


void
smtp_msg_to_init (
#ifdef USE_NEL
			 struct nel_eng *eng
#endif
	)
{
	create_mem_pool (&smtp_msg_to_pool,
			 sizeof (struct smtp_msg_to), SMTP_MEM_STACK_DEPTH);
}


void
smtp_msg_to_free (struct smtp_msg_to *to)
{
	if (to->to_addr_list != NULL)
		smtp_address_list_free (to->to_addr_list);
	free_mem (&smtp_msg_to_pool, (void *) to);
	DEBUG_SMTP (SMTP_MEM, "smtp_msg_to_free: pointer=%p, elm=%p\n",
		    &smtp_msg_to_pool, (void *) to);
}

struct smtp_msg_to *
smtp_msg_to_new (struct smtp_address_list *to_addr_list)
{
	struct smtp_msg_to *to;
	to = (struct smtp_msg_to *) alloc_mem (&smtp_msg_to_pool);
	if (to == NULL)
		return NULL;
	DEBUG_SMTP (SMTP_MEM, "smtp_msg_to_new: pointer=%p, elm=%p\n",
		    &smtp_msg_to_pool, (void *) to);

#ifdef USE_NEL
	NEL_REF_INIT (to);
#endif

	to->to_addr_list = to_addr_list;
	return to;
}



/*
 * to = "To:" address-list CRLF
 */
int
smtp_msg_to_parse (struct smtp_info *psmtp,
		   const char *message, 
			size_t length, 
			size_t * index,
		   struct smtp_msg_to **result)
{
	struct smtp_address_list *addr_list;
	struct smtp_msg_to *to = NULL;
	size_t cur_token;
	int r, res;

	cur_token = *index;
	r = smtp_address_list_parse (message, length, &cur_token, &addr_list);
	if (r != SMTP_NO_ERROR) {
		res = r;
		goto err;
	}

	DEBUG_SMTP (SMTP_DBG, "message+cur_token: %s\n", message + cur_token);
	r = smtp_unstrict_crlf_parse (message, length, &cur_token);
	if (r != SMTP_NO_ERROR) {
		res = r;
		goto free_addr_list;
	}

	to = smtp_msg_to_new (addr_list);
	if (to == NULL) {
		res = SMTP_ERROR_MEMORY;
		goto free_addr_list;
	}

#ifdef USE_NEL
	if ((r = nel_env_analysis (eng, &(psmtp->env), to_id,
				   (struct smtp_simple_event *) to)) < 0) {
		DEBUG_SMTP (SMTP_DBG,
			    "smtp_msg_to_parse: nel_env_analysis error\n");
		smtp_msg_to_free (to);
		return SMTP_ERROR_ENGINE;
	}
#endif

	if (psmtp->permit & SMTP_PERMIT_DENY) {
		res = SMTP_ERROR_POLICY;
		goto err;

	}
	else if (psmtp->permit & SMTP_PERMIT_DROP) {
		DEBUG_SMTP(SMTP_DBG,  "found a drop event, no drop for message, denied.\n");
		res = SMTP_ERROR_POLICY;
		goto err;
	}

	*result = to;
	*index = cur_token;

	return SMTP_NO_ERROR;

      free_addr_list:
	smtp_address_list_free (addr_list);

      err:
	return res;
}
