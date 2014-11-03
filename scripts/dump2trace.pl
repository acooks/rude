#!/usr/bin/perl
###############################################################################
#    dump2trace.pl - converst tcpdump-output to TRACE file
#                    No documentation (yet) available...
#
#    Copyright (C) 1999 Juha Laine and Sampo Saaristo
#
#    This program is free software; you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation; either version 2 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program; if not, write to the Free Software
#    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
#
#    Authors:      Juha Laine     <james@cs.tut.fi>
#                  Sampo Saaristo <sambo@cc.tut.fi>
#
###############################################################################
use strict;

#########################################################################
# Debug = Debug flag. DEFAULT = 0 (i.e. no debugging output)
#########################################################################
$Trace::Debug  = 0;

#########################################################################
# InFile = Name of input file. "-" == STDIN
#########################################################################
$Trace::InFile = "-";

#########################################################################
#########################################################################

parse_cmdline();
read_input();
exit 0;

#########################################################################
#########################################################################

###
##
###
sub parse_cmdline
{
    my $in_file_set = 0;
    my $usage_msg   =
	"usage: dump2trace.pl [-tx | -rx] [-debug] input_file\n";

    foreach $_ (@ARGV) {
	if (/^-debug/) {
	    $Trace::Debug=1;
	} elsif (/^-$/ || ! /^-(.*)/) {
	    $Trace::InFile = $_;
	    $in_file_set = 1;
	    last;
	} else {
	    die "$usage_msg";
	}
    }

    if(! $in_file_set) {
	die "ERROR: no input file set!\n";
    }

    print "InFile=$Trace::InFile\n" if $Trace::Debug;
}


###
##
###
sub read_input
{
    my $StartSec  = 0;
    my $StartUsec = 0;
    my $Sec       = 0;
    my $Usec      = 0;
    my $Size      = 0;
    my $Loop      = 0;

    open(INPUT,"$Trace::InFile") ||
	die "ERROR: can't open input file $Trace::InFile!\n";

    while (!eof(INPUT)){

	$_ = <INPUT>;

	($Sec, $Usec, $Size) = /^(\d+)\.(\d+) .* udp (\d+).*$/;

	if( $Loop == 0 ) {
	    $StartSec  = $Sec;
	    $StartUsec = $Usec;
	    $Loop++;
	}

	$Usec -= $StartUsec;
	if( $Usec < 0 ){
	    $Usec += 1000000;
	    $Sec  -= 1;
	}
	$Sec -= $StartSec;

	if( $Sec < 0 || $Usec < 0 || $Size <= 0){ next; }
	else { print sprintf("%d %d.%06d\n",$Size,$Sec,$Usec); }
    } # End Of While
}
