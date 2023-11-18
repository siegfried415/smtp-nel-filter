#ifndef DATA_H
#define DATA_H

#include "smtp.h"

struct smtp_cmd_data
{
#ifdef USE_NEL
	NEL_REF_DEF
#endif
	int len;
	/* no parameters */
};

void smtp_cmd_data_init (
#ifdef USE_NEL
				struct nel_eng *eng
#endif
	);

int smtp_ack_data_parse (struct smtp_info *psmtp);

void smtp_cmd_data_free (struct smtp_cmd_data *data);

struct smtp_cmd_data *smtp_cmd_data_new (int len);

int smtp_cmd_data_parse (struct smtp_info *psmtp,
			 char *message, size_t length, size_t * index);

#endif
