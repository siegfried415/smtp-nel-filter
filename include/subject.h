#ifndef SUBJECT_H
#define SUBJECT_H


/*
  smtp_subject is the parsed Subject field
  - sbj_value is the value of the field
*/
struct smtp_subject
{
	char *sbj_value;	/* != NULL */
};

struct smtp_subject *smtp_subject_new (char *sbj_value);
void smtp_subject_free (struct smtp_subject *subject);

#endif
