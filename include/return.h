#ifndef RETURN_H
#define RETURN_H

/*
  smtp_return is the parsed Return-Path field
  - ret_path is the parsed value of Return-Path
*/
struct smtp_return
{
	struct smtp_path *ret_path;	/* != NULL */
};

struct smtp_return *smtp_return_new (struct smtp_path *ret_path);
void smtp_return_free (struct smtp_return *return_path);

/*
return          =       "Return-Path:" path CRLF
*/

int smtp_return_parse (const char *message, size_t length,
		       size_t * index, struct smtp_return **result);


#endif
