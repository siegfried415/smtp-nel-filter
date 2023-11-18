
/*
 * main.c
 * $Id: main.c,v 1.1.1.1 2005/02/19 12:57:06 wyong Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include <signal.h>
#include <getopt.h>

#include <syslog.h>
#include <pcap.h>
#include <errno.h>
#include <unistd.h>
#include <config.h>
#include <sys/types.h>
#include <pthread.h>


#include "main.h"
#include "tcp.h"
#include "smtp.h"

#ifdef USE_NEL
#include "engine.h"
#include "nlib.h"
#endif

#ifdef USE_NEL
struct nel_eng *eng;
int compile_level;
int optimize_level;
int debug_level;
int verbose_level;
#endif

char *tcpdump_filename;
char *filter_string;
char *device;

struct option long_options[] = {
	{"read_from", required_argument, 0, 'r'},
#ifdef USE_NEL
	{"compile_level", required_argument, 0, 'c'},
	{"optimize_level", required_argument, 0, 'o'},
	{"verbose_level", required_argument, 0, 'v'},
	{"debug_level", required_argument, 0, 'd'},
#endif
	{"interface", required_argument, 0, 'i'},
	{"filter", required_argument, 0, 'f'},
	{"help", no_argument, 0, 'h'},
	{"version", no_argument, 0, 'V'},
	{0, 0, 0, 0}
};


void
usage (char *prog)
{
#ifdef USE_NEL
	fprintf (stdout, "%s [options] nelfile\n", prog);
	fprintf (stdout, "\t-c compile-level: set compile level \n");
	fprintf (stdout, "\t-o optimize-level: set optimize level \n");
	fprintf (stdout, "\t-v verbose-level: set verbose level \n");
	fprintf (stdout, "\t-d debug-level: set debug level \n");
#else
	fprintf (stdout, "%s [options] \n", prog);
#endif
	fprintf (stdout, "\t-r read_from: read packets from this file \n");
	fprintf (stdout, "\t-f filter: set tcpdump filter \n");
	fprintf (stdout, "\t-i eth[x]: set network interface \n");
	fprintf (stdout,
		 "\t-V, --version: show the version of this system\n");
	fprintf (stdout,
		 "\t-h, --help: show the help message of this system\n");
	return;
}

void
version (void)
{
	fprintf (stdout, "%s %s %d.%d.%d\n", "NEU SOFT", "NetEye Engine", 1,
		 0, 0);
}

int
main (int argc, char **argv)
{
	int ret = -1;
	int c, option_index;

#ifdef USE_NEL
	if (!(eng = nel_eng_alloc ())) {
		printf ("NetEye Engine malloc failed ! \n");
		exit (-1);
	}
#endif

	while ((c = getopt_long (argc, argv,
#ifdef USE_NEL
				 "r:c:o:v:d:i:f:hV",
#else
				 "r:i:f:hV",
#endif
				 long_options, &option_index)) != EOF) {

		switch (c) {
#ifdef USE_NEL
		case 'c':
			compile_level = atoi (optarg);
			eng->compile_level = compile_level;
			break;
		case 'o':
			optimize_level = atoi (optarg);
			eng->optimize_level = optimize_level;
			break;
		case 'v':
			verbose_level = atoi (optarg);
			eng->verbose_level = verbose_level;
			break;
		case 'd':
			debug_level = atoi (optarg);
			eng->stab_debug_level = debug_level;
			eng->term_debug_level = debug_level;
			eng->nonterm_debug_level = debug_level;
			eng->prod_debug_level = debug_level;
			eng->itemset_debug_level = debug_level;
			eng->termclass_debug_level = debug_level;
			eng->nontermclass_debug_level = debug_level;
			eng->function_debug_level = debug_level;
			eng->declaration_debug_level = debug_level;
			break;
#endif
		case 'i':
			device = strdup ((const char *) optarg);
			break;

		case 'f':
			filter_string = strdup ((const char *) optarg);
			break;

		case 'r':
			tcpdump_filename = strdup ((const char *) optarg);
			break;

		case 'V':
			version ();
			exit (0);

		default:
			usage (argv[0]);
			exit (0);
		}
	}

	if (optind !=
#ifdef USE_NEL
	    (argc - 1)
#else
	    argc
#endif
		) {
		usage (argv[0]);
		exit (1);
	}

#define PRODUCTINFO     "\
	  ___________________________________________________________ \n\
	 |                                                           |\n\
	 |     smtp-nel-filter (Ver 1.0)                             |\n\
	 |                                                           |\n\
	 |     Copyright (C) NEU Co., Ltd. 2004-2009  		     |\n\
	 |                 All Rights Reserved.                      |\n\
	 |___________________________________________________________|"
	fprintf (stderr, "\n%s\n\nSystem is initializing...\n\n",
		 PRODUCTINFO);
	sleep (1);

	//wyong, 20231015 
	nids_params.pcap_filter = filter_string;
	if (tcpdump_filename) {
		nids_params.filename = tcpdump_filename;
	}

	if (!nids_init ()) {
		fprintf (stderr, "%s\n", nids_errbuf);
		goto shutdown;
	}

	nids_register_tcp (process_tcpdata);

#ifdef  USE_NEL
	if (stab_file_parse (eng, argv[0]) < 0) {
		printf ("\nNetwork Event Engine init failed!\n");
		goto shutdown;
	}

	/* process the app policy file */
	if (nel_file_parse (eng, argc - optind, &argv[optind]) != 0) {
		printf ("\nNetwork Event Engine parse failed!\n");
		goto shutdown;
	}
#endif

	//wyong, 20230912 
	if (smtp_init () < 0) {
		printf ("\nSmtp Engine init failed!\n");
		goto shutdown;
	}

	//wyong, 20231015 
	nids_run ();

	//cleanup(EXIT_OK, "System normally exit!");
	//wyong, 20230912
	smtp_cleanup ();

      shutdown:
#ifdef USE_NEL
	//nel_eng_dealloc(&eng);
#endif

	return ret;

}
