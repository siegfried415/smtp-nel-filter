#ifndef KEYWORDS_H
#define KEYWORDS_H

/*
  smtp_keywords is the parsed Keywords field
  - kw_list is the list of keywords
*/
struct smtp_keywords
{
	clist *kw_list;		/* list of (char *), != NULL */
};

struct smtp_keywords *smtp_keywords_new (clist * kw_list);
void smtp_keywords_free (struct smtp_keywords *keywords);

/*
keywords        =       "Keywords:" phrase *("," phrase) CRLF
*/

int smtp_keywords_parse (const char *message, size_t length,
			 size_t * index, struct smtp_keywords **result);


#endif
