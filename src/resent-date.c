#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "mmapstring.h"
#include "smtp.h"
#include "mime.h"
#include "_time.h"

/*
resent-date     =       "Resent-Date:" date-time CRLF
*/

int
smtp_resent_date_parse (const char *message, size_t length,
			size_t * index, struct smtp_orig_date **result)
{
	struct smtp_orig_date *orig_date;
	struct smtp_date_time *date_time;
	size_t cur_token;
	int r;
	int res;

	cur_token = *index;

	r = smtp_date_time_parse (message, length, &cur_token, &date_time);
	if (r != SMTP_NO_ERROR) {
		res = r;
		goto err;
	}

	r = smtp_unstrict_crlf_parse (message, length, &cur_token);
	if (r != SMTP_NO_ERROR) {
		res = r;
		goto free_date_time;
	}

	orig_date = smtp_orig_date_new (date_time);
	if (orig_date == NULL) {
		res = SMTP_ERROR_MEMORY;
		goto free_date_time;
	}

	*result = orig_date;
	*index = cur_token;

	return SMTP_NO_ERROR;

      free_date_time:
	smtp_date_time_free (date_time);
      err:
	return res;
}
