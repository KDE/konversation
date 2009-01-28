#! /usr/bin/perl

use strict;

my $button0 = "";
my $button1 = "";
my $button2 = "";
my $button3 = "";
my $button4 = "";
my $button5 = "";
my $button6 = "";
my $button7 = "";

while (<>)
{
    chomp;
    if ($_ =~ /^Button0/)
    {
        $button0 = $_;
    }
    if ($_ =~ /^Button1/)
    {
        $button1 = $_;
    }
    if ($_ =~ /^Button2/)
    {
        $button2 = $_;
    }
    if ($_ =~ /^Button3/)
    {
        $button3 = $_;
    }
    if ($_ =~ /^Button4/)
    {
        $button4 = $_;
    }
    if ($_ =~ /^Button5/)
    {
        $button5 = $_;
    }
    if ($_ =~ /^Button6/)
    {
        $button6 = $_;
    }
    if ($_ =~ /^Button7/)
    {
        $button7 = $_;
    }
}

if ($button0 ne "")
{
    print("# DELETE [Button List]Button0\n");
    print("$button0\n");
}
else
{
    print("# DELETE [Button List]Button0\n");
    print("Button0=Op,/OP %u%n\n");
}

if ($button1 ne "")
{
    print("# DELETE [Button List]Button1\n");
    print("$button1\n");
}
else
{
    print("# DELETE [Button List]Button1\n");
    print("Button1=DeOp,/DEOP %u%n\n");
}

if ($button2 ne "")
{
    print("# DELETE [Button List]Button2\n");
    print("$button2\n");
}
else
{
    print("# DELETE [Button List]Button2\n");
    print("Button2=WhoIs,/WHOIS %s,%%u%n\n");
}

if ($button3 ne "")
{
    print("# DELETE [Button List]Button3\n");
    print("$button3\n");
}
else
{
    print("# DELETE [Button List]Button3\n");
    print("Button3=Version,/CTCP %s,%%u VERSION%n\n");
}

if ($button4 ne "")
{
    print("# DELETE [Button List]Button4\n");
    print("$button4\n");
}
else
{
    print("# DELETE [Button List]Button4\n");
    print("Button4=Kick,/KICK %u%n\n");
}

if ($button5 ne "")
{
    print("# DELETE [Button List]Button5\n");
    print("$button5\n");
}
else
{
    print("# DELETE [Button List]Button5\n");
    print("Button5=Ban,/BAN %u%n\n");
}

if ($button6 ne "")
{
    print("# DELETE [Button List]Button6\n");
    print("$button6\n");
}
else
{
    print("# DELETE [Button List]Button6\n");
    print("Button6=Part,/PART %c Leaving...%n\n");
}

if ($button7 ne "")
{
    print("# DELETE [Button List]Button7\n");
    print("$button7\n");
}
else
{
    print("# DELETE [Button List]Button7\n");
    print("Button7=Quit,/QUIT Leaving...%n\n");
}