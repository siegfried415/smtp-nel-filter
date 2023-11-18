#ifndef RESENT_BCC_H
#define RESENT_BCC_H

#include "bcc.h"

/*
resent-bcc      =       "Resent-Bcc:" (address-list / [CFWS]) CRLF
*/

int
smtp_resent_bcc_parse (const char *message, size_t length,
		       size_t * index, struct smtp_bcc **result);


#endif
