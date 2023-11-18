/*
 * $Id: to.c,v 1.12 2005/12/20 08:30:43 xiay Exp $
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
#include "to.h"

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
			 sizeof (struct smtp_to), SMTP_MEM_STACK_DEPTH);
}


void
smtp_msg_to_free (struct smtp_to *to)
{
	if (to->to_addr_list != NULL)
		smtp_address_list_free (to->to_addr_list);
	//free(to);
	free_mem (&smtp_msg_to_pool, (void *) to);
	DEBUG_SMTP (SMTP_MEM, "smtp_msg_to_free: pointer=%p, elm=%p\n",
		    &smtp_msg_to_pool, (void *) to);
}

struct smtp_to *
smtp_msg_to_new (struct smtp_address_list *to_addr_list)
{
	struct smtp_to *to;
	//to = malloc(sizeof(* to));
	to = (struct smtp_to *) alloc_mem (&smtp_msg_to_pool);
	if (to == NULL)
		return NULL;
	DEBUG_SMTP (SMTP_MEM, "smtp_msg_to_new: pointer=%p, elm=%p\n",
		    &smtp_msg_to_pool, (void *) to);

#ifdef USE_NEL
	//to->count = 0;
	NEL_REF_INIT (to);
#endif

	to->to_addr_list = to_addr_list;
	return to;
}



/*
 * to = "To:" address-list CRLF
 */
int
smtp_msg_to_parse ( /*struct neti_tcp_stream *ptcp, */ struct smtp_info
		   *psmtp,
		   const char *message, size_t length, size_t * index,
		   struct smtp_to **result)
{
	struct smtp_address_list *addr_list;
	struct smtp_to *to = NULL;
	size_t cur_token;
	int r, res;

	cur_token = *index;

	//r = smtp_msg_token_case_insensitive_parse(message, length,
	//                                         &cur_token, "To");
	//if (r != SMTP_NO_ERROR) {
	//      res = r;
	//      goto err;
	//}
	//
	//r = smtp_colon_parse(message, length, &cur_token);
	//if (r != SMTP_NO_ERROR) {
	//      res = r;
	//      goto err;
	//}

	DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__, __FUNCTION__,
		    __LINE__);
	r = smtp_address_list_parse (message, length, &cur_token, &addr_list);
	if (r != SMTP_NO_ERROR) {
		res = r;
		goto err;
	}

	DEBUG_SMTP (SMTP_DBG, "message+cur_token: %s\n", message + cur_token);
	r = smtp_unstrict_crlf_parse (message, length, &cur_token);
	if (r != SMTP_NO_ERROR) {
		DEBUG_SMTP (SMTP_DBG, "\n");
		res = r;
		goto free_addr_list;
	}

	DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__, __FUNCTION__,
		    __LINE__);
	to = smtp_msg_to_new (addr_list);
	if (to == NULL) {
		res = SMTP_ERROR_MEMORY;
		goto free_addr_list;
	}

#ifdef USE_NEL
	if ((r = nel_env_analysis (eng, &(psmtp->env), to_id,
				   (struct smtp_simple_event *) to)) < 0) {
		DEBUG_SMTP (SMTP_DBG,
			    "smtp_from_parse: nel_env_analysis error\n");
		smtp_msg_to_free (to);
		return SMTP_ERROR_ENGINE;
	}
#endif

	if (psmtp->permit & SMTP_PERMIT_DENY) {
		//fprintf(stderr, "found a deny event\n");
		//smtp_close_connection(psmtp);
		res = SMTP_ERROR_POLICY;
		goto err;

	}
	else if (psmtp->permit & SMTP_PERMIT_DROP) {
		fprintf (stderr,
			 "found a drop event, no drop for message, denied.\n");
		//smtp_close_connection(psmtp);
		res = SMTP_ERROR_POLICY;
		goto err;
	}

	*result = to;
	*index = cur_token;

	return SMTP_NO_ERROR;

      free_addr_list:
	DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__, __FUNCTION__,
		    __LINE__);
	smtp_address_list_free (addr_list);

      err:
	DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__, __FUNCTION__,
		    __LINE__);
	// *index = cur_token;
	return res;
}
