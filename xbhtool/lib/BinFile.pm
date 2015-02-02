package BinFile;

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
  
  my $openRes=open(BIN,$self->{filename});
  
  
  if(! $openRes) {
    $self->log("No such file");
    return 0;
  }
  
  $self->{data}="";
  
  binmode BIN;
  my $eof=0;
  while(!$eof)
  {
    my $buffer;
    my $bytesRead= read (BIN, $buffer, 65536);
    $eof=1 if($bytesRead < 65536);
    for(my $i=0;$i< $bytesRead; $i++) {
      my $c=substr($buffer,$i,1);
      $self->{data}.=sprintf("%02X",ord($c));
    }
  }
  close BIN;
  $self->{size}=length($self->{data})/2;
  $self->log("Size of programm code: ".$self->{size});
  return 1;
}

sub size
{
  my $self=shift;
  
  return $self->{size};
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

sub startAddress
{
  my $self=shift;
  return 0;
}
1;