#!/usr/bin/perl -w
use strict;
use FindBin;
use lib "$FindBin::Bin/lib";
        
use IHexFile;
use BinFile;
use KatFile;
use XBH;
use POSIX qw(ceil floor);
use Getopt::Long;    

# default values for options 
my $pageSize=256;
my $uploadFile="";
my $targetIp="192.168.10.99";
my $targetPort="22595";
my $verbose=0;
my $quiet=0;
my $targetProto="tcp";

#use Data::Dumper;


sub logMsg
{
  my $msg=shift;
  $quiet || print STDERR $msg."\n";
}

sub bailOut
{
  my $msg=shift;
  die($msg."\n");
}


sub median {
    my $arrayRef = shift;
    my @array = sort{$a <=> $b} (@$arrayRef);

    my $len=@array;

    if( ($len % 2) == 1 ) {
        return  $array[ int($len/2) ];
    } else {
        my $left=$array[ ($len/2) -1 ];
	my $right=$array[ ($len/2) ];
	
	# we don't want to get fractional numbers
	return $left if $left > $right;
	return $right;
    }
}

sub maximum {
    my $arrayRef = shift;
    my @array = sort{$a <=> $b} (@$arrayRef);

    return $array[0];
}



sub upload
{
  my $xbh=shift;
  my $sourceFile=shift;
  my $fileType=shift;
  my $pageSize=shift;
  
  my $source;
  if($fileType eq "hex") {
    $source=new IHexFile($sourceFile);
  } elsif($fileType eq "bin") {
    $source=new BinFile($sourceFile);
  } else {
    bailOut("Invalid file type $fileType");
  }
  
  $source->read || die("Unable to read $sourceFile");
 
  my $startAddress=$source->startAddress();
  my $numberOfPages=ceil($source->size()/$pageSize); #/ that's for editor's broken syntax highlighting

  if($numberOfPages==0) {
    logMsg "No data to upload\n";
    return 0;
  }
  $verbose && logMsg "Preparing to upload $numberOfPages block(s).";
  for(my $i=0;$i<$numberOfPages;$i++) {
    my $data=$source->getData($i*$pageSize,$pageSize);
    my $hexAddress=sprintf("%08X",$i*$pageSize+$startAddress);
    $xbh->uploadPage($hexAddress,$data);
    if($xbh->error) {
      logMsg "Error sending block to xbh";
      return 0;
    }
  }
  return 1;
}

sub uploadData
{
  my $xbh=shift;
  my $dataSize=shift;
  my $dataContent=shift;
  
   bailOut("Invalid number of bytes specified, min 0.") if($dataSize < 0); 
  
  my $hexType=sprintf("%08X",1);
  
 
  # the data block of type 1 begins with the size of the area to hash 
  my $data=sprintf("%08X",$dataSize); 

  # and is followed by a buffer of data at least the size of the area
  if(defined($dataContent)) {
    $data.=$dataContent;
  } else { # random data of size dataSize
    for(my $i=0; $i<$dataSize;$i++) {
      my $rnd=  int(rand(256));
      $data.=sprintf("%02X",$rnd);
    }
  }

  my $numberBlocks=ceil (($dataSize+4)/$pageSize);
  $verbose && logMsg "Preparing to upload $numberBlocks parameter block(s).";
  for(my $i=0;$i<$numberBlocks;$i++) {
    my $blockData=substr($data,$i*$pageSize*2,$pageSize*2);
    my $hexAddress=sprintf("%08X",$i*$pageSize);
    $xbh->uploadParameters($hexType,$hexAddress,$blockData);
    if($xbh->error) {
      logMsg "Error sending block to xbh";
      last;
    }
  }
  return 0 if $xbh->error;
  return 1;
}

sub printHelp
{
  my $exitCode=shift;
  
  print "Valid paramters:\n";
  print "SIMPLE COMMANDS:\n";
  print "  -h, --help\t\t This help\n";
  print "  -o, --comm MODE\t Set communcation mode for XBH<->XBD to I2C (I) or uart (U) or uart250k (O)\n";
  print "\t\t\t or tcp (T) or ethernet (E)\n"; 
  print "  -r, --report\t\t Report status of XBH\n";
  print "  -u, --upload FILE\t Upload application file to XBD\n";
  print "  -c, --checksum\t Calculate cipher checksum on the XBD\n"; 
  print "  -d, --data SIZE\t Uploads SIZE bytes of random data to the XBD\n";
  print "  -x, --execute\t\t Execute code on XBD\n";
  print "  -e, --results\t\t Fetch the results of the last execution\n";
  print "  -t, --timings\t\t Fetch the timings of the last execution\n";
  print "  -k, --reset [0/1]\t Set reset pin of XBD to off(0), on(1) or on,wait,off ()\n";
  print "  --xbhRev\t\t Report Git revision of XBH\n";
  print "  --blRev\t\t Report Git revision of XBD boot loader\n";
  print "  -z, --stackUsage\t Stack Usage report\n";
  print "  -y, --timingCal\t Timing Calibration report\n";
  print "\nOPTIONS:\n";
  print "  -i, --ip IP\t\t Connect to XBH at this address ($targetIp), or hostname\n";
  print "  -p, --port PORT\t Connect to XBH at this port 22594 => UDP, 22595 => TCP ($targetPort)\n";
  print "  -s, --pagesize SIZE\t Page size of the device's flash ($pageSize)\n"; 
  print "  -f, --filetype TYPE\t Type of application file to upload: hex (default), bin\n";
  print "  --cycles=NUM\t\t Output timings in cycles using NUM cycles per second\n";  
  print "  --data-content=HEX\t Upload HEX data instead of random data to the XBD when using --data\n";
  print "  -v, --verbose\t\t Be verbose (may be specified twice)\n";
  print "  -q, --quiet\t\t Be quiet\n";
  print "\nCOMBINED COMMANDS:\n";
  print "  -w, --drift\t\t Calculate relative timing error, needs --cycles\n";
  print "  -m, --measure COUNT\t Repeats --execute and --timings COUNT times, prints \n\t\t\t the median and the COUNT results on one line.\n\t\t\t Prints error on failure\n";
  print "  -n, --full SIZE COUNT\t Repeats -d SIZE --execute, --timings and \n\t\t\t --stackUsage COUNT times, prints the median time and \n\t\t\t the COUNT results on the first line followed by maximum stack\n\t\t\t and the list of stack measurements on the next line\n";  
  print "  -l, --tryQuality [NUM] Performs repeated (default 50) --full 1536 5 measurements and\n\t\t\t reports error towards ideal average, needs --cycles\n";
  print "  --kat FILE\t\t Runs a NIST-style known answer test file (only full-byte vectors)\n";
  print "\nADVANCED/DEBUG COMMANDS:\n";
  print " --mode 0/1\tSwitch to bootloader(0) or application  mode\n";
  
  exit $exitCode if(defined($exitCode));
}

# command line switches 
my $doUpload=0;
my $doExecute=0;
my $doStackUsage=0;
my $doTimingCalibration=0;
my $doTimingErrorCalculation=0;
my $doChecksum=0;
my $doHelp=0;
my $doneSomething=0;
my $doResults=0;
my $doStatus=0;
my $doXBHRev=0;
my $doBLRev=0;
my $doTimings=0;
my $doMeasurements=0;
my @doFullMeasurements;
my $doTryQuality;
my $fileType="hex";
my $resetPin;
my $commMode;
my $switchToMode;
my $dataSize;
my $dataContent;
my $cyclesPerSecond;
my $katFile;

my $optsOk=GetOptions (
  'h|help' => \$doHelp, 
  'u|upload=s' => \$uploadFile,
  'x|execute' => \$doExecute,
  'e|results' => \$doResults,
  't|timings' => \$doTimings,
  'Z|stackUsage' => \$doStackUsage,
  'Y|timingCal' => \$doTimingCalibration,
  'W|drift' => \$doTimingErrorCalculation,
  'cycles=i' => \$cyclesPerSecond,
  'm|measure=i' => \$doMeasurements,
  'n|full=i{2}' => \@doFullMeasurements,
  'r|report' => \$doStatus,
  'xbhRev' => \$doXBHRev,
  'blRev' => \$doBLRev,
  'c|checksum' => \$doChecksum,
  'k|reset:s' => \$resetPin,
  'o|comm=s' => \$commMode,
  'i|ip=s' => \$targetIp,
  'p|port=i' => \$targetPort,
  's|pagesize=i' => \$pageSize,
  'v|verbose+' => \$verbose,
  'q|quiet' => \$quiet,
  'd|data:-1' => \$dataSize,
  'f|filetype=s' => \$fileType,
  'l|tryQuality:i' => \$doTryQuality,
  '--data-content=s' => \$dataContent,
  'mode=s' => \$switchToMode,
  "kat=s" => \$katFile,
  '<>' => ( sub { print "Unknown argument: ".shift()."\n"; printHelp(-1); } )
);
  
if(!$optsOk || $pageSize==0) {
  printHelp -1;
}

if($doHelp) {
  printHelp 0;
}


if($targetPort==22594) { $targetProto =  "udp"; }
else { $targetProto = "tcp"; }

$verbose && logMsg "Using xbh at ".$targetIp.":$targetPort via $targetProto.";
my $xbh=new XBH($targetIp, $targetPort,$targetProto);
$verbose && $xbh->verbose($verbose);
$quiet && $xbh->quiet(1);

# action requested?
if( ($doMeasurements || @doFullMeasurements || $doTimingErrorCalculation || defined($doTryQuality)) && !defined($cyclesPerSecond)) {
  bailOut("Cycles per second not specified (--cycles)");
}


if($uploadFile || $doExecute || $doStatus || $doResults || $doTimings || $doMeasurements || @doFullMeasurements  
  || $doStackUsage ||  $doTimingCalibration || $doTimingErrorCalculation || defined($doTryQuality) || 
    defined($resetPin) || defined($commMode) || defined($switchToMode) || defined($dataSize) || 
    $doChecksum || defined($katFile) || $doXBHRev || $doBLRev ) {
  $doneSomething=1;
  $xbh->connect || bailOut("Unable to open port");
}

if($pageSize > 512) {
# pages > 512 bytes must be transferred in smaller blocks
  while($pageSize > 512) {
    $pageSize/=2;
  }
}

if(defined($resetPin)) {
  if($resetPin eq "") {
    $xbh->resetXBD(1) || bailOut("Unable to set reset pin to 1");
    sleep(1);
    $xbh->resetXBD(0) || bailOut("Unable to set reset pin to 0");
  } else {
    bailOut("Invalid parameter for reset") if($resetPin !~/^[01]$/);
    $xbh->resetXBD($resetPin) || bailOut("Unable to trigger reset pin");
  }
}

if(defined($commMode))
{
  my $result=$xbh->setCommunicationMode($commMode);
  bailOut("Unable to set communcation mode") if(!$result);
}


if($uploadFile) {
  upload($xbh,$uploadFile,$fileType,$pageSize) || bailOut("Upload failed");
}


if($doChecksum) {
  my $checksum=$xbh->calculateChecksum();
  bailOut("Unable to calculate checksum") unless defined($checksum);
}


if(defined($dataSize)) {
  uploadData($xbh, $dataSize, $dataContent);
}




if($doExecute) {
  $xbh->executeAndTime() || bailOut("Execution failed");
}


if($doTimings) {
  my ($seconds,$fractions,$fractionsPerSecond)=$xbh->getTimings();
  defined($seconds) || bailOut("Unable to get timings");
  
  my $fractionSeconds=$fractions/$fractionsPerSecond;

  if(defined($cyclesPerSecond)) { # output in cycles per second 
    my $cycles=int($seconds*$cyclesPerSecond+$fractions/$fractionsPerSecond*$cyclesPerSecond+0.5);
    print $cycles."\n";
  } else { # output time
    print "$seconds seconds + $fractions fractions at $fractionsPerSecond\n";
    print $seconds+$fractionSeconds."\n";
  }
}


if($doMeasurements) {
  my @timings;
  for(my $i=0;$i<$doMeasurements;$i++) {
    $xbh->executeAndTime() || bailOut("Execution failed");
    my ($seconds,$fractions,$fractionsPerSecond)=$xbh->getTimings();
    my $cycles=int($seconds*$cyclesPerSecond+$fractions/$fractionsPerSecond*$cyclesPerSecond+0.5);      
    
    push(@timings,$cycles);
  }
  print median(\@timings)." ";
  print join(" ",@timings)."\n";
}

if(defined($katFile)) {
  my $errors=0;
  my $file=new KatFile($katFile);
  $file->read() || bailOut("Unable to read kat file");
  for(my $i=0;$i<$file->size(); $i++) {
    my ($length, $message, $result) = $file->triple($i);
    next if($length % 8 != 0);
    $quiet || print "Kat $i ($length bit, ".($length/8)." byte): ";
    uploadData($xbh, $length/8,$message) || bailOut("Unable to upload data of kat $i");
    $xbh->executeAndTime() || bailOut("Execution of kat $i ($length bit,".($length/8)." byte) failed");

    my $results=$xbh->getResults();
    defined($results) || bailOut("Unable to get results");
  
    if($results=~ /^([0-9][0-9])000000([0-9A-F]+)$/) {
      my $rc=$1;
      my $hash=$2;
     
      if($rc!=0) {
        print "return code was $rc\n";
        $errors++;
      } elsif(lc($hash) ne lc($result))  { 
        print "\n";
        print "    answer was  '".$hash."'\n";
        print "      expected  '".$result."'\n";
        
        my $xor;
        for(my $c=0;$c<length $hash;$c++)
        {
          my $h=hex(substr($hash,$c,1));
          my $r=hex(substr($result,$c,1));
          my $x=$h ^ $r;
          $xor.=sprintf("%X",$x);
        }
        
        print "           xor  '".$xor."'\n";    
        
        $errors++;
      } else {
        $quiet || print "OK\n";
      }
      if($errors > 10) {
        bailOut("More than 10 errors found, aborting\n");
      }
    } else {
      bailOut("Invalid result data: $results");
    }
  }  
}


if(@doFullMeasurements)
{
  my $size  = $doFullMeasurements[0];
  my $count = $doFullMeasurements[1];
  my @timings;
  my @stacks;
  for(my $i=0;$i<$count;$i++) {
    $verbose && logMsg("Measurement $i/$count"); 
    uploadData($xbh,$size);
    $xbh->executeAndTime() || bailOut("Execution failed");
    my ($seconds,$fractions,$fractionsPerSecond)=$xbh->getTimings();
    defined($seconds) || bailOut("get Timings failed");
    my $cycles=int($seconds*$cyclesPerSecond+$fractions/$fractionsPerSecond*$cyclesPerSecond+0.5);      
    
    push(@timings,$cycles);
    
    my $stack=$xbh->getStackUsage(); 
    defined($stack) || bailOut("get StackUsage failed");
    
    push(@stacks,$stack);
  }
  print median(\@timings)." ";
  print join(" ",@timings)."\n";
  print maximum(\@stacks)." ";
  print join(" ",@stacks)."\n";
}

if(defined($doTryQuality)) {
  my $repetitions=$doTryQuality;
  $repetitions=50 if not defined($doTryQuality);
  my $size  = 1536;
  my $count = 5;
  
  
  my $medianSum=0;
  my $medianCount=0;
  my $maxPPM=0;
  
  for(my $k=0; $k<$repetitions;$k++) {
    my @timings;
    for(my $i=0;$i<$count;$i++) {
      uploadData($xbh,$size);
      $xbh->executeAndTime() || bailOut("Execution failed");
      my ($seconds,$fractions,$fractionsPerSecond)=$xbh->getTimings();
      defined($seconds) || bailOut("get Timings failed");
      my $cycles=int($seconds*$cyclesPerSecond+$fractions/$fractionsPerSecond*$cyclesPerSecond+0.5);      
    
      push(@timings,$cycles);
    }
    my $median=median(\@timings);
    
    $medianSum+=$median;
    $medianCount++;
    
    my $avg=int($medianSum/$medianCount);
    
    my $absError=abs($median-$avg);
    my $relPPM=int($absError/$avg*1000000);
    $maxPPM = $relPPM if $relPPM > $maxPPM;
    
    $quiet || print "Check $k: \tMedian \t$median, \tAvg $avg, \tError $relPPM PPM\n"; 
    bailOut("Error beyond 50000 PPM") if($relPPM > 50000);
  }
  print "Max error in PPM: $maxPPM\n";
}

if($doStatus) {
  my $status=$xbh->getStatus();
  defined($status) || bailOut("Unable to get status, wrong ip/port?");
  if($status eq "") {
    print "XBH found, XBD unknown\n\n";
  }
  print $status."\n"; 
}

if($doXBHRev) {
  my ($gitrev,$mac)=$xbh->getXBHRev();
  defined($gitrev) || bailOut("get xbh git revision failed");
  print "XBH Git Revision: ".$gitrev."\n";
  print "XBH MAC Address: ".$mac."\n"; 
}

if($doBLRev) {
  my $gitrev=$xbh->getBLRev();
  defined($gitrev) || bailOut("get bl git revision failed");
  print "XBD Git Revision: ".$gitrev."\n";
}


if(defined($switchToMode)) {
  $xbh->switchToBootLoader if($switchToMode==0);
  $xbh->switchToApplication if($switchToMode==1);
}

if($doStackUsage) {
  my $result=$xbh->getStackUsage(); 
  defined($result) || bailOut("get StackUsage failed");
  print "Stack usage:\n";
  print "$result\n";
}

if($doResults) {
  my $results=$xbh->getResults();
  defined($results) || bailOut("Unable to get results");
  
  if($results=~ /^([0-9][0-9])000000([0-9A-F]+)$/) {
    my $rc=$1;
    my $hash=$2;
    print "$rc\n";
    print "$hash\n";
  } else {
    bailOut("Invalid result data: $results");
  }
}


if($doTimingCalibration) {
  my $results=$xbh->getTimingCalibration(); 
  defined($results) || bailOut("get TimingCalibration failed");
  print "TimingCalibration:\n";
  print "$results\n";
}

if($doTimingErrorCalculation) {
  my $cyclesBurnt=$xbh->getTimingCalibration();
  if(!defined($cyclesBurnt) || $cyclesBurnt==0) {
    print "Unable to get xbd cycle count (cycles burnt)\n";
    exit -1;
  }     
   my ($seconds,$fractions,$fractionsPerSecond)=$xbh->getTimings();
  my $cyclesMeasured=int($seconds*$cyclesPerSecond+$fractions/$fractionsPerSecond*$cyclesPerSecond+0.5);
  if(!defined($cyclesMeasured) || $cyclesMeasured==0) {
    print "Unable to get xbh timing \n";
    exit -1;
  }      
  
  
  print "Nominal cycles: $cyclesBurnt\n";
  print "Measured cycles: $cyclesMeasured\n";
  my $absError=$cyclesMeasured-$cyclesBurnt;
  my $relError=$absError*1.0/$cyclesMeasured;
  print "Absolute error: $absError\n";
  print "Relative error: $relError\n";
  print "In ppm: ".(int($relError*10000000+0.5)/10)."\n";
 
}

if(!$doneSomething) {
  print "Nothing to do, need at least one of upload/execute/report.\n";
  printHelp;
}
