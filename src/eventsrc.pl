#!/usr/bin/perl

# helps finding missing translations in eventrc
# and proofreading one specific language

# part of konversation build support tools

# (C) 2006 Emil 'nobs' Obermayr

($file,$lang,$all,$dummy)=@ARGV;

if ($lang =~ /^[a-z]{2}$/){
    print "searching for missing translations to $lang\n\n";
    $stat=0;
} elsif ($lang eq "xxx") {
    print "giving statistics over all languages\n\n";
    $stat=1;
} else {
    print "
eventsrc.pl searches for missing translations

usage: eventsrc.pl <file> <lang> [all]

file: eventsrc-filename, normalley 'eventsrc'

lang: 2 lower-case letters; language-code
      e.g. de for german, fr for french
      use xxx for a overall statistic

all:  set to 1 for proofreading (show all)
";
    exit;
}

open file;

$found=1;

while (<file>){
    chomp;

    if (/^([^\[=]*)=(.*)$/){
	if ($found==0 && $stat==0){
	    print "[$group] $key: $english -> missing\n";
	} elsif ($found==1 && $all==1){
	    print "[$group] $key: $english -> $trans\n";
	}

	$key=$1;
	$found=0;
	$english=$2;
 
	# uninteressante keys als gefunden markieren
	if ($key eq "default_presentation"){
	    $found=2;
	} else {
	    $count++;
	}

    } elsif (/^${key}\[(..)\]=(.*)/){
	$countA{$1}++;

	if ($1 eq $lang){
	    $trans=$2;
	    $found=1;
	} else{}
    } elsif (/^\[(.*)\]$/){
	$group=$1;
    } else {}    
}

if ($stat==1){
    foreach $lang (sort keys %countA){
	$tmp=$countA{$lang};
	print "In language $lang $tmp / $count are translated.\n";
    }
}else{
    $tmp=$countA{$lang};
    print "\nIn language $lang $tmp / $count are translated.\n";
}

