
#ifndef ENCODING_H
#define ENCODING_H

#include "fields.h"

enum
{
	SMTP_MIME_MECHANISM_ERROR,
	SMTP_MIME_MECHANISM_7BIT,
	SMTP_MIME_MECHANISM_8BIT,
	SMTP_MIME_MECHANISM_BINARY,
	SMTP_MIME_MECHANISM_QUOTED_PRINTABLE,
	SMTP_MIME_MECHANISM_BASE64,
	SMTP_MIME_MECHANISM_TOKEN
};

struct smtp_mime_mechanism
{
	int enc_type;
	char *enc_token;
};

struct smtp_mime_mechanism *smtp_mime_mechanism_new (int enc_type,
						     char *enc_token);

void smtp_mime_mechanism_free (struct smtp_mime_mechanism *mechanism);

void smtp_mime_encoding_free (struct smtp_mime_mechanism *encoding);

int smtp_mime_encoding_parse (struct smtp_part *part,
			      const char *message, size_t length,
			      size_t * index,
			      int *result 
			     );

int smtp_mime_transfer_encoding_get (struct smtp_mime_fields *fields);

#endif
