

/*
 * $Id: keywords.c,v 1.4 2005/11/29 06:29:26 xiay Exp $
  RFC 2045, RFC 2046, RFC 2047, RFC 2048, RFC 2049, RFC 2231, RFC 2387
  RFC 2424, RFC 2557, RFC 2183 Content-Disposition, RFC 1766  Language
 */

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "mmapstring.h"

#include "smtp.h"
#include "mime.h"
#include "keywords.h"


struct smtp_keywords *
smtp_keywords_new (clist * kw_list)
{
	struct smtp_keywords *keywords;

	keywords = malloc (sizeof (*keywords));
	if (keywords == NULL)
		return NULL;
	DEBUG_SMTP (SMTP_MEM_1, "smtp_keywords_new: MALLOC pointer=%p\n",
		    keywords);

	keywords->kw_list = kw_list;

	return keywords;
}


void
smtp_keywords_free (struct smtp_keywords *keywords)
{
	clist_foreach (keywords->kw_list, (clist_func) smtp_phrase_free,
		       NULL);
	clist_free (keywords->kw_list);
	free (keywords);
	DEBUG_SMTP (SMTP_MEM_1, "smtp_keywords_free: FREE pointer=%p\n",
		    keywords);
}

/*
keywords        =       "Keywords:" phrase *("," phrase) CRLF
*/

int
smtp_keywords_parse (const char *message, size_t length,
		     size_t * index, struct smtp_keywords **result)
{
	struct smtp_keywords *keywords;
	clist *list;
	size_t cur_token;
	int r;
	int res;

	cur_token = *index;

#if 0				//xiayu 2005.11.18 commented
	r = smtp_token_case_insensitive_parse (message, length,
					       &cur_token, "Keywords");
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

	r = smtp_struct_list_parse (message, length, &cur_token,
				    &list, ',', (smtp_struct_parser *)
				    smtp_phrase_parse,
				    (smtp_struct_destructor *)
				    smtp_phrase_free);
	if (r != SMTP_NO_ERROR) {
		res = r;
		goto err;
	}

	r = smtp_unstrict_crlf_parse (message, length, &cur_token);
	if (r != SMTP_NO_ERROR) {
		res = r;
		goto free_list;
	}

	keywords = smtp_keywords_new (list);
	if (keywords == NULL) {
		res = SMTP_ERROR_MEMORY;
		goto free_list;
	}

	*result = keywords;
	*index = cur_token;

	return SMTP_NO_ERROR;

      free_list:
	clist_foreach (list, (clist_func) smtp_phrase_free, NULL);
	clist_free (list);
      err:
	return res;
}
