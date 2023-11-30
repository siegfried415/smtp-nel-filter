#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#ifdef HAVE_ICONV
#include <iconv.h>
#endif

#include "mmapstring.h"
#include "charconv.h"

#ifdef HAVE_ICONV
static size_t
mail_iconv (iconv_t cd, const char **inbuf, size_t * inbytesleft,
	    char **outbuf, size_t * outbytesleft,
	    char **inrepls, char *outrepl)
{
	size_t ret = 0, ret1;
	/* XXX - force const to mutable */
	char *ib = (char *) *inbuf;
	size_t ibl = *inbytesleft;
	char *ob = *outbuf;
	size_t obl = *outbytesleft;

	for (;;) {
#ifdef HAVE_ICONV_PROTO_CONST
		ret1 = iconv (cd, (const char **) &ib, &ibl, &ob, &obl);
#else
		ret1 = iconv (cd, &ib, &ibl, &ob, &obl);
#endif
		if (ret1 != (size_t) - 1)
			ret += ret1;
		if (ibl && obl && errno == EILSEQ) {
			if (inrepls) {
				/* Try replacing the input */
				char **t;
				for (t = inrepls; *t; t++) {
					char *ib1 = *t;
					size_t ibl1 = strlen (*t);
					char *ob1 = ob;
					size_t obl1 = obl;
#ifdef HAVE_ICONV_PROTO_CONST
					iconv (cd, (const char **) &ib1,
					       &ibl1, &ob1, &obl1);
#else
					iconv (cd, &ib1, &ibl1, &ob1, &obl1);
#endif
					if (!ibl1) {
						++ib, --ibl;
						ob = ob1, obl = obl1;
						++ret;
						break;
					}
				}
				if (*t)
					continue;
			}
			if (outrepl) {
				/* Try replacing the output */
				size_t n = strlen (outrepl);
				if (n <= obl) {
					memcpy (ob, outrepl, n);
					++ib, --ibl;
					ob += n, obl -= n;
					++ret;
					continue;
				}
			}
		}
		*inbuf = ib, *inbytesleft = ibl;
		*outbuf = ob, *outbytesleft = obl;
		return ret;
	}
}
#endif


int
charconv (const char *tocode, const char *fromcode,
	  const char *str, size_t length, char **result)
{
#ifndef HAVE_ICONV
	return MAIL_CHARCONV_ERROR_UNKNOWN_CHARSET;
#else
	iconv_t conv;
	size_t r;
	char *out;
	char *pout;
	size_t out_size;
	size_t old_out_size;
	size_t count;
	int res;

	conv = iconv_open (tocode, fromcode);
	if (conv == (iconv_t) - 1) {
		res = MAIL_CHARCONV_ERROR_UNKNOWN_CHARSET;
		goto err;
	}

	out_size = 4 * length;

	out = malloc (out_size + 1);
	if (out == NULL) {
		res = MAIL_CHARCONV_ERROR_MEMORY;
		goto close_iconv;
	}

	pout = out;
	old_out_size = out_size;

	r = mail_iconv (conv, &str, &length, &pout, &out_size, NULL, "?");

	if (r == (size_t) - 1) {
		res = MAIL_CHARCONV_ERROR_CONV;
		goto free;
	}

	iconv_close (conv);

	*pout = '\0';
	count = old_out_size - out_size;
	pout = realloc (out, count + 1);
	if (pout != NULL)
		out = pout;

	*result = out;

	return MAIL_CHARCONV_NO_ERROR;

      free:
	free (out);
      close_iconv:
	iconv_close (conv);
      err:
	return res;
#endif
};


int
charconv_buffer (const char *tocode, const char *fromcode,
		 const char *str, size_t length,
		 char **result, size_t * result_len)
{
#ifndef HAVE_ICONV
	return MAIL_CHARCONV_ERROR_UNKNOWN_CHARSET;
#else
	iconv_t conv;
	size_t iconv_r;
	int r;
	char *out;
	char *pout;
	size_t out_size;
	size_t old_out_size;
	size_t count;
	MMAPString *mmapstr;
	int res;

	conv = iconv_open (tocode, fromcode);
	if (conv == (iconv_t) - 1) {
		res = MAIL_CHARCONV_ERROR_UNKNOWN_CHARSET;
		goto err;
	}

	out_size = 4 * length;

	mmapstr = mmap_string_sized_new (out_size + 1);
	if (mmapstr == NULL) {
		res = MAIL_CHARCONV_ERROR_MEMORY;
		goto err;
	}

	out = mmapstr->str;

	pout = out;
	old_out_size = out_size;

	iconv_r =
		mail_iconv (conv, &str, &length, &pout, &out_size, NULL, "?");

	if (iconv_r == (size_t) - 1) {
		res = MAIL_CHARCONV_ERROR_CONV;
		goto free;
	}

	iconv_close (conv);

	*pout = '\0';

	count = old_out_size - out_size;

	r = mmap_string_ref (mmapstr);
	if (r < 0) {
		res = MAIL_CHARCONV_ERROR_MEMORY;
		goto free;
	}

	*result = out;
	*result_len = count;

	return MAIL_CHARCONV_NO_ERROR;

      free:
	mmap_string_free (mmapstr);
      err:
	return res;
#endif
};


void
charconv_buffer_free (char *str)
{
	mmap_string_unref (str);
}
