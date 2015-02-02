#!/usr/bin/perl

# simple perl script to output convert build stats to junit xml

use strict;
use warnings;
use Carp;
use XML::LibXML;


my $platform=`cat settings | grep -E '^\\s*platform.*=' | cut -d "=" -f 2`;
chomp($platform);
$platform=~ s/"//g;


confess "Platform setting not found\n" if not defined $platform;

my $dataFile="bench/compile_$platform/data";

confess "No compile results found: $dataFile\n" if ! -e $dataFile;

open(DATA,$dataFile) || confess "Unable to open $dataFile\n";

my @algos;
my @successFlags;
my @compilerTexts;

my $compilerOut="";

my $succ=0;
my $fail=0;

while(<DATA>)
{
  chomp;
  my $line=$_;
  
  my($a1,$a2,$a3,$a4,$a5,$a6,$type, $algo,$dirchecksum,$data)=split(" ",$line,10);
  next if not defined($type);
  
  if($type eq "fromcompiler") {
    $compilerOut.=$data."\n";
  } elsif($type eq "compilesuccess") {
    $algo=~ s/\//./g;

    push(@compilerTexts,$compilerOut);
    $compilerOut="";
    $succ++;
    push(@successFlags,1);
    push(@algos,$algo);
  } elsif($type eq "compilefailed" || $type eq "linkfailed" ) {
    $algo=~ s/\//./g;

    push(@compilerTexts,$compilerOut);
    $compilerOut="";
    $fail++;
    push(@successFlags,0);
    push(@algos,$algo);
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
$root->setAttribute("name", "PlatformCompileTest");

while(my $algo=shift(@algos)) {
  my $compilerText=shift(@compilerTexts);
  $succ=shift(@successFlags);
  
  my $tc=$doc->createElement("testcase");
  $root->addChild($tc);
  $tc->setAttribute("name",$algo);
  $tc->setAttribute("time",0);
  if(!$succ) {
    my $failure=$doc->createElement("failure");
    $tc->addChild($failure);
    $failure->setAttribute("type","compileFailed");
    $failure->setAttribute("message","Compilation failed");
  }
  my $text=$doc->createElement("system-out");
  $tc->addChild($text);
  $text->appendText($compilerText);
}



print $doc->toString(2);