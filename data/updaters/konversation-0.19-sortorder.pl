#! /usr/bin/perl

use strict;

my $currentGroup = "";
my $name;
my $value;
my $out;
my %list;

while (<>)
{
    chomp;
    if ( /^\[/ )
    {
        $currentGroup = $_;
        next;
    }
    elsif ($_ =~ /AdminValue/)
    {
        ($name,$value) = split("=",$_);  
        $list{"$value"} = "p";
        print("# DELETE $currentGroup$name\n");
    }
    elsif ($_ =~ /HalfopValue/)
    {
        ($name,$value) = split("=",$_);
        $list{"$value"} = "h";
        print("# DELETE $currentGroup$name\n");
    }
    elsif ($_ =~ /NoRightsValue/)
    {
        ($name,$value) = split("=",$_);
        $list{"$value"} = "-";
        print("# DELETE $currentGroup$name\n");
    }
    elsif ($_ =~ /OperatorValue/)
    {
        ($name,$value) = split("=",$_);
        $list{"$value"} = "o";
        print("# DELETE $currentGroup$name\n");
    }
    elsif ($_ =~ /OwnerValue/)
    {
        ($name,$value) = split("=",$_);
        $list{"$value"} = "q";
        print("# DELETE $currentGroup$name\n");
    }
    elsif ($_ =~ /VoiceValue/)
    {
        ($name,$value) = split("=",$_);
        $list{"$value"} = "v";
        print("# DELETE $currentGroup$name\n");
    }
    elsif ($_ =~ /AwayValue/)
    {
        ($name,$value) = split("=",$_);
        print("# DELETE $currentGroup$name\n");
    }
}

print "SortOrder=";

foreach $out (reverse sort { $a <=> $b } keys %list)
{
    print $list{$out};
}

print "\n";