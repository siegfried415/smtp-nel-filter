
#ifndef DISPOSITION_H
#define DISPOSITION_H

#include "clist.h"
#include "analysis.h"

enum
{
	SMTP_MIME_DISPOSITION_TYPE_ERROR,
	SMTP_MIME_DISPOSITION_TYPE_INLINE,
	SMTP_MIME_DISPOSITION_TYPE_ATTACHMENT,
	SMTP_MIME_DISPOSITION_TYPE_EXTENSION
};

struct smtp_mime_disposition_type
{
	int dsp_type;
	char *dsp_extension;
};

enum
{
	SMTP_MIME_DISPOSITION_PARM_FILENAME,
	SMTP_MIME_DISPOSITION_PARM_CREATION_DATE,
	SMTP_MIME_DISPOSITION_PARM_MODIFICATION_DATE,
	SMTP_MIME_DISPOSITION_PARM_READ_DATE,
	SMTP_MIME_DISPOSITION_PARM_SIZE,
	SMTP_MIME_DISPOSITION_PARM_PARAMETER
};

struct smtp_mime_disposition_parm
{
	int pa_type;
	union
	{
		char *pa_filename;
		char *pa_creation_date;
		char *pa_modification_date;
		char *pa_read_date;
		size_t pa_size;
		struct smtp_mime_parameter *pa_parameter;
	} pa_data;
};

struct smtp_mime_disposition
{
#ifdef USE_NEL
	NEL_REF_DEF
#endif
	struct smtp_mime_disposition_type *dsp_type;
	clist *dsp_parms;	/* struct smtp_mime_disposition_parm */
	int err_cnt;
};

int smtp_mime_disposition_parse (struct smtp_info *psmtp,
				 const char *message, size_t length,
				 size_t * index,
				 struct smtp_mime_disposition **result);

int
smtp_mime_disposition_type_parse (const char *message, size_t length,
				  size_t * index,
				  struct smtp_mime_disposition_type **result);

int 
smtp_mime_disposition_guess_type (const char *message, 
					size_t length,
				      	size_t index);


struct smtp_mime_disposition *
smtp_mime_disposition_new_with_data (int type,
					char *filename,
					char *creation_date,
					char *modification_date,
					char *read_date,
					size_t size);

struct smtp_mime_disposition *
smtp_mime_disposition_new_filename (int type, char *filename);
struct smtp_mime_single_fields; 
struct smtp_mime_disposition;

void
smtp_mime_disposition_single_fields_init (
			struct smtp_mime_single_fields *single_fields,
			struct smtp_mime_disposition *fld_disposition);

void
smtp_mime_disposition_free (struct smtp_mime_disposition *dsp);

void
smtp_mime_disposition_init (
#ifdef USE_NEL
                                   struct nel_eng *eng
#endif
        ) ;

#endif
