#!/usr/local/bin/perl
###############################################################################
#    crude_parse.pl - refines the output from CRUDE.
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
#         Calculate the time according to transmission (Tx) or reception
#         (Rx) timelabel. DEFAULT = Rx.
#########################################################################
$Parse::WTime = "Rx";

#########################################################################
# Prec = 1 | 10 | 100 | 1000
#        This is the "accuracy" for the calculated samples.
#        1  = sample is 1 second , 10 = sample is 1/10 of a second , ...
#        DEFAULT = 10 (i.e. 1/10 of a second per sample)
#########################################################################
$Parse::Prec = 10;

#########################################################################
# Debug     = Debug flag. DEFAULT = 0 (i.e. no debugging output)
#########################################################################
$Parse::Debug = 0;


#########################################################################
# INTERNAL VARIABLES AND THEIR USAGE:
#
# STime = Timelabels for the 1st sample (used to scale the "time-axis").
# CTime = Timelabels for the current sample.
# RTime = Reference timestamp (float).
#
# FSample, FTotal, FCount, FTimeS, FTimeR, FFTable, Loop, PLen, Step,
# LSpeed, Flow, Seq, Size, Retval, INPUT
#########################################################################
$Parse::PLen   = 0;
$Parse::Step   = 0;
$Parse::LSpeed = 0;
$Parse::InFile = "-";



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
    print "crude_parse.pl version 0.4, Copyright (C) 1999 Juha Laine and Sampo Saaristo\n";
    print "crude_parse.pl comes with ABSOLUTELY NO WARRANTY!\n";
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
	"usage: crude_parse.pl [-tx | -rx] [-prec=#] [-debug] input_file\n";
    
    foreach $_ (@ARGV) {
	if (/^-prec=(.*)/) {
            $Parse::Prec = $1;
	} elsif (/^-[Rr][Xx]/) {
	    $Parse::WTime = "Rx";
	} elsif (/^-[Tt][Xx]/) {
	    $Parse::WTime = "Tx";
	} elsif (/^-debug/) {
	    $Parse::Debug=1;
	} elsif (/^-$/ || ! /^-(.*)/) {
	    $Parse::InFile = $_;
	    $in_file_set = 1;
	    last;
	} else {
	    die "$usage_msg";
	}
    }
    
    if ($Parse::Prec != 1 && $Parse::Prec != 10 && $Parse::Prec != 100 && $Parse::Prec != 1000) {
        die "ERROR: invalid precision value: $Parse::Prec !!!\n";
    }
    
    if(! $in_file_set) {
        die "ERROR: no input file set!\n";
    }

    $Parse::PLen   = log($Parse::Prec)/log(10);
    $Parse::Step   = 10**(6-$Parse::PLen);
    $Parse::LSpeed = ($Parse::Prec * 8);

    print "Prec=$Parse::Prec PLen=$Parse::PLen Step=$Parse::Step LSpeed=$Parse::LSpeed WTime=$Parse::WTime\n"
	if $Parse::Debug;
}


###
##
###
sub time_calc
{
    my $time1;
    my $time2;

    die "ERROR: time_calc got wrong # of arguments!\n"
	if (scalar(@_) != 4);

    $time1  = "0.".substr($_[1],0,"$Parse::PLen");
    $time1 += $_[0];
    $time2  = "0.".substr($_[3],0,"$Parse::PLen");
    $time2 += $_[2];

    return($time2-$time1);
}


###
##
###
sub time_cmp
{
    my $time1r;
    my $time2r;

    die "ERROR: time_cmp got wrong # of arguments!\n"
	if (scalar(@_) != 4);

    $time1r = substr($_[1],0,"$Parse::PLen");
    $time2r = substr($_[3],0,"$Parse::PLen");

    print "$_[0] $_[1] $_[2] $_[3]\n" if $Parse::Debug;

    if($_[0] < $_[2]) { return(-1); }
    elsif($_[0] > $_[2]) { return(1);  }
    elsif($time1r < $time2r) { return(-1); }
    elsif($time1r > $time2r) { return(1);  }
    else { return 0; }
}


###
##
###
sub read_input
{
    my @STime;
    my @CTime;
    my $RTime;
    
    my @FSample;
    my @FTotal;
    my @FCount;
    my @FTimeS;
    my @FTimeR;
    my @FFTable;
    
    my $Loop = 0;
    my $Flow;
    my $Seq;
    my $Size;
    my $Retval;

    open(INPUT,"$Parse::InFile") ||
        die "ERROR: can't open input file $Parse::InFile!\n";

   while (<INPUT>) {
	if (/^ID/) {
	    if( $Parse::WTime =~ /Rx/ ) {
		($Flow, $Seq, $CTime[1], $CTime[2], $Size) =
		    /^ID\=(\d+) SEQ\=(\d+) SRC\=\S+ DST\=\S+ Tx\=\S+ Rx\=(\d+)\.(\d+) SIZE\=(\d+)$/;
	    } else {
		($Flow, $Seq, $CTime[1], $CTime[2], $Size) =
		    /^ID\=(\d+) SEQ\=(\d+) SRC\=\S+ DST\=\S+ Tx\=(\d+)\.(\d+) Rx\=\S+ SIZE\=(\d+)$/;
	    }

#
# Initialize the "Zero" timesample during the first loop...
#
	    if( $Loop == 0 ) {
		$STime[1] = $CTime[1];
		$STime[2] = $CTime[2];
	    }

#
# Open the file for output. If the entry was/is the 1st for this flow,
# truncate the file, if one already exists with the same name. Otherwise
# open the file in append mode.
#
            if( $FFTable[$Flow] == 0 ){
                open( OUTPUT, ">data.$Flow") ||
                    die "ERROR: can't create outputfile data.$Flow!\n";
                $FTimeS[$Flow]  = $CTime[1];
                $FTimeR[$Flow]  = $CTime[2];
		$FSample[$Flow] = 0;
                $FCount[$Flow]  = 0;
                $FTotal[$Flow]  = 0;
                $FFTable[$Flow] = 1;
		$RTime          = time_calc("$STime[1]","$STime[2]",
					    "$CTime[1]","$CTime[2]");
                print OUTPUT sprintf("%d\t%.4f\t%d\t%d\t%d\t%d\n",
                                     $Flow,$RTime,0,0,0,0);
            } else {
                open( OUTPUT, ">>data.$Flow") ||
                    die "ERROR: can't open outputfile data.$Flow!\n";
            }

#########################################################################
	    
	    $Retval =  time_cmp("$CTime[1]","$CTime[2]",
				"$FTimeS[$Flow]","$FTimeR[$Flow]");

	    if( $Retval < 0 ) {
		print "ERROR: timelabels do not match!\n";
	    } elsif( $Retval > 0 ){
                while( $Retval > 0 ){
                    $RTime = time_calc("$STime[1]","$STime[2]",
                                       "$FTimeS[$Flow]","$FTimeR[$Flow]");
                    print OUTPUT sprintf("%d\t%.4f\t%d\t%d\t%d\t%d\n",
                                         $Flow,$RTime,
					 $FSample[$Flow]*$Parse::LSpeed,
                                         $FTotal[$Flow],$FCount[$Flow],$Seq);
                    $FSample[$Flow] = 0;
                    $FTimeR[$Flow]  = sprintf("%06d",
					      $FTimeR[$Flow]+$Parse::Step);

                    if ($FTimeR[$Flow] > 999999){
                        $FTimeR[$Flow] = sprintf("%06d",$FTimeR[$Flow]-1000000);
                        $FTimeS[$Flow] += 1;
                    }

                    $Retval = time_cmp("$CTime[1]","$CTime[2]",
                                       "$FTimeS[$Flow]","$FTimeR[$Flow]");
		}
		$FSample[$Flow] = $Size;
		$FTotal[$Flow] += $Size;
		$FCount[$Flow] += 1;
	    } else {
		$FSample[$Flow] += $Size;
		$FTotal[$Flow]  += $Size;
		$FCount[$Flow]  += 1;
	    }
	    $Loop          += 1;
	    close OUTPUT;
	} else {
	    print "ERROR: invalid input line!\n";
	}
    } # End Of While
#
# FIXME: Print out the left over data (form last sample, that is not finished)
#
}
