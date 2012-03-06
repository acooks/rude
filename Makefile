# Generated automatically from Makefile.in by configure.
# $Id: Makefile.in,v 1.1 2005/05/31 07:12:38 ubik Exp $
#
# autoconf/Makefile.in - the main Makefile template for RUDE and CRUDE
#
# Authors: Juha Laine     <james@cs.tut.fi>
#	   Sampo Saaristo <sambo@cc.tut.fi>
#
# Copyright (C) 1999 Juha Laine, Tampere, Finland
#                    All rights reserved
#
##############################################################################
SHELL   = /bin/sh
SUBDIRS = rude crude


all:
	@for i in $(SUBDIRS); do \
		(cd $$i && $(MAKE) all) \
	done

clean:
	rm -f *~ config.cache config.log config.status
	@for i in $(SUBDIRS); do \
		(cd $$i && $(MAKE) clean) \
	done

distclean:
	rm -f *~ config.cache config.log config.status configure Makefile
	rm -f doc/*~ autoconf/config.hin include/config.h include/stamp.h
	@for i in $(SUBDIRS); do \
		(cd $$i && $(MAKE) distclean) \
	done

rude:
	cd rude && $(MAKE) all

crude:
	cd crude && $(MAKE) all

install:
	@for i in $(SUBDIRS); do \
		(cd $$i && $(MAKE) install) \
	done

##############################################################################
# Rules for autoconfiguration file REmaking
##############################################################################
update: include/stamp.h

include/stamp.h: autoconf/config.hin configure autoconf/Makefile.in rude/Makefile.in crude/Makefile.in
	./configure

autoconf/config.hin: autoconf/acconfig.h
	@autoheader -l ./autoconf
	@touch autoconf/config.hin
	@echo "****************************"
	@echo "* header templates updated *"
	@echo "****************************"

configure: autoconf/configure.in
	rm -f config.*
	@autoconf autoconf/configure.in > configure; chmod 755 configure
	@echo "****************************"
	@echo "* configure script updated *"
	@echo "****************************"

##############################################################################
