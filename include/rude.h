/*****************************************************************************
 *   rude.h
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
#ifndef _RUDE_H
#define _RUDE_H

#include <netinet/in.h>  /* for struct sockaddr_in */
#include <sys/time.h>    /* for struct timeval     */
#include <stdint.h>

#define DNMAXLEN 128
#define TMAXLEN  32
#define PMINSIZE 24     /* Minimum accepted UDP-data field/packet size, in previouse version it was 20      */
#define PMINSIZE_V6 40	/* Minimum accepted UDP-data field/packet size for ipv6      */
#define PMAXSIZE 32768  /* Maximum accepted UDP-data field/packet size      */
#define MINDURAT 0.001  /* Minimum allowed flow duration in seconds (float) */

#define VERSION "0.8.3"

/*
 * Enumeration definition for different (known) flow types
 */
typedef enum {
  UNKNOWN = -1,
  CBR = 1,
  CONSTANT = 1,
  TRACE = 2
} f_type;


/*
 * Private struct for CONSTANT BIT RATE traffic
 */
struct cbr_params{
  f_type ftype;                        /* Flow TRAFFIC TYPE              */
  int    rate;                         /* Flow PACKET RATE - PACKAGE RATE per PERIOD */
  int    psize;                        /* Flow PACKET SIZE               */
  int 	 package_size;				   /* Flow NUMBER OF PACKETS IN ONE PACKAGE*/
  int    time_period;					   /* Flow TIME_PERIOD */		
};

/*
 * Private structs for TRACE based traffic
 */
struct trace_list{
  int    psize;                        /* XMITTED PACKET SIZE        */
  struct timeval wait;                 /* TIME TO WAIT FOT NEXT XMIT */
};

struct trace_params{
  f_type ftype;                        /* Flow TRAFFIC TYPE       */
  int          max_psize;              /* PRECALCULATED VALUE...  */
  unsigned int list_size;              /* # OF PACKETS IN TRACE   */
  unsigned int list_index;             /* CURRENT INDEX IN TRACE  */
  struct trace_list* list;             /* ACTUAL TRACE PARAMETERS */
};


/*
 * The main building block for flows
 */
struct flow_cfg {
  struct flow_cfg     *next;            /* Pointer to NEXT flow           */
  struct flow_cfg     *mod_flow;        /* Next action-block for the flow */
  struct sockaddr_storage dst;              /* Destination information        */
  int                 send_sock;	       /* Socket to be used by this flow */

	
  uint32_t            flow_id;          /* Flow IDENTIFICATION number     */
  uint16_t            flow_sport;       /* Flow SOURCE PORT number        */
  struct timeval      flow_start;       /* Absolute flow cmd START TIME   */
  struct timeval      flow_stop;        /* Absolute flow cmd END TIME     */
  struct timeval      next_tx;          /* Absolute next packet TX TIME   */

  void (*send_func)(struct flow_cfg*); /* TX function for this flow */

  int errors;                          /*                   */
  int success;                         /* Internal counters */
  int sequence_nmbr;                   /*                   */

  int tos;                             /* IP TOS byte if positive */
  char 				*localIf;			/* local interface to be used with multicast */	
  char prefferedVersion; 				/*preffered ip version(4 or 6)*/
  union {
    f_type              ftype;
    struct cbr_params   cbr;
    struct trace_params trace;
  } params;
};


/*
 * Wrapper structure that helps filling the "header" to the buffer
 */
struct udp_data{
  uint32_t sequence_number;
  uint32_t tx_time_seconds; 
  uint32_t tx_time_useconds; 
  uint32_t flow_id;
  struct sockaddr_storage dest_addr;
}__attribute__ ((packed));


/*
 * Structure used by the CRUDE
 */
struct crude_struct{
  uint32_t  rx_time_seconds;
  uint32_t  rx_time_useconds;
  //struct in6_addr  src_addr;
  uint32_t           pkt_size;
  struct sockaddr_storage src; 
	//unsigned short src_port;
  uint16_t dest_port;
};

 
/*
 * Debug print macros - neat isn't it :)
 */
#if (DEBUG > 0)
#  define RUDEBUG1(msg...) fprintf(stderr, ## msg)
#else
#  define RUDEBUG1(msg...) {}
#endif

#if (DEBUG > 6)
#  define RUDEBUG7(msg...) fprintf(stderr, ## msg)
#else
#  define RUDEBUG7(msg...) {}
#endif

/* Some macro definitions. Added for non-Linux systems :) */
#ifndef timeradd
#define timeradd(a, b, result)                           \
  do {                                                   \
    (result)->tv_sec = (a)->tv_sec + (b)->tv_sec;        \
    (result)->tv_usec = (a)->tv_usec + (b)->tv_usec;     \
    if ((result)->tv_usec >= 1000000)                    \
      {                                                  \
        ++(result)->tv_sec;                              \
        (result)->tv_usec -= 1000000;                    \
      }                                                  \
  } while (0)
#endif

#ifndef timespecsub
  #define	timespecsub(tsp, usp, vsp)					\
	do {								\
		(vsp)->tv_sec = (tsp)->tv_sec - (usp)->tv_sec;		\
		(vsp)->tv_nsec = (tsp)->tv_nsec - (usp)->tv_nsec;	\
		if ((vsp)->tv_nsec < 0) {				\
			(vsp)->tv_sec--;				\
			(vsp)->tv_nsec += 1000000000L;			\
		}							\
	} while (0)
#endif
  
  
#endif /* _RUDE_H */
