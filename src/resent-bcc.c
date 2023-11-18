

/*
 * $Id: resent-bcc.c,v 1.3 2005/11/19 09:04:56 xiay Exp $
  RFC 2045, RFC 2046, RFC 2047, RFC 2048, RFC 2049, RFC 2231, RFC 2387
  RFC 2424, RFC 2557, RFC 2183 Content-Disposition, RFC 1766  Language
 */

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "mmapstring.h"

#include "smtp.h"
#include "address.h"
#include "bcc.h"
#include "resent-bcc.h"

/*
resent-bcc      =       "Resent-Bcc:" (address-list / [CFWS]) CRLF
*/

int
smtp_resent_bcc_parse (const char *message, size_t length,
		       size_t * index, struct smtp_bcc **result)
{
	struct smtp_address_list *addr_list;
	struct smtp_bcc *bcc;
	size_t cur_token;
	int r;
	int res;

	cur_token = *index;
	bcc = NULL;

#if 0				//xiayu 2005.11.18 commented
	r = smtp_token_case_insensitive_parse (message, length,
					       &cur_token, "Resent-Bcc");
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
	if ((r != SMTP_NO_ERROR) && (r != SMTP_ERROR_PARSE)) {
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

	return TRUE;

      free_addr_list:
	smtp_address_list_free (addr_list);
      err:
	return res;
}
