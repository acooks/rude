#!/usr/local/bin/perl
###############################################################################
#    crude_jitter.pl - refines the output from CRUDE.
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
# WTime = Tx | Rx 
#         Calculate the jitter according to transmission (Tx) or
#         reception (Rx) timelabel. DEFAULT = Rx.
#########################################################################
$Jitter::WTime  = "Rx";

#########################################################################
# Debug = Debug flag. DEFAULT = 0 (i.e. no debugging output)
#########################################################################
$Jitter::Debug  = 0;

#########################################################################
# InFile = Name of input file. "-" == STDIN
#########################################################################
$Jitter::InFile = "-";


#########################################################################
#########################################################################

print_info();
parse_cmdline();
read_input();
exit 0;

#########################################################################
#########################################################################

###
##
###
sub print_info
{
    print "crude_jitter.pl version 0.4, Copyright (C) 1999 Juha Laine and Sampo Saaristo\n";
    print "crude_jitter.pl comes with ABSOLUTELY NO WARRANTY!\n";
    print "This is free software, and you are welcome to redistribute it\n";
    print "under GNU GENERAL PUBLIC LICENSE Version 2.\n";
}

###
##
###
sub parse_cmdline
{
    my $in_file_set = 0;
    my $usage_msg   =
	"usage: crude_jitter.pl [-tx | -rx] [-debug] input_file\n";

    foreach $_ (@ARGV) {
	if (/^-[Rr][Xx]/) {
	    $Jitter::WTime = "Rx";
	} elsif (/^-[Tt][Xx]/) {
	    $Jitter::WTime = "Tx";
	} elsif (/^-debug/) {
	    $Jitter::Debug=1;
	} elsif (/^-$/ || ! /^-(.*)/) {
	    $Jitter::InFile = $_;
	    $in_file_set = 1;
	    last;
	} else {
	    die "$usage_msg";
	}
    }

    if(! $in_file_set) {
	die "ERROR: no input file set!\n";
    }

    print "WTime=$Jitter::WTime InFile=$Jitter::InFile\n"
	if $Jitter::Debug;
}


###
##
###
sub read_input
{
    my @PTime;
    my @FSeq;
    my @FErr;
    my @Loop;
    my $CTime;
    my $Flow;
    my $Flowx;
    my $Seq;
    my $POS_before = 0;
    my $POS_after  = 0;

    open(INPUT,"$Jitter::InFile") ||
	die "ERROR: can't open input file $Jitter::InFile!\n";

    while (!eof(INPUT)){

	$POS_before = tell(INPUT);
	$_ = <INPUT>;
	$POS_after  = tell(INPUT);

	if (/^ID/) {
	    if( $Jitter::WTime =~ /Rx/ ) {
		($Flow, $Seq, $CTime) = /^ID\=(\d+) SEQ\=(\d+) SRC\=\S+ DST\=\S+ Tx\=\S+ Rx\=(\S+) SIZE\=\d+$/;
	    } else {
		($Flow, $Seq, $CTime) = /^ID\=(\d+) SEQ\=(\d+) SRC\=\S+ DST\=\S+ Tx\=(\S+) Rx\=\S+ SIZE\=\d+$/;
	    }

	    print "FLOW=$Flow Seq=$Seq CTime=$CTime\n" if $Jitter::Debug;

	    if( $Loop[$Flow] == 0 ) {
		$FSeq[$Flow]   = $Seq;
		$Loop[$Flow]  += 1;
		$PTime[$Flow]  = $CTime;
                open( OUTPUT, ">jitter.$Flow") ||
                    die "ERROR: can't create outputfile jitter.$Flow!\n";
		print OUTPUT sprintf("%d\t%.6f\n",$Seq,0);
		close OUTPUT;
		next;
	    }

	    open( OUTPUT, ">>jitter.$Flow") ||
		die "ERROR: can't open outputfile jitter.$Flow!\n";

#########################################################################

	    if( $Seq < ($FSeq[$Flow] + 1) ){
		print "ERROR: skipping old packet...\n" if $Jitter::Debug;
		close OUTPUT;
		next;
	    } elsif( $Seq > ($FSeq[$Flow] + 1) ){
		print "ERROR: packet sequence anomaly!\n" if $Jitter::Debug;

		# Locate the correct entry and if it exists read and
		# process it.

		seek(INPUT,0,0);
		while(<INPUT>){
		    if (/^ID/) {
			if( $Jitter::WTime =~ /Rx/ ) {
			    ($Flowx, $Seq, $CTime) = /^ID\=(\d+) SEQ\=(\d+) SRC\=\S+ DST\=\S+ Tx\=\S+ Rx\=(\S+) SIZE\=\d+$/;
			} else {
			    ($Flowx, $Seq, $CTime) = /^ID\=(\d+) SEQ\=(\d+) SRC\=\S+ DST\=\S+ Tx\=(\S+) Rx\=\S+ SIZE\=\d+$/;
			}

			if(($Flowx != $Flow) || ($Seq != ($FSeq[$Flow] + 1))){
			    next;
			} else {
			    print OUTPUT sprintf("%d\t%.6f\n",
						 $Seq,($CTime-$PTime[$Flow]));
			    $FSeq[$Flow]   = $Seq;
			    $Loop[$Flow]  += 1;
			    $PTime[$Flow]  = $CTime;
			    last;
			}
		    }
		}
		seek(INPUT,$POS_before,0);

		if(($Flowx != $Flow) || ($FSeq[$Flow] != $Seq)){
		    print sprintf("ERROR: Packet#=%d for flow %d not found!\n",
				  ($FSeq[$Flow]+1),$Flow);
		    $FSeq[$Flow]  += 1;
		    $FErr[$Flow]  -= 1;
		    print OUTPUT sprintf("%d\t%.6f\t%d\n",
					 $FSeq[$Flow],"0.0",$FErr[$Flow]);
		}
	    } else {
		print OUTPUT sprintf("%d\t%.6f\n",$Seq,($CTime-$PTime[$Flow]));
		$FSeq[$Flow]   = $Seq;
		$Loop[$Flow]  += 1;
		$PTime[$Flow]  = $CTime;
	    }
	    close OUTPUT;
	} else {
	    print "ERROR: invalid input line!\n";
	}
    } # End Of While
}
