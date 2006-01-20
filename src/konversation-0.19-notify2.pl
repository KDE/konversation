#!/usr/bin/perl

# updater from notify in konversation 0.18 syntax to 0.19

# might also become framework for similar tasks

# for use in KDE UPD

# http://wiki.kdenews.org/tiki-index.php?page=Updating+User+Configuration+Files

# (C) 2006 Emil 'nobs' Obermayr, started 2006-01-20

use strict;

# define all variables needed

my ($group, $key, $value, %servername, %servernewnicks, %serveroldnicks, $nicks, %nicksA, $key2);

while (<>){

    chomp;

#
# decide what kind of line we got
#

    if (/^\s*$/){
# empty line
	next;
    } elsif (/^\s*#/){
# comment
	next;
    } elsif (/\[(.*)\]/){
# group
# handle end-of-group here if needed
	$group=$1;
	next;
    } elsif  (/([^=]*)=(.*)/) {
# key/value-pair
	$key=$1;
	$value=$2;
    } else {
# ignore other crap
	next;
    }

#
# collect data from current line
#

    if ($group =~ /ServerGroup/){
# we are in a server group in new syntax
# collecting name of server group name and already included notifies
	if ($key eq "Name"){
	    $servername{$group}=$value;
	} elsif ($key eq "NotifyList"){
	    $servernewnicks{$group}=$value;
	}
    } elsif ($group =~ /Notify Group Lists/){
# we are in notify group of old syntax
# key is the server group name, value are the nick
	$serveroldnicks{$key}=$value;
    }
}

#
# processing collected data
#

foreach $key (keys %servername) {
    $value = $servername{$key}; 

# servername: $key
# serverID:   $value

    $nicks=$serveroldnicks{$value} ." ". $servernewnicks{$key};

    undef %nicksA;

# putting nicks in hash as keys, automagically make uniq

    while ($nicks =~ /([\S]+)\s*/g){
	$nicksA{$1}="1";
    }

    undef $nicks;

# rebuild string from nick-hash, lower-case and sorted

    foreach $key2 (sort map {lc} keys %nicksA){
	$nicks .= "$key2 ";
    }    

# output stuff in KDE-UPD-syntax

    print "
# DELETE [Notify Group Lists]$value
[$key]
NotifyList=$nicks\n";
}
