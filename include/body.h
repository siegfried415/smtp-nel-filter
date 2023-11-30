#ifndef BODY_H
#define BODY_H

#include "clist.h"
#include "mime.h"

struct smtp_mime_multipart_body
{
	clist *bd_list;
};

struct smtp_mime_multipart_body *smtp_mime_multipart_body_new (clist *
							       bd_list);
void smtp_mime_multipart_body_free (struct smtp_mime_multipart_body *mp_body);

/*
  smtp_body is the text part of a message
  - text is the beginning of the text part, it is a substring
    of an other string
  - size is the size of the text part
*/
struct smtp_body
{
	const char *bd_text;	/* != NULL */
	size_t bd_size;
};

struct smtp_body *smtp_body_new (const char *bd_text, size_t bd_size);
void smtp_body_free (struct smtp_body *body);

/*
  smtp_body_parse will parse the given text part of a message
  
  @param message this is a string containing the message text part
  @param length this is the size of the given string
  @param index this is a pointer to the start of the message text part in
    the given string, (* index) is modified to point at the end
    of the parsed data
  @param result the result of the parse operation is stored in
    (* result)

  @return SMTP_NO_ERROR on success, SMTP_ERROR_XXX on error
*/

int smtp_body_parse (const char *message, size_t length,
		     size_t * index, struct smtp_body **result);


int smtp_mime_base64_body_parse (const char *message, size_t length,
				 size_t * index, char **result,
				 size_t * result_len);

int smtp_mime_quoted_printable_body_parse (const char *message, size_t length,
					   size_t * index, char **result,
					   size_t * result_len,
					   int in_header);

int smtp_mime_binary_body_parse (const char *message, size_t length,
				 size_t * index, char **result,
				 size_t * result_len);

int smtp_mime_body_parse (char *message, unsigned int length, int encoding,
			  struct smtp_mime_text_stream *stream);



int
smtp_mime_body_init (
#ifdef USE_NEL
                            struct nel_eng *eng
#else
                            void
#endif
        );

#endif
