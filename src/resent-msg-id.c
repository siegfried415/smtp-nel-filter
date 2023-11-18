


/*
 * $Id: resent-msg-id.c,v 1.3 2005/11/19 09:04:56 xiay Exp $
  RFC 2045, RFC 2046, RFC 2047, RFC 2048, RFC 2049, RFC 2231, RFC 2387
  RFC 2424, RFC 2557, RFC 2183 Content-Disposition, RFC 1766  Language
 */

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "mmapstring.h"

#include "smtp.h"
#include "mime.h"
#include "msg-id.h"
#include "message-id.h"
#include "resent-msg-id.h"

/*
resent-msg-id   =       "Resent-Message-ID:" msg-id CRLF
*/

int
smtp_resent_msg_id_parse (const char *message, size_t length,
			  size_t * index, struct smtp_message_id **result)
{
	char *value;
	size_t cur_token;
	struct smtp_message_id *message_id;
	int r;
	int res;

	cur_token = *index;

#if 0				//xiayu 2005.11.18 commented
	r = smtp_token_case_insensitive_parse (message, length,
					       &cur_token,
					       "Resent-Message-ID");
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

	r = smtp_msg_id_parse (message, length, &cur_token, &value);
	if (r != SMTP_NO_ERROR) {
		res = r;
		goto err;
	}

	r = smtp_unstrict_crlf_parse (message, length, &cur_token);
	if (r != SMTP_NO_ERROR) {
		res = r;
		goto free_value;
	}

	message_id = smtp_message_id_new (value);
	if (message_id == NULL) {
		res = SMTP_ERROR_MEMORY;
		goto free_value;
	}

	*result = message_id;
	*index = cur_token;

	return SMTP_NO_ERROR;

      free_value:
	smtp_msg_id_free (value);
      err:
	return res;
}
