package IHexFile;

use strict;
use warnings;

sub new
{
  my $class=shift;
  my $self={};
  $self->{filename}=shift;
  bless($self,$class);
  return($self);
}

sub log
{
  my $self=shift;
  my $msg=shift;
  
 # print "IHEX: ".$msg."\n";
}

sub read
{
  my $self=shift;
  
  my $openRes=open(IHEX,$self->{filename});
  
  
  if(! $openRes) {
    $self->log("No such file");
    return 0;
  }
  
  $self->{data}="";
  my $startAddress=undef;
  
  while(<IHEX>) {
    chomp;
    my $line=$_;
    $line =~ s/\r$//;
    #               address    type data checksum
    if($line =~ /\:([0-9A-F]{2})([0-9A-F]{4})00(.*)([0-9A-F]{2})$/i ) { 
    # @todo: check for holes 
      my $rowLength=hex($1);
      my $currentAddress=hex($2);
      if(!defined($startAddress)) {
        $startAddress=$currentAddress;
      }
      my $data=$3;
      
      if(length($data) != $rowLength*2) {
        die("Invalid length of ihex data: supposed: $rowLength, real:".(length($data)/2)."\n");
      }
      my $checksum=$4;
      $self->{data}.=$data;
      # $self->log("$currentAddress: $data");
    } 
  }
  close IHEX;
  $self->{size}=length($self->{data})/2;
  $self->{startAddress}=$startAddress;
  $self->log("Size of programm code: ".$self->{size}." beginning at ".$startAddress);
  return 1;
}

sub size
{
  my $self=shift;
  
  return $self->{size};
}

sub startAddress
{
  my $self=shift;
  
  return $self->{startAddress};
}

sub getData
{
  my $self=shift;
  my $start=shift;
  my $blockSize=shift;
  
  if($start > $self->{size}) {
    die("Request behind of of data");
  }  

  return substr($self->{data},$start*2,$blockSize*2);
}

1;