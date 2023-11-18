#ifndef HELP_H
#define HELP_H

struct smtp_cmd_help
{
#ifdef USE_NEL
	NEL_REF_DEF
#endif
	int len;
	/* no parameters */
};


#endif
