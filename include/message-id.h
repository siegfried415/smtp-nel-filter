#ifndef MESSAGE_ID_H
#define MESSAGE_ID_H

/*
  smtp_message_id is the parsed Message-ID field
  - mid_value is the message identifier
*/

struct smtp_message_id
{
#ifdef USE_NEL
	NEL_REF_DEF
#endif
	char *mid_value;	/* != NULL */
	int err_cnt;
};

struct smtp_message_id *smtp_message_id_new (char *mid_value);
void smtp_message_id_free (struct smtp_message_id *message_id);

/*
message-id      =       "Message-ID:" msg-id CRLF
*/

int smtp_message_id_parse (struct smtp_info *psmtp, const char *message,
			   size_t length, size_t * index,
			   struct smtp_message_id **result);

/*
  this function returns an allocated message identifier to
  use in a Message-ID or Resent-Message-ID field
*/
char *smtp_get_message_id (void);


#endif
