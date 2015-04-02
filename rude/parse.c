/******************************************************************************
 *   parse.c
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
 *-----------------------------------------------------------------------------
 *  This file holds the routines that build the transmitted flow data
 *  structure from the given configuration file. The data structure is
 *  accessible from the global pointer "head" and built as follows:
 *
 *   ___________      ____________     ____________
 *   | FLOW #01 |     | FLOW #02 |     | FLOW #NN |
 *   |     *next|---->|     *next|-...>|     *next|--|
 *   | *mod_flow|     | *mod_flow|     | *mod_flow|
 *   ------------     ------------     ------------
 *        |                |
 *       ---          ____________
 *                    | FLOW #02 |
 *                    |     *next|--|
 *                    | *mod_flow|
 *                    ------------
 *                         |
 *                        ---
 *
 *  Each flow has exactly one entry in the main one-way linked list
 *  accessible via the "*head" and "*next" pointers. Each flow can have
 *  different characteristics during unique time-slots, which are
 *  represented by "boxes" accessible via the "*mod_flow" pointer. None
 *  of these boxes can overlap for the specific flow - i.e. flow can't
 *  have more than one characteristics during certain time-interval.
 *
 *  After the "box" has been executed it will be removed and placed to
 *  the other list pointed by "*done" for (possible) later examination.
 *****************************************************************************/
#include <config.h>
#include <rude.h>

#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <inttypes.h>


/*
 * Global variables/functions defined elsewhere and used in this file.
 */
extern struct flow_cfg *head;
extern struct timeval  tester_start;
extern int             max_packet_size;

extern void            send_cbr(struct flow_cfg *);   /* flow_txmit.c */
extern void            send_trace(struct flow_cfg *); /* flow_txmit.c */

/*
 * Function that locates the 1st element for specific flow (if any)
 */
__inline__ struct flow_cfg *find_flow_id(long int id)
{
	struct flow_cfg *temp = head;

	while(temp != NULL){
		if(temp->flow_id == id){
			RUDEBUG7("find_flow_id() - flow found (id=%ld)\n",id);
			return temp;
		}
		temp=temp->next;
	}

	RUDEBUG7("find_flow_id() - flow not found (id=%ld)\n",id);
	return NULL;
} /* find_flow_id() */


/*
 * Parser for the flow type argument
 */
f_type check_type(char *type)
{
	f_type typeval = UNKNOWN;

	if(type == NULL){
		RUDEBUG1("check_type() - type parameter string is NULL\n");
	} else {
		if(strncasecmp(type,"CBR",3) == 0){ typeval = CBR; }
		else if(strncasecmp(type,"CONSTANT",8) == 0){ typeval = CONSTANT; }
		else if(strncasecmp(type,"TRACE",5) == 0){ typeval = TRACE; }
		/* ADD the other RECOGNIZED FLOW TYPES HERE !!! */
		else {
			RUDEBUG1("check_type() - no such type (%s)\n",type);
		}
	}

	RUDEBUG7("check_type() - EXIT(%d)\n",typeval);
	return(typeval);
} /* check_type() */


/*
 * Parse the destination IP addres/name and port
 */
int check_dst(char *dst, struct sockaddr_storage *f_dst,int withPort)
{
	char dname[DNMAXLEN];
	int dnamelen             = 0;
	unsigned short dport     = 0;
	struct hostent *dst_info = NULL;
	char *commaptr           = NULL;
	char**  a;

	RUDEBUG7("check_dst(): %s\n",dst);

	if(withPort == 1){
		if((dst == NULL) || (f_dst == NULL)){
		RUDEBUG1("check_dst() - NULL parameter error\n");
		return(-1);
	}

	if((commaptr=strrchr(dst,':')) == NULL){
		RUDEBUG1("check_dst() - dst address format error (%s)\n",dst);
		return(-2);
	} else {
		dnamelen = (commaptr - dst);
		if((dnamelen<1) && (dnamelen>(DNMAXLEN-2))){
			RUDEBUG1("check_dst() - too long dst name/address (%s)\n",dst);
			return(-2);
		} else {
			strncpy(dname,dst,dnamelen);
			dname[dnamelen]='\0';
			if(1 != sscanf(commaptr,":%hu",&dport)){
	RUDEBUG1("check_dst() - could not get dst port from (%s)\n",dst);
	return(-2);
			}
		}
	}

/*moje debugy*/
//  printf("\ncommaptr: %s\n", commaptr);
//  printf("\ndname: %s\n", dname);
//  printf("\ndst: %s\n", dst);

	RUDEBUG7("check_dst() - address=%s port=%hu\n",dname,dport);
	}else{
		strcpy(dname,dst);
	}

	//zkusime prvni jestli to je ipv6 nebo ne
	if((dst_info=gethostbyname2(dname, AF_INET6)) == NULL) {
		//asi to ipv6 nebyla
		if((dst_info=gethostbyname2(dname, AF_INET)) == NULL){
			//a asi ani ne ipv4
			RUDEBUG1("check_dst() - gethostbyname() error: %s\n",strerror(errno));
			return(-3);
		}else{
			//struct in_addr u;
			a = dst_info->h_addr_list;
			if (*a)
			{
				//RUDEBUG7("\rcheck_dst() - we use the first address: ");
				//u.s_addr = **(unsigned long **)a;
				//RUDEBUG7("%s", inet_ntoa (u));
				f_dst->ss_family = AF_INET;
				memcpy(&(((struct sockaddr_in *)f_dst)->sin_addr), dst_info->h_addr_list[0], sizeof(struct in_addr));
				//char temp[23]; //maximal length is 4*3+3dots+\0+length(::FFFF:))
				//sprintf(temp,"=::FFFF:%s",inet_ntoa (u));
				//RUDEBUG7("%s",temp);
				//convert to binary
				//inet_pton(AF_INET6,temp,&f_dst->sin6_addr);
			}
			else{
					RUDEBUG1("check_dst() - gethostbyname() error: %s\n",strerror(errno));
					return(-3);
			}

		}
	}else{
		f_dst->ss_family = AF_INET6;
		memcpy(&(((struct sockaddr_in6 *)f_dst)->sin6_addr), dst_info->h_addr_list[0], sizeof(struct in6_addr));
	}

	//z obrazku http://www.awprofessional.com/articles/article.asp?p=169505&seqNum=2&rl=1
	//vyplyva ze zacatek je stejnej pro ipv6 i ipv4 ale ze adresa je pro ipv4 jinde
	//ss_port is on the same offset for ipv4 and ipv6
		((struct sockaddr_in *)f_dst)->sin_port = htons(dport);

/*moje debugy*/
//  printf("\nAdresa v ukazateli: %s\n", inet_ntop(AF_INET6, &f_dst->sin6_addr, straddr, sizeof(straddr)));

	return 0;
} /* check_dst() */


/*
 * Special parse function for TRACE flows/files
 */
void trace_parse(char *buffer, struct trace_params *par)
{
	char         target[256];
	char         time2_array[7];
	FILE         *fptr = NULL;
	unsigned int l_size = 0;
	long int time1,time2;
	unsigned int i;
	int fraglen;
	int p_size;
	char action[11] = {0};

	/* Add the "ultimate" end character :) */
	time2_array[6] = '\0';

	/* Determine if ON or MODIFY */
	if(1 != sscanf(buffer,"%*d %*d %10s %*s",action)){
		RUDEBUG1("trace_parse() - couldn't obtain the action type\n");
		return;
	}

	/* Get the file name and open it. Report errors */
	if (strncasecmp(action,"ON",2) == 0) {
		if(1 != sscanf(buffer,"%*d %*d %*10s %*u %*127s %*31s %250s",target)){
			RUDEBUG1("trace_parse() - couldn't obtain the trace file name\n");
			return;
		}
	} else if (strncasecmp(action,"MODIFY",6) == 0) {
		if(1 != sscanf(buffer,"%*d %*d %*10s %*31s %250s",target)){
			RUDEBUG1("trace_parse() - couldn't obtain the trace file name\n");
			return;
		}
	} else {
		RUDEBUG1("trace_parse() - action is not ON or MODIFY\n");
		return;
	}
	if((fptr = fopen(target,"r")) == NULL){
		RUDEBUG1("trace_parse() - fopen() failed: %s\n",strerror(errno));
		return;
	}

	/* Count the number of lines in the trace/target file */
	while(1){
		fgets(target,255,fptr);
		if(ferror(fptr)){
			RUDEBUG1("trace_parse() - linecount failed: %s\n",strerror(errno));
			fclose(fptr);
			return;
		}
		if(feof(fptr)){ break; }
		l_size++;
	}
	rewind(fptr);

	/* Allocate memory and parse the file to the list */
	par->list = (struct trace_list*)malloc(l_size*sizeof(struct trace_list));
	if(par->list == NULL){
		RUDEBUG1("trace_parse() - malloc() failed\n");
		fclose(fptr);
		return;
	}
	memset(par->list,0,l_size*sizeof(struct trace_list));


	for(i=0; i<l_size; i++){
		/* Read a line into buffer "target" */
		if(fgets(target,255,fptr) == NULL){
			RUDEBUG1("trace_parse() - fgets() error\n");
			free(par->list);
			par->list = NULL;
			l_size    = 0;
			break;
		}
		/* Parse the read line */
		if(3 != sscanf(target,"%d %ld.%6[0-9]\n",&p_size,&time1,time2_array)){
			RUDEBUG1("trace_parse() - illegal file format\n");
			free(par->list);
			par->list = NULL;
			l_size    = 0;
			break;
		}
		/* Add the missing zeros - if any */
		for(fraglen=strlen(time2_array); fraglen<6; fraglen++){
			time2_array[fraglen]='0';
		}
		errno = 0;
		time2 = strtoul(time2_array, NULL, 10);

		if(p_size<PMINSIZE || p_size>PMAXSIZE || time1<0 || time2<0 || errno!=0){
			RUDEBUG1("trace_parse() - illegal parameter(s)\n");
			free(par->list);
			par->list = NULL;
			l_size    = 0;
			break;
		}
		par->list[i].psize        = p_size;
		par->list[i].wait.tv_sec  = time1;
		par->list[i].wait.tv_usec = time2;

		if(p_size > par->max_psize){ par->max_psize = p_size; }
		RUDEBUG7("trace_parse() - %u/%u (psize=%d wait=%ld.%06ld)\n",
		         (i+1), l_size, p_size, time1, time2);
	} /* for(...) */

	/* Return */
	fclose(fptr);
	par->list_size = l_size;
	return;
} /* trace_parse() */


/*
 * Parser for the FLOW ON command
 */
int flow_on(char *buffer)
{
	struct flow_cfg *new  = NULL;
	struct flow_cfg *temp = NULL;
	struct timeval  stime = {0,0};
	f_type typenum        = UNKNOWN;
	unsigned short sport  = 0;
	char dst[DNMAXLEN],type[TMAXLEN];
	char src[DNMAXLEN];//port + local interface to use - format eg 2000(eth0)
	long int time,id,rate,psize,package_size,time_period;
	char *ch,*ch2;
	char srcif[DNMAXLEN]; //local interface to use
	char *srcifp = NULL;
	RUDEBUG7("flow_on(): %s",buffer);
	char prefferedVersion = 0;
	srcif[0] = '\0';

	//if(5 != sscanf(buffer,"%ld %ld %*10s %hu %127s %31s %*s ",
	// &time,&id,&sport,dst,type)){
	if(5 != sscanf(buffer,"%ld %ld %*10s %30s %127s %31s %*s ",
		&time,&id,&src,dst,type)){
		RUDEBUG1("flow_on() - invalid (number of) arguments\n");
		return(-1);
	}
	/* Parse src */
	sport = strtol(src,&ch,10);
	if((*ch) != '\0'){
		if((*ch)!='(' && (*ch)!=':'){
			RUDEBUG1("flow_on() - invalid arguments:%c\n",*ch);
				return(-1);
		}
		//je tam uvedeny i adapter nebo verze- ma vyznam zatim jenom u multicastu
		if((*ch)=='('){
			if((ch2 = strchr(ch,')')) && (strlen(src)==(ch2-src+1)||strlen(src)==(ch2-src+3))){
			strncpy(srcif,ch+1,ch2-ch);
			srcif[ch2-ch-1]='\0';
			srcifp = malloc(strlen(srcif)+1);
			strcpy(srcifp,srcif);
			RUDEBUG7("Interface to use: %s\n",srcifp);
			}else{
			RUDEBUG1("flow_on() - invalid arguments2\n");
				return(-1);
			}
		}

		if(ch2 = strchr(ch,':')){
			if((*(++ch2)=='4') ||(*ch2 =='6')){
				prefferedVersion = *ch2;
				RUDEBUG7("Preffered version: %c\n",*ch2);
			}
			else{
				RUDEBUG1("flow_on() - invalid arguments3\n");
					return(-1);
			}
		}
	}
	RUDEBUG7("flow_on() - Local port: %hu\n",sport);

	/* Allocate new block */
	if((new = (struct flow_cfg *)malloc(sizeof(struct flow_cfg))) == NULL){
		RUDEBUG1("flow_on() - malloc() error: %s\n",strerror(errno));
		return(-2);
	}
	memset(new,0,sizeof(struct flow_cfg));
	new->tos = -1;  /* By default, don't set the TOS */
	new->localIf = srcifp;
	new->prefferedVersion = prefferedVersion;

	/* Do sanity check to the given parameters */
	if((time < 0) || (sport < 1024) || ((typenum=check_type(type)) < 0) ||
		(check_dst(dst,&new->dst,1) != 0)){
		free(new);
		RUDEBUG1("flow_on() - illegal argument values\n");
		return(-3);
	}

	/* Calculate the time to timeval structure amd set the START and */
	/* 1st packet transmission time...                               */
	stime.tv_sec  = (time/1000);
	stime.tv_usec = ((time-(stime.tv_sec*1000))*1000);
	timeradd(&stime,&tester_start,&new->flow_start);
	new->next_tx = new->flow_start;

	switch(typenum){

		case(CBR):
			//if(2 != sscanf(buffer,"%*d %*d %*10s %*u %*127s %*31s %ld %ld",
		if(2 != sscanf(buffer,"%*d %*d %*10s %*30s %*127s %*31s %ld %ld",
			&rate,&psize)){
				free(new);
				RUDEBUG1("flow_on() - invalid (number of) CBR flow arguments\n");
				return(-4);
			} else if((rate < 0) || (psize < PMINSIZE) || (psize > PMAXSIZE)){
				free(new);
				RUDEBUG1("flow_on() - illegal CBR flow arguments\n");
				return(-4);
			}
			//read next two optional parameters
			if((1 != sscanf(buffer,"%*d %*d %*10s %*30s %*127s %*31s %*ld %*ld %ld",
				&package_size)) || (package_size < 0)) {
				package_size = 1;
			}
			if((1 != sscanf(buffer,"%*d %*d %*10s %*30s %*127s %*31s %*ld %*ld %*ld %ld",
				&time_period)) || (time_period< 0)) {
				time_period = 1;
			}

			new->flow_id            = id;
			new->flow_sport         = sport;
			new->send_func          = send_cbr;
			new->params.cbr.ftype   = CBR;
			new->params.cbr.rate    = rate;
			new->params.cbr.psize   = psize;
			new->params.cbr.package_size = package_size;
			new->params.cbr.time_period = time_period;
			RUDEBUG7("flow_on() - CBR flow id=%ld created(package size: %ld,time period: %ld)\n",id,package_size,time_period);
			break;

		case(TRACE):
			new->flow_id                 = id;
			new->flow_sport              = sport;
			new->send_func               = send_trace;
			new->params.trace.ftype      = TRACE;
			trace_parse(buffer, &new->params.trace);
			if(new->params.trace.list_size == 0){
				free(new);
				return(-4);
			}
			RUDEBUG7("flow_on() - TRACE flow id=%ld created\n",id);
			break;

		default:
			RUDEBUG1("flow_on() - invalid flow type (%32s)\n",type);
			free(new);
			return(-5);
			break;
	}

	/* Add the flow to the list pointed by HEAD */
	if(head == NULL){ head = new; }
	else {
		temp = head;
		while(temp->next != NULL){ temp = temp->next; }
		temp->next = new;
	}

	return 0;
} /* flow_on() */


/*
 * Parser for the FLOW OFF command
 */
int flow_off(struct flow_cfg *target, long int time)
{
	struct flow_cfg *temp = target;
	struct timeval  otime = {0,0};

	/* Find the last object for this flow */
	while(temp->mod_flow != NULL){ temp = temp->mod_flow; }

	/* Check if the OFF time is already set! */
	if((temp->flow_stop.tv_sec != 0) || (temp->flow_stop.tv_usec !=0)){
		RUDEBUG1("flow_off() - STOP already set (id=%"PRIu32")\n",temp->flow_id);
		return(-1);
	}

	/* Calculate the time to timeval structure and set the STOP time */
	otime.tv_sec  = (time/1000);
	otime.tv_usec = ((time-(otime.tv_sec*1000))*1000);
	timeradd(&otime,&tester_start,&temp->flow_stop);

	/* Do sanity check */
	if(timercmp(&temp->flow_stop,&temp->flow_start,<)){
		temp->flow_stop = temp->flow_start;
		RUDEBUG1("flow_off() - STOP < START time (id=%"PRIu32")\n",temp->flow_id);
		return(-2);
	}

	RUDEBUG7("flow_off() - flow (id=%ld) turned off\n",temp->flow_id);
	return 0;
}


/*
 * Here is the cool part - FLOW MODIFY command parser :)
 */
int flow_modify(struct flow_cfg *target, char *buffer)
{
	struct flow_cfg *mod  = NULL;
	struct flow_cfg *temp = target;
	struct timeval  mtime = {0,0};
	f_type typenum        = UNKNOWN;
	long int time,rate,psize,package_size,time_period;
	char type[TMAXLEN];

	if(2 != sscanf(buffer,"%ld %*d %*10s %31s %*s",
		&time,type)){
		RUDEBUG1("flow_modify() - invalid (number of) arguments\n");
		return(-1);
	}

	/* Do sanity check to the given parameters */
	if(time<0 || (typenum=check_type(type))<0){
		RUDEBUG1("flow_modify() - invalid argument values\n");
		return(-2);
	}

	/* Create new flow_cfg structure */
	if((mod = (struct flow_cfg *)malloc(sizeof(struct flow_cfg))) == NULL){
		RUDEBUG1("flow_modify() - malloc() error: %s\n",strerror(errno));
		return(-3);
	}
	memset(mod,0,sizeof(struct flow_cfg));

	/* Turn the "current" flow block off at the same time when this */
	/* new behaviour should start. flow_off() checks also that the  */
	/* time parameter is valid...                                   */
	while(temp->mod_flow != NULL){ temp = temp->mod_flow; }
	if(flow_off(temp,time) < 0){
		free(mod);
		RUDEBUG1("flow_modify() - start time error (id=%"PRIu32")\n",target->flow_id);
		return(-4);
	}

	/* Calculate the time to timeval structure and set the START time */
	mtime.tv_sec  = (time/1000);
	mtime.tv_usec = ((time-(mtime.tv_sec*1000))*1000);
	timeradd(&mtime,&tester_start,&mod->flow_start);
	mod->next_tx = mod->flow_start;

	switch(typenum){
	case(CBR):
		if(2 != sscanf(buffer,"%*d %*d %*10s %*31s %ld %ld", &rate, &psize)){
			free(mod);
			RUDEBUG1("flow_modify() - invalid (number of) CBR args (id=%"PRIu32")\n",
			         temp->flow_id);
			return(-4);
		} else if((rate < 0) || (psize < PMINSIZE) || (psize > PMAXSIZE)){
			free(mod);
			RUDEBUG1("flow_modify() - invalid CBR arguments (id=%"PRIu32")\n",
			         temp->flow_id);
			return(-4);
		}
	//read next two optional parameters
	if((1 != sscanf(buffer,"%*d %*d %*10s %*31s %*ld %*ld %ld",
		&package_size)) || (package_size < 0)) {
		package_size = 1;
		}
	if((1 != sscanf(buffer,"%*d %*d %*10s %*31s %*ld %*ld %*ld %ld",
		&time_period)) || (time_period< 0)) {
		time_period = 1;
		}

		mod->flow_id            = temp->flow_id;
		mod->dst                = temp->dst;
		mod->flow_sport         = temp->flow_sport;
		mod->send_func          = send_cbr;
		mod->params.cbr.ftype   = typenum;
		mod->params.cbr.rate    = rate;
		mod->params.cbr.psize   = psize;
		mod->params.cbr.package_size = package_size;
		mod->params.cbr.time_period = time_period;
		temp->mod_flow          = mod;
		RUDEBUG7("flow_modify() - flow id=%ld modified\n",mod->flow_id);
		break;

	case(TRACE):
		mod->flow_id            = temp->flow_id;
		mod->dst                = temp->dst;
		mod->flow_sport         = temp->flow_sport;
		mod->send_func          = send_trace;
		mod->params.trace.ftype = typenum;
		trace_parse(buffer, &mod->params.trace);
		temp->mod_flow          = mod;
		if(mod->params.trace.list_size == 0){
			free(mod);
			return(-4);
		}
		RUDEBUG7("flow_modify() - flow id=%ld modified\n",mod->flow_id);
		break;

	default:
		free(mod);
		RUDEBUG1("flow_modify() - MODIFY not supported for %32s flows\n",type);
		return(-5);
		break;
	}

	return 0;
}


/*
 * Count the START time in the future...
 */
int start_time(long int hour, long int min, long int sec)
{
	struct tm c_time;
	time_t    current;
	long int  temp = 0;
	long int  h    = hour;
	long int  m    = min;
	long int  s    = sec;

	if(h<0 || h>23 || m<0 || m>59 || s<0 || s>59){
		RUDEBUG1("start_time() - invalid START time\n");
		return(-1);
	}

	/* Get the current time and do the calculations... */
	time(&current);
	gettimeofday(&tester_start,NULL);
	memcpy(&c_time,localtime(&current),sizeof(struct tm));

	/* Set the struct for the real START time */
	if(s < c_time.tm_sec){
		m--;
		temp += (s+60)-c_time.tm_sec;
	} else {
		temp += s-c_time.tm_sec;
	}
	if(m < c_time.tm_min){
		h--;
		temp += ((m+60)-c_time.tm_min)*60;
	} else {
		temp += (m-c_time.tm_min)*60;
	}
	if(h < c_time.tm_hour){
		temp += ((h+24)-c_time.tm_hour)*3600;
	} else {
		temp += (h-c_time.tm_hour)*3600;
	}

	/* ... and finally add the difference to the START time. */
	tester_start.tv_sec += temp;

	RUDEBUG7("start_time() - (%02ld:%02ld:%02ld)-(%02d:%02d:%02d) = %ld sec\n",
	         hour,min,sec,c_time.tm_hour,c_time.tm_min,c_time.tm_sec,temp);
	return 0;
}


/*
 * The main parsing routine ( with some limitations/features ;)
 */
int read_cfg(FILE *infile)
{
	struct flow_cfg *temp  = NULL;
	struct flow_cfg *tmp   = NULL;
	int  errors            = 0;
	int  commands          = 0;
	int  read_lines        = 0;
	int  start_set         = 0;
	long int h,m,s,time,id = 0;
	int  tos               = 0;
	char buffer[1024],cmd[12];

	/* Read the file line by line and parse the commands */
	while(fgets(buffer,1023,infile) != NULL){
		read_lines++;

		if((buffer[0]=='#') || (buffer[0]=='\n')){
			RUDEBUG7("read_cfg() - read comment (line #=%d)\n",read_lines);
			commands++;
			continue;
		}

		if(strncasecmp(buffer,"START NOW",9) == 0){
			RUDEBUG7("read_cfg() - read START NOW (line #=%d)\n",read_lines);
			if(start_set != 0){
				errors--;
				RUDEBUG1("read_cfg() - START already set error\n");
						} else {
				gettimeofday(&tester_start,NULL);
				start_set = 1;
				commands++;
			}
			continue;
		}

		if(strncasecmp(buffer,"START",5) == 0){
			RUDEBUG7("read_cfg() - read START (line #=%d)\n",read_lines);
			if((3!=sscanf(buffer,"%*5s %ld:%ld:%ld",&h,&m,&s)) || (start_set!=0)){
				errors--;
				RUDEBUG1("read_cfg() - START argument/already set error\n");
			} else {
				if(start_time(h,m,s) == 0){
					start_set = 1;
					commands++;
				} else {
					errors--;
					RUDEBUG1("read_cfg() - START time error\n");
				}
			}
			continue;
		}

		/* check if START is set, so we can proceed... */
		if(start_set != 0){
			if(strncasecmp(buffer,"TOS",3) == 0){
				if(2 != sscanf(buffer,"%*3s %ld %i",&id,&tos)){
					errors--;
					RUDEBUG1("read_cfg() - invalid TOS clause (line #=%d)\n",read_lines);
					continue;
				}
				temp = find_flow_id(id);
				if (temp == NULL){
					errors--;
					RUDEBUG1("read_cfg() - invalid TOS flow (line #=%d)\n",read_lines);
				} else {
					if (tos < 0 || tos > 0xFF){
						errors--;
						RUDEBUG1("read_cfg() - invalid TOS value (line #=%d)\n",read_lines);
					} else {
						temp->tos = tos;
					}
				}
				continue;
			}
			if((3 != sscanf(buffer,"%ld %ld %10s",&time,&id,cmd)) || (time < 0)){
				errors--;
				RUDEBUG1("read_cfg() - invalid CMD (line #=%d)\n",read_lines);
			} else {
				RUDEBUG7("read_cfg() - read CMD %s (line #=%d)\n",cmd,read_lines);
				temp = find_flow_id(id);

				/* Check for the valid commands: ON, MODIFY and OFF */
				if((strncasecmp(cmd,"ON",2) == 0) && (temp == NULL)){
					if(flow_on(buffer) < 0){
						errors--;
						RUDEBUG1("read_cfg() - ON command parse error\n");
					} else { commands++; }
				} else if((strncasecmp(cmd,"OFF",3) == 0) && (temp != NULL)){
					if(flow_off(temp,time) < 0){
						errors--;
						RUDEBUG1("read_cfg() - OFF command parse error\n");
					} else { commands++; }
				} else if((strncasecmp(cmd,"MODIFY",6) == 0) && (temp != NULL)){
					if(flow_modify(temp,buffer) < 0){
						errors--;
						RUDEBUG1("read_cfg() - MODIFY command parse error\n");
					} else { commands++; }
				} else {
					errors--;
					RUDEBUG1("read_cfg() - invalid CMD line %d\n",read_lines);
				}
			}
			continue;
		}

		/* No match or START was not set before FLOW CMDs - ERROR */
		errors--;
		RUDEBUG1("read_cfg() - invalid CMD (line #=%d)\n", read_lines);
	}/* End of while() */

	/* Check that every flow block has Stop time set and set max_packet size */
	tmp = temp = head;
	while(temp != NULL){
		if((temp->flow_stop.tv_sec == 0) && (temp->flow_stop.tv_usec == 0)){
			errors--;
			RUDEBUG1("read_cfg() - no STOP time for flow id=%"PRIu32"\n",temp->flow_id);
		}

		switch(temp->params.ftype){
		case(CBR):
			if(temp->params.cbr.psize > max_packet_size){
				max_packet_size = temp->params.cbr.psize;
				RUDEBUG7("read_cfg() - max_packet_size set to %d\n",max_packet_size);
			}
			/* NEW in 0.50: Enable flow "stop" - ZERO transmission rate... */
			if(temp->params.cbr.rate == 0){
				temp->next_tx = temp->flow_stop;
				temp->next_tx.tv_sec++;
			}
			break;
		case(TRACE):
			if(temp->params.trace.max_psize > max_packet_size){
				max_packet_size = temp->params.trace.max_psize;
				RUDEBUG7("read_cfg() - max_packet_size set to %d\n",max_packet_size);
			}
			break;
		default:
			RUDEBUG1("read_cfg() - unknown flow type\n");
			errors--;
			break;
		}

		if(temp->mod_flow != NULL){ temp = temp->mod_flow; }
		else {
			temp = tmp->next;
			tmp  = temp;
		}
	}

	RUDEBUG7("read_cfg() - EXIT (cmd/err=%d/%d, lines read=%d)\n",
	         commands, errors, read_lines);

	if(errors != 0){ return errors; }
	return commands;
}
