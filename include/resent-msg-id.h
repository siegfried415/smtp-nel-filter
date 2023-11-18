#ifndef RESENT_MSG_ID_H
#define RESENT_MSG_ID_H

#include "message-id.h"

/*
resent-msg-id   =       "Resent-Message-ID:" msg-id CRLF
*/

int
smtp_resent_msg_id_parse (const char *message, size_t length,
			  size_t * index, struct smtp_message_id **result);


#endif
