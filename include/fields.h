#ifndef SMTP_HERDERS_H
#define SMTP_HERDERS_H


#ifdef __cplusplus
extern "C"
{
#endif

#include <inttypes.h>
#include <sys/types.h>
#include "clist.h"

#include "content-type.h"
#include "time.h"
#include "disposition.h"
#include "from.h"
#include "to.h"

enum smtp_mime_field_type
{

	SMTP_FIELD_NONE = 1,	/* on parse error */
	SMTP_FIELD_RETURN_PATH,	/* Return-Path */
	SMTP_FIELD_RESENT_DATE,	/* Resent-Date */
	SMTP_FIELD_RESENT_FROM,	/* Resent-From */
	SMTP_FIELD_RESENT_SENDER,	/* Resent-Sender */
	SMTP_FIELD_RESENT_TO,	/* Resent-To */
	SMTP_FIELD_RESENT_CC,	/* Resent-Cc */
	SMTP_FIELD_RESENT_BCC,	/* Resent-Bcc */
	SMTP_FIELD_RESENT_MSG_ID,	/* Resent-Message-ID */
	SMTP_FIELD_ORIG_DATE,	/* Date */
	SMTP_FIELD_FROM,	/* From */
	SMTP_FIELD_SENDER,	/* Sender */
	SMTP_FIELD_REPLY_TO,	/* Reply-To */
	SMTP_FIELD_TO,	/* To */
	SMTP_FIELD_CC,	/* Cc */
	SMTP_FIELD_BCC,	/* Bcc */
	SMTP_FIELD_MESSAGE_ID,	/* Message-ID */
	SMTP_FIELD_IN_REPLY_TO,	/* In-Reply-To */
	SMTP_FIELD_REFERENCES,	/* References */
	SMTP_FIELD_SUBJECT,	/* Subject */
	SMTP_FIELD_COMMENTS,	/* Comments */
	SMTP_FIELD_KEYWORDS,	/* Keywords */
	SMTP_FIELD_OPTIONAL_FIELD,	/* other field */
	SMTP_FIELD_RECEIVED,	/* Received */
	SMTP_FIELD_SOLICITATION,
	SMTP_FIELD_RESENT_REPLY_TO,
	SMTP_FIELD_ENCRYPTED,

	SMTP_MIME_FIELD_NONE,
	SMTP_MIME_FIELD_TYPE,
	SMTP_MIME_FIELD_TRANSFER_ENCODING,
	SMTP_MIME_FIELD_ID,
	SMTP_MIME_FIELD_DESCRIPTION,
	SMTP_MIME_FIELD_VERSION,
	SMTP_MIME_FIELD_DISPOSITION,
	SMTP_MIME_FIELD_LANGUAGE,
	SMTP_MIME_FIELD_CONVERSION,
	SMTP_MIME_FIELD_FEATURES,
	SMTP_MIME_FIELD_BASE,
	SMTP_MIME_FIELD_LOCATION,
	SMTP_MIME_FIELD_DURATION,

	SMTP_FIELD_EOH	/* end of headers */
};

struct smtp_mime_field
{
#ifdef USE_NEL
	NEL_REF_DEF
#endif
	int fld_type;
	union
	{
		struct smtp_msg_from *fld_from;
		struct smtp_msg_to *fld_to;
		struct smtp_mime_content_type *fld_content_type;
		struct smtp_mime_mechanism *fld_encoding;
		char *fld_id;
		char *fld_description;
		unsigned long fld_version;
		struct smtp_mime_disposition *fld_disposition;
		struct smtp_mime_language *fld_language;
	} fld_data;
};

struct smtp_mime_field *
smtp_mime_field_new (int fld_type, void *value);

void 
smtp_mime_field_free (struct smtp_mime_field *field);

struct smtp_mime_fields
{
#ifdef USE_NEL
	NEL_REF_DEF
#endif
	clist * fld_list;	/* list of (struct smtp_mime_field *) */
	int body_type;
};

struct smtp_mime_fields *
smtp_mime_fields_new (clist * fld_list);

void 
smtp_mime_fields_free (struct smtp_mime_fields *fields);

struct smtp_optional_field
{
	char *fld_name;	
	char *fld_value;
};

struct smtp_optional_field *
smtp_optional_field_new (char *fld_name, char *fld_value);

void 
smtp_optional_field_free (struct smtp_optional_field *opt_field);

struct smtp_mime_single_fields
{
	struct smtp_mime_content_type *fld_content_type;
	char *fld_content_charset;
	char *fld_content_boundary;
	char *fld_content_name;
	struct smtp_mime_mechanism *fld_encoding;
	char *fld_id;
	char *fld_description;
	uint32_t fld_version;
	struct smtp_mime_disposition *fld_disposition;
	char *fld_disposition_filename;
	char *fld_disposition_creation_date;
	char *fld_disposition_modification_date;
	char *fld_disposition_read_date;
	size_t fld_disposition_size;
	struct smtp_mime_language *fld_language;
};

void 
smtp_mime_single_fields_init (struct smtp_mime_single_fields
				   *single_fields,
				   struct smtp_mime_fields
				   *fld_fields,
				   struct smtp_mime_content_type
				   *fld_content_type);

struct smtp_mime_single_fields *
smtp_mime_single_fields_new (struct smtp_mime_fields *fld_fields,
			     struct smtp_mime_content_type *fld_content_type);

void 
smtp_mime_single_fields_free (struct smtp_mime_single_fields
				   *single_fields);

struct smtp_mime_fields * 
smtp_mime_fields_new_empty (void);

int 
smtp_mime_fields_add (struct smtp_mime_fields *fields,
			  struct smtp_mime_field *field);

struct smtp_mime_fields *
smtp_mime_fields_new_with_data (struct smtp_mime_mechanism *encoding,
				char *id,
				char *description,
				struct smtp_mime_disposition *disposition,
				struct smtp_mime_language *language);

struct smtp_mime_fields *
smtp_mime_fields_new_with_version (struct smtp_mime_mechanism *encoding,
				char *id,
				char *description,
				struct smtp_mime_disposition *disposition,
				struct smtp_mime_language *language);

int 
smtp_field_name_parse (const char *message, size_t length,
			   size_t * index, char **result);

int 
smtp_optional_field_parse (struct smtp_info *psmtp,
			       const char *message, size_t length,
			       size_t * index,
			       struct smtp_optional_field **result);

int 
smtp_envelope_or_optional_field_parse (struct smtp_info *psmtp,
				   char *message,
				   size_t length,
				   size_t * index,
				   struct smtp_mime_field
				   **result, int *fld_type);

struct smtp_date_time *
smtp_get_current_date (void);

struct smtp_mime_fields *
smtp_mime_fields_new_filename (	int dsp_type,
				char *filename,
				int encoding_type);

struct smtp_mime_fields *
smtp_mime_fields_new_encoding (int type);

void smtp_field_name_free (char *field_name);

struct smtp_mime_content_type *
get_content_type_from_fields (struct smtp_mime_fields *fields);

int 
get_body_type_from_fields (struct smtp_mime_fields *fields);

#ifdef __cplusplus
}
#endif

#endif
