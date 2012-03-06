# Generated automatically from Makefile.in by configure.
# $Id: Makefile.in,v 1.2 2005/06/30 12:02:49 juranek Exp $
#
# Makefile.in for CRUDE
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
MAN8     = crude.8

CRUDE_OBJS = main.o ../rude/mcast.o

##############################################################################
# Rules for make
##############################################################################
.c.o:
	$(CC) $(CFLAGS) $(DEFS) -c -o $*.o $<

all:	crude

crude:	$(CRUDE_OBJS)
	$(CC) $(CFLAGS) $(LIBS) $(DEFS) $(LDFLAGS) -o crude $(CRUDE_OBJS)

clean:
	rm -f *~ *.o crude

distclean:
	rm -f *~ *.o crude Makefile

install: crude
	@if [ ! -d $(bindir) ]; then mkdir -m 0755 $(bindir); fi;
	$(INSTALL) crude $(bindir)
	@if [ ! -d $(mandir) ]; then mkdir -m 0755 $(mandir); fi;
	$(INSTALL) crude.8 $(mandir)
##############################################################################
