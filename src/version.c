#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "mmapstring.h"
#include "smtp.h"
#include "mime.h"

/*
x  version := "MIME-Version" ":" 1*DIGIT "." 1*DIGIT
*/


int
smtp_mime_version_parse (const char *message, size_t length,
			 size_t * index, uint32_t * result)
{
	size_t cur_token;
	uint32_t hi;
	uint32_t low;
	uint32_t version;
	int r;

	cur_token = *index;

	r = smtp_wsp_parse (message, length, &cur_token);
	if ((r != SMTP_NO_ERROR) && (r != SMTP_ERROR_PARSE))
		return r;

	DEBUG_SMTP (SMTP_DBG, "message + cur_token: %s\n",
		    message + cur_token);
	r = smtp_number_parse (message, length, &cur_token, &hi);
	if (r != SMTP_NO_ERROR)
		return r;

	r = smtp_unstrict_char_parse (message, length, &cur_token, '.');
	if (r != SMTP_NO_ERROR)
		return r;

	r = smtp_cfws_parse (message, length, &cur_token);
	if ((r != SMTP_NO_ERROR) && (r != SMTP_ERROR_PARSE))
		return r;

	r = smtp_number_parse (message, length, &cur_token, &low);
	if (r != SMTP_NO_ERROR)
		return r;

	r = smtp_unstrict_crlf_parse (message, length, &cur_token);
	if (r != SMTP_NO_ERROR) {
		return r;
	}

	version = (hi << 16) + low;

	*result = version;
	*index = cur_token;

	return SMTP_NO_ERROR;
}
