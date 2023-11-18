#ifndef RECEIVED_H
#define RECEIVED_H

/*
  smtp_received is the parsed value of Received 
*/
struct smtp_received
{
	char *pt_server_spec;	/* can be NULL */
};

struct smtp_received *smtp_received_new (char *pt_server_spec);
void smtp_received_free (struct smtp_received *server);

int smtp_received_parse (const char *message, size_t length,
			 size_t * index, struct smtp_received **result);


#endif
