#ifndef LANGUAGE_H
#define LANGUAGE_H

struct smtp_mime_language
{
	clist *lg_list;		/* atom (char *) */
};

struct smtp_mime_language *smtp_mime_language_new (clist * lg_list);

void smtp_mime_language_free (struct smtp_mime_language *lang);

int smtp_mime_language_parse (const char *message, size_t length,
			      size_t * index,
			      struct smtp_mime_language **result);
#endif
