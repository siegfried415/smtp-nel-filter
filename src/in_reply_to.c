

/*
 * $Id: in_reply_to.c,v 1.4 2005/11/29 06:29:26 xiay Exp $
  RFC 2045, RFC 2046, RFC 2047, RFC 2048, RFC 2049, RFC 2231, RFC 2387
  RFC 2424, RFC 2557, RFC 2183 Content-Disposition, RFC 1766  Language
 */

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "mmapstring.h"

#include "smtp.h"
#include "mime.h"
#include "in_reply_to.h"
#include "msg-id.h"

struct smtp_in_reply_to *
smtp_in_reply_to_new (clist * mid_list)
{
	struct smtp_in_reply_to *in_reply_to;

	in_reply_to = malloc (sizeof (*in_reply_to));
	if (in_reply_to == NULL)
		return NULL;
	DEBUG_SMTP (SMTP_MEM_1, "smtp_in_reply_to_new: MALLOC pointer=%p\n",
		    in_reply_to);

	in_reply_to->mid_list = mid_list;

	return in_reply_to;
}


void
smtp_in_reply_to_free (struct smtp_in_reply_to *in_reply_to)
{
	clist_foreach (in_reply_to->mid_list,
		       (clist_func) smtp_msg_id_free, NULL);
	clist_free (in_reply_to->mid_list);
	free (in_reply_to);
	DEBUG_SMTP (SMTP_MEM_1, "smtp_in_reply_to_free: FREE pointer=%p\n",
		    in_reply_to);
}

/*
in-reply-to     =       "In-Reply-To:" 1*msg-id CRLF
*/

int
smtp_msg_id_list_parse (const char *message, size_t length,
			size_t * index, clist ** result)
{
	return smtp_struct_multiple_parse (message, length, index,
					   result, (smtp_struct_parser *)
					   smtp_unstrict_msg_id_parse,
					   (smtp_struct_destructor *)
					   smtp_msg_id_free);
}


int
smtp_in_reply_to_parse (const char *message, size_t length,
			size_t * index, struct smtp_in_reply_to **result)
{
	struct smtp_in_reply_to *in_reply_to;
	size_t cur_token;
	clist *msg_id_list;
	int res;
	int r;

	cur_token = *index;

/*
  r = mailimf_token_case_insensitive_parse(message, length,
					   &cur_token, "In-Reply-To");
  if (r != SMTP_NO_ERROR) {
    res = r;
    goto err;
  }

  r = smtp_colon_parse(message, length, &cur_token);
  if (r != SMTP_NO_ERROR) {
    res = r;
    goto err;
  }
*/

	r = smtp_msg_id_list_parse (message, length, &cur_token,
				    &msg_id_list);
	if (r != SMTP_NO_ERROR) {
		res = r;
		goto err;
	}

	r = smtp_unstrict_crlf_parse (message, length, &cur_token);
	if (r != SMTP_NO_ERROR) {
		res = r;
		goto free_list;
	}

	in_reply_to = smtp_in_reply_to_new (msg_id_list);
	if (in_reply_to == NULL) {
		res = SMTP_ERROR_MEMORY;
		goto free_list;
	}

	*result = in_reply_to;
	*index = cur_token;

	return SMTP_NO_ERROR;

      free_list:
	clist_foreach (msg_id_list, (clist_func) smtp_msg_id_free, NULL);
	clist_free (msg_id_list);
      err:
	return res;
}
