


/*
 * $Id: x-token.c,v 1.4 2005/11/29 06:29:26 xiay Exp $
  RFC 2045, RFC 2046, RFC 2047, RFC 2048, RFC 2049, RFC 2231, RFC 2387
  RFC 2424, RFC 2557, RFC 2183 Content-Disposition, RFC 1766  Language
 */

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

#if 0
	if (!smtp_char_parse (message, length, &cur_token, 'x')) {
		if (!smtp_char_parse (message, length, &cur_token, 'X'))
			return FALSE;
		min_x = FALSE;
	}
	else
		min_x = TRUE;

	if (!smtp_char_parse (message, length, &cur_token, '-'))
		return FALSE;
#else
	min_x = FALSE;
#endif


	DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__, __FUNCTION__,
		    __LINE__);
	r = smtp_mime_token_parse (message, length, &cur_token, &token);
	if (r != SMTP_NO_ERROR) {
		res = r;
		goto free_token;
	}

	DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__, __FUNCTION__,
		    __LINE__);
	x_token = (char *) malloc (2 + strlen (token) + 1);
	if (x_token == NULL) {
		res = SMTP_ERROR_MEMORY;
		goto free_token;
	}
	DEBUG_SMTP (SMTP_MEM_1,
		    "smtp_mime_x_token_parse: MALLOC pointer=%p\n", x_token);

	if (min_x) {
		/* x_token = g_strconcat("x-", token, NULL); */
		sprintf (x_token, "x-%s", token);
	}
	else {
		/* x_token = g_strconcat("X-", token, NULL); */
		sprintf (x_token, "X-%s", token);
	}

	DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__, __FUNCTION__,
		    __LINE__);
	r = smtp_colon_parse (message, length, &cur_token);
	if (r != SMTP_NO_ERROR) {
		res = r;
		goto free_x_token;
	}

	DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__, __FUNCTION__,
		    __LINE__);
	r = smtp_unstructured_parse (message, length, &cur_token, &value);
	if (r != SMTP_NO_ERROR) {
		res = r;
		goto free_x_token;
	}

	DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__, __FUNCTION__,
		    __LINE__);
	r = smtp_unstrict_crlf_parse (message, length, &cur_token);
	if (r != SMTP_NO_ERROR) {
		res = r;
		goto free_x_token;
	}

	DEBUG_SMTP (SMTP_DBG, "%s_%s[%d]\n", __FILE__, __FUNCTION__,
		    __LINE__);
	smtp_mime_token_free (token);
	*result = x_token;
	*index = cur_token;

	return SMTP_NO_ERROR /*TRUE*/;


      free_x_token:
	free (x_token);
	DEBUG_SMTP (SMTP_MEM_1, "smtp_mime_x_token_parse: FREE pointer=%p\n",
		    x_token);

      free_token:
	smtp_mime_token_free (token);

      err:
	return res;

}
