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
	DEBUG_SMTP (SMTP_MEM, "smtp_keywords_new: MALLOC pointer=%p\n",
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
	DEBUG_SMTP (SMTP_MEM, "smtp_keywords_free: FREE pointer=%p\n",
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
