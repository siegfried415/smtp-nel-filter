

/*
 * $Id: cc.c,v 1.4 2005/11/29 06:29:26 xiay Exp $
  RFC 2045, RFC 2046, RFC 2047, RFC 2048, RFC 2049, RFC 2231, RFC 2387
  RFC 2424, RFC 2557, RFC 2183 Content-Disposition, RFC 1766  Language
 */

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "mmapstring.h"

#include "smtp.h"
#include "address.h"
#include "cc.h"
#include "mime.h"

struct smtp_cc *
smtp_cc_new (struct smtp_address_list *cc_addr_list)
{
	struct smtp_cc *cc;

	cc = malloc (sizeof (*cc));
	if (cc == NULL)
		return NULL;
	DEBUG_SMTP (SMTP_MEM_1, "smtp_cc_new: MALLOC pointer=%p\n", cc);

	cc->cc_addr_list = cc_addr_list;

	return cc;
}


void
smtp_cc_free (struct smtp_cc *cc)
{
	if (cc->cc_addr_list != NULL)
		smtp_address_list_free (cc->cc_addr_list);
	free (cc);
	DEBUG_SMTP (SMTP_MEM_1, "smtp_cc_free: FREE pointer=%p\n", cc);
}

/*
cc              =       "Cc:" address-list CRLF
*/


int
smtp_cc_parse (const char *message, size_t length,
	       size_t * index, struct smtp_cc **result)
{
	struct smtp_address_list *addr_list;
	struct smtp_cc *cc;
	size_t cur_token;
	int r;
	int res;

	cur_token = *index;
#if 0
	r = smtp_token_case_insensitive_parse (message, length,
					       &cur_token, "Cc");
	if (r != SMTP_NO_ERROR) {
		res = r;
		DEBUG_SMTP (SMTP_DBG, "\n");
		goto err;
	}

	DEBUG_SMTP (SMTP_DBG, "\n");
	r = smtp_colon_parse (message, length, &cur_token);
	if (r != SMTP_NO_ERROR) {
		res = r;
		DEBUG_SMTP (SMTP_DBG, "\n");
		goto err;
	}
#endif

	DEBUG_SMTP (SMTP_DBG, "\n");
	r = smtp_address_list_parse (message, length, &cur_token, &addr_list);
	if (r != SMTP_NO_ERROR) {
		res = r;
		DEBUG_SMTP (SMTP_DBG, "\n");
		goto err;
	}

	DEBUG_SMTP (SMTP_DBG, "\n");
	r = smtp_unstrict_crlf_parse (message, length, &cur_token);
	if (r != SMTP_NO_ERROR) {
		res = r;
		DEBUG_SMTP (SMTP_DBG, "\n");
		goto free_addr_list;
	}

	DEBUG_SMTP (SMTP_DBG, "\n");
	cc = smtp_cc_new (addr_list);
	if (cc == NULL) {
		res = SMTP_ERROR_MEMORY;
		DEBUG_SMTP (SMTP_DBG, "\n");
		goto free_addr_list;
	}

	DEBUG_SMTP (SMTP_DBG, "\n");
	*result = cc;
	*index = cur_token;

	return SMTP_NO_ERROR;

free_addr_list:
	DEBUG_SMTP (SMTP_DBG, "Free\n");
	smtp_address_list_free (addr_list);
err:
	DEBUG_SMTP (SMTP_DBG, "Error\n");
	return res;
}
