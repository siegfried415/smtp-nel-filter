
/*
 * $Id: bcc.c,v 1.4 2005/11/29 06:29:26 xiay Exp $
  RFC 2045, RFC 2046, RFC 2047, RFC 2048, RFC 2049, RFC 2231, RFC 2387
  RFC 2424, RFC 2557, RFC 2183 Content-Disposition, RFC 1766  Language
 */

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "mmapstring.h"
//#include "smtp.h"
//#include "headers.h"

#include "smtp.h"
#include "address.h"
#include "bcc.h"
#include "mime.h"


struct smtp_bcc *
smtp_bcc_new (struct smtp_address_list *bcc_addr_list)
{
	struct smtp_bcc *bcc;

	bcc = malloc (sizeof (*bcc));
	if (bcc == NULL)
		return NULL;
	DEBUG_SMTP (SMTP_MEM_1, "smtp_bcc_new: MALLOC pointer=%p\n", bcc);

	bcc->bcc_addr_list = bcc_addr_list;

	return bcc;
}


void
smtp_bcc_free (struct smtp_bcc *bcc)
{
	if (bcc->bcc_addr_list != NULL)
		smtp_address_list_free (bcc->bcc_addr_list);
	free (bcc);
	DEBUG_SMTP (SMTP_MEM_1, "smtp_bcc_free: FREE pointer=%p\n", bcc);
}

/*
bcc             =       "Bcc:" (address-list / [CFWS]) CRLF
*/


int
smtp_bcc_parse (const char *message, size_t length,
		size_t * index, struct smtp_bcc **result)
{
	struct smtp_address_list *addr_list;
	struct smtp_bcc *bcc;
	size_t cur_token;
	int r;
	int res;

	cur_token = *index;
	addr_list = NULL;

#if 0
	r = smtp_token_case_insensitive_parse (message, length,
					       &cur_token, "Bcc");
	if (r != SMTP_NO_ERROR) {
		res = r;
		goto err;
	}

	r = smtp_colon_parse (message, length, &cur_token);
	if (r != SMTP_NO_ERROR) {
		res = r;
		goto err;
	}
#endif

	r = smtp_address_list_parse (message, length, &cur_token, &addr_list);
	switch (r) {
	case SMTP_NO_ERROR:
		/* do nothing */
		break;
	case SMTP_ERROR_PARSE:
		r = smtp_cfws_parse (message, length, &cur_token);
		if ((r != SMTP_NO_ERROR) && (r != SMTP_ERROR_PARSE)) {
			res = r;
			goto err;
		}
		break;
	default:
		res = r;
		goto err;
	}

	r = smtp_unstrict_crlf_parse (message, length, &cur_token);
	if (r != SMTP_NO_ERROR) {
		res = r;
		goto free_addr_list;
	}

	bcc = smtp_bcc_new (addr_list);
	if (bcc == NULL) {
		res = SMTP_ERROR_MEMORY;
		goto free_addr_list;
	}

	*result = bcc;
	*index = cur_token;

	return SMTP_NO_ERROR;

free_addr_list:
	smtp_address_list_free (addr_list);
err:
	return res;
}