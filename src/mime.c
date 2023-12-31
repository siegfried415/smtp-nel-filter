#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "mem_pool.h"
#include "mmapstring.h"
#include "smtp.h"
#include "mime.h"
#include "fields.h"
#include "content-type.h"
#include "encoding.h"
#include "x-token.h"
#include "message.h"

#include "nlib/stream.h"

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

extern ObjPool_t smtp_string_pool;
int is_dtext (char ch);

int
is_wsp (char ch)
{
	if ((ch == ' ') || (ch == '\t'))
		return TRUE;

	return FALSE;
}

int
smtp_mime_lwsp_parse (const char *message, size_t length, size_t * index)
{
	size_t cur_token;

	cur_token = *index;
	if (cur_token >= length)
		return SMTP_ERROR_CONTINUE;
	if (cur_token >= length)
		return SMTP_ERROR_PARSE;

	while (is_wsp (message[cur_token])) {
		cur_token++;
		if (cur_token >= length)
			break;
	}

	if (cur_token == *index)
		return SMTP_ERROR_PARSE;

	*index = cur_token;

	return SMTP_NO_ERROR;
}

/*
atext           =       ALPHA / DIGIT / ; Any character except controls,
                        "!" / "#" /     ;  SP, and specials.
                        "$" / "%" /     ;  Used for atoms
                        "&" / "'" /
                        "*" / "+" /
                        "-" / "/" /
                        "=" / "?" /
                        "^" / "_" /
                        "`" / "{" /
                        "|" / "}" /
                        "~"
*/

int
is_atext (char ch)
{
	switch (ch) {
	case ' ':
	case '\t':
	case '\n':
	case '\r':
	case '<':
	case '>':
	case ',':
	case '"':
	case ':':
	case ';':
		return FALSE;
	default:
		return TRUE;
	}
}


int
smtp_str_crlf_parse (char *message, int length, size_t *index)
{
	size_t cur_token = *index;
	int i;

	for (i = cur_token; i < length; i++) {
		if (message[i] == '\r') {
			if (i + 1 < length) {
				if (message[i + 1] == '\n') {
					DEBUG_SMTP (SMTP_DBG, "\n");
					*index = i + 2;
					return SMTP_NO_ERROR;
				}
				/* else continue */

			}
			else {
				DEBUG_SMTP (SMTP_DBG,
					    "no CRLF for this SMTP line, continue...\n");
				DEBUG_SMTP (SMTP_DBG,
					    "message+(*index): %s\n",
					    message + (*index));
				return SMTP_ERROR_CONTINUE;
			}

		}
		/* else continue */
	}

	DEBUG_SMTP (SMTP_DBG, "no CRLF for this SMTP line, continue...\n");
	DEBUG_SMTP (SMTP_DBG, "message+(*index): %s\n", message + (*index));
	return SMTP_ERROR_CONTINUE;
}

void
smtp_string_free (char *str)
{
	if (str) {
		free_mem (&smtp_string_pool, str);
		DEBUG_SMTP (SMTP_MEM,
			    "smtp_string_free: pointer=%p, elm=%p\n",
			    &smtp_string_pool, str);
	}
}

char *
smtp_string_new (char *str, int len)
{
	char *string;
	if (str == NULL)
		return NULL;
	if (len > SMTP_STRING_BUF_LEN - 1)
		return NULL;
	string = (char *) alloc_mem (&smtp_string_pool);
	if (!string)
		return NULL;
	DEBUG_SMTP (SMTP_MEM, "smtp_string_new: pointer=%p, elm=%p\n",
		    &smtp_string_pool, str);
	memcpy (string, str, len);
	string[len] = '\0';
	return string;
}


/* WSPS = *WSP */
int
smtp_wsps_parse (const char *message, size_t length, size_t * index)
{
	size_t cur_token;
	int r;

	cur_token = *index;

	while (1) {
		r = smtp_wsp_parse (message, length, &cur_token);
		if (r != SMTP_NO_ERROR) {
			if (r == SMTP_ERROR_PARSE)
				break;
			else
				return r;
		}
	}

	*index = cur_token;

	return SMTP_NO_ERROR;
}

int
smtp_wsp_unstrict_char_parse (const char *message, size_t length,
			      size_t * index, char token)
{
	size_t cur_token;
	int r;

	cur_token = *index;

	r = smtp_wsps_parse (message, length, &cur_token);
	if ((r != SMTP_NO_ERROR) && (r != SMTP_ERROR_PARSE))
		return r;

	r = smtp_char_parse (message, length, &cur_token, token);
	if (r != SMTP_NO_ERROR)
		return r;

	*index = cur_token;

	return SMTP_NO_ERROR;
}

int
smtp_wsp_lower_parse (const char *message, size_t length, size_t * index)
{
	return smtp_wsp_unstrict_char_parse (message, length, index, '<');
}

int
smtp_wsp_greater_parse (const char *message, size_t length, size_t * index)
{
	return smtp_wsp_unstrict_char_parse (message, length, index, '>');
}

int
smtp_wsp_at_sign_parse (const char *message, size_t length, size_t * index)
{
	return smtp_wsp_unstrict_char_parse (message, length, index, '@');
}

int
smtp_wsp_atom_parse (const char *message, size_t length,
		     size_t * index, char **result)
{
	size_t cur_token;
	int r;
	int res;
	char *atom;
	size_t end;

	cur_token = *index;

	r = smtp_wsps_parse (message, length, &cur_token);
	if ((r != SMTP_NO_ERROR) && (r != SMTP_ERROR_PARSE)) {
		res = r;
		goto err;
	}

	end = cur_token;
	if (end >= length) {
		res = SMTP_ERROR_PARSE;
		goto err;
	}

	DEBUG_SMTP (SMTP_DBG, "length = %lu\n", length);
	DEBUG_SMTP (SMTP_DBG, "message[end] = %c\n", message[end]);
	while (is_atext (message[end])) {
		end++;
		if (end >= length)
			break;
	}
	if (end == cur_token) {
		res = SMTP_ERROR_PARSE;
		goto err;
	}

	atom = malloc (end - cur_token + 1);
	if (atom == NULL) {
		res = SMTP_ERROR_MEMORY;
		goto err;
	}
	DEBUG_SMTP (SMTP_MEM, "smtp_wsp_atom_parse: MALLOC pointer=%p\n",
		    atom);

	strncpy (atom, message + cur_token, end - cur_token);
	atom[end - cur_token] = '\0';

	cur_token = end;

	*index = cur_token;
	*result = atom;

	return SMTP_NO_ERROR;

      err:
	return res;
}


int
smtp_wsp_word_parse (const char *message, size_t length,
		     size_t * index, char **result)
{
	size_t cur_token;
	char *word;
	int r;

	cur_token = *index;

	r = smtp_wsp_atom_parse (message, length, &cur_token, &word);
	if (r != SMTP_NO_ERROR) {
		return r;
	}

	*result = word;
	*index = cur_token;

	return SMTP_NO_ERROR;
}


int
smtp_wsp_phrase_parse (const char *message, size_t length,
		       size_t * index, char **result)
{
	MMAPString *gphrase;
	char *word;
	int first;
	size_t cur_token;
	int r;
	int res;
	char *str;

	cur_token = *index;

	gphrase = mmap_string_new ("");
	if (gphrase == NULL) {
		res = SMTP_ERROR_MEMORY;
		goto err;
	}

	first = TRUE;

	while (1) {
		r = smtp_wsp_word_parse (message, length, &cur_token, &word);
		if (r == SMTP_NO_ERROR) {
			if (!first) {
				if (mmap_string_append_c (gphrase, ' ') == NULL) {
					DEBUG_SMTP (SMTP_MEM,
						    "BEFORE 418 smtp_word_free\n");
					smtp_word_free (word);
					res = SMTP_ERROR_MEMORY;
					goto free;
				}
			}
			if (mmap_string_append (gphrase, word) == NULL) {
				DEBUG_SMTP (SMTP_MEM,
					    "BEFORE 426 smtp_word_free\n");
				smtp_word_free (word);
				res = SMTP_ERROR_MEMORY;
				goto free;
			}
			DEBUG_SMTP (SMTP_MEM,
				    "BEFORE 432 smtp_word_free\n");
			smtp_word_free (word);
			first = FALSE;
		}
		else if (r == SMTP_ERROR_PARSE) {
			break;
		}
		else {
			res = r;
			goto free;
		}
	}

	if (first) {
		res = SMTP_ERROR_PARSE;
		goto free;
	}

	str = strdup (gphrase->str);
	if (str == NULL) {
		res = SMTP_ERROR_MEMORY;
		goto free;
	}
	DEBUG_SMTP (SMTP_MEM, "STRDUP smtp_wsp_phrase_parse: pointer=%p\n",
		    str);
	mmap_string_free (gphrase);

	*result = str;
	*index = cur_token;

	return SMTP_NO_ERROR;

      free:
	mmap_string_free (gphrase);
      err:
	return res;
}


int
smtp_wsp_unstrict_crlf_parse (const char *message,
			      size_t length, size_t * index)
{
	size_t cur_token;
	int r;

	cur_token = *index;

	smtp_wsps_parse (message, length, &cur_token);

	r = smtp_char_parse (message, length, &cur_token, '\r');
	if ((r != SMTP_NO_ERROR) && (r != SMTP_ERROR_PARSE))
		return r;

	r = smtp_char_parse (message, length, &cur_token, '\n');
	if (r != SMTP_NO_ERROR)
		return r;

	*index = cur_token;
	return SMTP_NO_ERROR;
}

void
smtp_atom_free (char *atom)
{
	free (atom);
	DEBUG_SMTP (SMTP_MEM, "smtp_atom_free: FREE pointer=%p\n", atom);
}


void
smtp_quoted_string_free (char *quoted_string)
{
	free (quoted_string);
	DEBUG_SMTP (SMTP_MEM, "smtp_quoted_string_free: FREE pointer=%p\n",
		    quoted_string);
}

void
smtp_word_free (char *word)
{
	free (word);
	DEBUG_SMTP (SMTP_MEM, "smtp_word_free: FREE pointer=%p\n", word);
}

void
smtp_phrase_free (char *phrase)
{
	free (phrase);
	DEBUG_SMTP (SMTP_MEM, "smtp_phrase_free: FREE pointer=%p\n",
		    phrase);
}

void
smtp_unstructured_free (char *unstructured)
{
	free (unstructured);
	DEBUG_SMTP (SMTP_MEM, "smtp_unstructured_free: FREE pointer=%p\n",
		    unstructured);
}



/*
#define MIME_VERSION (1 << 16)
*/

/*
x  value := token / quoted-string
*/


int
smtp_mime_value_parse (const char *message, size_t length,
		       size_t * index, char **result)
{
	int r;

	r = smtp_mime_token_parse (message, length, index, result);
	if (r == SMTP_ERROR_PARSE)
		r = smtp_quoted_string_parse (message, length, index, result);

	if (r != SMTP_NO_ERROR)
		return r;

	DEBUG_SMTP (SMTP_DBG, "*result: %s\n", *result);
	return SMTP_NO_ERROR;
}

/*
x  parameter := attribute "=" value
   moved from fields.re to here
*/

int
smtp_mime_parameter_parse (const char *message, size_t length,
			   size_t * index,
			   struct smtp_mime_parameter **result)
{
	char *attribute;
	char *value;
	struct smtp_mime_parameter *parameter;
	size_t cur_token;
	int r;
	int res;

	cur_token = *index;

	DEBUG_SMTP (SMTP_DBG, "smtp_mime_parameter_parse\n");
	r = smtp_mime_attribute_parse (message, length, &cur_token,
				       &attribute);
	if (r != SMTP_NO_ERROR) {
		res = r;
		goto err;
	}

	DEBUG_SMTP (SMTP_DBG, "attribute: %s\n", attribute);
	r = smtp_unstrict_char_parse (message, length, &cur_token, '=');
	if (r != SMTP_NO_ERROR) {
		res = r;
		goto free_attr;
	}

	r = smtp_cfws_parse (message, length, &cur_token);
	if ((r != SMTP_NO_ERROR) && (r != SMTP_ERROR_PARSE)) {
		res = r;
		goto free_attr;
	}

	r = smtp_mime_value_parse (message, length, &cur_token, &value);
	if (r != SMTP_NO_ERROR) {
		DEBUG_SMTP (SMTP_DBG, "smtp_mime_value_parse RETURN = %d\n",
			    r);
		res = r;
		goto free_attr;
	}
	DEBUG_SMTP (SMTP_DBG, "value: %s\n", value);

	parameter = smtp_mime_parameter_new (attribute, value);
	if (parameter == NULL) {
		res = SMTP_ERROR_MEMORY;
		goto free_value;
	}

	*result = parameter;
	*index = cur_token;

	return SMTP_NO_ERROR;

      free_value:
	smtp_mime_value_free (value);
      free_attr:
	smtp_mime_attribute_free (attribute);
      err:
	return res;
}


struct mailmime_list *
mailmime_list_new (void)
{
	clist *list;
	struct mailmime_list *mm_list;

	list = clist_new ();
	if (list == NULL)
		goto err;

	mm_list = malloc (sizeof (struct mailmime_list));
	if (mm_list == NULL)
		goto free;

	mm_list->mm_list = list;
	return mm_list;

      free:
	clist_free (list);
      err:
	return NULL;

}

void
mailmime_list_free (struct mailmime_list *mm_list)
{
	DEBUG_SMTP (SMTP_MEM, "mailmime_list_free: FREE pointer=%p\n",
		    mm_list);
	clist_foreach (mm_list->mm_list, (clist_func) smtp_mime_free, NULL);
	clist_free (mm_list->mm_list);
	free (mm_list);
}

int
add_to_mailmime_list (struct mailmime_list *list, struct mailmime *mm)
{
	int r;

	r = clist_append (list->mm_list, mm);
	if (r < 0)
		return SMTP_ERROR_MEMORY;

	return SMTP_NO_ERROR;

}

void
smtp_mime_attribute_free (char *attribute)
{
	smtp_mime_token_free (attribute);
}


void
smtp_mime_description_free (char *description)
{
	free (description);
	DEBUG_SMTP (SMTP_MEM,
		    "smtp_mime_description_free: FREE pointer=%p\n",
		    description);
}

void
smtp_mime_encoding_free (struct smtp_mime_mechanism *encoding)
{
	smtp_mime_mechanism_free (encoding);
}


void
smtp_mime_extension_token_free (char *extension)
{
	smtp_mime_token_free (extension);
}

struct smtp_mime_parameter *
smtp_mime_parameter_new (char *pa_name, char *pa_value)
{
	struct smtp_mime_parameter *parameter;

	parameter = malloc (sizeof (*parameter));
	if (parameter == NULL)
		return NULL;
	DEBUG_SMTP (SMTP_MEM,
		    "smtp_mime_parameter_new: MALLOC pointer=%p\n",
		    parameter);

	parameter->pa_name = pa_name;
	parameter->pa_value = pa_value;

	return parameter;
}

void
smtp_mime_parameter_free (struct smtp_mime_parameter *parameter)
{
	DEBUG_SMTP (SMTP_MEM, "parameter=%p\n", parameter);
	DEBUG_SMTP (SMTP_MEM, "parameter->pa_name=%p\n", parameter->pa_name);
	DEBUG_SMTP (SMTP_MEM, "parameter->pa_name=%s\n", parameter->pa_name);
	smtp_mime_attribute_free (parameter->pa_name);
	DEBUG_SMTP (SMTP_MEM, "parameter->pa_value=%p\n",
		    parameter->pa_value);
	DEBUG_SMTP (SMTP_MEM, "parameter->pa_value=%s\n",
		    parameter->pa_value);
	smtp_mime_value_free (parameter->pa_value);
	free (parameter);
	DEBUG_SMTP (SMTP_MEM, "smtp_mime_parameter_free: FREE pointer=%p\n",
		    parameter);
}

void
smtp_mime_subtype_free (char *subtype)
{
	smtp_mime_extension_token_free (subtype);
}


void
smtp_mime_token_free (char *token)
{
	free (token);
	DEBUG_SMTP (SMTP_MEM, "smtp_mime_token_free: FREE pointer=%p\n",
		    token);
}

void
smtp_mime_value_free (char *value)
{
	free (value);
	DEBUG_SMTP (SMTP_MEM, "smtp_mime_value_free: FREE pointer=%p\n",
		    value);
}


struct smtp_mime_text_stream *
smtp_mime_text_stream_new (int encoding, int encoded)
{
	struct smtp_mime_text_stream *mime_text_stream;
	mime_text_stream = malloc (sizeof (struct smtp_mime_text_stream));
	if (mime_text_stream == NULL)
		return NULL;
	DEBUG_SMTP (SMTP_MEM,
		    "smtp_mime_text_stream_new: MALLOC pointer=%p\n",
		    mime_text_stream);

#ifdef USE_NEL
	NEL_REF_INIT (mime_text_stream);
#endif

	mime_text_stream->ts_encoding = encoding;
	mime_text_stream->ts_encoded = encoded;

	mime_text_stream->ts_length = 0;
	mime_text_stream->ts_stream = nel_stream_alloc (2048);
}

void
smtp_mime_text_stream_free (struct smtp_mime_text_stream *mime_text_stream)
{
	DEBUG_SMTP (SMTP_DBG, "smtp_mime_text_free\n");
	nel_stream_free (mime_text_stream->ts_stream);
	free (mime_text_stream);
	DEBUG_SMTP (SMTP_MEM,
		    "smtp_mime_text_stream_free: FREE pointer=%p\n",
		    mime_text_stream);
}

struct smtp_mime_text *
smtp_mime_text_new (int encoding, int encoded,
		    char *text, unsigned int length)
{
	struct smtp_mime_text *mime_text;
	mime_text = malloc (sizeof (*mime_text));
	if (mime_text == NULL)
		return NULL;
	DEBUG_SMTP (SMTP_MEM, "smtp_mime_text_new: MALLOC pointer=%p\n",
		    mime_text);

#ifdef USE_NEL
	NEL_REF_INIT (mime_text);
#endif

	mime_text->encoding = encoding;
	mime_text->encoded = encoded;

	mime_text->data = NULL;
	mime_text->length = 0;

	if (text != NULL && length > 0) {
		char *new_text = (char *) malloc (length + 1);
		DEBUG_SMTP (SMTP_MEM,
			    "smtp_mime_text_new: MALLOC pointer=%p\n",
			    new_text);
		if (new_text == NULL)
			return NULL;
		memcpy (new_text, text, length);
		new_text[length] = '\0';
		mime_text->length = length;
		mime_text->data = new_text;
	}

	return mime_text;
}

void
smtp_mime_text_free (struct smtp_mime_text *mime_text)
{
	DEBUG_SMTP (SMTP_DBG, "smtp_mime_text_free\n");
	free ((char *) mime_text->data);
	DEBUG_SMTP (SMTP_MEM, "smtp_mime_text_free: FREE pointer=%p\n",
		    (char *) mime_text->data);
	free (mime_text);
	DEBUG_SMTP (SMTP_MEM, "smtp_mime_text_free: FREE pointer=%p\n",
		    mime_text);
}


struct smtp_mime_data *
smtp_mime_data_new (int dt_type, int dt_encoding,
		    int dt_encoded, const char *dt_data,
		    unsigned int dt_length, char *dt_filename)
{
	struct smtp_mime_data *mime_data;
	char *data;

	mime_data = malloc (sizeof (*mime_data));
	if (mime_data == NULL)
		return NULL;
	DEBUG_SMTP (SMTP_MEM, "smtp_mime_data_new: MALLOC pointer=%p\n",
		    mime_data);


#ifdef USE_NEL
	NEL_REF_INIT (mime_data);
#endif

	mime_data->dt_type = dt_type;
	mime_data->dt_encoding = dt_encoding;
	mime_data->dt_encoded = dt_encoded;
	switch (dt_type) {
	case SMTP_MIME_DATA_TEXT:
		mime_data->dt_data.dt_text.dt_data = dt_data;
		mime_data->dt_data.dt_text.dt_length = dt_length;
		break;
	case SMTP_MIME_DATA_FILE:
		mime_data->dt_data.dt_filename = dt_filename;
		break;
	}

	return mime_data;
}



void
smtp_mime_data_free (struct smtp_mime_data *mime_data)
{
	DEBUG_SMTP (SMTP_DBG, "smtp_mime_data_free\n");
	switch (mime_data->dt_type) {

	case SMTP_MIME_DATA_TEXT:
		DEBUG_SMTP (SMTP_DBG, "free data\n");
		free ((char *) mime_data->dt_data.dt_text.dt_data);
		DEBUG_SMTP (SMTP_MEM,
			    "smtp_mime_data_free: FREE pointer=%p\n",
			    (char *) mime_data->dt_data.dt_text.dt_data);
		break;

	case SMTP_MIME_DATA_FILE:
		free (mime_data->dt_data.dt_filename);
		DEBUG_SMTP (SMTP_MEM,
			    "smtp_mime_data_free: FREE pointer=%p\n",
			    mime_data->dt_data.dt_filename);
		break;
	}
	free (mime_data);
	DEBUG_SMTP (SMTP_MEM, "smtp_mime_data_free: FREE pointer=%p\n",
		    mime_data);
}

/* *************************************************************** */
int
is_digit (char ch)
{
	return (ch >= '0') && (ch <= '9');
}

int
smtp_digit_parse (const char *message, size_t length,
		  size_t * index, int *result)
{
	size_t cur_token;

	cur_token = *index;

	if (cur_token >= length)
		return SMTP_ERROR_CONTINUE;
	if (cur_token >= length)
		return SMTP_ERROR_PARSE;

	DEBUG_SMTP (SMTP_DBG, "message[cur_token] = %c\n",
		    message[cur_token]);
	if (is_digit (message[cur_token])) {
		*result = message[cur_token] - '0';
		cur_token++;
		*index = cur_token;
		return SMTP_NO_ERROR;
	}
	else {
		return SMTP_ERROR_PARSE;
	}
}


int
smtp_number_parse (const char *message, size_t length,
		   size_t * index, uint32_t * result)
{
	size_t cur_token;
	int digit;
	uint32_t number;
	int parsed;
	int r;

	cur_token = *index;
	parsed = FALSE;

	number = 0;
	while (1) {
		r = smtp_digit_parse (message, length, &cur_token, &digit);
		if (r != SMTP_NO_ERROR) {
			if (r == SMTP_ERROR_PARSE)
				break;
			else
				return r;
		}
		number *= 10;
		number += digit;
		parsed = TRUE;
	}

	if (!parsed)
		return SMTP_ERROR_PARSE;

	*result = number;
	*index = cur_token;

	return SMTP_NO_ERROR;
}


int
smtp_char_parse (const char *message, size_t length,
		 size_t * index, char token)
{
	size_t cur_token;

	cur_token = *index;

	if (cur_token >= length)
		return SMTP_ERROR_CONTINUE;
	if (cur_token >= length)
		return SMTP_ERROR_PARSE;

	if (message[cur_token] == token) {
		cur_token++;
		*index = cur_token;
		return SMTP_NO_ERROR;
	}
	else
		return SMTP_ERROR_PARSE;
}

int
smtp_unstrict_char_parse (const char *message, size_t length,
			  size_t * index, char token)
{
	size_t cur_token;
	int r;

	cur_token = *index;

	r = smtp_cfws_parse (message, length, &cur_token);
	if ((r != SMTP_NO_ERROR) && (r != SMTP_ERROR_PARSE))
		return r;

	r = smtp_char_parse (message, length, &cur_token, token);
	if (r != SMTP_NO_ERROR)
		return r;

	*index = cur_token;

	return SMTP_NO_ERROR;
}

int
smtp_token_case_insensitive_len_parse (const char *message, size_t length,
				       size_t * index, char *token,
				       size_t token_length)
{
	size_t cur_token;

	cur_token = *index;
	if (cur_token + token_length - 1 >= length)
		return SMTP_ERROR_CONTINUE;
	if (cur_token + token_length - 1 >= length)
		return SMTP_ERROR_PARSE;

	if (strncasecmp (message + cur_token, token, token_length) == 0) {
		cur_token += token_length;
		*index = cur_token;
		return SMTP_NO_ERROR;
	}
	else
		return SMTP_ERROR_PARSE;
}


int
smtp_oparenth_parse (const char *message, size_t length, size_t * index)
{
	return smtp_char_parse (message, length, index, '(');
}

int
smtp_cparenth_parse (const char *message, size_t length, size_t * index)
{
	return smtp_char_parse (message, length, index, ')');
}

int
smtp_comma_parse (const char *message, size_t length, size_t * index)
{
	return smtp_unstrict_char_parse (message, length, index, ',');
}

int
smtp_dquote_parse (const char *message, size_t length, size_t * index)
{
	return smtp_char_parse (message, length, index, '\"');
}

int
smtp_colon_parse (const char *message, size_t length, size_t * index)
{
	return smtp_unstrict_char_parse (message, length, index, ':');
}

int
smtp_semi_colon_parse (const char *message, size_t length, size_t * index)
{
	return smtp_unstrict_char_parse (message, length, index, ';');
}

int
smtp_plus_parse (const char *message, size_t length, size_t * index)
{
	return smtp_unstrict_char_parse (message, length, index, '+');
}

int
smtp_minus_parse (const char *message, size_t length, size_t * index)
{
	return smtp_unstrict_char_parse (message, length, index, '-');
}

int
smtp_lower_parse (const char *message, size_t length, size_t * index)
{
	return smtp_unstrict_char_parse (message, length, index, '<');
}

int
smtp_greater_parse (const char *message, size_t length, size_t * index)
{
	return smtp_unstrict_char_parse (message, length, index, '>');
}

int
smtp_obracket_parse (const char *message, size_t length, size_t * index)
{
	return smtp_unstrict_char_parse (message, length, index, '[');
}

int
smtp_cbracket_parse (const char *message, size_t length, size_t * index)
{
	return smtp_unstrict_char_parse (message, length, index, ']');
}

int
smtp_at_sign_parse (const char *message, size_t length, size_t * index)
{
	return smtp_unstrict_char_parse (message, length, index, '@');
}

int
smtp_point_parse (const char *message, size_t length, size_t * index)
{
	return smtp_unstrict_char_parse (message, length, index, '.');
}


int
smtp_custom_string_parse (const char *message, size_t length,
			  size_t * index, char **result,
			  int (*is_custom_char) (char))
{
	size_t begin;
	size_t end;
	char *gstr;

	begin = *index;

	end = begin;

	DEBUG_SMTP (SMTP_DBG, "end = %lu, length = %lu\n", end, length);
	if (end >= length)
		return SMTP_ERROR_CONTINUE;
	if (end >= length)
		return SMTP_ERROR_PARSE;

	while (is_custom_char (message[end])) {
		end++;
		if (end >= length)
			break;
	}

	if (end != begin) {
		/*
		   gstr = strndup(message + begin, end - begin);
		 */
		gstr = malloc (end - begin + 1);
		if (gstr == NULL)
			return SMTP_ERROR_MEMORY;
		DEBUG_SMTP (SMTP_MEM,
			    "smtp_custom_string_parse: MALLOC pointer=%p\n",
			    gstr);
		strncpy (gstr, message + begin, end - begin);
		gstr[end - begin] = '\0';

		*index = end;
		*result = gstr;
		return SMTP_NO_ERROR;
	}
	else {
		return SMTP_ERROR_PARSE;
	}
}



int
smtp_struct_multiple_parse (const char *message, size_t length,
			    size_t * index, clist ** result,
			    smtp_struct_parser * parser,
			    smtp_struct_destructor * destructor)
{
	clist *struct_list;
	size_t cur_token;
	void *value;
	int r;
	int res;

	cur_token = *index;

	r = parser (message, length, &cur_token, &value);
	if (r != SMTP_NO_ERROR) {
		res = r;
		goto err;
	}

	struct_list = clist_new ();
	if (struct_list == NULL) {
		destructor (value);
		res = SMTP_ERROR_MEMORY;
		goto err;
	}

	r = clist_append (struct_list, value);
	if (r < 0) {
		destructor (value);
		res = SMTP_ERROR_MEMORY;
		goto free;
	}

	while (1) {
		r = parser (message, length, &cur_token, &value);
		if (r != SMTP_NO_ERROR) {
			if (r == SMTP_ERROR_PARSE) {
				break;
			}
			else {
				res = r;
				goto free;
			}
		}

		r = clist_append (struct_list, value);
		if (r < 0) {
			(*destructor) (value);
			res = SMTP_ERROR_MEMORY;
			goto free;
		}
	}

	*result = struct_list;
	*index = cur_token;

	return SMTP_NO_ERROR;

      free:
	clist_foreach (struct_list, (clist_func) destructor, NULL);
	clist_free (struct_list);
      err:
	return res;
}


int
smtp_struct_list_parse (const char *message, size_t length,
			size_t * index, clist ** result,
			char symbol,
			smtp_struct_parser * parser,
			smtp_struct_destructor * destructor)
{
	clist *struct_list;
	size_t cur_token;
	void *value;
	size_t final_token;
	int r;
	int res;

	cur_token = *index;

	r = parser (message, length, &cur_token, &value);
	if (r != SMTP_NO_ERROR) {
		res = r;
		goto err;
	}

	struct_list = clist_new ();
	if (struct_list == NULL) {
		destructor (value);
		res = SMTP_ERROR_MEMORY;
		goto err;
	}

	r = clist_append (struct_list, value);
	if (r < 0) {
		destructor (value);
		res = SMTP_ERROR_MEMORY;
		goto free;
	}

	final_token = cur_token;

	while (1) {
		r = smtp_unstrict_char_parse (message, length, &cur_token,
					      symbol);
		if (r != SMTP_NO_ERROR) {
			if (r == SMTP_ERROR_PARSE)
				break;
			else {
				res = r;
				goto free;
			}
		}

		r = parser (message, length, &cur_token, &value);
		if (r != SMTP_NO_ERROR) {
			if (r == SMTP_ERROR_PARSE)
				break;
			else {
				res = r;
				goto free;
			}
		}

		r = clist_append (struct_list, value);
		if (r < 0) {
			destructor (value);
			res = SMTP_ERROR_MEMORY;
			goto free;
		}

		final_token = cur_token;
	}

	*result = struct_list;
	*index = final_token;

	return SMTP_NO_ERROR;

      free:
	clist_foreach (struct_list, (clist_func) destructor, NULL);
	clist_free (struct_list);
      err:
	return res;
}


int
smtp_wsp_parse (const char *message, size_t length, size_t * index)
{
	size_t cur_token;

	cur_token = *index;
	if (cur_token >= length) {
		return SMTP_ERROR_CONTINUE;
	}
	if (cur_token >= length) {
		return SMTP_ERROR_PARSE;
	}
	if ((message[cur_token] != ' ') && (message[cur_token] != '\t'))
		return SMTP_ERROR_PARSE;

	cur_token++;
	*index = cur_token;

	return SMTP_NO_ERROR;
}

int
smtp_crlf_parse (const char *message, size_t length, size_t * index)
{
	size_t cur_token;
	int r;

	cur_token = *index;
	r = smtp_char_parse (message, length, &cur_token, '\r');
	if ((r != SMTP_NO_ERROR) && (r != SMTP_ERROR_PARSE))
		return r;

	r = smtp_char_parse (message, length, &cur_token, '\n');
	if (r != SMTP_NO_ERROR)
		return r;

	*index = cur_token;
	return SMTP_NO_ERROR;
}

int
smtp_unstrict_crlf_parse (const char *message, size_t length, size_t * index)
{
	size_t cur_token;
	int r;

	cur_token = *index;
	smtp_cfws_parse (message, length, &cur_token);

	r = smtp_char_parse (message, length, &cur_token, '\r');
	if ((r != SMTP_NO_ERROR) && (r != SMTP_ERROR_PARSE))
		return r;

	r = smtp_char_parse (message, length, &cur_token, '\n');
	if (r != SMTP_NO_ERROR)
		return r;

	*index = cur_token;
	return SMTP_NO_ERROR;
}


/* ************************************************************************ */



/* RFC 2822 grammar */

/*
NO-WS-CTL       =       %d1-8 /         ; US-ASCII control characters
                        %d11 /          ;  that do not include the
                        %d12 /          ;  carriage return, line feed,
                        %d14-31 /       ;  and white space characters
                        %d127
*/

int
is_no_ws_ctl (char ch)
{
	if ((ch == 9) || (ch == 10) || (ch == 13))
		return FALSE;

	if (ch == 127)
		return TRUE;

	return (ch >= 1) && (ch <= 31);
}

/*
text            =       %d1-9 /         ; Characters excluding CR and LF
                        %d11 /
                        %d12 /
                        %d14-127 /
                        obs-text
*/

/*
specials        =       "(" / ")" /     ; Special characters used in
                        "<" / ">" /     ;  other parts of the syntax
                        "[" / "]" /
                        ":" / ";" /
                        "@" / "\" /
                        "," / "." /
                        DQUOTE
*/

/*
quoted-pair     =       ("\" text) / obs-qp
*/

int
smtp_quoted_pair_parse (const char *message, size_t length,
			size_t * index, char *result)
{
	size_t cur_token;

	cur_token = *index;
	if (cur_token + 1 >= length)
		return SMTP_ERROR_CONTINUE;
	if (cur_token + 1 >= length)
		return SMTP_ERROR_PARSE;

	if (message[cur_token] != '\\')
		return SMTP_ERROR_PARSE;

	cur_token++;
	*result = message[cur_token];
	cur_token++;
	*index = cur_token;

	return SMTP_NO_ERROR;
}


/*
FWS             =       ([*WSP CRLF] 1*WSP) /   ; Folding white space
                        obs-FWS
*/

int
smtp_fws_parse (const char *message, size_t length, size_t * index)
{
	size_t cur_token;
	size_t final_token;
	int fws_1;
	int fws_2;
	int fws_3;
	int r;

	cur_token = *index;

	fws_1 = FALSE;
	while (1) {
		r = smtp_wsp_parse (message, length, &cur_token);
		if (r != SMTP_NO_ERROR) {
			if (r == SMTP_ERROR_PARSE)
				break;
			else
				return r;
		}
		fws_1 = TRUE;
	}
	final_token = cur_token;

	r = smtp_crlf_parse (message, length, &cur_token);
	switch (r) {
	case SMTP_NO_ERROR:
		fws_2 = TRUE;
		break;
	case SMTP_ERROR_PARSE:
		fws_2 = FALSE;
		break;
	default:
		return r;
	}

	fws_3 = FALSE;
	if (fws_2) {
		while (1) {
			r = smtp_wsp_parse (message, length, &cur_token);
			if (r != SMTP_NO_ERROR) {
				if (r == SMTP_ERROR_PARSE)
					break;
				else
					return r;
			}
			fws_3 = TRUE;
		}
	}

	if ((!fws_1) && (!fws_3)) {
		return SMTP_ERROR_PARSE;
	}

	if (!fws_3) {
		cur_token = final_token;
	}

	*index = cur_token;
	return SMTP_NO_ERROR;
}



/*
ctext           =       NO-WS-CTL /     ; Non white space controls

                        %d33-39 /       ; The rest of the US-ASCII
                        %d42-91 /       ;  characters not including "(",
                        %d93-126        ;  ")", or "\"
*/

int
is_ctext (char ch)
{
	unsigned char uch = (unsigned char) ch;

	if (is_no_ws_ctl (ch))
		return TRUE;

	if (uch < 33)
		return FALSE;

	if ((uch == 40) || (uch == 41))
		return FALSE;

	if (uch == 92)
		return FALSE;

	if (uch == 127)
		return FALSE;

	return TRUE;
}


/*
ccontent        =       ctext / quoted-pair / comment
*/

int
smtp_ccontent_parse (const char *message, size_t length, size_t * index)
{
	size_t cur_token;
	char ch;
	int r;

	cur_token = *index;
	if (cur_token >= length)
		return SMTP_ERROR_CONTINUE;
	if (cur_token >= length)
		return SMTP_ERROR_PARSE;

	if (is_ctext (message[cur_token])) {
		cur_token++;
	}
	else {
		r = smtp_quoted_pair_parse (message, length, &cur_token, &ch);

		if (r == SMTP_ERROR_PARSE)
			r = smtp_comment_parse (message, length, &cur_token);

		if (r == SMTP_ERROR_PARSE)
			return r;
	}

	*index = cur_token;

	return SMTP_NO_ERROR;
}



/*
[FWS] ccontent
*/

int
smtp_comment_fws_ccontent_parse (const char *message, size_t length,
				 size_t * index)
{
	size_t cur_token;
	int r;

	cur_token = *index;

	r = smtp_fws_parse (message, length, &cur_token);
	if ((r != SMTP_NO_ERROR) && (r != SMTP_ERROR_PARSE))
		return r;

	r = smtp_ccontent_parse (message, length, &cur_token);
	if (r != SMTP_NO_ERROR)
		return r;

	*index = cur_token;

	return SMTP_NO_ERROR;
}


/*
comment         =       "(" *([FWS] ccontent) [FWS] ")"
*/

int
smtp_comment_parse (const char *message, size_t length, size_t * index)
{
	size_t cur_token;
	int r;

	cur_token = *index;

	r = smtp_oparenth_parse (message, length, &cur_token);
	if (r != SMTP_NO_ERROR)
		return r;

	while (1) {
		r = smtp_comment_fws_ccontent_parse (message, length,
						     &cur_token);
		if (r != SMTP_NO_ERROR) {
			if (r == SMTP_ERROR_PARSE)
				break;
			else
				return r;
		}
	}

	r = smtp_fws_parse (message, length, &cur_token);
	if ((r != SMTP_NO_ERROR) && (r != SMTP_ERROR_PARSE))
		return r;

	r = smtp_cparenth_parse (message, length, &cur_token);
	if (r != SMTP_NO_ERROR)
		return r;

	*index = cur_token;

	return SMTP_NO_ERROR;
}


/*
[FWS] comment
*/

int
smtp_cfws_fws_comment_parse (const char *message, size_t length,
			     size_t * index)
{
	size_t cur_token;
	int r;

	cur_token = *index;

	r = smtp_fws_parse (message, length, &cur_token);
	if ((r != SMTP_NO_ERROR) && (r != SMTP_ERROR_PARSE))
		return r;

	r = smtp_comment_parse (message, length, &cur_token);
	if (r != SMTP_NO_ERROR)
		return r;

	*index = cur_token;

	return SMTP_NO_ERROR;
}


/*
CFWS            =       *([FWS] comment) (([FWS] comment) / FWS)
*/

int
smtp_cfws_parse (const char *message, size_t length, size_t * index)
{
	size_t cur_token;
	int has_comment;
	int r;

	cur_token = *index;

	has_comment = FALSE;
	while (1) {
		//DEBUG_SMTP(SMTP_DBG, "message+cur_token = %s\n", message+cur_token);
		r = smtp_cfws_fws_comment_parse (message, length, &cur_token);
		if (r != SMTP_NO_ERROR) {
			if (r == SMTP_ERROR_PARSE) {
				//DEBUG_SMTP(SMTP_DBG, "message+cur_token = %s\n", message+cur_token);
				break;
			}
			else {
				//DEBUG_SMTP(SMTP_DBG, "message+cur_token = %s\n", message+cur_token);
				return r;
			}
		}
		has_comment = TRUE;
	}

	if (!has_comment) {
		//DEBUG_SMTP(SMTP_DBG, "message+cur_token = %s\n", message+cur_token);
		r = smtp_fws_parse (message, length, &cur_token);
		if (r != SMTP_NO_ERROR) {
			//DEBUG_SMTP(SMTP_DBG, "message+cur_token = %s\n", message+cur_token);
			return r;
		}
	}

	*index = cur_token;
	//DEBUG_SMTP(SMTP_DBG, "message+cur_token = %s\n", message+cur_token);

	return SMTP_NO_ERROR;
}


/*
atom            =       [CFWS] 1*atext [CFWS]
*/

int
smtp_atom_parse (const char *message, size_t length,
		 size_t * index, char **result)
{
	size_t cur_token;
	int r;
	int res;
	char *atom;
	size_t end;

	cur_token = *index;

	r = smtp_cfws_parse (message, length, &cur_token);
	if ((r != SMTP_NO_ERROR) && (r != SMTP_ERROR_PARSE)) {
		res = r;
		goto err;
	}

	end = cur_token;
	if (end >= length) {
		res = SMTP_ERROR_PARSE;
		goto err;
	}

	while (is_atext (message[end])) {
		end++;
		if (end >= length)
			break;
	}
	if (end == cur_token) {
		res = SMTP_ERROR_PARSE;
		goto err;
	}

	atom = malloc (end - cur_token + 1);
	if (atom == NULL) {
		res = SMTP_ERROR_MEMORY;
		goto err;
	}
	DEBUG_SMTP (SMTP_MEM, "smtp_atom_parse: MALLOC pointer=%p\n", atom);

	strncpy (atom, message + cur_token, end - cur_token);
	atom[end - cur_token] = '\0';

	cur_token = end;

	*index = cur_token;
	*result = atom;

	return SMTP_NO_ERROR;

      err:
	return res;
}

int
smtp_fws_atom_parse (const char *message, size_t length,
		     size_t * index, char **result)
{
	size_t cur_token;
	int r;
	int res;
	char *atom;
	size_t end;

	cur_token = *index;

	r = smtp_fws_parse (message, length, &cur_token);
	if ((r != SMTP_NO_ERROR) && (r != SMTP_ERROR_PARSE)) {
		res = r;
		goto err;
	}

	end = cur_token;
	if (end >= length) {
		res = SMTP_ERROR_PARSE;
		goto err;
	}

	while (is_atext (message[end])) {
		end++;
		if (end >= length)
			break;
	}
	if (end == cur_token) {
		res = SMTP_ERROR_PARSE;
		goto err;
	}

	atom = malloc (end - cur_token + 1);
	if (atom == NULL) {
		res = SMTP_ERROR_MEMORY;
		goto err;
	}
	DEBUG_SMTP (SMTP_MEM, "smtp_fws_atom_parse: MALLOC pointer=%p\n",
		    atom);

	strncpy (atom, message + cur_token, end - cur_token);
	atom[end - cur_token] = '\0';

	cur_token = end;

	*index = cur_token;
	*result = atom;

	return SMTP_NO_ERROR;

      err:
	return res;
}


/*
dot-atom        =       [CFWS] dot-atom-text [CFWS]
*/
int
smtp_dot_atom_parse (const char *message, size_t length,
		     size_t * index, char **result)
{
	return smtp_atom_parse (message, length, index, result);
}


/*
qtext           =       NO-WS-CTL /     ; Non white space controls

                        %d33 /          ; The rest of the US-ASCII
                        %d35-91 /       ;  characters not including "\"
                        %d93-126        ;  or the quote character
*/

int
is_qtext (char ch)
{
	unsigned char uch = (unsigned char) ch;

	if (is_no_ws_ctl (ch))
		return TRUE;

	if (uch < 33)
		return FALSE;

	if (uch == 34)
		return FALSE;

	if (uch == 92)
		return FALSE;

	if (uch == 127)
		return FALSE;

	return TRUE;
}


/*
qcontent        =       qtext / quoted-pair
*/

int
smtp_qcontent_parse (const char *message, size_t length,
		     size_t * index, char *result)
{
	size_t cur_token;
	char ch;
	int r;

	cur_token = *index;
	if (cur_token >= length)
		return SMTP_ERROR_CONTINUE;
	if (cur_token >= length)
		return SMTP_ERROR_PARSE;

	if (is_qtext (message[cur_token])) {
		ch = message[cur_token];
		cur_token++;
	}
	else {
		r = smtp_quoted_pair_parse (message, length, &cur_token, &ch);

		if (r != SMTP_NO_ERROR)
			return r;
	}

	*result = ch;
	*index = cur_token;

	return SMTP_NO_ERROR;
}


/*
quoted-string   =       [CFWS]
                        DQUOTE *([FWS] qcontent) [FWS] DQUOTE
                        [CFWS]
*/

int
smtp_quoted_string_parse (const char *message, size_t length,
			  size_t * index, char **result)
{
	size_t cur_token;
	MMAPString *gstr;
	char ch;
	char *str;
	int r;
	int res;

	cur_token = *index;

	r = smtp_cfws_parse (message, length, &cur_token);
	if ((r != SMTP_NO_ERROR) && (r != SMTP_ERROR_PARSE)) {
		res = r;
		goto err;
	}

	r = smtp_dquote_parse (message, length, &cur_token);
	if (r != SMTP_NO_ERROR) {
		res = r;
		goto err;
	}

	gstr = mmap_string_new ("");
	if (gstr == NULL) {
		res = SMTP_ERROR_MEMORY;
		goto err;
	}

	while (1) {
		r = smtp_fws_parse (message, length, &cur_token);
		if (r == SMTP_NO_ERROR) {
			if (mmap_string_append_c (gstr, ' ') == NULL) {
				res = SMTP_ERROR_MEMORY;
				goto free_gstr;
			}
		}
		else if (r != SMTP_ERROR_PARSE) {
			res = r;
			goto free_gstr;
		}

		r = smtp_qcontent_parse (message, length, &cur_token, &ch);
		if (r == SMTP_NO_ERROR) {
			if (mmap_string_append_c (gstr, ch) == NULL) {
				res = SMTP_ERROR_MEMORY;
				goto free_gstr;
			}
		}
		else if (r == SMTP_ERROR_PARSE)
			break;
		else {
			res = r;
			goto free_gstr;
		}
	}

	r = smtp_dquote_parse (message, length, &cur_token);
	if (r != SMTP_NO_ERROR) {
		res = r;
		goto free_gstr;
	}


	str = strdup (gstr->str);
	if (str == NULL) {
		res = SMTP_ERROR_MEMORY;
		goto free_gstr;
	}
	DEBUG_SMTP (SMTP_MEM,
		    "STRDUP smtp_quoted_string_parse: pointer=%p\n", str);
	mmap_string_free (gstr);

	*index = cur_token;
	*result = str;

	return SMTP_NO_ERROR;

      free_gstr:
	mmap_string_free (gstr);
      err:
	return res;
}


int
smtp_fws_quoted_string_parse (const char *message, size_t length,
			      size_t * index, char **result)
{
	size_t cur_token;
	MMAPString *gstr;
	char ch;
	char *str;
	int r;
	int res;

	cur_token = *index;

	r = smtp_fws_parse (message, length, &cur_token);
	if ((r != SMTP_NO_ERROR) && (r != SMTP_ERROR_PARSE)) {
		res = r;
		goto err;
	}

	r = smtp_dquote_parse (message, length, &cur_token);
	if (r != SMTP_NO_ERROR) {
		res = r;
		goto err;
	}

	gstr = mmap_string_new ("");
	if (gstr == NULL) {
		res = SMTP_ERROR_MEMORY;
		goto err;
	}

	while (1) {
		DEBUG_SMTP (SMTP_DBG, "\n");
		r = smtp_fws_parse (message, length, &cur_token);
		if (r == SMTP_NO_ERROR) {
			if (mmap_string_append_c (gstr, ' ') == NULL) {
				res = SMTP_ERROR_MEMORY;
				goto free_gstr;
			}
		}
		else if (r != SMTP_ERROR_PARSE) {
			res = r;
			goto free_gstr;
		}

		r = smtp_qcontent_parse (message, length, &cur_token, &ch);
		if (r == SMTP_NO_ERROR) {
			if (mmap_string_append_c (gstr, ch) == NULL) {
				res = SMTP_ERROR_MEMORY;
				goto free_gstr;
			}
		}
		else if (r == SMTP_ERROR_PARSE)
			break;
		else {
			res = r;
			goto free_gstr;
		}
	}

	DEBUG_SMTP (SMTP_DBG, "\n");
	r = smtp_dquote_parse (message, length, &cur_token);
	if (r != SMTP_NO_ERROR) {
		res = r;
		goto free_gstr;
	}

	str = strdup (gstr->str);
	if (str == NULL) {
		res = SMTP_ERROR_MEMORY;
		goto free_gstr;
	}
	DEBUG_SMTP (SMTP_MEM,
		    "STRDUP smtp_fws_quoted_string_parse: pointer=%p\n", str);
	mmap_string_free (gstr);

	*index = cur_token;
	*result = str;

	return SMTP_NO_ERROR;

      free_gstr:
	mmap_string_free (gstr);
      err:
	return res;
}


/*
word            =       atom / quoted-string
*/

int
smtp_word_parse (const char *message, size_t length,
		 size_t * index, char **result)
{
	size_t cur_token;
	char *word;
	int r;

	cur_token = *index;
	r = smtp_atom_parse (message, length, &cur_token, &word);

	if (r == SMTP_ERROR_PARSE)
		r = smtp_quoted_string_parse (message, length, &cur_token,
					      &word);

	if (r != SMTP_NO_ERROR)
		return r;

	*result = word;
	*index = cur_token;

	return SMTP_NO_ERROR;
}



int
smtp_fws_word_parse (const char *message, size_t length,
		     size_t * index, char **result)
{
	size_t cur_token;
	char *word;
	int r;

	cur_token = *index;
	r = smtp_fws_atom_parse (message, length, &cur_token, &word);

	DEBUG_SMTP (SMTP_DBG, "\n");
	if (r == SMTP_ERROR_PARSE) {
		r = smtp_fws_quoted_string_parse (message, length, &cur_token,
						  &word);
	}

	DEBUG_SMTP (SMTP_DBG, "\n");
	if (r != SMTP_NO_ERROR) {
		return r;
	}

	*result = word;
	*index = cur_token;

	return SMTP_NO_ERROR;
}


/*
phrase          =       1*word / obs-phrase
*/

int
smtp_phrase_parse (const char *message, size_t length,
		   size_t * index, char **result)
{
	MMAPString *gphrase;
	char *word;
	int first;
	size_t cur_token;
	int r;
	int res;
	char *str;

	cur_token = *index;

	gphrase = mmap_string_new ("");
	if (gphrase == NULL) {
		res = SMTP_ERROR_MEMORY;
		goto err;
	}

	first = TRUE;

	while (1) {
		r = smtp_fws_word_parse (message, length, &cur_token, &word);
		if (r == SMTP_NO_ERROR) {
			DEBUG_SMTP (SMTP_DBG, "word = %s\n", word);
			if (!first) {
				if (mmap_string_append_c (gphrase, ' ') ==
				    NULL) {
					DEBUG_SMTP (SMTP_MEM,
						    "BEFORE smtp_word_free\n");
					smtp_word_free (word);
					res = SMTP_ERROR_MEMORY;
					goto free;
				}
			}
			DEBUG_SMTP (SMTP_DBG, "\n");
			if (mmap_string_append (gphrase, word) == NULL) {
				DEBUG_SMTP (SMTP_MEM,
					    "BEFORE smtp_word_free\n");
				smtp_word_free (word);
				res = SMTP_ERROR_MEMORY;
				goto free;
			}
			DEBUG_SMTP (SMTP_MEM, "BEFORE smtp_word_free\n");
			smtp_word_free (word);
			first = FALSE;
		}
		else if (r == SMTP_ERROR_PARSE) {
			break;
		}
		else {
			res = r;
			goto free;
		}
	}

	if (first) {
		res = SMTP_ERROR_PARSE;
		goto free;
	}

	str = strdup (gphrase->str);
	if (str == NULL) {
		res = SMTP_ERROR_MEMORY;
		goto free;
	}
	DEBUG_SMTP (SMTP_MEM, "STRDUP smtp_phrase_parse: pointer=%p\n",
		    str);
	mmap_string_free (gphrase);

	*result = str;
	*index = cur_token;

	return SMTP_NO_ERROR;

      free:
	mmap_string_free (gphrase);
      err:
	return res;
}



/*
utext           =       NO-WS-CTL /     ; Non white space controls
                        %d33-126 /      ; The rest of US-ASCII
                        obs-utext

added : WSP
*/


enum
{
	UNSTRUCTURED_START,
	UNSTRUCTURED_CR,
	UNSTRUCTURED_LF,
	UNSTRUCTURED_WSP,
	UNSTRUCTURED_OUT
};

int
smtp_unstructured_parse (const char *message, size_t length,
			 size_t * index, char **result)
{
	size_t cur_token;
	int state;
	size_t begin;
	size_t terminal;
	char *str;

	cur_token = *index;


	DEBUG_SMTP (SMTP_DBG, "\n");
	while (1) {
		int r;

		r = smtp_wsp_parse (message, length, &cur_token);
		if (r == SMTP_NO_ERROR) {
			/* do nothing */
		}
		else if (r == SMTP_ERROR_PARSE) {
			DEBUG_SMTP (SMTP_DBG, "\n");
			break;
		}
		else {
			DEBUG_SMTP (SMTP_DBG, "\n");
			return r;
		}
	}

	state = UNSTRUCTURED_START;
	begin = cur_token;
	terminal = cur_token;

	while (state != UNSTRUCTURED_OUT) {

		switch (state) {
		case UNSTRUCTURED_START:
			if (cur_token >= length)
				return SMTP_ERROR_CONTINUE;
			if (cur_token >= length)
				return SMTP_ERROR_PARSE;

			terminal = cur_token;
			switch (message[cur_token]) {
			case '\r':
				state = UNSTRUCTURED_CR;
				break;
			case '\n':
				state = UNSTRUCTURED_LF;
				break;
			default:
				state = UNSTRUCTURED_START;
				break;
			}
			break;
		case UNSTRUCTURED_CR:
			if (cur_token >= length)
				return SMTP_ERROR_CONTINUE;
			if (cur_token >= length)
				return SMTP_ERROR_PARSE;

			switch (message[cur_token]) {
			case '\n':
				state = UNSTRUCTURED_LF;
				break;
			default:
				state = UNSTRUCTURED_START;
				break;
			}
			break;

		case UNSTRUCTURED_LF:
			if (cur_token >= length) {
				state = UNSTRUCTURED_OUT;
				break;
			}

			switch (message[cur_token]) {
			case '\t':
			case ' ':
				state = UNSTRUCTURED_WSP;
				break;
			default:
				state = UNSTRUCTURED_OUT;
				break;
			}
			break;
		case UNSTRUCTURED_WSP:
			if (cur_token >= length)
				return SMTP_ERROR_CONTINUE;
			if (cur_token >= length)
				return SMTP_ERROR_PARSE;

			switch (message[cur_token]) {
			case '\r':
				state = UNSTRUCTURED_CR;
				break;
			case '\n':
				state = UNSTRUCTURED_LF;
				break;
			default:
				state = UNSTRUCTURED_START;
				break;
			}
			break;
		}

		cur_token++;
	}

	str = malloc (terminal - begin + 1);
	if (str == NULL)
		return SMTP_ERROR_MEMORY;
	DEBUG_SMTP (SMTP_MEM,
		    "smtp_unstructured_parse: MALLOC pointer=%p\n", str);

	strncpy (str, message + begin, terminal - begin);
	str[terminal - begin] = '\0';

	*index = terminal;
	*result = str;

	return SMTP_NO_ERROR;
}


int
smtp_ignore_unstructured_parse (const char *message, size_t length,
				size_t * index)
{
	size_t cur_token;
	int state;
	size_t terminal;

	cur_token = *index;

	state = UNSTRUCTURED_START;
	terminal = cur_token;

	while (state != UNSTRUCTURED_OUT) {

		switch (state) {
		case UNSTRUCTURED_START:
			if (cur_token >= length)
				return SMTP_ERROR_CONTINUE;
			if (cur_token >= length)
				return SMTP_ERROR_PARSE;
			terminal = cur_token;
			switch (message[cur_token]) {
			case '\r':
				state = UNSTRUCTURED_CR;
				break;
			case '\n':
				state = UNSTRUCTURED_LF;
				break;
			default:
				state = UNSTRUCTURED_START;
				break;
			}
			break;
		case UNSTRUCTURED_CR:
			if (cur_token >= length)
				return SMTP_ERROR_CONTINUE;
			if (cur_token >= length)
				return SMTP_ERROR_PARSE;
			switch (message[cur_token]) {
			case '\n':
				state = UNSTRUCTURED_LF;
				break;
			default:
				state = UNSTRUCTURED_START;
				break;
			}
			break;
		case UNSTRUCTURED_LF:
			if (cur_token >= length) {
				state = UNSTRUCTURED_OUT;
				break;
			}
			switch (message[cur_token]) {
			case '\t':
			case ' ':
				state = UNSTRUCTURED_WSP;
				break;
			default:
				state = UNSTRUCTURED_OUT;
				break;
			}
			break;
		case UNSTRUCTURED_WSP:
			if (cur_token >= length)
				return SMTP_ERROR_CONTINUE;
			if (cur_token >= length)
				return SMTP_ERROR_PARSE;
			switch (message[cur_token]) {
			case '\r':
				state = UNSTRUCTURED_CR;
				break;
			case '\n':
				state = UNSTRUCTURED_LF;
				break;
			default:
				state = UNSTRUCTURED_START;
				break;
			}
			break;
		}

		cur_token++;
	}

	*index = terminal;

	return SMTP_NO_ERROR;
}



/*
dcontent        =       dtext / quoted-pair
*/

int
smtp_dcontent_parse (const char *message, size_t length,
		     size_t * index, char *result)
{
	size_t cur_token;
	char ch;
	int r;

	cur_token = *index;
	if (cur_token >= length)
		return SMTP_ERROR_CONTINUE;
	if (cur_token >= length)
		return SMTP_ERROR_PARSE;

	if (is_dtext (message[cur_token])) {
		ch = message[cur_token];
		cur_token++;
	}
	else {
		r = smtp_quoted_pair_parse (message, length, &cur_token, &ch);

		if (r != SMTP_NO_ERROR)
			return r;
	}

	*index = cur_token;
	*result = ch;

	return SMTP_NO_ERROR;
}

/*
dtext           =       NO-WS-CTL /     ; Non white space controls

                        %d33-90 /       ; The rest of the US-ASCII
                        %d94-126        ;  characters not including "[",
                                        ;  "]", or "\"
*/

int
is_dtext (char ch)
{
	unsigned char uch = (unsigned char) ch;

	if (is_no_ws_ctl (ch))
		return TRUE;

	if (uch < 33)
		return FALSE;

	if ((uch >= 91) && (uch <= 93))
		return FALSE;

	if (uch == 127)
		return FALSE;

	return TRUE;
}

/*
CHANGE TO THE RFC 2822

original :

fields          =       *(trace
                          *(resent-date /
                           resent-from /
                           resent-sender /
                           resent-to /
                           resent-cc /
                           resent-bcc /
                           resent-msg-id))
                        *(orig-date /
                        from /
                        sender /
                        reply-to /
                        to /
                        cc /
                        bcc /
                        message-id /
                        in-reply-to /
                        references /
                        subject /
                        comments /
                        keywords /
                        optional-field)

INTO THE FOLLOWING :
*/

/*
resent-fields-list =      *(resent-date /
                           resent-from /
                           resent-sender /
                           resent-to /
                           resent-cc /
                           resent-bcc /
                           resent-msg-id))
*/


/*
field           =       delivering-info /
                        orig-date /
                        from /
                        sender /
                        reply-to /
                        to /
                        cc /
                        bcc /
                        message-id /
                        in-reply-to /
                        references /
                        subject /
                        comments /
                        keywords /
                        optional-field
*/


int is_ftext (char ch);


/*
ftext           =       %d33-57 /               ; Any character except
                        %d59-126                ;  controls, SP, and
                                                ;  ":".
*/

int
is_ftext (char ch)
{
	unsigned char uch = (unsigned char) ch;

	if (uch < 33)
		return FALSE;

	if (uch == 58)
		return FALSE;

	return TRUE;
}




/* ********************************************************************** */

/*
x  attribute := token
               ; Matching of attributes
               ; is ALWAYS case-insensitive.
*/

int
smtp_mime_attribute_parse (const char *message, size_t length,
			   size_t * index, char **result)
{
	return smtp_mime_token_parse (message, length, index, result);
}


/*
x  extension-token := ietf-token / x-token
*/


int
smtp_mime_extension_token_parse (const char *message, size_t length,
				 size_t * index, char **result)
{
	return smtp_mime_token_parse (message, length, index, result);
}


/*
x  tspecials :=  "(" / ")" / "<" / ">" / "@" /
                "," / ";" / ":" / "\" / <">
                "/" / "[" / "]" / "?" / "="
                ; Must be in quoted-string,
                ; to use within parameter values
*/

int
is_tspecials (char ch)
{
	switch (ch) {
	case '(':
	case ')':
	case '<':
	case '>':
	case '@':
	case ',':
	case ';':
	case ':':
	case '\\':
	case '\"':
	case '/':
	case '[':
	case ']':
	case '?':
	case '=':
	case '\r':
		return TRUE;
	default:
		return FALSE;
	}
}



/*
x  token := 1*<any (US-ASCII) CHAR except SPACE, CTLs,
              or tspecials>
*/

int
is_token (char ch)
{
	unsigned char uch = (unsigned char) ch;

	if (uch > 0x7F)
		return FALSE;

	if (uch == ' ')
		return FALSE;

	if (is_tspecials (ch))
		return FALSE;

	return TRUE;
}

int
smtp_mime_token_parse (const char *message, size_t length,
		       size_t * index, char **token)
{
	return smtp_custom_string_parse (message, length, index, token,
					 is_token);
}



struct mailmime *
smtp_mime_new (int mm_type,
		const char *mm_mime_start,
	       size_t mm_length,
	       struct smtp_mime_fields *mm_mime_fields,
	       struct smtp_mime_content_type *mm_content_type,
	       //for single message 
	       struct smtp_mime_data *mm_body,
	       //for multipart 
	       struct smtp_mime_data *mm_preamble,
	       struct smtp_mime_data *mm_epilogue,
	       struct mailmime_list *mm_mp_list,
	       //for rfc message 
	       struct smtp_mime_fields *mm_fields, 
		struct mailmime *mm_msg_mime)
{
	struct mailmime *mime;
	clistiter *cur;

	mime = malloc (sizeof (*mime));
	if (mime == NULL)
		return NULL;
	DEBUG_SMTP (SMTP_MEM, "smtp_mime_new: MALLOC pointer=%p\n", mime);


#ifdef USE_NEL
	NEL_REF_INIT (mime);
#endif

	mime->mm_state = SMTP_MSG_FIELDS_PARSING;
	mime->mm_parent = NULL;
	mime->mm_parent_type = SMTP_MIME_NONE;
	mime->mm_multipart_pos = NULL;

	mime->mm_type = mm_type;
	mime->mm_mime_start = mm_mime_start;
	mime->mm_length = mm_length;
	mime->mm_mime_fields = mm_mime_fields;
	mime->mm_content_type = mm_content_type;

	switch (mm_type) {
	case SMTP_MIME_SINGLE:
		mime->mm_data.mm_single = mm_body;
		break;

	case SMTP_MIME_MULTIPLE:
		mime->mm_data.mm_multipart.mm_preamble = mm_preamble;
		mime->mm_data.mm_multipart.mm_epilogue = mm_epilogue;
		mime->mm_data.mm_multipart.mm_mp_list = mm_mp_list;

		for (cur = clist_begin (mm_mp_list->mm_list); cur != NULL;
		     cur = clist_next (cur)) {
			struct mailmime *submime;

			submime = clist_content (cur);
			submime->mm_parent = mime;
			submime->mm_parent_type = SMTP_MIME_MULTIPLE;
			submime->mm_multipart_pos = cur;
		}
		break;

	case SMTP_MIME_MESSAGE:
		mime->mm_data.mm_message.mm_fields = mm_fields;
		mime->mm_data.mm_message.mm_msg_mime = mm_msg_mime;
		if (mm_msg_mime != NULL) {
			mm_msg_mime->mm_parent = mime;
			mm_msg_mime->mm_parent_type = SMTP_MIME_MESSAGE;
		}
		break;

	}

	DEBUG_SMTP (SMTP_DBG, "mime = %p \n", mime);
	return mime;
}

void
smtp_mime_free (struct mailmime *mime)
{
	DEBUG_SMTP (SMTP_DBG, "mime = %p \n", mime);
	switch (mime->mm_type) {
	case SMTP_MIME_SINGLE:
		DEBUG_SMTP (SMTP_DBG, "mime = %p \n", mime);
		if ( (mime->mm_data.mm_single != NULL))
			smtp_mime_data_free (mime->mm_data.mm_single);
		/* do nothing */
		break;

	case SMTP_MIME_MULTIPLE:
		DEBUG_SMTP (SMTP_DBG, "mime = %p \n", mime);
		if (mime->mm_data.mm_multipart.mm_preamble != NULL)
			smtp_mime_data_free (mime->mm_data.mm_multipart.mm_preamble);
		if (mime->mm_data.mm_multipart.mm_epilogue != NULL)
			smtp_mime_data_free (mime->mm_data.mm_multipart.mm_epilogue);
		clist_foreach (mime->mm_data.mm_multipart.mm_mp_list->mm_list,
			       (clist_func) smtp_mime_free, NULL);
		clist_free (mime->mm_data.mm_multipart.mm_mp_list->mm_list);
		break;

	case SMTP_MIME_MESSAGE:
		DEBUG_SMTP (SMTP_DBG, "mime = %p \n", mime);
		
		if (mime->mm_data.mm_message.mm_fields != NULL)
		   smtp_mime_fields_free(mime->mm_data.mm_message.mm_fields);

		if (mime->mm_data.mm_message.mm_msg_mime != NULL)
			smtp_mime_free (mime->mm_data.mm_message.mm_msg_mime);

		break;
	}

	if (mime->mm_mime_fields != NULL)
		smtp_mime_fields_free (mime->mm_mime_fields);
	free (mime);
	DEBUG_SMTP (SMTP_DBG, "mime = %p \n", mime);
	DEBUG_SMTP (SMTP_MEM, "smtp_mime_free: FREE pointer=%p\n", mime);
}
