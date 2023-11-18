#ifndef RESENT_CC_H
#define RESENT_CC_H

#include "cc.h"

/*
resent-cc       =       "Resent-Cc:" address-list CRLF
*/

int
smtp_resent_cc_parse (const char *message, size_t length,
		      size_t * index, struct smtp_cc **result);


#endif
