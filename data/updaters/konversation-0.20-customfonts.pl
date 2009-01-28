#! /usr/bin/perl

use strict;


while (<>)
{
    chomp;
    if ($_ =~ /^TextFont/)
    {
        print("CustomTextFont=true\n");
    }
    if ($_ =~ /^ListFont/)
    {
        print("CustomListFont=true\n");
    }
    if ($_ =~ /^TabFont/)
    {
        print("CustomTabFont=true\n");
    }
}
