


/*
 * $Id: return.c,v 1.5 2005/12/07 09:44:31 wyong Exp $
  RFC 2045, RFC 2046, RFC 2047, RFC 2048, RFC 2049, RFC 2231, RFC 2387
  RFC 2424, RFC 2557, RFC 2183 Content-Disposition, RFC 1766  Language
 */

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "mmapstring.h"

#include "smtp.h"
#include "return.h"
#include "path.h"

struct smtp_return *
smtp_return_new (struct smtp_path *ret_path)
{
	struct smtp_return *return_path;

	return_path = malloc (sizeof (*return_path));
	if (return_path == NULL)
		return NULL;
	DEBUG_SMTP (SMTP_MEM_1, "smtp_return_new: MALLOC pointer=%p\n",
		    return_path);

	return_path->ret_path = ret_path;

	return return_path;
}


void
smtp_return_free (struct smtp_return *return_path)
{
	smtp_path_free (return_path->ret_path);
	free (return_path);
	DEBUG_SMTP (SMTP_MEM_1, "smtp_return_free: FREE pointer=%p\n",
		    return_path);
}

/*
return          =       "Return-Path:" path CRLF
*/

int
smtp_return_parse (const char *message, size_t length,
		   size_t * index, struct smtp_return **result)
{
	struct smtp_path *path;
	struct smtp_return *return_path;
	size_t cur_token;
	int r;
	int res;

	cur_token = *index;

/* xiayu 2005.11.10 we've already checked the "Return-Path" string
  r = smtp_token_case_insensitive_parse(message, length,
					   &cur_token, "Return-Path");
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

	path = NULL;
	r = smtp_path_parse (message, length, &cur_token, &path);
	if (r != SMTP_NO_ERROR) {
		res = r;
		goto err;
	}

	r = smtp_unstrict_crlf_parse (message, length, &cur_token);
	if (r != SMTP_NO_ERROR) {
		res = r;
		goto free_path;
	}

	return_path = smtp_return_new (path);
	if (return_path == NULL) {
		res = SMTP_ERROR_MEMORY;
		goto free_path;
	}

	*result = return_path;
	*index = cur_token;

	return SMTP_NO_ERROR;

      free_path:
	smtp_path_free (path);
      err:
	return res;
}
