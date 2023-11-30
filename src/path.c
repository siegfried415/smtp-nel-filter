
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "mem_pool.h"
#include "mmapstring.h"
#include "smtp.h"
#include "mime.h"
#include "path.h"
#include "address.h"

extern ObjPool_t smtp_mailbox_addr_pool;

struct smtp_path *
smtp_path_new (char *pt_addr_spec)
{
	struct smtp_path *path;

	path = malloc (sizeof (*path));
	if (path == NULL)
		return NULL;
	DEBUG_SMTP (SMTP_MEM, "smtp_path_new: MALLOC pointer=%p\n", path);

	path->pt_addr_spec = pt_addr_spec;

	return path;
}


void
smtp_path_free (struct smtp_path *path)
{
	if (path->pt_addr_spec != NULL)
		smtp_addr_spec_free (path->pt_addr_spec);
	free (path);
	DEBUG_SMTP (SMTP_MEM, "smtp_path_free: FREE pointer=%p\n", path);
}

/*
path            =       ([CFWS] "<" ([CFWS] / addr-spec) ">" [CFWS]) /
                        obs-path
*/

int
smtp_path_parse (const char *message, size_t length,
		 size_t * index, struct smtp_path **result)
{
	size_t cur_token;
	char *addr_spec;
	struct smtp_path *path;
	int res;
	int r;

	cur_token = *index;
	addr_spec = NULL;

	r = smtp_cfws_parse (message, length, &cur_token);
	if ((r != SMTP_NO_ERROR) && (r != SMTP_ERROR_PARSE)) {
		res = r;
		goto err;
	}

	r = smtp_lower_parse (message, length, &cur_token);
	if (r != SMTP_NO_ERROR) {
		res = r;
		goto err;
	}

	r = smtp_addr_spec_parse (message, length, &cur_token, &addr_spec);
	switch (r) {
	case SMTP_NO_ERROR:
		break;
	case SMTP_ERROR_PARSE:
		r = smtp_cfws_parse (message, length, &cur_token);
		if ((r != SMTP_NO_ERROR) && (r != SMTP_ERROR_PARSE)) {
			res = r;
			goto err;
		}
		break;
	default:
		return r;
	}

	r = smtp_greater_parse (message, length, &cur_token);
	if (r != SMTP_NO_ERROR) {
		res = r;
		goto err;
	}

	path = smtp_path_new (addr_spec);
	if (path == NULL) {
		res = SMTP_ERROR_MEMORY;
		goto free_addr_spec;
	}

	*index = cur_token;
	*result = path;

	return SMTP_NO_ERROR;

      free_addr_spec:
	if (addr_spec == NULL)
		smtp_addr_spec_free (addr_spec);
      err:
	return res;
}

/*
 * Path = "<" [ A-d-l ":" ] Mailbox ">"
 * A-d-l = At-domain *( "," A-d-l )
 *        ; Note that this form, the so-called "source route",
 *        ; MUST BE accepted, SHOULD NOT be generated, and SHOULD be
 *        ; ignored.
 */
int
smtp_adl_path_parse (char *message, size_t length, size_t * index,
		     char **address)
{
	char a[9096] = "";
	size_t cur_token;
	struct smtp_mailbox_list *mb_list = NULL;
	char *colon;
	int r, res;

	/*
	 * Some mailers send RFC822-style address forms (with comments and such)
	 * in SMTP envelopes. We cannot blame users for this: the blame is with
	 * programmers violating the RFC, and with sendmail for being permissive.
	 * 
	 * XXX The SMTP command tokenizer must leave the address in externalized
	 * (quoted) form, so that the address parser can correctly extract the
	 * address from surrounding junk.
	 * 
	 * XXX We have only one address parser, written according to the rules of
	 * RFC 822. That standard differs subtly from RFC 821.
	 */

	DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__, __FUNCTION__,
		    __LINE__);
	*address = NULL;
	cur_token = *index;

	DEBUG_SMTP (SMTP_DBG, "message + cur_token: %s\n",
		    message + cur_token);
	r = smtp_wsp_lower_parse (message, length, &cur_token);
	if (r != SMTP_NO_ERROR) {
		res = SMTP_ERROR_PARSE;
		goto err;
	}

	DEBUG_SMTP (SMTP_DBG, "message + cur_token: %s\n",
		    message + cur_token);
	r = smtp_wsp_at_sign_parse (message, length, &cur_token);
	if (r == SMTP_NO_ERROR) {
		DEBUG_SMTP (SMTP_DBG,
			    "smtp_adl_path_parse: message+cur_token: %s\n",
			    message + cur_token);
		if ((colon = strchr (message + cur_token, ':')) != 0) {
			DEBUG_SMTP (SMTP_DBG,
				    "smtp_adl_path_parse: cur_token = %lu\n",
				    cur_token);
			cur_token = colon - message + 1;
			DEBUG_SMTP (SMTP_DBG,
				    "smtp_adl_path_parse: message+cur_token: %s\n",
				    message + cur_token);
			DEBUG_SMTP (SMTP_DBG,
				    "smtp_adl_path_parse: colon: %s\n",
				    colon);
		}
		else {
			res = SMTP_ERROR_PARSE;
			goto err;
		}
	}

	DEBUG_SMTP (SMTP_DBG, "message + cur_token: %s\n",
		    message + cur_token);
	DEBUG_SMTP (SMTP_DBG, "cur_token = %lu\n", cur_token);
	r = smtp_wsp_mailbox_list_parse (message, length, &cur_token,
					 &mb_list);
	if (r != SMTP_NO_ERROR) {
		if (r == SMTP_ERROR_PARSE) {
			res = SMTP_ERROR_PARSE;
			goto err;
		}
		res = r;
		goto err;
	}

	DEBUG_SMTP (SMTP_DBG, "message+cur_token: %s\n", message + cur_token);
	r = smtp_wsp_greater_parse (message, length, &cur_token);
	if (r != SMTP_NO_ERROR) {
		res = SMTP_ERROR_PARSE;
		smtp_mailbox_list_free (mb_list);
		goto err;
	}

	DEBUG_SMTP (SMTP_DBG, "message+cur_token: %s\n", message + cur_token);
	DEBUG_SMTP (SMTP_DBG, "cur_token = %lu length = %lu\n", cur_token,
		    length);
	/* 
	 *  should be more flexible
	 *    strict_rfc822
	 *    allow_empty_addr
	 *        PERMIT_EMPTY_ADDR     1
	 *        REJECT_EMPTY_ADDR     0
	 */
	DEBUG_SMTP (SMTP_DBG, "mb_list->number: %d\n",
		    mb_list->mb_list->number);
	if (mb_list->mb_list->number == 0) {
		DEBUG_SMTP (SMTP_DBG, "\n");
		res = SMTP_ERROR_PARSE;
		smtp_mailbox_list_free (mb_list);
		goto err;
	}
	if (mb_list->mb_list->number > 1) {
		DEBUG_SMTP (SMTP_DBG, "\n");
		res = SMTP_ERROR_PARSE;
		smtp_mailbox_list_free (mb_list);
		goto err;
	}

	DEBUG_SMTP (SMTP_DBG, "first = %p\n", mb_list->mb_list->first);
	DEBUG_SMTP (SMTP_DBG, "first->data = %p\n",
		    mb_list->mb_list->first->data);
	DEBUG_SMTP (SMTP_DBG, "mb_addr_spec = %p\n",
		    ((struct smtp_mailbox *) (mb_list->mb_list->first->
					      data))->mb_addr_spec);
	*address =
		mailbox_addr_dup (((struct smtp_mailbox *) (mb_list->mb_list->
							    first->data))->
				  mb_addr_spec);
	if (*address == NULL) {
		res = SMTP_ERROR_MEMORY;
		smtp_mailbox_list_free (mb_list);
		goto err;
	}
	smtp_mailbox_list_free (mb_list);
	DEBUG_SMTP (SMTP_DBG, "*address = %s\n", *address);

	*index = cur_token;

	return SMTP_NO_ERROR;

      err:
	DEBUG_SMTP (SMTP_DBG, "free mb_list\n");
	if (mb_list)
		smtp_mailbox_list_free (mb_list);

	DEBUG_SMTP (SMTP_DBG, "free address\n");
	if (*address) {
		free_mem (&smtp_mailbox_addr_pool, *address);
		DEBUG_SMTP (SMTP_MEM,
			    "smtp_adl_path_parse: pointer=%p, elm=%p\n",
			    &smtp_mailbox_addr_pool, address);
		*address = NULL;
	}
	DEBUG_SMTP (SMTP_DBG, "resturn \n");

	return res;

}

int
smtp_reverse_path_parse (char *message, size_t length, size_t * index,
			 char **address)
{
	return smtp_adl_path_parse (message, length, index, address);
}

int
smtp_forward_path_parse (char *message, size_t length, size_t * index,
			 char **address)
{
	return smtp_adl_path_parse (message, length, index, address);
}
