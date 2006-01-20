#!/usr/bin/perl

while (<>){

    chomp;

    if (/^\w*$/){
# empty line
	next;
    } elsif (/^\w*#/){
# comment
	next;
    } elsif (/\[(.*)\]/){
# group
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

    if ($group =~ /ServerGroup/){
	if ($key eq "Name"){
	    $servername{$group}=$value;
	    $serverid{$value}=$group;
	}
	if ($key eq "NotifyList"){
	    $servernewnicks{$group}=$value;
	}
    }   

    if ($group =~ /Notify Group Lists/){
	$serveroldnicks{$key}=$value;
    }
}

foreach $key (keys %servername) {
    $value = $servername{$key}; 
# servername: $key
# serverID $value

    $nicks=$serveroldnicks{$value} ." ". $servernewnicks{$key};

    undef %nicksA;

    while ($nicks =~ /([\S]+)\s*/g){
	$nicksA{$1}="1";
    }

    undef $nicks;

    foreach $key2 (sort map {lc} keys %nicksA){
	$nicks .= "$key2 ";
    }    

    print "# DELETE [$key]Notify Group Lists\n";
    print "[$value]NotifyList=$nicks\n";

    print "\n";
}
