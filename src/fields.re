#include <ctype.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#include "mmapstring.h"
#include "mem_pool.h"
#include "smtp.h"
#include "fields.h"
#include "disposition.h"
#include "mime.h"
#include "language.h"
#include "version.h"
#include "encoding.h"
#include "content-type.h"
#include "reference.h"
#include "_time.h"
#include "sender.h"
#include "reply-to.h"
#include "cc.h"
#include "resent-cc.h"
#include "bcc.h"
#include "resent-bcc.h"
#include "message-id.h"
#include "resent-msg-id.h"
#include "in_reply_to.h"
#include "subject.h"
#include "comments.h"
#include "keywords.h"
#include "return.h"
#include "from.h"
#include "to.h"
#include "message.h"
#include "address.h"
#include "msg-id.h"

#ifdef USE_NEL
#include "engine.h"

int eoh_id;
extern struct nel_eng *eng;
#endif

int
get_body_type_from_fields (struct smtp_mime_fields *fields)
{
	clistiter *cur;
	int body_type = SMTP_MIME_NONE;

	for (cur = clist_begin (fields->fld_list); cur != NULL;
	     cur = clist_next (cur)) {
		struct smtp_mime_field *field = clist_content (cur);
		if (field->fld_type == SMTP_MIME_FIELD_TYPE) {
			body_type =
				get_body_type_from_content_type (field->
								 fld_data.
								 fld_content_type);
			DEBUG_SMTP (SMTP_DBG, "body_type = %d\n", body_type);
			break;
		}
	}

	return body_type;

}

struct smtp_mime_content_type *
get_content_type_from_fields (struct smtp_mime_fields *fields)
{
	clistiter *cur;
	for (cur = clist_begin (fields->fld_list); cur != NULL;
	     cur = clist_next (cur)) {
		struct smtp_mime_field *field = clist_content (cur);
		if (field->fld_type == SMTP_MIME_FIELD_TYPE) {
			return (field->fld_data.fld_content_type);
		}
	}

	return NULL;

}

enum
{
	UNSTRUCTURED_START,
	UNSTRUCTURED_CR,
	UNSTRUCTURED_LF,
	UNSTRUCTURED_WSP,
	UNSTRUCTURED_OUT
};


/*
field-name      =       1*ftext
*/
int
smtp_field_name_parse (const char *message, size_t length,
		       size_t * index, char **result)
{
	char *field_name;
	size_t cur_token;
	size_t end;

	cur_token = *index;

	end = cur_token;
	if (end >= length)
		return SMTP_ERROR_CONTINUE;
	if (end >= length)
		return SMTP_ERROR_PARSE;

	while (is_ftext (message[end])) {
		end++;
		if (end >= length)
			break;
	}
	if (end == cur_token) {
		return SMTP_ERROR_PARSE;
	}

	field_name = malloc (end - cur_token + 1);
	if (field_name == NULL) {
		return SMTP_ERROR_MEMORY;
	}
	DEBUG_SMTP (SMTP_MEM, "smtp_field_name_parse: MALLOC pointer=%p\n",
		    field_name);

	strncpy (field_name, message + cur_token, end - cur_token);
	field_name[end - cur_token] = '\0';

	cur_token = end;

	*index = cur_token;
	*result = field_name;

	return SMTP_NO_ERROR;
}


void
smtp_mime_content_type_single_fields_init (struct smtp_mime_single_fields
					   *single_fields,
					   struct smtp_mime_content_type
					   *fld_content_type)
{
	clistiter *cur;

	single_fields->fld_content_type = fld_content_type;

	for (cur = clist_begin (fld_content_type->ct_parameters);
	     cur != NULL; cur = clist_next (cur)) {
		struct smtp_mime_parameter *param;

		param = clist_content (cur);

		if (strcasecmp (param->pa_name, "boundary") == 0)
			single_fields->fld_content_boundary = param->pa_value;

		if (strcasecmp (param->pa_name, "charset") == 0)
			single_fields->fld_content_charset = param->pa_value;

		if (strcasecmp (param->pa_name, "name") == 0)
			single_fields->fld_content_name = param->pa_value;

		if (strcasecmp (param->pa_name, "format") == 0)
			single_fields->fld_content_name = param->pa_value;
	}
}

void
smtp_mime_single_fields_init (struct smtp_mime_single_fields *single_fields,
			      struct smtp_mime_fields *fld_fields,
			      struct smtp_mime_content_type *fld_content_type)
{
	clistiter *cur;

	memset (single_fields, 0, sizeof (struct smtp_mime_single_fields));

	if (fld_content_type != NULL)
		smtp_mime_content_type_single_fields_init (single_fields,
							   fld_content_type);

	if (fld_fields == NULL)
		return;

	for (cur = clist_begin (fld_fields->fld_list); cur != NULL;
	     cur = clist_next (cur)) {
		struct smtp_mime_field *field;

		field = clist_content (cur);

		switch (field->fld_type) {
		case SMTP_MIME_FIELD_TYPE:
			smtp_mime_content_type_single_fields_init
				(single_fields,
				 field->fld_data.fld_content_type);
			break;

		case SMTP_MIME_FIELD_TRANSFER_ENCODING:
			single_fields->fld_encoding =
				field->fld_data.fld_encoding;
			break;

		case SMTP_MIME_FIELD_ID:
			single_fields->fld_id = field->fld_data.fld_id;
			break;

		case SMTP_MIME_FIELD_DESCRIPTION:
			single_fields->fld_description =
				field->fld_data.fld_description;
			break;

		case SMTP_MIME_FIELD_VERSION:
			single_fields->fld_version =
				field->fld_data.fld_version;
			break;

		case SMTP_MIME_FIELD_DISPOSITION:
			smtp_mime_disposition_single_fields_init (single_fields,
				 field->fld_data.fld_disposition);
			break;

		case SMTP_MIME_FIELD_LANGUAGE:
			single_fields->fld_language =
				field->fld_data.fld_language;
			break;
		}
	}
}

struct smtp_mime_single_fields *
smtp_mime_single_fields_new (struct smtp_mime_fields *fld_fields,
			     struct smtp_mime_content_type *fld_content_type)
{
	struct smtp_mime_single_fields *single_fields;

	single_fields = malloc (sizeof (struct smtp_mime_single_fields));
	if (single_fields == NULL)
		goto err;
	DEBUG_SMTP (SMTP_MEM,
		    "smtp_mime_single_fields_new: MALLOC pointer=%p\n",
		    single_fields);

	smtp_mime_single_fields_init (single_fields, fld_fields,
				      fld_content_type);

	return single_fields;

      err:
	return NULL;
}


void
smtp_mime_single_fields_free (struct smtp_mime_single_fields *single_fields)
{
	free (single_fields);
	DEBUG_SMTP (SMTP_MEM,
		    "smtp_mime_single_fields_free: FREE pointer=%p\n",
		    single_fields);
}

struct smtp_mime_fields *
smtp_mime_fields_new_filename (int dsp_type,
			       char *filename, int encoding_type)
{
	struct smtp_mime_disposition *dsp;
	struct smtp_mime_mechanism *encoding;
	struct smtp_mime_fields *mime_fields;

	dsp = smtp_mime_disposition_new_with_data (dsp_type,
						   filename, NULL, NULL, NULL,
						   (size_t) - 1);
	if (dsp == NULL)
		goto err;

	encoding = smtp_mime_mechanism_new (encoding_type, NULL);
	if (encoding == NULL)
		goto free_dsp;

	mime_fields = smtp_mime_fields_new_with_data (encoding,
						      NULL, NULL, dsp, NULL);
	if (mime_fields == NULL)
		goto free_encoding;

	return mime_fields;

      free_encoding:
	smtp_mime_encoding_free (encoding);
      free_dsp:
	smtp_mime_disposition_free (dsp);
      err:
	return NULL;
}

/* create MIME fields with only the field Content-Transfer-Encoding */

struct smtp_mime_fields *
smtp_mime_fields_new_encoding (int type)
{
	struct smtp_mime_mechanism *encoding;
	struct smtp_mime_fields *mime_fields;

	encoding = smtp_mime_mechanism_new (type, NULL);
	if (encoding == NULL)
		goto err;

	mime_fields = smtp_mime_fields_new_with_data (encoding,
						      NULL, NULL, NULL, NULL);
	if (mime_fields == NULL)
		goto free;

	return mime_fields;

      free:
	smtp_mime_mechanism_free (encoding);
      err:
	return NULL;
}

struct smtp_mime_fields *
smtp_mime_fields_new_empty (void)
{
	clist *list;
	struct smtp_mime_fields *fields;

	list = clist_new ();
	if (list == NULL)
		goto err;

	fields = smtp_mime_fields_new (list);
	if (fields == NULL)
		goto free;

	return fields;

      free:
	clist_free (list);
      err:
	return NULL;
}

int
smtp_mime_fields_add (struct smtp_mime_fields *fields,
		      struct smtp_mime_field *field)
{
	int r;

	r = clist_append (fields->fld_list, field);
	if (r < 0)
		return SMTP_ERROR_MEMORY;

	return SMTP_NO_ERROR;
}

void
smtp_mime_field_detach (struct smtp_mime_field *field)
{
	switch (field->fld_type) {
	case SMTP_MIME_FIELD_TYPE:
		field->fld_data.fld_content_type = NULL;
		break;
	case SMTP_MIME_FIELD_TRANSFER_ENCODING:
		field->fld_data.fld_encoding = NULL;
		break;
	case SMTP_MIME_FIELD_ID:
		field->fld_data.fld_id = NULL;
		break;
	case SMTP_MIME_FIELD_DESCRIPTION:
		field->fld_data.fld_description = NULL;
		break;
	case SMTP_MIME_FIELD_DISPOSITION:
		field->fld_data.fld_disposition = NULL;
		break;
	case SMTP_MIME_FIELD_LANGUAGE:
		field->fld_data.fld_language = NULL;
		break;
	}
}

struct smtp_mime_fields *
smtp_mime_fields_new_with_data (struct smtp_mime_mechanism *encoding,
				char *id,
				char *description,
				struct smtp_mime_disposition *disposition,
				struct smtp_mime_language *language)
{
	struct smtp_mime_field *field;
	struct smtp_mime_fields *fields;
	int r;

	fields = smtp_mime_fields_new_empty ();
	if (fields == NULL)
		goto err;

	if (encoding != NULL) {
		field = smtp_mime_field_new
			(SMTP_MIME_FIELD_TRANSFER_ENCODING, encoding);
		if (field == NULL)
			goto free;

		r = smtp_mime_fields_add (fields, field);
		if (r != SMTP_NO_ERROR) {
			smtp_mime_field_detach (field);
			smtp_mime_field_free (field);
			goto free;
		}
	}

	if (id != NULL) {
		field = smtp_mime_field_new (SMTP_MIME_FIELD_ID, id);
		if (field == NULL)
			goto free;

		r = smtp_mime_fields_add (fields, field);
		if (r != SMTP_NO_ERROR) {
			smtp_mime_field_detach (field);
			smtp_mime_field_free (field);
			goto free;
		}
	}

	if (description != NULL) {
		field = smtp_mime_field_new (SMTP_MIME_FIELD_DESCRIPTION,
					     description);
		if (field == NULL)
			goto free;

		r = smtp_mime_fields_add (fields, field);
		if (r != SMTP_NO_ERROR) {
			smtp_mime_field_detach (field);
			smtp_mime_field_free (field);
			goto free;
		}
	}

	if (disposition != NULL) {
		field = smtp_mime_field_new (SMTP_MIME_FIELD_DISPOSITION,
					     disposition);
		if (field == NULL)
			goto free;

		r = smtp_mime_fields_add (fields, field);
		if (r != SMTP_NO_ERROR) {
			smtp_mime_field_detach (field);
			smtp_mime_field_free (field);
			goto free;
		}
	}

	if (language != NULL) {
		field = smtp_mime_field_new (SMTP_MIME_FIELD_LANGUAGE,
					     language);
		if (field == NULL)
			goto free;

		r = smtp_mime_fields_add (fields, field);
		if (r != SMTP_NO_ERROR) {
			smtp_mime_field_detach (field);
			smtp_mime_field_free (field);
			goto free;
		}
	}

	return fields;

      free:
	clist_foreach (fields->fld_list, (clist_func) smtp_mime_field_detach,
		       NULL);
	smtp_mime_fields_free (fields);
      err:
	return NULL;
}

struct smtp_mime_fields *
smtp_mime_fields_new_with_version (struct smtp_mime_mechanism *encoding,
				   char *id,
				   char *description,
				   struct smtp_mime_disposition *disposition,
				   struct smtp_mime_language *language)
{
	struct smtp_mime_field *field;
	struct smtp_mime_fields *fields;
	int r;

	fields = smtp_mime_fields_new_with_data (encoding, id, description,
						 disposition, language);
	if (fields == NULL)
		goto err;

	field = smtp_mime_field_new (SMTP_MIME_FIELD_VERSION, (void *)MIME_VERSION);
	if (field == NULL)
		goto free;

	r = smtp_mime_fields_add (fields, field);
	if (r != SMTP_NO_ERROR) {
		smtp_mime_field_detach (field);
		smtp_mime_field_free (field);
		goto free;
	}

	return fields;

      free:
	clist_foreach (fields->fld_list, (clist_func) smtp_mime_field_detach,
		       NULL);
	smtp_mime_fields_free (fields);
      err:
	return NULL;
}

struct smtp_optional_field *
smtp_optional_field_new (char *fld_name, char *fld_value)
{
	struct smtp_optional_field *opt_field;

	opt_field = malloc (sizeof (*opt_field));
	if (opt_field == NULL)
		return NULL;
	DEBUG_SMTP (SMTP_MEM,
		    "smtp_optional_field_new: MALLOC pointer=%p\n",
		    opt_field);

	opt_field->fld_name = fld_name;
	opt_field->fld_value = fld_value;

	return opt_field;
}

void
smtp_optional_field_free (struct smtp_optional_field *opt_field)
{
	smtp_field_name_free (opt_field->fld_name);
	smtp_unstructured_free (opt_field->fld_value);
	free (opt_field);
	DEBUG_SMTP (SMTP_MEM, "smtp_optional_field_free: FREE pointer=%p\n",
		    opt_field);
}

void
smtp_field_name_free (char *field_name)
{
	free (field_name);
	DEBUG_SMTP (SMTP_MEM, "smtp_field_name_free: FREE pointer=%p\n",
		    field_name);
}

time_t mkgmtime (struct tm *tmp);


struct smtp_date_time *
smtp_get_current_date (void)
{
	struct tm gmt;
	struct tm lt;
	int off;
	time_t now;
	struct smtp_date_time *date_time;

	now = time (NULL);

	if (gmtime_r (&now, &gmt) == NULL)
		return NULL;

	if (localtime_r (&now, &lt) == NULL)
		return NULL;

	off = (mkgmtime (&lt) - mkgmtime (&gmt)) / (60 * 60) * 100;

	date_time =
		smtp_date_time_new (lt.tm_mday, lt.tm_mon + 1,
				    lt.tm_year + 1900, lt.tm_hour, lt.tm_min,
				    lt.tm_sec, off);

	return date_time;
}

#ifndef WRONG
#define WRONG	(-1)
#endif 

int
tmcomp (struct tm *atmp, struct tm *btmp)
{
	register int result;

	if ((result = (atmp->tm_year - btmp->tm_year)) == 0 &&
	    (result = (atmp->tm_mon - btmp->tm_mon)) == 0 &&
	    (result = (atmp->tm_mday - btmp->tm_mday)) == 0 &&
	    (result = (atmp->tm_hour - btmp->tm_hour)) == 0 &&
	    (result = (atmp->tm_min - btmp->tm_min)) == 0)
		result = atmp->tm_sec - btmp->tm_sec;
	return result;
}

time_t
mkgmtime (struct tm * tmp)
{
	register int dir;
	register int bits;
	register int saved_seconds;
	time_t t;
	struct tm yourtm, *mytm;

	yourtm = *tmp;
	saved_seconds = yourtm.tm_sec;
	yourtm.tm_sec = 0;
	/*
	 ** Calculate the number of magnitude bits in a time_t
	 ** (this works regardless of whether time_t is
	 ** signed or unsigned, though lint complains if unsigned).
	 */
	for (bits = 0, t = 1; t > 0; ++bits, t <<= 1);
	/*
	 ** If time_t is signed, then 0 is the median value,
	 ** if time_t is unsigned, then 1 << bits is median.
	 */
	t = (t < 0) ? 0 : ((time_t) 1 << bits);
	for (;;) {
		mytm = gmtime (&t);
		dir = tmcomp (mytm, &yourtm);
		if (dir != 0) {
			if (bits-- < 0)
				return WRONG;
			if (bits < 0)
				--t;
			else if (dir > 0)
				t -= (time_t) 1 << bits;
			else
				t += (time_t) 1 << bits;
			continue;
		}
		break;
	}
	t += saved_seconds;
	return t;
}


#if 1
struct smtp_mime_field *
smtp_mime_field_new (int fld_type, void *value)
{
	struct smtp_mime_field *field;

	field = malloc (sizeof (*field));
	if (field == NULL)
		return NULL;
	DEBUG_SMTP (SMTP_MEM, "smtp_mime_field_new: MALLOC pointer=%p\n",
		    field);

#ifdef USE_NEL
	NEL_REF_INIT (field);
#endif

	field->fld_type = fld_type;

	switch (fld_type) {

	case SMTP_FIELD_FROM:
		field->fld_data.fld_from = (struct smtp_msg_from *) value;
		break;
	case SMTP_FIELD_TO:
		field->fld_data.fld_to = (struct smtp_msg_to *) value;
		break;

	case SMTP_MIME_FIELD_TYPE:
		field->fld_data.fld_content_type =
			(struct smtp_mime_content_type *) value;
		break;
	case SMTP_MIME_FIELD_TRANSFER_ENCODING:
		field->fld_data.fld_encoding =
			(struct smtp_mime_mechanism *) value;
		break;
	case SMTP_MIME_FIELD_ID:
		field->fld_data.fld_id = (char *) value;
		break;
	case SMTP_MIME_FIELD_DESCRIPTION:
		field->fld_data.fld_description = (char *) value;
		break;
	case SMTP_MIME_FIELD_VERSION:
		field->fld_data.fld_version = ( unsigned long ) value;
		break;
	case SMTP_MIME_FIELD_DISPOSITION:
		field->fld_data.fld_disposition =
			(struct smtp_mime_disposition *) value;
		break;
	case SMTP_MIME_FIELD_LANGUAGE:
		field->fld_data.fld_language =
			(struct smtp_mime_language *) value;
		break;
	}
	return field;
}

#else
struct smtp_mime_field *
smtp_mime_field_new (int fld_type,
		     struct smtp_mime_content_type *fld_content_type,
		     struct smtp_mime_mechanism *fld_encoding,
		     char *fld_id,
		     char *fld_description,
		     uint32_t fld_version,
		     struct smtp_mime_disposition *fld_disposition,
		     struct smtp_mime_language *fld_language)
{
	struct smtp_mime_field *field;

	field = malloc (sizeof (*field));
	if (field == NULL)
		return NULL;
	field->fld_type = fld_type;

	switch (fld_type) {
	case SMTP_MIME_FIELD_TYPE:
		field->fld_data.fld_content_type = fld_content_type;
		break;
	case SMTP_MIME_FIELD_TRANSFER_ENCODING:
		field->fld_data.fld_encoding = fld_encoding;
		break;
	case SMTP_MIME_FIELD_ID:
		field->fld_data.fld_id = fld_id;
		break;
	case SMTP_MIME_FIELD_DESCRIPTION:
		field->fld_data.fld_description = fld_description;
		break;
	case SMTP_MIME_FIELD_VERSION:
		field->fld_data.fld_version = fld_version;
		break;
	case SMTP_MIME_FIELD_DISPOSITION:
		field->fld_data.fld_disposition = fld_disposition;
		break;
	case SMTP_MIME_FIELD_LANGUAGE:
		field->fld_data.fld_language = fld_language;
		break;
	}
	return field;
}
#endif


void
smtp_mime_field_free (struct smtp_mime_field *field)
{
	DEBUG_SMTP (SMTP_DBG, "smtp_mime_field_free\n");
	switch (field->fld_type) {
	case SMTP_FIELD_FROM:
		if (field->fld_data.fld_from != NULL)
			smtp_msg_from_free (field->fld_data.fld_from);
		break;
	case SMTP_FIELD_TO:
		if (field->fld_data.fld_to != NULL)
			smtp_msg_to_free (field->fld_data.fld_to);
		break;

	case SMTP_MIME_FIELD_TYPE:
		if (field->fld_data.fld_content_type != NULL)
			smtp_mime_content_type_free (field->fld_data.
						     fld_content_type);
		break;
	case SMTP_MIME_FIELD_TRANSFER_ENCODING:
		if (field->fld_data.fld_encoding != NULL)
			smtp_mime_encoding_free (field->fld_data.
						 fld_encoding);
		break;
	case SMTP_MIME_FIELD_ID:
		if (field->fld_data.fld_id != NULL)
			smtp_mime_id_free (field->fld_data.fld_id);
		break;
	case SMTP_MIME_FIELD_DESCRIPTION:
		if (field->fld_data.fld_description != NULL)
			smtp_mime_description_free (field->fld_data.
						    fld_description);
		break;
	case SMTP_MIME_FIELD_DISPOSITION:
		if (field->fld_data.fld_disposition != NULL)
			smtp_mime_disposition_free (field->fld_data.
						    fld_disposition);
		break;
	case SMTP_MIME_FIELD_LANGUAGE:
		if (field->fld_data.fld_language != NULL)
			smtp_mime_language_free (field->fld_data.
						 fld_language);
		break;
	}

	free (field);
	DEBUG_SMTP (SMTP_MEM, "smtp_mime_field_free: FREE pointer=%p\n",
		    field);
}

struct smtp_mime_fields *
smtp_mime_fields_new (clist * fld_list)
{
	struct smtp_mime_fields *fields;

	fields = malloc (sizeof (*fields));
	if (fields == NULL)
		return NULL;
	DEBUG_SMTP (SMTP_MEM, "smtp_mime_fields_new: MALLOC pointer=%p\n",
		    fields);

#ifdef USE_NEL
	NEL_REF_INIT (fields);
#endif

	fields->fld_list = fld_list;

	return fields;
}

void
smtp_mime_fields_free (struct smtp_mime_fields *fields)
{
	clist_foreach (fields->fld_list, (clist_func) smtp_mime_field_free,
		       NULL);
	clist_free (fields->fld_list);
	free (fields);
	DEBUG_SMTP (SMTP_MEM, "smtp_mime_fields_free: FREE pointer=%p\n",
		    fields);
}

enum
{
	HEADER_START,
	HEADER_C,
	HEADER_R,
	HEADER_RE,
	HEADER_S,
	HEADER_RES,
};

int
guess_header_type (const char *message, size_t length, size_t index)
{
	int state;
	int r;

	state = HEADER_START;

	while (1) {

		if (index >= length)
			return SMTP_FIELD_NONE;

		switch (state) {
		case HEADER_START:
			switch ((char)
				toupper ((unsigned char) message[index])) {
			case 'B':
				return SMTP_FIELD_BCC;
			case 'C':
				state = HEADER_C;
				break;
			case 'D':
				return SMTP_FIELD_ORIG_DATE;
			case 'F':
				return SMTP_FIELD_FROM;
			case 'I':
				return SMTP_FIELD_IN_REPLY_TO;
			case 'K':
				return SMTP_FIELD_KEYWORDS;
			case 'M':
				return SMTP_FIELD_MESSAGE_ID;
			case 'R':
				state = HEADER_R;
				break;
			case 'T':
				return SMTP_FIELD_TO;
				break;
			case 'S':
				state = HEADER_S;
				break;
			default:
				return SMTP_FIELD_NONE;
			}
			break;
		case HEADER_C:
			switch ((char)
				toupper ((unsigned char) message[index])) {
			case 'O':
				return SMTP_FIELD_COMMENTS;
			case 'C':
				return SMTP_FIELD_CC;
			default:
				return SMTP_FIELD_NONE;
			}
			break;
		case HEADER_R:
			switch ((char)
				toupper ((unsigned char) message[index])) {
			case 'E':
				state = HEADER_RE;
				break;
			default:
				return SMTP_FIELD_NONE;
			}
			break;
		case HEADER_RE:
			switch ((char)
				toupper ((unsigned char) message[index])) {
			case 'F':
				return SMTP_FIELD_REFERENCES;
			case 'P':
				return SMTP_FIELD_REPLY_TO;
			case 'S':
				state = HEADER_RES;
				break;
			case 'T':
				return SMTP_FIELD_RETURN_PATH;
			default:
				return SMTP_FIELD_NONE;
			}
			break;
		case HEADER_S:
			switch ((char)
				toupper ((unsigned char) message[index])) {
			case 'E':
				return SMTP_FIELD_SENDER;
			case 'U':
				return SMTP_FIELD_SUBJECT;
			default:
				return SMTP_FIELD_NONE;
			}
			break;

		case HEADER_RES:
			r = smtp_token_case_insensitive_parse (message,
							       length, &index,
							       "ent-");
			if (r != SMTP_NO_ERROR)
				return SMTP_FIELD_NONE;

			if (index >= length)
				return SMTP_FIELD_NONE;

			switch ((char)
				toupper ((unsigned char) message[index])) {
			case 'D':
				return SMTP_FIELD_RESENT_DATE;
			case 'F':
				return SMTP_FIELD_RESENT_FROM;
			case 'S':
				return SMTP_FIELD_RESENT_SENDER;
			case 'T':
				return SMTP_FIELD_RESENT_TO;
			case 'C':
				return SMTP_FIELD_RESENT_CC;
			case 'B':
				return SMTP_FIELD_RESENT_BCC;
			case 'M':
				return SMTP_FIELD_RESENT_MSG_ID;
			default:
				return SMTP_FIELD_NONE;
			}
			break;
		}
		index++;
	}
}


/*
optional-field  =       field-name ":" unstructured CRLF
*/

int
smtp_optional_field_parse (struct smtp_info *psmtp, const char *message,
			   size_t length, size_t * index,
			   struct smtp_optional_field **result)
{
	char *name;
	char *value;
	struct smtp_optional_field *optional_field;
	size_t cur_token;
	int r;
	int res;

	cur_token = *index;

	r = smtp_field_name_parse (message, length, &cur_token, &name);
	if (r != SMTP_NO_ERROR) {
		res = r;
		goto err;
	}

	r = smtp_colon_parse (message, length, &cur_token);
	if (r != SMTP_NO_ERROR) {
		res = r;
		goto free_name;
	}

	r = smtp_unstructured_parse (message, length, &cur_token, &value);
	if (r != SMTP_NO_ERROR) {
		res = r;
		goto free_name;
	}

	r = smtp_unstrict_crlf_parse (message, length, &cur_token);
	if (r != SMTP_NO_ERROR) {
		res = r;
		goto free_value;
	}

	optional_field = smtp_optional_field_new (name, value);
	if (optional_field == NULL) {
		res = SMTP_ERROR_MEMORY;
		goto free_value;
	}

	*result = optional_field;
	*index = cur_token;

	return SMTP_NO_ERROR;

      free_value:
	smtp_unstructured_free (value);
      free_name:
	smtp_field_name_free (name);
      err:
	return res;
}



#define YYCURSOR  p1
#define YYCTYPE   char
#define YYLIMIT   p2
#define YYMARKER  p3
#define YYRESCAN  yy2
#define YYFILL(n)

/*!re2c
any     = [\000-\377];
print	= [\040-\176];
digit   = [0-9];
letter  = [a-zA-Z];
space	= [\040];
*/

int
smtp_envelope_field_parse (struct smtp_info *psmtp, char *message,
			   size_t length, size_t * index,
			   struct smtp_mime_field **result, int *fld_type)
{
	size_t cur_token;
	int type;
	struct smtp_msg_from *from;
	struct smtp_msg_to *to;

	/* MIME stuff */
	struct smtp_mime_content_type *content_type;
	struct smtp_mime_disposition *disposition;

	struct smtp_mime_field *field;
	int r, res;
	char *p, *p1, *p2, *p3;

	cur_token = *index;
	p1 = (char *) (message + cur_token);
	p2 = p1 + length;

	from = NULL;
	to = NULL;

	/* MIME stuff */
	content_type = NULL;
	disposition = NULL;

	/* *INDENT-OFF* */
	/*!re2c
	"\r\n"  
	{ 
		DEBUG_SMTP(SMTP_DBG, "smtp_envelope_field_parse: CRLF\n");
		cur_token = ( p1 - message); 

#ifdef USE_NEL
		if (( r = nel_env_analysis(eng, &(psmtp->env), eoh_id,  
				(struct smtp_simple_event *)0  )) < 0 ) {
			DEBUG_SMTP(SMTP_DBG, "smtp_from_parse: nel_env_analysis error\n");
			res = SMTP_ERROR_ENGINE;
			goto err;
		}
#endif
		* fld_type = SMTP_FIELD_EOH;
		goto succ;
	}

	[Ff][Rr][Oo][Mm][:]
	{ 
		DEBUG_SMTP(SMTP_DBG, "smtp_envelope_field_parse: From:\n");
		cur_token = ( p1 - message); 
		r = smtp_msg_from_parse(psmtp, message, length, &cur_token,&from);
		if (r == SMTP_NO_ERROR) {
			* fld_type = SMTP_FIELD_FROM;
			goto succ;
		}
		else {
			res = r;
			goto err;
		}
	}

	[Tt][Oo][:]
	{ 
		DEBUG_SMTP(SMTP_DBG, "smtp_envelope_field_parse: To:\n");
		cur_token = ( p1 - message); 
		r = smtp_msg_to_parse(psmtp, message, length, &cur_token, &to);
		if (r == SMTP_NO_ERROR) {
			* fld_type = SMTP_FIELD_TO;
			goto succ;
		}
		else {
			res = r;
			goto err;
		}

	}

	[Cc][Oo][Nn][Tt][Ee][Nn][Tt][-][Tt][Yy][Pp][Ee][:]
	{
		DEBUG_SMTP(SMTP_DBG, "smtp_envelope_field_parse: Content-Type:\n");
		cur_token = ( p1 - message); 

		r = smtp_mime_content_type_parse(psmtp, message, length, &cur_token, &content_type);
		if (r == SMTP_NO_ERROR){
			DEBUG_SMTP(SMTP_DBG, "content=%p\n", content_type);
			* fld_type = SMTP_MIME_FIELD_TYPE;
			goto succ;
		}
		else {
			res = r;
			goto err;
		}
	} 

	[Cc][Oo][Nn][Tt][Ee][Nn][Tt][-][Tt][Rr][Aa][Nn][Ss][Ff][Ee][Rr][-][Ee][Nn][Cc][Oo][Dd][Ii][Nn][Gg][:]
	{ 
		int encode_type;
		DEBUG_SMTP(SMTP_DBG, "smtp_envelope_field_parse: Content-Transfer-Encoding:\n");
		cur_token = ( p1 - message); 

		r = smtp_mime_encoding_parse(&psmtp->part_stack[psmtp->part_stack_top], message, length, &cur_token, &encode_type);
		if (r == SMTP_NO_ERROR){
			* fld_type = SMTP_MIME_FIELD_TRANSFER_ENCODING;
			goto succ;
		}
		else {
			res = r;
			goto err;
		}
	}

	[Cc][Oo][Nn][Tt][Ee][Nn][Tt][-][Dd][Ii][Ss][Pp][Oo][Ss][Ii][Tt][Ii][Oo][Nn][:]
	{ 
		struct smtp_mime_disposition *disposition;
		DEBUG_SMTP(SMTP_DBG, "smtp_envelope_field_parse: Content-Disposition:\n");
		cur_token = ( p1 - message); 

		r = smtp_mime_disposition_parse(psmtp, message, length, &cur_token, &disposition);

		if (r == SMTP_NO_ERROR){
			* fld_type = SMTP_MIME_FIELD_DISPOSITION;
			goto succ;
		}
		else {
			DEBUG_SMTP(SMTP_DBG, "\n");
			res = r;
			goto err;
		}
	}

	".\r\n"
	{
		//dealing with telnet end with CRLF.CRLF
		DEBUG_SMTP(SMTP_DBG, "\n");
		res = SMTP_ERROR_PARSE;
		goto err;
	}

	any
	{
		DEBUG_SMTP(SMTP_DBG, "ANY cur_token = %lu, length = %lu\n", cur_token, length);
		DEBUG_SMTP(SMTP_DBG, "message+cur_token: %s\n", message+cur_token);

		r = smtp_str_crlf_parse (message, length, &cur_token);
		DEBUG_SMTP(SMTP_DBG, "message+cur_token: %s\n", message+cur_token);
		if (r != SMTP_NO_ERROR) {
			/* when meet unknown keywords and didn't find it's CRLF,
			   we temperaly skip those data to avoid system crashing down */
			DEBUG_SMTP(SMTP_DBG, "\n");
			res = r;
			goto err;

		} else {
			DEBUG_SMTP(SMTP_DBG, "\n");
			*fld_type = SMTP_FIELD_NONE;
			*result = NULL;
			*index = cur_token;
			return SMTP_NO_ERROR;
		}

	}
	*/
	/* *INDENT-ON* */

      succ:

	DEBUG_SMTP (SMTP_DBG, "\n");
	DEBUG_SMTP (SMTP_DBG, "cur_token = %lu\n", cur_token);
	*result = field;
	*index = cur_token;
	return SMTP_NO_ERROR;

      err:
	DEBUG_SMTP (SMTP_DBG, "\n");
	return res;

}



int
smtp_envelope_or_optional_field_parse (struct smtp_info *psmtp,
				       char *message, 
					size_t length,
				       size_t * index,
				       struct smtp_mime_field **result,
				       int *fld_type)
{
	int r;
	size_t cur_token;
	struct smtp_optional_field *optional_field;
	struct smtp_mime_field *field;

	DEBUG_SMTP (SMTP_DBG, "index=%p, *index=%lu\n", index, *index);

	cur_token = *index;
	r = smtp_envelope_field_parse ( psmtp, message, length,
				       &cur_token, result, fld_type);
	if (r == SMTP_NO_ERROR) {
		DEBUG_SMTP (SMTP_DBG, "\n");
		*index = cur_token;
		return r;
	}
	else {
		DEBUG_SMTP (SMTP_DBG, "\n");
		return r;
	}

	r = smtp_optional_field_parse (psmtp, message, length, &cur_token,
				       &optional_field);
	DEBUG_SMTP (SMTP_DBG, "\n");
	if (r != SMTP_NO_ERROR)
		return r;

	DEBUG_SMTP (SMTP_DBG, "\n");
	field = smtp_mime_field_new (SMTP_FIELD_OPTIONAL_FIELD,
				     optional_field);
	DEBUG_SMTP (SMTP_DBG, "\n");
	if (field == NULL) {
		DEBUG_SMTP (SMTP_DBG, "\n");
		smtp_optional_field_free (optional_field);
		return SMTP_ERROR_MEMORY;
	}

	DEBUG_SMTP (SMTP_DBG, "\n");
	*result = field;
	*index = cur_token;
	*fld_type = SMTP_FIELD_OPTIONAL_FIELD;

	return SMTP_NO_ERROR;
}

/*
x  entity-headers := [ content CRLF ]
                    [ encoding CRLF ]
                    [ id CRLF ]
                    [ description CRLF ]
                    *( MIME-extension-field CRLF )
		    */

enum
{
	FIELD_STATE_START,
	FIELD_STATE_T,
	FIELD_STATE_D
};

int
guess_field_type (char *name)
{
	int state;

	if (*name == 'M')
		return SMTP_MIME_FIELD_VERSION;

	if (strncasecmp (name, "Content-", 8) != 0)
		return SMTP_MIME_FIELD_NONE;

	name += 8;

	state = FIELD_STATE_START;

	while (1) {

		switch (state) {

		case FIELD_STATE_START:
			switch ((char) toupper ((unsigned char) *name)) {
			case 'T':
				state = FIELD_STATE_T;
				break;
			case 'I':
				return SMTP_MIME_FIELD_ID;
			case 'D':
				state = FIELD_STATE_D;
				break;
			case 'L':
				return SMTP_MIME_FIELD_LANGUAGE;
			default:
				return SMTP_MIME_FIELD_NONE;
			}
			break;

		case FIELD_STATE_T:
			switch ((char) toupper ((unsigned char) *name)) {
			case 'Y':
				return SMTP_MIME_FIELD_TYPE;
			case 'R':
				return SMTP_MIME_FIELD_TRANSFER_ENCODING;
			default:
				return SMTP_MIME_FIELD_NONE;
			}
			break;

		case FIELD_STATE_D:
			switch ((char) toupper ((unsigned char) *name)) {
			case 'E':
				return SMTP_MIME_FIELD_DESCRIPTION;
			case 'I':
				return SMTP_MIME_FIELD_DISPOSITION;
			default:
				return SMTP_MIME_FIELD_NONE;
			}
			break;
		}
		name++;
	}
}
