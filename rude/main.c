/*****************************************************************************
 *   main.c - the body for RUDE
 *
 *   Copyright (C) 1999 Juha Laine and Sampo Saaristo
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 *   Authors:      Juha Laine     <james@cs.tut.fi>
 *                 Sampo Saaristo <sambo@cc.tut.fi>
 *
 *****************************************************************************/
#include <config.h>
#include <rude.h>
#include <mcast.h>

#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sched.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
 #include <net/if.h>

/*
 *  Function prototypes
 */
static void             usage(char *);
static void             dump_config(void);
static void             print_results(void);
static int              open_sockets(void);
static int              init_rude(void);
void                    rude_handler(int);

extern int              read_cfg(FILE *);  /* parse.c     */
extern void             clean_up(void);    /* flow_cntl.c */
extern struct flow_cfg* find_next(void);   /* flow_cntl.c */


/*
 * Global variables
 */
struct flow_cfg *head        = NULL;   /* Our global linked list            */
struct flow_cfg *done        = NULL;   /* Our 2nd global linked list        */
struct timeval  tester_start = {0,0};  /* Absolute process START TIME       */
struct udp_data *data        = NULL;   /* */
char *buffer                 = NULL;   /* */
int max_packet_size          = 0;      /* Size of the largest packet        */


/****************************************************************************/
/*                           MAIN LOOP FUNCTIONS                            */
/****************************************************************************/
int main(int argc, char **argv)
{
	extern char *optarg;
	extern int  optind, opterr, optopt, errno;

	struct flow_cfg *flow = NULL;
	FILE *scriptfile      = NULL;
	int priority          = 0;
	int opt_ret           = 0;
//  unsigned long citac   = 0;
	int retval            = 0;
	uid_t user_id         = getuid();
	struct sigaction action;
	struct sched_param p;

	printf("rude version %s, Copyright (C) 1999 Juha Laine and Sampo Saaristo\n"
	       "rude comes with ABSOLUTELY NO WARRANTY!\n"
	       "This is free software, and you are welcome to redistribute it\n"
	       "under GNU GENERAL PUBLIC LICENSE Version 2.\n",VERSION);

	if(argc < 2){
		fprintf(stderr,"rude: at least 1 argument expected\n");
		usage(argv[0]);
		exit(1);
	}

	while(((opt_ret=getopt(argc,argv,"s:P:hv")) != EOF) && (retval == 0)){
		switch(opt_ret){
		case 'v':
			if((optind == 2) && (argc == 2)){
	printf("rude version is %s\n",VERSION);
	retval = -1;
			} else {
	RUDEBUG1("rude: invalid commandline arguments!\n");
	retval = -2;
			}
			break;

		case 'h':
			if((optind == 2) && (argc == 2)){
	usage(argv[0]);
	retval = -1;
			} else {
	RUDEBUG1("rude: invalid commandline arguments!\n");
	retval = -2;
			}
			break;

		case 's':
			if(optarg != NULL){
	if((scriptfile=fopen((const char *)optarg,"r")) == NULL){
		fprintf(stderr,"rude: could not open %s: %s\n",
			optarg,strerror(errno));
		retval = -2;
	}
			} else {
	RUDEBUG1("rude: invalid commandline arguments!\n");
	retval = -2;
			}
			break;

		case 'P':
			if(optarg != NULL){
	priority = atoi(optarg);
	if((priority < 1) || (priority > 90)){
		fprintf(stderr,"rude: priority must be between 1 to 90\n");
		retval = -2;
	}
	if(user_id != 0){
		fprintf(stderr,"rude: must be root to set the priority level\n");
		retval = -2;
	}
			} else {
	RUDEBUG1("rude: invalid commandline arguments!\n");
	retval = -2;
			}
			break;

		default:
			usage(argv[0]);
			retval = -2;
			break;
		}
	}

	if((retval < 0) || (scriptfile == NULL)){
		if(scriptfile != NULL){ fclose(scriptfile); }
		RUDEBUG7("rude: parse error");
		exit(-2);
	}

	/* Read & parse the given configuration file */
	retval = read_cfg(scriptfile);
	fclose(scriptfile);

	if(retval <= 0){
		RUDEBUG1("rude: EXIT - %d parse errors in configuration file\n",retval);
		clean_up();
		exit(3);
	}else{
		RUDEBUG7("rude: config file readed");
	}
	dump_config();

	/* Add the SIGNAL HANDLER function initialization(s) here... */
	/* FIXME: SIGSTOP ??? SIGQUIT ???                            */
	memset(&action,0,sizeof(struct sigaction));
	action.sa_handler = rude_handler;

	if(sigaction(SIGHUP,&action,NULL)){
		RUDEBUG1("rude: signal SIGHUP catching failed\n");
		retval = -1;
		goto rude_exit1;
	}
	if(sigaction(SIGINT,&action,NULL)){
		RUDEBUG1("rude: signal SIGINT catching failed\n");
		retval = -1;
		goto rude_exit1;
	}
	if(sigaction(SIGTERM,&action,NULL)){
		RUDEBUG1("rude: signal SIGTERM catching failed\n");
		retval = -1;
		goto rude_exit1;
	}

	/* Initialize the system... */
	if(init_rude() != 0){
		RUDEBUG1("rude: EXIT - initialization failed\n");
		retval = -1;
		goto rude_exit1;
	}

	/*
	 * If this process is owned by root we can do some tricks to
	 * improve the performance... (the -P option)
	 */
	if((user_id == 0) && (priority > 0)){
		/* Try to lock the memory to avoid paging delays */
		if(mlockall(MCL_CURRENT | MCL_FUTURE) < 0){
			RUDEBUG1("rude: memory lock failed: %s\n", strerror(errno));
		}

		/* Switch to Round-Robin-Real-Time Scheduling */
		p.sched_priority = priority;
		if(sched_setscheduler(0, SCHED_RR, &p) < 0){
			RUDEBUG1("rude: sched_setscheduler() failed: %s\n",strerror(errno));
			retval = -1;
			goto rude_exit;
		}
		RUDEBUG7("rude: program priority set to %d\n", p.sched_priority);
	}

	/*
	 * All is fine - start looping & transmitting
	 */
	RUDEBUG7("rude: start LOOPING\n");
	while((flow = find_next()) != NULL){
		flow->send_func(flow);
	}
	RUDEBUG7("rude: stop LOOPING\n");

 rude_exit:

	/*
	 * Restore the tweaked settings...
	 */
	if((user_id == 0) && (priority > 0)){
		/* Restore the program priority */
		p.sched_priority = 0;
		if(sched_setscheduler(0, SCHED_OTHER, &p) < 0){
			RUDEBUG1("rude: program priority restoring failed: %s\n",strerror(errno));
			retval = -1;
		} else {
			RUDEBUG7("rude: program priority restored\n");
		}

		/* Release the locked memory */
		munlockall();
	}

 rude_exit1:

	/* FIXME: Remove SIGNAL HANDLERS ??? */
	/* dump_config(); */
	print_results();
	clean_up();
	exit(retval);
}


/*
 * rude_handler(): Smar clean-up function for several signals...
 */
void rude_handler(int value)
{
	int ret_val = (value * (-1));
	struct sched_param pri;
	RUDEBUG7("\nrude_handler(): receiver SIGNAL %d - clean-up & exit\n",value);

	/* Check & restore process priority */
	if((getuid() == 0) && (sched_getscheduler(0) != SCHED_OTHER)){
		pri.sched_priority = 0;
		if(sched_setscheduler(0, SCHED_OTHER, &pri) < 0){
			RUDEBUG1("rude_handler: rude priority failure: %s\n",strerror(errno));
		} else {
			RUDEBUG7("rude_handler: rude priority restored\n");
		}
	}

	munlockall();
	clean_up();
	exit(ret_val);
} /* main() */


/*
 * usage(): Print the usage information
 */
static void usage(char *name)
{
	printf("\nusage: %s -h | -v | -s scriptfile [-p priority]\n\n"
	       "\t-h            = print (this) short help and usage information\n"
	       "\t-v            = print the version number and exit\n"
	       "\t-s scriptfile = path to the flow configuration file\n"
	       "\t-P priority   = process realtime priority {1-90}\n\n",name);
} /* usage() */


/*
 * dump_config(): Dumps the current flow configuration to the stderr
 *                but only if DEBUG level is set to 7 (or higher...).
 *                This function requires attention if one adds new flow
 *                types to this program...
 */
static void dump_config(void)
{
#if (DEBUG > 6)
	struct flow_cfg *flow   = head;
	struct flow_cfg *flow_m = head;
	char straddr[INET6_ADDRSTRLEN];

	/* Print the header, if there is any data to dump */
	if(flow != NULL){
		fprintf(stderr,"RUDE START = %ld.%06ld\n\nF_ID: F_START: F_STOP: "
			"F_SPORT: F_DADD: F_DPORT: F_TYPE: [+ type params]\n",
			tester_start.tv_sec,tester_start.tv_usec);
	}

	while(flow_m != NULL){
		switch ((flow->dst).ss_family) {
			case AF_INET:{
					struct sockaddr_in* dstin = (struct sockaddr_in *)&(flow->dst);
					fprintf(stderr,"%ld %ld.%06ld %ld.%06ld %hu %s %hu ",
					        flow->flow_id, flow->flow_start.tv_sec,flow->flow_start.tv_usec,
					        flow->flow_stop.tv_sec,flow->flow_stop.tv_usec,
					        flow->flow_sport,inet_ntop(AF_INET, &(dstin->sin_addr), straddr, INET_ADDRSTRLEN),
					ntohs(dstin->sin_port));
				}
				break;
				case AF_INET6:{
					struct sockaddr_in6* dstin = (struct sockaddr_in6 *)&(flow->dst);
					fprintf(stderr,"%ld %ld.%06ld %ld.%06ld %hu %s %hu ",
						flow->flow_id, flow->flow_start.tv_sec,flow->flow_start.tv_usec,
						flow->flow_stop.tv_sec,flow->flow_stop.tv_usec,
						flow->flow_sport,inet_ntop(AF_INET6, &(dstin->sin6_addr), straddr, sizeof(straddr)),
				ntohs(dstin->sin6_port));
				}
				break;
				default:
					fprintf(stderr,"Protocol is not known:%s ",(flow->dst).ss_family);
		}


		switch(flow->params.ftype){
			case(CBR):
				fprintf(stderr,"CBR [r:%d s:%d ps:%d tp:%d]\n",
					flow->params.cbr.rate,flow->params.cbr.psize,flow->params.cbr.package_size,flow->params.cbr.time_period);
				break;
			case(TRACE):
				fprintf(stderr,"TRACE [list_size:%u]\n",flow->params.trace.list_size);
				break;
			default:
				fprintf(stderr,"<UNKNOWN TYPE>\n");
				break;
		}

		/* Set flow to the next possible flow_cfg (either modified or next id) */
		if(flow->mod_flow != NULL){
			flow = flow->mod_flow;
		} else {
			flow   = flow_m->next;
			flow_m = flow;
		}
	}
	fprintf(stderr,"\n");


	/* Do the same for the already sent flows... */
	if((flow = done) != NULL){
		fprintf(stderr,"\nF_ID: F_START: F_STOP: F_SPORT: F_DADD: F_DPORT:"
		               " F_err: F_suc: F_seq: F_TYPE: [+ type params]\n");
	}

	while(flow != NULL){
		switch ((flow->dst).ss_family) {
			case AF_INET:{
					struct sockaddr_in* dstin = (struct sockaddr_in *)&(flow->dst);
					fprintf(stderr,"%ld %ld.%06ld %ld.%06ld %hu %s %hu %d %d %d ",
					        flow->flow_id, flow->flow_start.tv_sec,flow->flow_start.tv_usec,
					        flow->flow_stop.tv_sec,flow->flow_stop.tv_usec,
					        flow->flow_sport,inet_ntop(AF_INET, &(dstin->sin_addr), straddr, INET_ADDRSTRLEN),
					        ntohs(dstin->sin_port),flow->errors, flow->success,
					        flow->sequence_nmbr);
				}
				break;
			case AF_INET6:{
					struct sockaddr_in6* dstin = (struct sockaddr_in6 *)&(flow->dst);
					fprintf(stderr,"%ld %ld.%06ld %ld.%06ld %hu %s %hu %d %d %d ",
					        flow->flow_id, flow->flow_start.tv_sec,flow->flow_start.tv_usec,
					        flow->flow_stop.tv_sec,flow->flow_stop.tv_usec,
					        flow->flow_sport,inet_ntop(AF_INET6, &(dstin->sin6_addr), straddr, sizeof(straddr)),
					        ntohs(dstin->sin6_port),flow->errors, flow->success,
					        flow->sequence_nmbr);
			}
				break;
			default:
				fprintf(stderr,"Protocol is not known:%s ",(flow->dst).ss_family);
		}

		switch(flow->params.ftype){
			case(CBR):
				fprintf(stderr,"CBR [r:%d s:%d ps:%d tp:%d]\n",
					flow->params.cbr.rate,flow->params.cbr.psize,flow->params.cbr.package_size,flow->params.cbr.time_period);
				break;
			case(TRACE):
				fprintf(stderr,"TRACE [list_size:%u]\n",flow->params.trace.list_size);
				break;
			default:
				fprintf(stderr,"<UNKNOWN TYPE>\n");
				break;
		}
		flow = flow->next;
	}
	fprintf(stderr,"\n");
#endif
	return;
} /* dump_config() */


/*
 * print_results(): Print the Xmit status after completed. This function
 *                  requires attention if one adds new flow types to this
 *                  program...
 */
static void print_results(void)
{
	struct flow_cfg *flow   = NULL;
	char straddr[INET6_ADDRSTRLEN];

	if((flow = done) != NULL){
		printf("\nF_ID: F_START: F_STOP: F_SPORT: F_DADD: F_DPORT:"
		       " F_err: F_suc: F_seq: F_TYPE: [+ type params]\n");
	}

	while(flow != NULL){
		switch ((flow->dst).ss_family) {
			case AF_INET:{
					struct sockaddr_in* dstin = (struct sockaddr_in *)&(flow->dst);
					fprintf(stderr,"%ld %ld.%06ld %ld.%06ld %hu %s %hu %d %d %d ",
						flow->flow_id, flow->flow_start.tv_sec,flow->flow_start.tv_usec,
						flow->flow_stop.tv_sec,flow->flow_stop.tv_usec,
						flow->flow_sport,inet_ntop(AF_INET, &(dstin->sin_addr), straddr, INET_ADDRSTRLEN),
						ntohs(dstin->sin_port),flow->errors, flow->success,
						flow->sequence_nmbr);
				}
				break;
			case AF_INET6:{
					struct sockaddr_in6* dstin = (struct sockaddr_in6 *)&(flow->dst);
					fprintf(stderr,"%ld %ld.%06ld %ld.%06ld %hu %s %hu %d %d %d ",
						flow->flow_id, flow->flow_start.tv_sec,flow->flow_start.tv_usec,
						flow->flow_stop.tv_sec,flow->flow_stop.tv_usec,
						flow->flow_sport,inet_ntop(AF_INET6, &(dstin->sin6_addr), straddr, sizeof(straddr)),
						ntohs(dstin->sin6_port),flow->errors, flow->success,
						flow->sequence_nmbr);
				}
				break;
			default:
				fprintf(stderr,"Protocol is not known:%s ",(flow->dst).ss_family);
		}

		switch(flow->params.ftype){
			case(CBR):
				fprintf(stderr,"CBR [r:%d s:%d ps:%d tp:%d]\n",
					flow->params.cbr.rate,flow->params.cbr.psize,flow->params.cbr.package_size,flow->params.cbr.time_period);
				break;
			case(TRACE):
				printf("TRACE [list_size:%u]\n",flow->params.trace.list_size);
				break;
			default:
				printf("<UNKNOWN TYPE>\n");
				break;
		}
		flow = flow->next;
	}
	return;
} /* print_results() */


/*
 * open_sockets(): Open socket for every flow
 */
static int open_sockets(void)
{
	struct sockaddr_storage our_addr;
	struct flow_cfg *flow = head;
	int retval = 0;
	int flags  = 0;
	int tos;
	char addr[127];

	while(flow != NULL){
		//pokud je adresa kam chceme vysilat ip6 tak kaslem na to jakou verzi jsme chteli a otevreme soket na ip6
		if((flow->dst).ss_family==AF_INET6 ||flow->prefferedVersion == '6'){
			memset(&our_addr, 0, sizeof(struct sockaddr_storage));
			((struct sockaddr_in6 *)&our_addr)->sin6_family = AF_INET6;
			((struct sockaddr_in6 *)&our_addr)->sin6_addr = in6addr_any;
			inet_ntop(AF_INET6,&((struct sockaddr_in6 *)&our_addr)->sin6_addr,addr,sizeof(addr));
			RUDEBUG7("open_socket(): using address: %s\n",addr);
			if((flow->send_sock = socket(AF_INET6, SOCK_DGRAM, 0)) < 0){
				RUDEBUG1("open_sockets() - socket() failed for flow %ld: %s\n",
				flow->flow_id,strerror(errno));
				retval--;
				goto socket_error;
			}
			//check if packet_size is enough
			if(flow->params.cbr.psize < PMINSIZE_V6){
				RUDEBUG1("open_sockets() - additional check for ipv6 packet size failed, flow %ld, minimal size is %d\n",
				flow->flow_id,PMINSIZE_V6);
				retval--;
				goto socket_error;
			}


		}
		else{
			memset(&our_addr, 0, sizeof(struct sockaddr_storage));
			((struct sockaddr_in *)&our_addr)->sin_family = AF_INET;
			((struct sockaddr_in *)&our_addr)->sin_addr.s_addr =htonl(INADDR_ANY);
			inet_ntop(AF_INET,&((struct sockaddr_in *)&our_addr)->sin_addr,addr,sizeof(addr));
			RUDEBUG7("open_socket(): using address: %s\n",addr);
			if((flow->send_sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
				RUDEBUG1("open_sockets() - socket() failed for flow %ld: %s\n",
				flow->flow_id,strerror(errno));
					retval--;
					goto socket_error;
			}
		}

		if(isMulticastAddr(&(flow->dst)) > 0){
			RUDEBUG7("open_sockets() - dst address is a multicast one. \n");
			if(flow -> localIf != NULL){
				RUDEBUG7("open_sockets() - using specified interface for multicast, name: %s\n",flow->localIf);
				unsigned int outif;
				if(0==(outif = if_nametoindex(flow->localIf))){
					RUDEBUG1("open_sockets() - specified interface does not exist\n");
					retval--;
					goto socket_error;
				}
				if((flow->dst).ss_family == AF_INET6 && setsockopt(flow->send_sock, IPPROTO_IPV6, IPV6_MULTICAST_IF, &outif, sizeof(outif))<0){
					RUDEBUG1("open_sockets() - setsockopt failed for setting multicast interface: %s\n",strerror(errno));
					retval--;
					goto socket_error;
				}
				//pro ipv4 multicast se neposila multicast interface ale adresa
				if((flow->dst).ss_family == AF_INET){
					//zjistime adresu toho rozhrani
					struct ifreq ifr;
					strcpy(ifr.ifr_name, flow->localIf);
					//int tempSock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
					if(ioctl(flow->send_sock,SIOCGIFADDR,&ifr)==-1){
					//&& setsockopt(flow->send_sock, IPPROTO_IP, IP_MULTICAST_IF, &outif, sizeof(outif))<0){
						RUDEBUG1("open_sockets() - ioctl failed for getting interface address: %s\n",strerror(errno));
						retval--;
						goto socket_error;
					}
					//close(tempSock);
					struct sockaddr_in *si = (struct sockaddr_in*)&ifr.ifr_addr;
					RUDEBUG7("open_sockets(): interface adresa: %s",inet_ntoa(si->sin_addr));
					struct in_addr add = si->sin_addr;
					if(setsockopt(flow->send_sock, IPPROTO_IP, IP_MULTICAST_IF, &add, sizeof(add))<0){
						RUDEBUG1("open_sockets() - setsockopt failed for setting multicast interface: %s\n",strerror(errno));
						retval--;
						goto socket_error;
					}
				}
			}
			//jeste TTL
			unsigned int hlim = 127;      /* range: 0 to 255, default = 1 */
			if((flow->dst).ss_family == AF_INET6 && setsockopt(flow->send_sock, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, &hlim, sizeof(hlim))<0){
				RUDEBUG1("open_sockets() - setsockopt failed for setting multicast ttl: %s\n",strerror(errno));
				retval--;
				goto socket_error;
			}
			if((flow->dst).ss_family == AF_INET && setsockopt(flow->send_sock, IPPROTO_IP, IP_MULTICAST_TTL, &hlim, sizeof(hlim))<0){
				RUDEBUG1("open_sockets() - setsockopt failed for setting multicast interface: %s\n",strerror(errno));
				retval--;
				goto socket_error;
			}
		}else
		RUDEBUG7("open_sockets() - dst address is not a multicast one. \n");


		((struct sockaddr_in6 *)&our_addr)->sin6_port = htons(flow->flow_sport);
		if(bind(flow->send_sock, (struct sockaddr *)&our_addr, sizeof(struct sockaddr_in6)) < 0){
			close(flow->send_sock);
			flow->send_sock = 0;
			RUDEBUG1("open_sockets() - bind() failed for flow %ld: %s\n",
			         flow->flow_id,strerror(errno));
			retval--;
			goto socket_error;
		}


		if((flags = fcntl(flow->send_sock, F_GETFL, 0)) < 0) {
			RUDEBUG1("open_sockets() - GETFLAGS error\n");
			retval--;
		} else if((fcntl(flow->send_sock, F_SETFL, flags | O_NONBLOCK)) < 0){
			RUDEBUG1("open_sockets() - SETFLAGS O_NONBLOCK error\n");
			retval--;
		}
#ifndef SOL_IP
#define SOL_IP IPPROTO_IP
#endif
		if(flow->tos > 0){
			tos = (unsigned char) flow->tos;
			if(setsockopt(flow->send_sock, SOL_IP, IP_TOS, &tos, sizeof(tos)) == -1){
				RUDEBUG1("Can't set TOS for flow %ld: using default...\n",
								 flow->flow_id);
			}
		}

	socket_error:
		flow = flow->next;
	}

	RUDEBUG7("open_sockets() - EXIT(%d)\n",retval);
	return(retval);
} /* open_sockets() */


/*
 * init_rude(): initializes sockets and packet "buffer"
 */
static int init_rude(void)
{
	struct timeval now  = {0,0};
	unsigned long wait  = 0;
	int retval          = 0;

	/* Initialize/Check the global variables */
	if(max_packet_size <= 0){
		RUDEBUG1("init_rude() - max_packet_size not set\n");
		retval -= 1;
		goto init_exit;
	}

	if((buffer = malloc(max_packet_size)) == NULL){
		RUDEBUG1("init_rude() - malloc() failed: %s\n", strerror(errno));
		retval -= 2;
		goto init_exit;
	}
	memset(buffer,0,max_packet_size);
	data = (struct udp_data *)buffer;

	if(open_sockets() != 0){
		retval -= 4;
		goto init_exit;
	}

	/* Check the time when we should start/have started transmission */
	gettimeofday(&now,NULL);
	if(timercmp(&tester_start,&now,>)){
		wait = ((tester_start.tv_sec - now.tv_sec)*1000000) +
			(tester_start.tv_usec - now.tv_usec);
		RUDEBUG7("init_rude() - wait for start %lu microseconds...",wait);
		usleep(wait);
		RUDEBUG7("done!\n");
	}

 init_exit:
	RUDEBUG7("init_rude() - EXIT(%d)\n",retval);
	return(retval);
}
