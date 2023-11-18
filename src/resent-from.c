


/*
 * $Id: resent-from.c,v 1.4 2005/11/19 09:04:56 xiay Exp $
  RFC 2045, RFC 2046, RFC 2047, RFC 2048, RFC 2049, RFC 2231, RFC 2387
  RFC 2424, RFC 2557, RFC 2183 Content-Disposition, RFC 1766  Language
 */

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "mmapstring.h"

#include "smtp.h"
#include "address.h"

/*
resent-from     =       "Resent-From:" mailbox-list CRLF
*/

int
smtp_resent_from_parse (const char *message, size_t length,
			size_t * index, struct smtp_from **result)
{
	struct smtp_mailbox_list *mb_list;
	struct smtp_from *from;
	size_t cur_token;
	int r;
	int res;

	cur_token = *index;

#if 0				//xiayu 2005.11.18 commented
	r = smtp_token_case_insensitive_parse (message, length,
					       &cur_token, "Resent-From");
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

	*result = from;
	*index = cur_token;

	return SMTP_NO_ERROR;

      free_mb_list:
	smtp_mailbox_list_free (mb_list);
      err:
	return res;
}
