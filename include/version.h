#ifndef VERSION_H
#define VERSION_H

int smtp_mime_version_parse (const char *message, size_t length,
			     size_t * index, uint32_t * result);

#endif
