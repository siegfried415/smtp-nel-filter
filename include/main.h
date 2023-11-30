/*
  Copyright (c) 1999 Rafal Wojtczuk <nergal@avet.com.pl>. All rights reserved.
  See the file COPYING for license details.
*/

#ifndef _NIDS_NIDS_H
#define _NIDS_NIDS_H

#define NIDS_MAJOR 1
#define NIDS_MINOR 17

#define NIDS_SYSTEM_NAME "NIDS"

#include <sys/types.h>
#include <pcap.h>
struct tcp_stream;

enum
{
	NIDS_WARN_IP = 1,
	NIDS_WARN_TCP,
	NIDS_WARN_UDP,
	NIDS_WARN_SCAN
};

enum
{
	NIDS_WARN_UNDEFINED = 0,
	NIDS_WARN_IP_OVERSIZED,
	NIDS_WARN_IP_INVLIST,
	NIDS_WARN_IP_OVERLAP,
	NIDS_WARN_IP_HDR,
	NIDS_WARN_IP_SRR,
	NIDS_WARN_TCP_TOOMUCH,
	NIDS_WARN_TCP_HDR,
	NIDS_WARN_TCP_BIGQUEUE,
	NIDS_WARN_TCP_BADFLAGS
};

struct nids_prm
{
	int n_tcp_streams;
	int n_hosts;
	char *device;
	char *filename;
	int sk_buff_size;
	int dev_addon;
	void (*syslog) ();
	int syslog_level;
	int scan_num_hosts;
	int scan_delay;
	int scan_num_ports;
	void (*no_mem) (char *);
	int (*ip_filter) ();
	char *pcap_filter;
	int promisc;
	int one_loop_less;
};


int nids_init ();
void nids_register_ip_frag (void (*));
void nids_register_ip (void (*));
void nids_register_tcp (void (*));
void nids_register_udp (void (*));
void nids_killtcp (struct tcp_stream *);
void nids_discard (struct tcp_stream *, int);
void nids_run ();
int nids_getfd ();
int nids_next ();

extern struct nids_prm nids_params;
extern char *nids_warnings[];
extern char nids_errbuf[];

#endif /* _NIDS_NIDS_H */
