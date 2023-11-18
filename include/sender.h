#ifndef SENDER_H
#define SENDER_H

/*
  smtp_sender is the parsed Sender field
  - snd_mb is the parsed mailbox
*/
struct smtp_sender
{
	struct smtp_mailbox *snd_mb;	/* != NULL */
};

struct smtp_sender *smtp_sender_new (struct smtp_mailbox *snd_mb);
void smtp_sender_free (struct smtp_sender *sender);

int smtp_sender_parse (const char *message, size_t length,
		       size_t * index, struct smtp_sender **result);


#endif
