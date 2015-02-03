#!/usr/bin/perl

# simple perl script to output convert try output to junit xml

use strict;
use warnings;
use Carp;
use XML::LibXML;

my $xbhIp=shift @ARGV;
confess "Please specify XBH ip as parameter\n" if not defined $xbhIp;

my $xbhPort=22595;
$xbhPort = shift(@ARGV)*1 if @ARGV > 0;

my $platform=`cat settings | grep -E '^\\s*platform.*=' | cut -d "=" -f 2`;
chomp($platform);
$platform=~ s/"//g;

confess "Platform setting not found\n" if not defined $platform;

my $resultDir="bench/".$platform."_".$xbhIp."_".$xbhPort;

confess "No try result dir: $resultDir\n" if ! -d $resultDir;

my $dataFile=$resultDir."/data";
open(DATA,$dataFile) || confess "Unable to open $dataFile\n";

my @algos;
my @wasSuccess;
my @errorTexts;
my @tryCycles;

my $succ=0;
my $fail=0;

while(<DATA>)
{
  chomp;
  my $line=$_;
  
  my ($b1,$b2,$b3,$b4,$b5,$b6,$type) =split(" ",$line);
  next if not defined($type);
  
  if($type eq "try") {
    my ($a1,$a2,$a3,$a4,$a5,$cipher,$a7,$a8,$checksumOk,$benchCycles,$checkCycles,$a12,$implementation,$compiler)=split(" ",$line);
    push(@algos,$implementation);
    push(@tryCycles,$benchCycles);
  
    if(uc($checksumOk) eq "OK") {
      push(@wasSuccess,1);
      push(@errorTexts,"");
      $succ++;
    } else {
      push(@wasSuccess,0);
      push(@errorTexts,"Checksum $checksumOk");
      $fail++;
    }
  }
  elsif($type eq "tryfails")
  {
    my ($a1,$a2,$a3,$a4,$a5,$cipher,$a7,$implementation,$compiler,$result)=split(" ",$line,10);
    push(@algos,$implementation);
    push(@tryCycles,0);
    push(@wasSuccess,0);
    push(@errorTexts,"$result");
    $fail++;
  }
}

my $total=$succ+$fail;

my $doc = XML::LibXML::Document->new('1.0',"UTF-8");
my $root = $doc->createElement("testsuite");
$doc->addChild($root);
$root->setAttribute("errors",0);
$root->setAttribute("tests",$total);
$root->setAttribute("time",0);
$root->setAttribute("failures",$fail);
$root->setAttribute("name", "PlatformExecuteTest");

while(my $algo=shift(@algos)) {
   $algo =~ s/\//./g;
 
  my $errorText=shift(@errorTexts);
  my $tryCycles=shift(@tryCycles);
  $succ=shift(@wasSuccess);
  
  my $tc=$doc->createElement("testcase");
  $root->addChild($tc);
  $tc->setAttribute("name",$algo);
  $tc->setAttribute("time",$tryCycles/1000);
  if(!$succ) {
    my $failure=$doc->createElement("failure");
    $tc->addChild($failure);
    $failure->setAttribute("type","executeFailed");
    $failure->setAttribute("message","$errorText");
  }
}

print $doc->toString(2);