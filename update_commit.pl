#!/usr/bin/perl
# Copyright 2004 İsmail Dönmez <ismail.donmez@boun.edu.tr>
# Licensed under GPL v2 or later at your option
#
# Script will parse CVS/Entries file and find the latest modification date


use strict;
use File::Find;

use vars qw/*name *find/;
*name = *File::Find::name;
*find = *File::Find::find;

our $branch = "CVS HEAD";
our $max_date = "0 Jan 0000 00:00:00";
our %months = ( 
    "Jan"=>0,
    "Feb"=>1,
    "Mar"=>2,
    "Apr"=>3,
    "May"=>4,
    "Jun"=>5,
    "Jul"=>6,
    "Aug"=>7,
    "Sep"=>8,
    "Oct"=>8,
    "Nov"=>10,
    "Dec"=>11
);

sub process
{
    if($name =~ /Entries$/ ) {
	parse($name);
    }

}

sub parse
{
    my $entries = shift;
    my $path = $ENV{"PWD"};
    $entries = $path.'/'.$entries;

    open(ENTRIES,$entries) or die "Can't open file ",$entries ;

    while(<ENTRIES>) {
	chomp;
	if( $_ !~ /^D\/*/ ) {
	    my @arg_list = split('/',$_);
	    $arg_list[3] =~ s/(.*)\+(.*)/\2/;
	    my @date_args = split(" ",$arg_list[3]);
	    
	    $arg_list[5] =~ s/^T//;
	    compare_dates($date_args[2],$date_args[1],$date_args[3],$date_args[4],$arg_list[5]);
	}
    }
}

sub compare_dates
{
    my $day = shift;
    my $month = shift;
    my $time = shift;
    my $year = shift;
    my $file_branch = shift;

    
    if( $file_branch =~ /\w+/ )
    {
	$branch = $file_branch;
    }

    my @max_date_args = split(" ",$max_date);
    my @max_time_args = split(":",$max_date_args[3]);
    my @time_args = split(":",$time);

    $max_date_args[1] = $months{$max_date_args[1]};
    my $bmonth = $month;
    $month = $months{$month};
  
    if( $year > $max_date_args[2] ) {
	$max_date = $day." ".$bmonth." ".$year." ".$time." ".$branch;
    }
    elsif( $year == $max_date_args[2] && $month > $max_date_args[1] ){
	$max_date = $day." ".$bmonth." ".$year." ".$time." ".$branch;
    }
    elsif( $year == $max_date_args[2] && $month == $max_date_args[1] && $day > $max_date_args[0] ){
	$max_date = $day." ".$bmonth." ".$year." ".$time." ".$branch;
    }
    elsif( $year == $max_date_args[2] && $month == $max_date_args[1] && $day == $max_date_args[0] ) {
	if( $time_args[0] > $max_time_args[0] ) {
	    $max_date = $day." ".$bmonth." ".$year." ".$time." ".$branch;
	}
	elsif( $time_args[0] == $max_time_args[0] && $time_args[1] > $max_time_args[1] ) {
	    $max_date = $day." ".$bmonth." ".$year." ".$time." ".$branch;
	}
	elsif( $time_args[0] == $max_time_args[0] && $time_args[1] == $max_time_args[1] && 
	       $time_args[0] > $max_time_args[0] ) {
	    $max_date = $day." ".$bmonth." ".$year." ".$time." ".$branch;
	}
    }
}


find({ wanted => \&process}, '.');
print $max_date."\n";
open(COMMIT,"> ./konversation/commit.h") or die "Can't open commit.h file" ;
print COMMIT '#define COMMIT '.$max_date." ".$branch."\n";
