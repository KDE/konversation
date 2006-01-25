#!/usr/bin/perl

# helps finding missing translations in eventrc

$lang="fr";

while (<>){
    chomp;

    if (/^([^\[=]*)=(.*)$/){
	if ($found==0){
	    print "$key -> $english\n";
	}
	$key=$1;
	$english=$2;
	$found=0;
    } elsif (/^${key}\[${lang}\]=/){
	$found=1;
    }
    
}
