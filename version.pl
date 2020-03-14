#!/usr/bin/perl -w
use strict;
use warnings;
#
# declare local variables
#
my $n;
my @infile;
my $ver;
my $i;
# get version number from version.h
#
$n = "version.h";
open FILE,$n or die "can't open ".$n;
@infile = <FILE>;
foreach(@infile)
{
    if($_ =~ /#define\sVER.*/) { ($ver) = $_ =~ /.*(\d\.\d\d).*/; }
}
close FILE;
print "version is $ver\n";

# update version in arsc.iss
#
$n = "arsc.iss";
open FILE,$n or die "can't open ".$n; 
@infile = <FILE>;
$i = 0;
foreach(@infile)
{
    if($_ =~ /AppVer.*\d\.\d\d/) { $infile[$i] =~ s/\d.\d\d/$ver/; }
    $i ++;
} 
close FILE;
open FILE, ">".$n;
print FILE @infile;
close FILE;

# update version in arsc_VS16/_README.txt
#
$n = "arsc_VS16/_README.txt";
open FILE,$n or die "can't open ".$n;
@infile = <FILE>;
$i = 0;
foreach(@infile)
{
    if($_ =~ /version.*\d\.\d\d/) { $infile[$i] =~ s/\d.\d\d/$ver/; }
    $i ++;
} 
close FILE;
open FILE,">".$n;
print FILE @infile;
close FILE;

