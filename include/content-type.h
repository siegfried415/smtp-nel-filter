#ifndef CONTENT_TYPE_H
#define CONTENT_TYPE_H

#include "clist.h"
#include "fields.h"

enum
{
	SMTP_MIME_TYPE_ERROR,
	SMTP_MIME_TYPE_DISCRETE_TYPE,
	SMTP_MIME_TYPE_COMPOSITE_TYPE
};

struct smtp_mime_type
{
	int tp_type;
	union
	{
		struct smtp_mime_discrete_type *tp_discrete_type;
		struct smtp_mime_composite_type *tp_composite_type;
	} tp_data;
};

enum
{
	SMTP_MIME_COMPOSITE_TYPE_ERROR,
	SMTP_MIME_COMPOSITE_TYPE_MESSAGE,
	SMTP_MIME_COMPOSITE_TYPE_MULTIPART,
	SMTP_MIME_COMPOSITE_TYPE_EXTENSION
};

struct smtp_mime_composite_type
{
	int ct_type;
	char *ct_token;
};

struct smtp_mime_composite_type *smtp_mime_composite_type_new (int ct_type,
							       char
							       *ct_token);

void smtp_mime_composite_type_free (struct smtp_mime_composite_type *ct);

enum
{
	SMTP_MIME_DISCRETE_TYPE_ERROR,
	SMTP_MIME_DISCRETE_TYPE_TEXT,
	SMTP_MIME_DISCRETE_TYPE_IMAGE,
	SMTP_MIME_DISCRETE_TYPE_AUDIO,
	SMTP_MIME_DISCRETE_TYPE_VIDEO,
	SMTP_MIME_DISCRETE_TYPE_APPLICATION,
	SMTP_MIME_DISCRETE_TYPE_EXTENSION
};

struct smtp_mime_discrete_type
{
	int dt_type;
	char *dt_extension;
};

struct smtp_mime_discrete_type *smtp_mime_discrete_type_new (int dt_type,
							     char
							     *dt_extension);

void smtp_mime_discrete_type_free (struct smtp_mime_discrete_type
				   *discrete_type);

struct smtp_mime_content_type
{
#ifdef USE_NEL
	NEL_REF_DEF
#endif
	struct smtp_mime_type *ct_type;
	char *ct_subtype;
	clist *ct_parameters;	/* elements are (struct smtp_mime_parameter *) */
	int err_cnt;
};

int smtp_mime_subtype_parse (const char *message, size_t length,
			     size_t * index, char **result);

int smtp_mime_content_type_parse (struct smtp_info *psmtp,
				  const char *message, size_t length,
				  size_t * index,
				  struct smtp_mime_content_type **result);


int smtp_mime_description_parse (const char *message, size_t length,
				 size_t * index, char **result);


struct smtp_mime_content_type *smtp_mime_get_content_type_of_message (void);

struct smtp_mime_content_type *smtp_mime_get_content_type_of_text (void);


void smtp_mime_content_type_free (struct smtp_mime_content_type
				  *content_type);

#endif
