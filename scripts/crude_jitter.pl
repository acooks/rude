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
    print "crude_jitter.pl version 0.5, Copyright (C) 1999 Juha Laine and Sampo Saaristo\n";
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

    print STDERR "WTime=$Jitter::WTime InFile=$Jitter::InFile\n"
	if $Jitter::Debug;
}


###
##
###
sub read_input
{
    my ($tmp1,$tmp2,$tmp3,$flowcnt,$seqcnt);
    my ($Flow,$Seq,$Time,$Oks,$Errs,$Lost);

    my @InArray  = ();
    my @MaxSeq   = ();
    my @MinSeq   = ();

    open(INPUT,"$Jitter::InFile") ||
	die "ERROR: can't open input file $Jitter::InFile!\n";

    $Oks=$Errs=0;
    while(<INPUT>){
	if (/^ID/) {
	    if( $Jitter::WTime =~ /Rx/ ) {
		($Flow, $Seq, $Time) = /^ID\=(\d+) SEQ\=(\d+) .* Rx\=(\S+) /;
	    } else {
		($Flow, $Seq, $Time) = /^ID\=(\d+) SEQ\=(\d+) .* Tx\=(\S+) /;
	    }
	    $InArray[$Flow][$Seq] = [$Time];
	    $Oks++;
	    if (! defined $MaxSeq[$Flow] || $Seq > $MaxSeq[$Flow]) {
		$MaxSeq[$Flow] = $Seq;
	    }
	    if (! defined $MinSeq[$Flow] || $Seq < $MinSeq[$Flow]) {
		$MinSeq[$Flow] = $Seq;
	    }
	} else {
	    $Errs++;
	}
    }
    close(INPUT);

    # Do ERROR CHECKING...
    die "ERROR: no acceptable input lines in file $Jitter::InFile!\n"
	if ($Oks == 0);
    print STDERR "Input line errors/OK=$Errs/$Oks in file $Jitter::InFile\n"
	if $Jitter::Debug;

    # Print out the gathered DATA in two loops. The 1st loop
    # goes through each flow and the inner/2nd loop prints
    # out the packet level information for the flow.

    $flowcnt=$seqcnt=$Errs=$Oks=$Time=$Lost=0;
    for $tmp1 (@InArray) {
	if (defined @$tmp1) {
	    print STDERR "flow=$flowcnt MINSEQ=$MinSeq[$flowcnt] MAXSEQ=$MaxSeq[$flowcnt]\n"
		if $Jitter::Debug;
	    open(OUTPUT,">$Jitter::InFile.jitter.$flowcnt") ||
		die "ERROR: can't open output file $Jitter::InFile.jitter.$flowcnt!\n";

	    for $tmp2 (@$tmp1) {
		if (defined $InArray[$flowcnt][$seqcnt] && defined $InArray[$flowcnt][$seqcnt - 1]) {
		    if ($seqcnt == 0) { $tmp3 = 0.0; }
		    else { $tmp3 = ($InArray[$flowcnt][$seqcnt][0] - $InArray[$flowcnt][$seqcnt-1][0]); }
		    print OUTPUT sprintf("%d\t%.6f\n",$seqcnt,$tmp3);
		    $Oks++;
		    if ($tmp3 > $Time) { $Time = $tmp3; }
		} else {
		    # Either this packet or the previous packet was lost -> ERROR.
		    print OUTPUT sprintf("%d\t%.6f\n",$seqcnt,-1.0);
		    if( ! defined $InArray[$flowcnt][$seqcnt] ) { $Lost++; }
		    else { $Errs++; }
		}
		$seqcnt++;
	    }

	    close(OUTPUT);
	    print STDERR "flow=$flowcnt, lost/errors/OK=$Lost/$Errs/$Oks MAX_JITTER=",
	    sprintf("%.6f\n",$Time);
	}
	$flowcnt++; $seqcnt=$Errs=$Oks=$Time=$Lost=0;
    }
}
