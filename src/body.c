#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "mmapstring.h"
#include "smtp.h"
#include "body.h"
#include "encoding.h"
#include "mime.h"

#include "engine.h"
#include "nlib/stream.h"


struct smtp_mime_multipart_body *
smtp_mime_multipart_body_new (clist * bd_list)
{
	struct smtp_mime_multipart_body *mp_body;

	mp_body = malloc (sizeof (*mp_body));
	if (mp_body == NULL)
		return NULL;
	DEBUG_SMTP (SMTP_MEM,
		    "smtp_mime_multipart_body_new: MALLOC pointer=%p\n",
		    mp_body);

	mp_body->bd_list = bd_list;

	return mp_body;
}

void
smtp_mime_multipart_body_free (struct smtp_mime_multipart_body *mp_body)
{
	clist_foreach (mp_body->bd_list, (clist_func) smtp_body_free, NULL);
	clist_free (mp_body->bd_list);
	free (mp_body);
	DEBUG_SMTP (SMTP_MEM,
		    "smtp_mime_multipart_body_free: FREE pointer=%p\n",
		    mp_body);
}

struct smtp_body *
smtp_body_new (const char *bd_text, size_t bd_size)
{
	struct smtp_body *body;

	body = malloc (sizeof (*body));
	if (body == NULL)
		return NULL;
	DEBUG_SMTP (SMTP_MEM, "smtp_body_new: pointer=%p\n", body);

	body->bd_text = bd_text;
	body->bd_size = bd_size;

	return body;
}


void
smtp_body_free (struct smtp_body *body)
{
	free (body);
	DEBUG_SMTP (SMTP_MEM, "smtp_body_free: pointer=%p\n", body);
}

/*
body            =       *(*998text CRLF) *998text
*/

int
smtp_body_parse (const char *message, size_t length,
		 size_t * index, struct smtp_body **result)
{
	size_t cur_token;
	struct smtp_body *body;

	cur_token = *index;

	body = smtp_body_new (message + cur_token, length - cur_token);
	if (body == NULL)
		return SMTP_ERROR_MEMORY;

	cur_token = length;

	*result = body;
	*index = cur_token;

	return SMTP_NO_ERROR;
}

/* ************************************************************************* */
/* MIME part decoding */

static signed char
get_base64_value (char ch)
{
	if ((ch >= 'A') && (ch <= 'Z'))
		return ch - 'A';
	if ((ch >= 'a') && (ch <= 'z'))
		return ch - 'a' + 26;
	if ((ch >= '0') && (ch <= '9'))
		return ch - '0' + 52;
	switch (ch) {
	case '+':
		return 62;
	case '/':
		return 63;
	case '=':		/* base64 padding */
		return -1;
	default:
		return -1;
	}
}

//char buf[256 * 1024];
int
smtp_mime_base64_body_parse (const char *message, size_t length,
			     size_t * index, char **result,
			     size_t * result_len)
{
	size_t cur_token;
	size_t i;
	char chunk[4];
	int chunk_index;
	char out[3];
	MMAPString *mmapstr;
	int res;
	int r;
	size_t written;

	char *buf = NULL;

	DEBUG_SMTP (SMTP_DBG, "message = %s, index = %lu,  length= %lu \n",
		    message, *index, length);
	cur_token = *index;
	chunk_index = 0;
	written = 0;

	DEBUG_SMTP (SMTP_DBG, "\n");
	buf = malloc ((length - cur_token) * 3 / 4 + 3);
	if (buf == NULL) {
		res = SMTP_ERROR_MEMORY;
		goto err;

	}

	DEBUG_SMTP (SMTP_DBG, "\n");
	i = 0;

	while (1) {
		signed char value;

		value = -1;
		while (value == -1) {

			if (cur_token >= length)
				break;

			value = get_base64_value (message[cur_token]);
			cur_token++;
		}

		if (value == -1)
			break;

		chunk[chunk_index] = value;
		chunk_index++;

		if (chunk_index == 4) {
			out[0] = (chunk[0] << 2) | (chunk[1] >> 4);
			out[1] = (chunk[1] << 4) | (chunk[2] >> 2);
			out[2] = (chunk[2] << 6) | (chunk[3]);

			chunk[0] = 0;
			chunk[1] = 0;
			chunk[2] = 0;
			chunk[3] = 0;

			chunk_index = 0;
			buf[written++] = out[0];
			buf[written++] = out[1];
			buf[written++] = out[2];

		}
	}

	DEBUG_SMTP (SMTP_DBG, "\n");
	if (chunk_index != 0) {
		size_t len;

		len = 0;
		out[0] = (chunk[0] << 2) | (chunk[1] >> 4);
		len++;

		if (chunk_index >= 3) {
			out[1] = (chunk[1] << 4) | (chunk[2] >> 2);
			len++;
		}

		for (i = 0; i < len; i++) {
			buf[written++] = out[i];
		}

	}

	DEBUG_SMTP (SMTP_DBG, "\n");
	*index = cur_token;
	*result = buf;
	*result_len = written;


	DEBUG_SMTP (SMTP_MEM,
		    "smtp_mime_base64_body_parse: FREE pointer=%p\n",
		    mmapstr);
	return SMTP_NO_ERROR;

      free:
	free (buf);

      err:
	return res;
}



static int
hexa_to_char (char hexdigit)
{
	if ((hexdigit >= '0') && (hexdigit <= '9'))
		return hexdigit - '0';
	if ((hexdigit >= 'a') && (hexdigit <= 'f'))
		return hexdigit - 'a' + 10;
	if ((hexdigit >= 'A') && (hexdigit <= 'F'))
		return hexdigit - 'A' + 10;
	return 0;
}

static char
to_char (const char *hexa)
{
	return (hexa_to_char (hexa[0]) << 4) | hexa_to_char (hexa[1]);
}

enum
{
	STATE_NORMAL,
	STATE_CODED,
	STATE_OUT,
	STATE_CR,
};


static int
write_decoded_qp (MMAPString * mmapstr, const char *start, size_t count)
{
	if (mmap_string_append_len (mmapstr, start, count) == NULL)
		return SMTP_ERROR_MEMORY;

	return SMTP_NO_ERROR;
}


#define WRITE_MAX_QP 512

int
smtp_mime_quoted_printable_body_parse (const char *message, size_t length,
				       size_t * index, char **result,
				       size_t * result_len, int in_header)
{
	size_t cur_token;
	int state;
	int r;
	char ch;
	size_t count;
	const char *start;
	MMAPString *mmapstr;
	int res;
	size_t written;

	state = STATE_NORMAL;
	cur_token = *index;

	count = 0;
	start = message + cur_token;
	written = 0;

	mmapstr = mmap_string_sized_new (length - cur_token);
	if (mmapstr == NULL) {
		res = SMTP_ERROR_MEMORY;
		goto err;
	}

	while (state != STATE_OUT) {

		if (cur_token >= length) {
			state = STATE_OUT;
			break;
		}

		switch (state) {

		case STATE_CODED:

			if (count > 0) {
				r = write_decoded_qp (mmapstr, start, count);
				if (r != SMTP_NO_ERROR) {
					res = r;
					goto free;
				}
				written += count;
				count = 0;
			}

			switch (message[cur_token]) {
			case '=':
				if (cur_token + 1 >= length) {
					/* error but ignore it */
					state = STATE_NORMAL;
					start = message + cur_token;
					cur_token++;
					count++;
					break;
				}

				switch (message[cur_token + 1]) {

				case '\n':
					cur_token += 2;

					start = message + cur_token;

					state = STATE_NORMAL;
					break;

				case '\r':
					if (cur_token + 2 >= length) {
						state = STATE_OUT;
						break;
					}

					if (message[cur_token + 2] == '\n')
						cur_token += 3;
					else
						cur_token += 2;

					start = message + cur_token;

					state = STATE_NORMAL;

					break;

				default:
					if (cur_token + 2 >= length) {
						/* error but ignore it */
						cur_token++;

						start = message + cur_token;

						count++;
						state = STATE_NORMAL;
						break;
					}

					ch = to_char (message + cur_token +
						      1);

					if (mmap_string_append_c (mmapstr, ch)
					    == NULL) {
						res = SMTP_ERROR_MEMORY;
						goto free;
					}

					cur_token += 3;
					written++;

					start = message + cur_token;

					state = STATE_NORMAL;
					break;
				}
				break;
			}
			break;	/* end of STATE_ENCODED */

		case STATE_NORMAL:

			switch (message[cur_token]) {

			case '=':
				state = STATE_CODED;
				break;

			case '\n':
				/* flush before writing additionnal information */
				if (count > 0) {
					r = write_decoded_qp (mmapstr, start,
							      count);
					if (r != SMTP_NO_ERROR) {
						res = r;
						goto free;
					}
					written += count;

					count = 0;
				}

				r = write_decoded_qp (mmapstr, "\r\n", 2);
				if (r != SMTP_NO_ERROR) {
					res = r;
					goto free;
				}
				written += 2;
				cur_token++;
				start = message + cur_token;
				break;

			case '\r':
				state = STATE_CR;
				cur_token++;
				break;

			case '_':
				if (in_header) {
					if (count > 0) {
						r = write_decoded_qp (mmapstr,
								      start,
								      count);
						if (r != SMTP_NO_ERROR) {
							res = r;
							goto free;
						}
						written += count;
						count = 0;
					}

					if (mmap_string_append_c
					    (mmapstr, ' ') == NULL) {
						res = SMTP_ERROR_MEMORY;
						goto free;
					}

					written++;
					cur_token++;
					start = message + cur_token;

					break;
				}
				/* WARINING : must be followed by switch default action */

			default:
				if (count >= WRITE_MAX_QP) {
					r = write_decoded_qp (mmapstr, start,
							      count);
					if (r != SMTP_NO_ERROR) {
						res = r;
						goto free;
					}
					written += count;
					count = 0;
					start = message + cur_token;
				}

				count++;
				cur_token++;
				break;
			}
			break;	/* end of STATE_NORMAL */

		case STATE_CR:
			switch (message[cur_token]) {

			case '\n':
				/* flush before writing additionnal information */
				if (count > 0) {
					r = write_decoded_qp (mmapstr, start,
							      count);
					if (r != SMTP_NO_ERROR) {
						res = r;
						goto free;
					}
					written += count;
					count = 0;
				}

				r = write_decoded_qp (mmapstr, "\r\n", 2);
				if (r != SMTP_NO_ERROR) {
					res = r;
					goto free;
				}
				written += 2;
				cur_token++;
				start = message + cur_token;
				state = STATE_NORMAL;
				break;

			default:
				/* flush before writing additionnal information */
				if (count > 0) {
					r = write_decoded_qp (mmapstr, start,
							      count);
					if (r != SMTP_NO_ERROR) {
						res = r;
						goto free;
					}
					written += count;
					count = 0;
				}

				start = message + cur_token;

				r = write_decoded_qp (mmapstr, "\r\n", 2);
				if (r != SMTP_NO_ERROR) {
					res = r;
					goto free;
				}
				written += 2;
				state = STATE_NORMAL;
			}
			break;	/* end of STATE_CR */
		}
	}

	if (count > 0) {
		r = write_decoded_qp (mmapstr, start, count);
		if (r != SMTP_NO_ERROR) {
			res = r;
			goto free;
		}
		written += count;
		count = 0;
	}

	*index = cur_token;
	*result = mmapstr->str;
	*result_len = written;

	free (mmapstr);
	DEBUG_SMTP (SMTP_MEM,
		    "smtp_mime_quoted_printable_body_parse: FREE pointer=%p\n",
		    mmapstr);

	return SMTP_NO_ERROR;

      free:
	mmap_string_free (mmapstr);
      err:
	return res;
}

int
smtp_mime_binary_body_parse (const char *message, size_t length,
			     size_t * index, char **result,
			     size_t * result_len)
{
	MMAPString *mmapstr;
	size_t cur_token;
	int r;
	int res;

	cur_token = *index;

	if (length >= 1) {
		if (message[length - 1] == '\n') {
			length--;
			if (length >= 1)
				if (message[length - 1] == '\r')
					length--;
		}
	}

	DEBUG_SMTP (SMTP_MEM, "before mmap_string_new_len: len = %lu\n",
		    length - cur_token);
	mmapstr =
		mmap_string_new_len (message + cur_token, length - cur_token);
	if (mmapstr == NULL) {
		res = SMTP_ERROR_MEMORY;
		goto err;
	}


	*index = length;
	*result = mmapstr->str;
	*result_len = length - cur_token;

	free (mmapstr);
	DEBUG_SMTP (SMTP_MEM,
		    "smtp_mime_binary_body_parse: FREE pointer=%p\n",
		    mmapstr);

	return SMTP_NO_ERROR;

      free:
	mmap_string_free (mmapstr);
      err:
	return res;
}

int
smtp_mime_body_parse (char *message, unsigned int length, int encoding,
		      struct smtp_mime_text_stream *stream)
{
	size_t index = 0;
	char *result;
	size_t result_len = 0;

	DEBUG_SMTP (SMTP_DBG, "smtp_mime_body_parse\n");
	if (length == 0 || message == NULL) {
		//nel_stream_put(stream->dt_stream, NULL, 0);
		return SMTP_NO_ERROR;
	}

	switch (encoding) {
	case SMTP_MIME_MECHANISM_BASE64:
		smtp_mime_base64_body_parse (message, length, &index, &result,
					     &result_len);
		nel_stream_put (stream->ts_stream, result, result_len);
		break;


	case SMTP_MIME_MECHANISM_QUOTED_PRINTABLE:
		smtp_mime_quoted_printable_body_parse (message, length,
						       &index, &result,
						       &result_len, FALSE);
		nel_stream_put (stream->ts_stream, result, result_len);
		break;

	case SMTP_MIME_MECHANISM_7BIT:
	case SMTP_MIME_MECHANISM_8BIT:
	case SMTP_MIME_MECHANISM_BINARY:
	default:
		smtp_mime_binary_body_parse (message, length, &index, &result,
					     &result_len);
		nel_stream_put (stream->ts_stream, result, result_len);
		break;
	}

	return SMTP_NO_ERROR;

}

int
smtp_mime_body_init (
#ifdef USE_NEL
			    struct nel_eng *eng
#else
			    void
#endif
	)
{
	//dummy function just to let body.c linked to main program. 
}
