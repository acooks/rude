# Generated automatically from Makefile.in by configure.
# $Id: Makefile.in,v 1.2 2005/06/30 12:02:49 juranek Exp $
#
# Makefile.in for RUDE
#
# Authors: Juha Laine     <james@cs.tut.fi>
#	   Sampo Saaristo <sambo@cc.tut.fi>
#
# Copyright (C) 1999 Juha Laine, Tampere, Finland
#                    All rights reserved
#
##############################################################################
SHELL = /bin/sh

prefix        = /usr/local
exec_prefix   = ${prefix}

bindir        = ${exec_prefix}/bin
mandir        = ${prefix}/man/man8
sbindir       = ${exec_prefix}/sbin
srcdir        = .

INSTALL  = /usr/bin/install -c -o root -g root -m 0755

CC       = gcc
CFLAGS   =  -O2 -I../include
LDFLAGS  = 
DEFS     = -DHAVE_CONFIG_H
LIBS     = -lm 
MAN8     = rude.8

RUDE_OBJS = flow_cntl.o flow_txmit.o main.o parse.o mcast.o

##############################################################################
# Rules for make
##############################################################################
.c.o:
	$(CC) $(CFLAGS) $(DEFS) -c -o $*.o $<

all:	rude

rude:	$(RUDE_OBJS)
	$(CC) $(CFLAGS) $(LIBS) $(DEFS) $(LDFLAGS) -o rude $(RUDE_OBJS)

clean:
	rm -f *~ *.o rude

distclean:
	rm -f *~ *.o rude Makefile

install: rude
	@if [ ! -d $(bindir) ]; then mkdir -m 0755 $(bindir); fi;
	$(INSTALL) rude $(bindir)
	@if [ ! -d $(mandir) ]; then mkdir -m 0755 $(mandir); fi;
	$(INSTALL) rude.8 $(mandir)
 

##############################################################################
