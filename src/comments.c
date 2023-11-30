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
	DEBUG_SMTP (SMTP_MEM, "smtp_comments_new: MALLOC pointer=%p\n",
		    comments);

	comments->cm_value = cm_value;

	return comments;
}


void
smtp_comments_free (struct smtp_comments *comments)
{
	smtp_unstructured_free (comments->cm_value);
	free (comments);
	DEBUG_SMTP (SMTP_MEM, "smtp_comments_free: FREE pointer=%p\n",
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
