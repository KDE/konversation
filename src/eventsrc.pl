#!/usr/bin/perl

# helps finding missing translations in eventrc
# and proofreading one specific language

# part of konversation build support tools

# (C) 2006 Emil 'nobs' Obermayr

($file,$lang,$all)=@ARGV;

if($lang =~ /^[a-z]{2}$/){
    print "searching for missing translations to $lang\n";
} else {
    print "
eventsrc.pl searches for missing translations

usage: eventsrc.pl <file> <lang> [all]

file: eventsrc-file in /src/

lang: 2 lower-case letters; language-code
      e.g. de for german, fr for french

all:  set to 1 for proofreading (show all)
";
    exit;
}

open file;

while (<file>){
    chomp;

    if (/^([^\[=]*)=(.*)$/){
	if ($found==0){
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
	} else {}

    } elsif (/^${key}\[${lang}\]=(.*)/){
	$trans=$1;
	$found=1;
    } elsif (/^\[(.*)\]$/){
	$group=$1;
    } else {}
    
}
