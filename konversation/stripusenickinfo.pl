#!/usr/bin/perl
use warnings;
$level_ifdef = 0;  #Number of levels deep in #ifdef's we are
$use_nickinfo = 0;
$not_use_nickinfo = 0;
while(<>) {
	if (/#ifdef USE_NICKINFO/ or /#if USE_NICKINFO/) {
		if ($use_nickinfo) {
			print "In USE_NICKINFO ALREADY!!  INTERNAL ERROR!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
		} elsif ($level_ifdef != 0) {
			print "NOT IN TOP LEVEL IFDEF WHEN FOUND NICKINFO!!  INTERNAL ERROR!!!!!!!!!!!!!!!!!!!!!!!!!!!"
		}
		$level_ifdef=1;
		$use_nickinfo = 1;
		$not_use_nickinfo = 0;
	} elsif (/#ifdef/ or /#ifndef/ or /#if/) {  #some other ifdef found - need to keep count of them
		$level_ifdef++ if $level_ifdef>=1; #don't increment if we haven't found a nickinfo yet
		print if !$not_use_nickinfo;
	} elsif (/#else/) {
		if($level_ifdef == 1) {
			#else for usenickinfo
			$use_nickinfo = 0;
			$not_use_nickinfo = 1;
		} else {
			print if !$not_use_nickinfo;
		}
	} elsif (/#endif/) {
		if($level_ifdef == 1) {
			$use_nickinfo = 0;
			$not_use_nickinfo= 0;
			$level_ifdef--;
		} elsif ($level_ifdef > 1) {
			$level_ifdef--;
			print if !$not_use_nickinfo;
		} else {
			print if !$not_use_nickinfo;
		}
	} else {
		print if !$not_use_nickinfo;
	}
}
