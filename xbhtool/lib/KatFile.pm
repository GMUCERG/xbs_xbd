package KatFile;

use strict;
use warnings;

sub new
{
  my $class=shift;
  my $self={};
  $self->{filename}=shift;
  $self->{lengths}=[];
  $self->{msgs}=[];
  $self->{results}=[];
  bless($self,$class);
  return($self);
}

sub log
{
  my $self=shift;
  my $msg=shift;
  
  print "KAT: ".$msg."\n";
}

sub read
{
  my $self=shift;
  
  my $openRes=open(KAT,$self->{filename});
  
  
  if(! $openRes) {
    $self->log("No such file");
    return 0;
  }
  
  while(<KAT>) {
    chomp;
    my $line=$_;
    # remove comments
    $line =~ s/#.*//;
    # remove empty lines
    next if $line =~ /^\s*$/;
    
    my ($key,$val) = split(" = ",$line);
    $val=~ s/\s//g;
    if($key eq "Len") {
      push(@{$self->{lengths}},$val);
    } 
    elsif($key eq "Msg") {
      push(@{$self->{msgs}},$val);
    } 
    elsif($key eq "MD") {
     push(@{$self->{results}},$val);
    } 
    else {
      $self->log("Unable to parse line $line\n");
      return 0;
    }
  }
  my $lengthCount=@{$self->{lengths}};
  my $messageCount=@{$self->{msgs}};
  my $resultCount=@{$self->{results}};
  
  if ($messageCount != $lengthCount) {
    $self->log("Message/Length count mismatch: $messageCount, $lengthCount");
    return 0;
  }
 
  if ($messageCount != $resultCount) {
    $self->log("Message/Result count mismatch: $messageCount, $resultCount");
    return 0;
  }
  
  close KAT;
  return 1;
}

sub size
{
  my $self=shift;
  
  return scalar @{$self->{lengths}};
}

sub triple
{
  my $self=shift;
  my $num=shift;
  
  my $len=${$self->{lengths}}[$num];
  my $msg=${$self->{msgs}}[$num];
  my $result=${$self->{results}}[$num];
  
  return ($len,$msg,$result);
}

1;