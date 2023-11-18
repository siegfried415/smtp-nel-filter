/*
 * $Id: charconv.h,v 1.1.1.1 2005/09/21 07:58:14 zhaozh Exp $
 */

#ifndef CHARCONV_H
#define CHARCONV_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <sys/types.h>

	enum
	{
		MAIL_CHARCONV_NO_ERROR = 0,
		MAIL_CHARCONV_ERROR_UNKNOWN_CHARSET,
		MAIL_CHARCONV_ERROR_MEMORY,
		MAIL_CHARCONV_ERROR_CONV,
	};

	int charconv (const char *tocode, const char *fromcode,
		      const char *str, size_t length, char **result);

	int charconv_buffer (const char *tocode, const char *fromcode,
			     const char *str, size_t length,
			     char **result, size_t * result_len);

	void charconv_buffer_free (char *str);

#ifdef __cplusplus
}
#endif

#endif
