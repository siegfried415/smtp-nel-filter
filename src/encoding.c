
/*
 * $Id: encoding.c,v 1.5 2005/11/24 02:49:22 xiay Exp $
  RFC 2045, RFC 2046, RFC 2047, RFC 2048, RFC 2049, RFC 2231, RFC 2387
  RFC 2424, RFC 2557, RFC 2183 Content-Disposition, RFC 1766  Language
 */

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "mmapstring.h"

#include "smtp.h"
#include "mime.h"
#include "encoding.h"
#include "x-token.h"
#include "fields.h"
#include "message.h"


struct smtp_mime_mechanism *
smtp_mime_mechanism_new (int enc_type, char *enc_token)
{
	struct smtp_mime_mechanism *mechanism;

	mechanism = malloc (sizeof (*mechanism));
	if (mechanism == NULL)
		return NULL;

	mechanism->enc_type = enc_type;
	mechanism->enc_token = enc_token;

	return mechanism;
}

void
smtp_mime_mechanism_free (struct smtp_mime_mechanism *mechanism)
{
	if (mechanism->enc_token != NULL)
		smtp_mime_token_free (mechanism->enc_token);
	free (mechanism);
}

int
smtp_mime_transfer_encoding_get (struct smtp_mime_fields *fields)
{
	clistiter *cur;

	for (cur = clist_begin (fields->fld_list);
	     cur != NULL; cur = clist_next (cur)) {
		struct smtp_mime_field *field;

		field = clist_content (cur);
		if (field->fld_type == SMTP_MIME_FIELD_TRANSFER_ENCODING)
			return field->fld_data.fld_encoding->enc_type;
	}

	return SMTP_MIME_MECHANISM_8BIT;
}


/*
x  mechanism := "7bit" / "8bit" / "binary" /
               "quoted-printable" / "base64" /
               ietf-token / x-token
*/

int
smtp_mime_mechanism_parse (struct smtp_part *part, const char *message,
			   size_t length, size_t * index, int *result)
{
	char *token;
	int type;
	struct smtp_mime_mechanism *mechanism;
	size_t cur_token;
	int r;
	int res;

	cur_token = *index;

	type = SMTP_MIME_MECHANISM_ERROR;	/* XXX - removes a gcc warning */

	token = NULL;

	r = smtp_wsp_parse (message, length, &cur_token);
	if ((r != SMTP_NO_ERROR) && (r != SMTP_ERROR_PARSE))
		return r;

	r = smtp_token_case_insensitive_parse (message, length,
					       &cur_token, "7bit");
	if (r == SMTP_NO_ERROR) {
		DEBUG_SMTP (SMTP_DBG, "\n");
		type = SMTP_MIME_MECHANISM_7BIT;
	}

	DEBUG_SMTP (SMTP_DBG, "\n");
	if (r == SMTP_ERROR_PARSE) {
		DEBUG_SMTP (SMTP_DBG, "\n");
		r = smtp_token_case_insensitive_parse (message, length,
						       &cur_token, "8bit");
		if (r == SMTP_NO_ERROR) {
			DEBUG_SMTP (SMTP_DBG, "\n");
			type = SMTP_MIME_MECHANISM_8BIT;
		}
	}

	DEBUG_SMTP (SMTP_DBG, "\n");
	if (r == SMTP_ERROR_PARSE) {
		DEBUG_SMTP (SMTP_DBG, "\n");
		r = smtp_token_case_insensitive_parse (message, length,
						       &cur_token, "binary");
		if (r == SMTP_NO_ERROR) {
			DEBUG_SMTP (SMTP_DBG, "\n");
			type = SMTP_MIME_MECHANISM_BINARY;
		}
	}

	DEBUG_SMTP (SMTP_DBG, "\n");
	if (r == SMTP_ERROR_PARSE) {
		DEBUG_SMTP (SMTP_DBG, "\n");
		r = smtp_token_case_insensitive_parse (message, length,
						       &cur_token,
						       "quoted-printable");
		if (r == SMTP_NO_ERROR) {
			DEBUG_SMTP (SMTP_DBG, "\n");
			type = SMTP_MIME_MECHANISM_QUOTED_PRINTABLE;
		}
	}

	DEBUG_SMTP (SMTP_DBG, "\n");
	if (r == SMTP_ERROR_PARSE) {
		DEBUG_SMTP (SMTP_DBG, "\n");
		r = smtp_token_case_insensitive_parse (message, length,
						       &cur_token, "base64");
		if (r == SMTP_NO_ERROR) {
			DEBUG_SMTP (SMTP_DBG, "\n");
			type = SMTP_MIME_MECHANISM_BASE64;
		}
	}

	DEBUG_SMTP (SMTP_DBG, "\n");
	if (r == SMTP_ERROR_PARSE) {
		DEBUG_SMTP (SMTP_DBG, "\n");
		r = smtp_mime_token_parse (message, length, &cur_token,
					   &token);
		if (r == SMTP_NO_ERROR) {
			DEBUG_SMTP (SMTP_DBG, "\n");
			type = SMTP_MIME_MECHANISM_TOKEN;
		}
	}

	DEBUG_SMTP (SMTP_DBG, "\n");
	if (r != SMTP_NO_ERROR) {
		res = r;
		goto err;
	}

	//xiayu 2005.10.28
	//mechanism = smtp_mime_mechanism_new(type, token);
	//if (mechanism == NULL) {
	//  res = SMTP_ERROR_MEMORY;
	//  goto free;
	//}

	//xiayu 2005.10.28
	r = smtp_unstrict_crlf_parse (message, length, &cur_token);
	if (r != SMTP_NO_ERROR) {
		res = r;
		goto err;
	}

	part->encode_type = type;

	//xiayu 2005.10.28
	// * result = mechanism;
	*result = type;
	*index = cur_token;

	return SMTP_NO_ERROR;

//xiayu 2005.10.28
// free:
//  if (token != NULL)
//    smtp_mime_token_free(token);
      err:
	return res;
}

/*
x  encoding := "Content-Transfer-Encoding" ":" mechanism
*/


int
smtp_mime_encoding_parse (struct smtp_part *part, const char *message,
			  size_t length, size_t * index,
			  struct smtp_mime_mechanism **result)
{
	return smtp_mime_mechanism_parse (part, message, length, index,
					  result);
}
