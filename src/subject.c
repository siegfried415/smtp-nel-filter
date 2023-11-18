


/*
 * $Id: subject.c,v 1.4 2005/11/29 06:29:26 xiay Exp $
  RFC 2045, RFC 2046, RFC 2047, RFC 2048, RFC 2049, RFC 2231, RFC 2387
  RFC 2424, RFC 2557, RFC 2183 Content-Disposition, RFC 1766  Language
 */

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "mmapstring.h"

#include "smtp.h"
#include "mime.h"
#include "subject.h"


struct smtp_subject *
smtp_subject_new (char *sbj_value)
{
	struct smtp_subject *subject;

	subject = malloc (sizeof (*subject));
	if (subject == NULL)
		return NULL;
	DEBUG_SMTP (SMTP_MEM_1, "smtp_subject_new: MALLOC pointer=%p\n",
		    subject);

	subject->sbj_value = sbj_value;

	return subject;
}


void
smtp_subject_free (struct smtp_subject *subject)
{
	smtp_unstructured_free (subject->sbj_value);
	free (subject);
	DEBUG_SMTP (SMTP_MEM_1, "smtp_subject_free: FREE pointer=%p\n",
		    subject);
}

/*
subject         =       "Subject:" unstructured CRLF
*/

int
smtp_subject_parse (const char *message, size_t length,
		    size_t * index, struct smtp_subject **result)
{
	struct smtp_subject *subject;
	char *value;
	size_t cur_token;
	int r;
	int res;

	cur_token = *index;

#if 0
	r = smtp_token_case_insensitive_parse (message, length,
					       &cur_token, "Subject");
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

	DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__, __FUNCTION__,
		    __LINE__);
	r = smtp_unstructured_parse (message, length, &cur_token, &value);
	if (r != SMTP_NO_ERROR) {
		res = r;
		goto err;
	}

	DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__, __FUNCTION__,
		    __LINE__);
	r = smtp_unstrict_crlf_parse (message, length, &cur_token);
	if (r != SMTP_NO_ERROR) {
		res = r;
		goto free_value;
	}

	DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__, __FUNCTION__,
		    __LINE__);
	subject = smtp_subject_new (value);
	if (subject == NULL) {
		res = SMTP_ERROR_MEMORY;
		goto free_value;
	}

	DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__, __FUNCTION__,
		    __LINE__);
	*result = subject;
	*index = cur_token;

	return SMTP_NO_ERROR;

      free_value:
	DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__, __FUNCTION__,
		    __LINE__);
	smtp_unstructured_free (value);
      err:
	DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__, __FUNCTION__,
		    __LINE__);
	return res;
}
