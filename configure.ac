# $Id: configure.in,v 1.1 2005/05/31 07:12:38 ubik Exp $
#
# autoconf/configure.in for RUDE and CRUDE
#
# Authors: Juha Laine     <james@cs.tut.fi>
#          Sampo Saaristo <sambo@cc.tut.fi>
#
# Copyright (C) 1999 Juha Laine, Tampere, Finland
#                    All rights reserved
#
##############################################################################

# Process any command-line arguments and find the source code directory
AC_INIT([RUDE and CRUDE], m4_esyscmd([grep VERSION include/rude.h | cut -d\" -f2 | tr -d '\n']))

# Set the auxilary directory, make and install
AC_CONFIG_AUX_DIR(autoconf)
AC_PROG_MAKE_SET
AC_PROG_INSTALL

# Make AX_OUTPUT() create following files with the #define statements
AC_CONFIG_HEADER(include/config.h:autoconf/config.hin)

# Check the machine type
AC_CANONICAL_HOST

# If CC is not set, set it to "gcc" (if found) or "cc"
# ...and after that get rid of the debugging flag '-g'
AC_PROG_CC

my_flags="$CFLAGS"
CFLAGS=
for my_opt in $my_flags; do
	if test "$my_opt" != "-g"; then
		CFLAGS="$CFLAGS $my_opt"
	fi
done

# Check if the system is POSIXized
AC_ISC_POSIX

# Check some required headers
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_HEADER_TIME
AC_CHECK_HEADERS(sys/time.h unistd.h signal.h)

# Check required structures and types
AC_C_CONST
AC_STRUCT_TM
AC_TYPE_UID_T

# Check required structures and types
AC_CHECK_FUNCS(gettimeofday select strerror munlockall sigaction)

AC_CHECK_FUNC(gethostbyname,[true],
	AC_SEARCH_LIBS(gethostbyname, nsl))

AC_CHECK_FUNC(socket,[true],
	AC_SEARCH_LIBS(socket, socket nsl))

AC_CHECK_FUNC(inet_addr,[true],
	AC_SEARCH_LIBS(inet_addr, xnet))

AC_CHECK_FUNC(sched_setscheduler,[true],
	AC_SEARCH_LIBS(sched_setscheduler, posix4))

AC_CHECK_FUNC(pow,[true],
	AC_SEARCH_LIBS(pow, m))

#########################
# Private configuration #
#########################

### Do machine dependent stuff right here
#case "$host" in
#    *-*-solaris*)
#    LIBS="-lsocket -lnsl $LIBS"
#    ;;
#
#    *)
#    ;;
#esac

### Enable debugging code
AC_MSG_CHECKING([if we should enable -g])
AC_ARG_ENABLE(debug,
[  --enable-debug          Enable compiler option -g],
[ case "$enableval" in
  yes)
	AC_MSG_RESULT(yes)
	CFLAGS="-g $CFLAGS"
	;;
  *)
	AC_MSG_RESULT(no)
	;;
  esac ], AC_MSG_RESULT(no))

### Enable -Wall option for compiler ???
AC_MSG_CHECKING([if we should enable -Wall])
AC_ARG_ENABLE(wall,
[  --enable-wall           Enable compiler option -Wall (gcc only)],
[ if test "$CC" = "gcc"; then
	AC_MSG_RESULT(yes)
	CFLAGS="$CFLAGS -Wall"
  else
	AC_MSG_WARN(Your compiler is not gcc - -Wall not enabled.)
  fi],AC_MSG_RESULT(no))

### Enable debugging output for program ???
AC_MSG_CHECKING([if we print debugging info])
AC_ARG_WITH(debug-lvl,
[  --with-debug-lvl=[0-7]  Set program debugging output level],
[ if test $withval -ge 0; then
	AC_MSG_RESULT(yes)
	AC_DEFINE_UNQUOTED(DEBUG, $withval, Amount of DEBUGGING output)
  else
	AC_MSG_WARN(Invalid value $withval.)
  fi],AC_MSG_RESULT(no))

###################
# Make it happen. #
###################
CFLAGS="$CFLAGS -I../include"
AC_OUTPUT(Makefile:autoconf/Makefile.in rude/Makefile crude/Makefile, \
	echo timestamp > include/stamp.h)
