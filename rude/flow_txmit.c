/*****************************************************************************
 *   flow_txmit.c
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sched.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


/* Introduce our global variables */
extern struct flow_cfg *head;
extern struct udp_data *data;
extern char            *buffer;


/*
 * wait_for_xmit(): Wait for certain period of time in busy-loop
 */
__inline__ void wait_for_xmit(struct timeval *target, struct timeval *now)
{
  while(1){
    gettimeofday(now,NULL);
    if(timercmp(now,target,<)){
      /* FIXME: check if the timegap is large => use [u | nano]sleep() */
      continue;
    }
    return;
  };

  /* Next line is for compiler :) */
  return;
} /* wait_for_xmit() */


/*
 * send_cbr(): Transmission & calculation function for CBR flows
 */
void send_cbr(struct flow_cfg *flow)
{
  struct cbr_params *p = &flow->params.cbr;
  struct timeval now   = {0,0};
  int written          = 0;

  /* Do the initialization and wait if necessary */
  data->dest_addr       = flow->dst.sin_addr.s_addr;
  data->flow_id         = htonl(flow->flow_id);
  data->sequence_number = htonl(flow->sequence_nmbr);
  wait_for_xmit(&flow->next_tx, &now);

  /* ...and fill in the rest of the data */
  data->tx_time_seconds  = htonl(now.tv_sec);
  data->tx_time_useconds = htonl(now.tv_usec);

  /* Write the data to the socket and check the result for errors */
  /* Increase the status and sequence number counters.            */
  written = sendto(flow->send_sock, buffer, p->psize, 0,
		   (struct sockaddr *)&flow->dst,sizeof(struct sockaddr));
  if(written != p->psize){
    flow->errors++;
    RUDEBUG7("send_cbr(): sendto() error for flow=%ld (%d/%d): %s\n",
	     flow->flow_id,written,p->psize,strerror(errno));
  } else {
    flow->success++;
  }
  flow->sequence_nmbr++;

  /* Caclulate and store the next transmission time */
  flow->next_tx.tv_usec += 1000000.0/p->rate;
  if(flow->next_tx.tv_usec >= 1000000){
    flow->next_tx.tv_usec -= 1000000;
    flow->next_tx.tv_sec  += 1;
  }

  return;
}


/*
 * send_trace(): Transmission & calculation function for TRACE flows
 */
void send_trace(struct flow_cfg *flow)
{
  struct trace_params *p = &flow->params.trace;
  int xmit_size          = p->list[p->list_index].psize;
  struct timeval now     = {0,0};
  int written            = 0;

  /* Do the initialization and wait if necessary... */
  data->dest_addr       = flow->dst.sin_addr.s_addr;
  data->flow_id         = htonl(flow->flow_id);
  data->sequence_number = htonl(flow->sequence_nmbr);
  wait_for_xmit(&flow->next_tx, &now);

  /* ...and fill in the rest of the data */
  data->tx_time_seconds  = htonl(now.tv_sec);
  data->tx_time_useconds = htonl(now.tv_usec);

  /* Write the data to the socket and check the result for errors */
  /* Increase the status and sequence number counters.            */
  written = sendto(flow->send_sock, buffer, xmit_size, 0,
		   (struct sockaddr *)&flow->dst,sizeof(struct sockaddr));
  if(written != xmit_size){
    flow->errors++;
    RUDEBUG7("send_trace(): sendto() error for flow=%ld (%d/%d): %s\n",
	     flow->flow_id,written,xmit_size,strerror(errno));
  } else {
    flow->success++;
  }
  flow->sequence_nmbr++;

  /* Caclulate the next transmission time & other stuff */
  now = flow->next_tx;
  timeradd(&now,&p->list[p->list_index].wait,&flow->next_tx);
  p->list_index++;
  p->list_index %= p->list_size;

  return;
}
