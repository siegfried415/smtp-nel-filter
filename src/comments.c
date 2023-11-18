

/*
 * $Id: comments.c,v 1.4 2005/11/29 06:29:26 xiay Exp $
  RFC 2045, RFC 2046, RFC 2047, RFC 2048, RFC 2049, RFC 2231, RFC 2387
  RFC 2424, RFC 2557, RFC 2183 Content-Disposition, RFC 1766  Language
 */

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "mmapstring.h"

#include "smtp.h"
#include "comments.h"
#include "mime.h"

struct smtp_comments *
smtp_comments_new (char *cm_value)
{
	struct smtp_comments *comments;

	comments = malloc (sizeof (*comments));
	if (comments == NULL)
		return NULL;
	DEBUG_SMTP (SMTP_MEM_1, "smtp_comments_new: MALLOC pointer=%p\n",
		    comments);

	comments->cm_value = cm_value;

	return comments;
}


void
smtp_comments_free (struct smtp_comments *comments)
{
	smtp_unstructured_free (comments->cm_value);
	free (comments);
	DEBUG_SMTP (SMTP_MEM_1, "smtp_comments_free: FREE pointer=%p\n",
		    comments);
}


/*
comments        =       "Comments:" unstructured CRLF
*/

int
smtp_comments_parse (const char *message, size_t length,
		     size_t * index, struct smtp_comments **result)
{
	struct smtp_comments *comments;
	char *value;
	size_t cur_token;
	int r;
	int res;

	cur_token = *index;

#if 0				//xiayu 2005.11.18 commented
	r = smtp_token_case_insensitive_parse (message, length,
					       &cur_token, "Comments");
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
	r = smtp_unstructured_parse (message, length, &cur_token, &value);
	if (r != SMTP_NO_ERROR) {
		res = r;
		goto err;
	}

	r = smtp_unstrict_crlf_parse (message, length, &cur_token);
	if (r != SMTP_NO_ERROR) {
		res = r;
		goto free_value;
	}

	comments = smtp_comments_new (value);
	if (comments == NULL) {
		res = SMTP_ERROR_MEMORY;
		goto free_value;
	}

	*result = comments;
	*index = cur_token;

	return SMTP_NO_ERROR;

      free_value:
	smtp_unstructured_free (value);
      err:
	return res;
}
