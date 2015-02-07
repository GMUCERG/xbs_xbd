package XBH;

use IO::Socket::INET;
use IO::Select;
use strict;
use warnings;

our $bootLoaderMode=1;
our $applicationMode=2;

my $protoVersion="05";

my $CMDLEN_SZ="4";

sub new
{
    my $class=shift;
    my $self={};
    $self->{ip}=shift;
    $self->{port}=shift;
    $self->{proto}=shift;

    $self->{proto}="tcp" if ! defined $self->{proto};

    if(!defined($self->{ip}) ||  !defined($self->{port}) ) {
        die("Invalid call to XBH::new\n");
    }
    $self->{verbose}=0;
    $self->{quiet}=0;
    $self->{localPort}=$self->{port}+1;
    $self->{cmdPending}="";
    $self->{error}=0;
    $self->{lastAnswerData}="";
    $self->{currentMode}=-1;
    $self->{timeoutSeconds}=10;
    bless($self,$class);
    return($self);
}

sub log
{
    my $self=shift;
    my $msg=shift;

    $self->{quiet} || print STDERR "XBH: ".$msg."\n";
}

sub verbose
{
    my $self=shift;
    my $verbose=shift;

    $self->{verbose}=$verbose if defined $verbose;
    return $self->{verbose};
}

sub quiet
{
    my $self=shift;
    my $quiet=shift;

    $self->{quiet}=$quiet if defined $quiet;
    return $self->{quiet};
}

sub timeout
{
    my $self=shift;
    my $timeout=shift;

    $self->{timeoutSeconds}=$timeout if defined $timeout;
    return $self->{timeout};
}

sub resetTimeout
{
    my $self=shift;
    $self->timeout(10);
}


sub connect
{
    my $self=shift;

    $self->{sock}=new IO::Socket::INET->new(PeerPort=>$self->{port},Proto=>$self->{proto},PeerAddr=>$self->{ip});
    die("Unable to open socket\n") unless defined($self->{sock});
    $self->{select}=new IO::Select($self->{sock});
}

sub execXBHCommand {
    my $self=shift;
    my $command=shift;
    my $data=shift;


    defined($command) || die "Invalid call to XBH::execXBHCommand";
    $data="" if(!defined($data));

    ($self->{verbose} >=2) &&  $self->log("Executing ".$command."r".$data);
    $self->{cmdPending}=$command;  
    $self->{error}=0;
    $self->{lastAnswerData}=0;
#    if($self->{proto} eq "tcp"){
#        # TCP is streambased, need to delineate messages
#        my $msg = "XBH".$protoVersion.$command."r".$data;
#        $msg = sprintf("%0".$CMDLEN_SZ."x:", length $msg).$msg;
#        $self->{sock}->send($msg);
#
#    } else{
        $self->{sock}->send("XBH".$protoVersion.$command."r".$data);
#    }

}

sub waitForMessage
{
    my $self=shift;

    my @ready = $self->{select}->can_read($self->{timeoutSeconds});
    return 0 if @ready==0;
    return 1;
}

sub readFromXBH
{
    my $self=shift;

    if(!$self->waitForMessage) {
        $self->{error}=-1;
        return;
    }
    ($self->{verbose} >=2) && $self->log("Waiting for answer");

    my $msg;
    $self->{sock}->recv($msg,512);
    ($self->{verbose} >=2) && $self->log("received $msg");

    my $intro="XBH".$protoVersion.$self->{cmdPending};

    if($msg !~ /^$intro/) {
        if($msg =~ /XBH([0-9][0-9])/) {
            my $xbhProtoVersion=$1;
            if($xbhProtoVersion ne $protoVersion) {
                $self->log("XBH protocol version was $xbhProtoVersion, this tool requires $protoVersion.");
                $self->{error}=1;
                return; 
            }
        }

        $self->log("Received invalid answer to ".$self->{cmdPending}.": ".$msg);
        $self->{error}=1;
        return;
    } 

    $msg =~ s/^$intro//;

    if($msg=~ /^a/) {
        $self->{verbose} && $self->log("Received ack");
        return;
    } 

    if($msg=~ /^o/) {
        $self->{verbose} && $self->log("Received ok");
        $self->{cmdPending}="";
        $self->{lastAnswerData}=substr($msg,1);
        return;
    }

    if($msg=~ /^f/) {
        $self->log("Received fail: ".$msg);
        $self->{cmdPending}="";
        $self->{error}=1;
        return;
    }

    $self->log("Received invalid answer: ".$msg);
    $self->{cmdPending}="";
    $self->{error}=1;
}

sub waitForCompletion
{
    my $self=shift;

    while(!$self->{error} && $self->{cmdPending} ne "") {
        $self->readFromXBH;
    }
}

sub error
{
    my $self=shift;

    return $self->{error};
}

sub switchToApplication
{
    my $self=shift;
    $self->{verbose} && $self->log("Switching to application mode");

    $self->execXBHCommand("sa");
    $self->waitForCompletion();  
    $self->{currentMode}=$applicationMode;
    return $self->{error} == 0;
}  

sub switchToBootLoader
{
    my $self=shift;
    $self->{verbose} && $self->log("Switching to boot loader mode");

    $self->execXBHCommand("sb");
    $self->waitForCompletion();  
    $self->{currentMode}=$bootLoaderMode;

    return $self->{error} == 0;
}  

sub requireBootLoaderMode
{
    my $self=shift;
    return 1 if($self->{currentMode}==$bootLoaderMode);
    return $self->switchToBootLoader();
}

sub requireApplicationMode
{
    my $self=shift;
    return 1 if($self->{currentMode}==$applicationMode);
    return $self->switchToApplication();
}

sub uploadPage
{
    my $self=shift;
    my $hexAddress=shift;
    my $data=shift;

    $self->requireBootLoaderMode()  || return undef;
    $self->{verbose} && $self->log("Uploading a page to address $hexAddress");

    $self->execXBHCommand("cd",$hexAddress.$data);
    $self->waitForCompletion();  

    return $self->{error} == 0;
}

sub executeAndTime
{
    my $self=shift;

    $self->requireApplicationMode()  || return undef;
    $self->{verbose} && $self->log("Executing code");

    $self->timeout(5*60);
    $self->execXBHCommand("ex");
    $self->waitForCompletion();
    $self->resetTimeout();

    return $self->{error} == 0;
}

sub uploadParameters
{
    my $self=shift;
    my $hexType=shift;
    my $hexAddress=shift;
    my $data=shift;

    $self->requireApplicationMode()  || return undef;
    $self->{verbose} && $self->log("Uploading parameters to address $hexAddress");

    $self->execXBHCommand("dp",$hexType.$hexAddress.$data);
    $self->waitForCompletion();

    return $self->{error} == 0;
}

sub getTimings
{
    my $self=shift;

    $self->{verbose} && $self->log("Downloading timing results");

    $self->execXBHCommand("rp");
    $self->waitForCompletion();

    return undef if $self->{error};

    if($self->{lastAnswerData} =~ /^([0-9A-F]{8})([0-9A-F]{8})([0-9A-F]{8})$/) {
        my $seconds=hex($1);
        my $fractions=hex($2);
        my $fractionsPerSecond=hex($3);
        $self->{verbose} && $self->log("Received $seconds $fractions $fractionsPerSecond"); 
        return($seconds,$fractions,$fractionsPerSecond);
    }
    return undef;
}

sub resetXBD
{
    my $self=shift;
    my $resetState=shift;

    my $resetParam="n";
    $resetParam="y" if $resetState;

    $self->{verbose} && $self->log("Setting XBD reset to $resetState ($resetParam)");

    $self->execXBHCommand("rc",$resetParam);
    $self->waitForCompletion();

    $self->{currentMode}=-1;
    return $self->{error} == 0;
}

sub getStatus
{
    my $self=shift;

    $self->{verbose} && $self->log("Getting status of XBH");

    $self->execXBHCommand("st");
    $self->waitForCompletion();

    my $status=$self->{lastAnswerData};
    $self->{currentMode}=$bootLoaderMode if($status eq "XBD".$protoVersion."BLo");
    $self->{currentMode}=$applicationMode if($status eq "XBD".$protoVersion."AFo");

    return undef if $self->{error};
    return $self->{lastAnswerData};
}

sub getXBHRev
{
    my $self=shift;

    $self->{verbose} && $self->log("Getting git revision (and MAC address) of XBH");

    $self->execXBHCommand("sr");
    $self->waitForCompletion();

    return undef if $self->{error};
    my ($gitRev,$mac) = split(",",$self->{lastAnswerData});
    return $gitRev, $mac;
}

sub getBLRev
{
    my $self=shift;

    $self->requireBootLoaderMode() || return undef;
    $self->{verbose} && $self->log("Getting git revision of XBD boot loader");

    $self->execXBHCommand("tr");
    $self->waitForCompletion();

    my $status=$self->{lastAnswerData};

    return undef if $self->{error};
    return $self->{lastAnswerData};
}


sub setCommunicationMode
{
    my $self=shift;
    my $mode=shift;

    if($mode ne "I" && $mode ne "U" && $mode ne "O" && $mode ne "T" && $mode ne "E") {
        $self->log("Invalid communcation mode $mode");
        return 0;
    }

    $self->{verbose} && $self->log("Setting communication mode to $mode");

    $self->execXBHCommand("sc",$mode);
    $self->waitForCompletion();

    return $self->{error} == 0;
}


sub calculateChecksum
{
    my $self=shift;

    $self->requireApplicationMode()  || return undef;
    $self->log("Calculating checksum");

    $self->timeout(15*60);
    $self->execXBHCommand("cc");
    $self->waitForCompletion();
    $self->resetTimeout();

    my $answer=$self->{lastAnswerData};

    return $self->{error} == 0;
}

sub getResults
{
    my $self=shift;

    $self->requireApplicationMode()  || return undef;
    $self->{verbose} && $self->log("Downloading results");

    $self->execXBHCommand("ur");
    $self->waitForCompletion();

    my $answer=$self->{lastAnswerData};

    return undef if $self->{error};
    return $answer;
}

sub getStackUsage
{
    my $self=shift;

    $self->requireApplicationMode()  || return undef;
    $self->{verbose} && $self->log("Getting Stack Usage");

    $self->execXBHCommand("su");
    $self->waitForCompletion();

    return undef if $self->{error};
    if($self->{lastAnswerData} =~ /^.?([0-9A-F]{7,8})$/) {
        my $stackBytes=hex($1);
        return($stackBytes);
    }
    return undef;
}

sub getTimingCalibration
{
    my $self=shift;
    $self->requireBootLoaderMode()  || return undef;
    $self->{verbose} && $self->log("Getting Timing Calibration");

    $self->execXBHCommand("tc");
    $self->waitForCompletion();

    return undef if $self->{error};


    if($self->{lastAnswerData} =~ /^.?([0-9A-F]{7,8})$/) {
        my $nativeClockCycles=hex($1);
        $self->{verbose} && $self->log("Received $nativeClockCycles XBD clock cycles"); 
        return($nativeClockCycles);
    } else {
        $self->log("Invalid timing calibration answer: ".$self->{lastAnswerData});
        return undef;
    }
}

1;
