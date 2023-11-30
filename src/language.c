#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "mmapstring.h"
#include "smtp.h"
#include "mime.h"
#include "language.h"


#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

struct smtp_mime_language *
smtp_mime_language_new (clist * lg_list)
{
	struct smtp_mime_language *lang;

	lang = malloc (sizeof (*lang));
	if (lang == NULL)
		return NULL;
	DEBUG_SMTP (SMTP_MEM, "smtp_mime_language_new: MALLOC pointer=%p\n",
		    lang);

	lang->lg_list = lg_list;

	return lang;
}

void
smtp_mime_language_free (struct smtp_mime_language *lang)
{
	clist_foreach (lang->lg_list, (clist_func) smtp_atom_free, NULL);
	clist_free (lang->lg_list);
	free (lang);
	DEBUG_SMTP (SMTP_MEM, "smtp_mime_language_free: FREE pointer=%p\n",
		    lang);
}

int
smtp_mime_language_parse (const char *message, size_t length,
			  size_t * index, struct smtp_mime_language **result)
{
	size_t cur_token;
	int r;
	int res;
	clist *list;
	int first;
	struct smtp_mime_language *language;

	cur_token = *index;

	list = clist_new ();
	if (list == NULL) {
		res = SMTP_ERROR_MEMORY;
		goto err;
	}

	first = TRUE;

	while (1) {
		char *atom;

		r = smtp_unstrict_char_parse (message, length, &cur_token,
					      ',');
		if (r == SMTP_NO_ERROR) {
			/* do nothing */
		}
		else if (r == SMTP_ERROR_PARSE) {
			break;
		}
		else {
			res = r;
			goto err;
		}

		r = smtp_atom_parse (message, length, &cur_token, &atom);
		if (r == SMTP_NO_ERROR) {
			/* do nothing */
		}
		else if (r == SMTP_ERROR_PARSE) {
			break;
		}
		else {
			res = r;
			goto err;
		}

		r = clist_append (list, atom);
		if (r < 0) {
			smtp_atom_free (atom);
			res = SMTP_ERROR_MEMORY;
			goto free;
		}
	}

	r = smtp_unstrict_crlf_parse (message, length, &cur_token);
	if (r != SMTP_NO_ERROR) {
		return r;
	}

	language = smtp_mime_language_new (list);
	if (language == NULL) {
		res = SMTP_ERROR_MEMORY;
		goto free;
	}

	*result = language;
	*index = cur_token;

	return SMTP_NO_ERROR;

      free:
	clist_foreach (list, (clist_func) smtp_atom_free, NULL);
	clist_free (list);
      err:
	return res;
}
