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
	DEBUG_SMTP (SMTP_MEM, "smtp_subject_new: MALLOC pointer=%p\n",
		    subject);

	subject->sbj_value = sbj_value;

	return subject;
}


void
smtp_subject_free (struct smtp_subject *subject)
{
	smtp_unstructured_free (subject->sbj_value);
	free (subject);
	DEBUG_SMTP (SMTP_MEM, "smtp_subject_free: FREE pointer=%p\n",
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

	subject = smtp_subject_new (value);
	if (subject == NULL) {
		res = SMTP_ERROR_MEMORY;
		goto free_value;
	}

	*result = subject;
	*index = cur_token;

	return SMTP_NO_ERROR;

      free_value:
	smtp_unstructured_free (value);
      err:
	return res;
}
