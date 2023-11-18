#ifndef PATH_H
#define PATH_H

/*
  smtp_path is the parsed value of Return-Path
  - pt_addr_spec is a mailbox
*/
struct smtp_path
{
	char *pt_addr_spec;	/* can be NULL */
};

struct smtp_path *smtp_path_new (char *pt_addr_spec);
void smtp_path_free (struct smtp_path *path);

/*
path            =       ([CFWS] "<" ([CFWS] / addr-spec) ">" [CFWS]) /
                        obs-path
*/

int smtp_path_parse (const char *message, size_t length,
		     size_t * index, struct smtp_path **result);


#endif
