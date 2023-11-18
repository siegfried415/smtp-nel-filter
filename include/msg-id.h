#ifndef MSG_ID_H
#define MSG_ID_H


void smtp_mime_id_free (char *id);

void smtp_msg_id_free (char *msg_id);

/*
msg-id          =       [CFWS] "<" id-left "@" id-right ">" [CFWS]
*/

int smtp_msg_id_parse (const char *message, size_t length,
		       size_t * index, char **result);

int smtp_parse_unwanted_msg_id (const char *message, size_t length,
				size_t * index);

int smtp_unstrict_msg_id_parse (const char *message, size_t length,
				size_t * index, char **result);

void smtp_id_left_free (char *id_left);

void smtp_id_right_free (char *id_right);

#endif
