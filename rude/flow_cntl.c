/*****************************************************************************
 *   flow_cntl.c
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

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>


/* Introduce our global variables */
extern char            *buffer;
extern struct flow_cfg *head;
extern struct flow_cfg *done;


/*
 * remove_flow() - This function links the given flow_cfg structure
 *                 to the done list, which holds the already processed
 *                 blocks. It processes only the "active" block of the flow.
 */
static int remove_flow(struct flow_cfg *to_remove)
{
  struct flow_cfg *flow = head;
  struct flow_cfg *prev = NULL;

  /* Look up the "to_remove" and the previous flow object. */
  /* Check the results for possible "overflow" error(s).   */
  while((flow != to_remove) && (flow != NULL)){
    prev = flow;
    flow = flow->next;
  }
  if(flow == NULL){
    RUDEBUG1("remove_flow(): flow to remove was not found!\n");
    return 0;
  }

  /* Initialize the next block for this flow - if any */
  if((flow = to_remove->mod_flow) != NULL){
    flow->next          = to_remove->next;
    flow->send_sock     = to_remove->send_sock;
    flow->sequence_nmbr = to_remove->sequence_nmbr;
  } else {
    if(to_remove->send_sock>0){
      close(to_remove->send_sock);
    }
    to_remove->send_sock = 0;
    flow = to_remove->next;
  }

  /* Unlink the object "to_remove" from the active list */
  if(prev == NULL){ head = flow; }
  else { prev->next = flow; }

  /* Link the object to the "done" list and return SUCCESS */
  /* FIXME: to_remove->mod_flow = NULL; ???? */
  to_remove->next = done;
  done = to_remove;
  RUDEBUG7("remove_flow(): block removed from flow id=%ld\n",done->flow_id);
  return 1;
} /* remove_flow() */


/*
 * find_next() - This function locates the next "active" block/flow which
 *               has the turn to send the next packet. It also removes
 *               the already processed "active" blocks (if any) from the
 *               list.
 */
struct flow_cfg *find_next(void)
{  
  struct flow_cfg *flow   = head;
  struct flow_cfg *prev   = NULL;
  struct flow_cfg *target = NULL;
  struct timeval   now;
  
  /* Get the current time */
  gettimeofday(&now, NULL);

  /* Find the next flow from which should send the packet */
  while(flow){
    /* Remove the flows that are already "done". The remove_flow() */
    /* function modifies the active and passive lists (pointed by  */
    /* head and done respectively)...                              */
    if(timercmp(&flow->flow_stop,&now,<) ||
       timercmp(&flow->next_tx,&flow->flow_stop,>)){
      remove_flow(flow);
      if(prev != NULL){ flow = prev->next; }
      else { flow = head; }
      continue;
    }

    /* Mark the current flow as target, if certain conditions are met... */
    if((target == NULL) || (timercmp(&flow->next_tx,&target->next_tx,<))){
      target = flow;
    }
    prev = flow;
    flow = flow->next;
  }

  return target;
} /* find_next() */


/*
 * clean_up() - This function frees the reserved resources and empties
 *              the active and passive lists. This function requires
 *              attention if one adds new flow types to this program...
 */
void clean_up(void)
{  
  struct flow_cfg *tmp1;

  /*
   * Clear the active flow list
   */
  while((tmp1 = head) != NULL){
    /* Close the open connections (if any) */
    if(head->send_sock > 0){
      close(head->send_sock);
      head->send_sock = 0;
    }

    /* Unlink this flow and update the pointers before destruction */
    if(head->mod_flow != NULL){
      head       = head->mod_flow;
      head->next = tmp1->next;
    } else {
      head = head->next;
    }

    /*** DO THE EXTRA CLEAN-UP HERE ***/
    if(tmp1->params.ftype == TRACE){
      free(tmp1->params.trace.list);
    }
    /*** DO THE EXTRA CLEAN-UP HERE ***/

    /* Clean up the rest of this block/flow */
    free(tmp1);
  }

  /*
   * Clear the passive (already done) list
   */
  while((tmp1 = done) != NULL){
    /* Unlink the block from the list */
    done = done->next;
    
    /*** DO THE EXTRA CLEAN-UP HERE ***/
    if(tmp1->params.ftype == TRACE){
      free(tmp1->params.trace.list);
    }
    /*** DO THE EXTRA CLEAN-UP HERE ***/

    /* Clean up the rest of this block/flow */
    free(tmp1);
  }

  /* Free the globally reserver memory */
  free(buffer);

  RUDEBUG7("clean_up(): DONE\n");
  return;
} /* clean_up() */
