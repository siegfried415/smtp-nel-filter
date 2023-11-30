#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "mmapstring.h"
#include "smtp.h"
#include "mime.h"
#include "x-token.h"

/*
x  x-token := <The two characters "X-" or "x-" followed, with
              no  intervening white space, by any token>
*/


int
smtp_mime_x_token_parse (const char *message, size_t length,
			 size_t * index, char **result)
{
	size_t cur_token;
	char *token;
	char *x_token;
	int min_x;
	int res, r;
	char *value;

	cur_token = *index;
	min_x = FALSE;

	r = smtp_mime_token_parse (message, length, &cur_token, &token);
	if (r != SMTP_NO_ERROR) {
		res = r;
		goto free_token;
	}

	x_token = (char *) malloc (2 + strlen (token) + 1);
	if (x_token == NULL) {
		res = SMTP_ERROR_MEMORY;
		goto free_token;
	}
	DEBUG_SMTP (SMTP_MEM,
		    "smtp_mime_x_token_parse: MALLOC pointer=%p\n", x_token);

	if (min_x) {
		/* x_token = g_strconcat("x-", token, NULL); */
		sprintf (x_token, "x-%s", token);
	}
	else {
		/* x_token = g_strconcat("X-", token, NULL); */
		sprintf (x_token, "X-%s", token);
	}

	r = smtp_colon_parse (message, length, &cur_token);
	if (r != SMTP_NO_ERROR) {
		res = r;
		goto free_x_token;
	}

	r = smtp_unstructured_parse (message, length, &cur_token, &value);
	if (r != SMTP_NO_ERROR) {
		res = r;
		goto free_x_token;
	}

	r = smtp_unstrict_crlf_parse (message, length, &cur_token);
	if (r != SMTP_NO_ERROR) {
		res = r;
		goto free_x_token;
	}

	smtp_mime_token_free (token);
	*result = x_token;
	*index = cur_token;

	return SMTP_NO_ERROR /*TRUE*/;


      free_x_token:
	free (x_token);
	DEBUG_SMTP (SMTP_MEM, "smtp_mime_x_token_parse: FREE pointer=%p\n",
		    x_token);

      free_token:
	smtp_mime_token_free (token);

      err:
	return res;

}
