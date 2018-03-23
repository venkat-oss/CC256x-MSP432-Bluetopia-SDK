#!/usr/bin/perl
#
# File:        fbl_gen.pl
# Author:      Tim Cook
# Changed on 14 june 2015- Add the processor build option for the library creation.
# Description: Builds Flexible Build Library for specific Toolchain
# Usage:       fbl_gen.pl --IAR|CCS|KEIL
#                         [--sppserver] [--sppclient] [--sdpserver] [--sdpclient]
#                         [--lemaster] [--leslave] [--gattserver] [--gattclient]
#                         [--hidhost]  [--hiddevice] [--sco] [--goepserver] [--goepclient] 
#                         [--mapserver] [--mapclient] [--pbapserver] [--pbapclient]
#                         [--help]
#
use strict;
use Getopt::Long;

#my $ObjectsNameSuffix = shift or die "\n Error! Please provide processor build option, e.g. 16_M3, 16_M4, 16_M4.fp_HW_FP, 32_M3, 32_M4, 32_M4.fp_HW_FP. \n";
my $ObjectsNameSuffix = "";

if($ObjectsNameSuffix eq "16_M3") {
	print "\n Building libraries for ARM Cortex M3, wchar=16bit \n";
}
elsif($ObjectsNameSuffix eq "16_M4") {
	print "\n Building libraries for ARM Cortex M4, wchar=16bit \n";
}
elsif($ObjectsNameSuffix eq "16_M4.fp_HW_FP") {
	print "\n Building libraries for ARM Cortex M4, wchar=16bit with HW Floating point \n";
}
elsif($ObjectsNameSuffix eq "32_M3") {
	print "\n Building libraries for ARM Cortex M3, wchar=32bit \n";
}
elsif($ObjectsNameSuffix eq "32_M4") {
	print "\n Building libraries for ARM Cortex M4, wchar=32bit \n";
}
elsif($ObjectsNameSuffix eq "32_M4.fp_HW_FP") {
	print "\n Building libraries for ARM Cortex M4, wchar=32bit with HW Floating point \n";
}
elsif($ObjectsNameSuffix eq "--help") {
	print "\n Printing help... \n";
}
elsif($ObjectsNameSuffix ne "") {
	print "\n Error! Please provide valid processor build option, e.g. 16_M3, 16_M4, 16_M4.fp_HW_FP, 32_M3, 32_M4, 32_M4.fp_HW_FP. \n";
	exit(0);
}
if($ObjectsNameSuffix ne "") {
	$ObjectsNameSuffix = "_" . $ObjectsNameSuffix;
}

if($ObjectsNameSuffix ne "--help") {
	print "\n Building libraries from objects with name suffix= objectName$ObjectsNameSuffix \n";
}

my $SPPServer;
my $SPPClient;
my $GOEPServer;
my $GOEPClient;
my $SDPServer=1;
my $SDPClient;
my $LEMaster;
my $LESlave;
my $GATTServer;
my $GATTClient;
my $MAPServer;
my $MAPClient;
my $PBAPServer;
my $PBAPClient;
my $HIDHost;
my $HIDDevice;
my $SCO;
my @Objects;
my $BluetopiaLibrary = "Bluetopia$ObjectsNameSuffix";
my @GATTObjects;
my $GATTLibrary = "SS1BTGAT$ObjectsNameSuffix";
my @MAPObjects;
my $MAPLibrary = "SS1BTMAP$ObjectsNameSuffix";
my @PBAPObjects;
my $PBAPLibrary = "SS1BTPBA$ObjectsNameSuffix";
my @HIDObjects;
my $HIDLibrary  = "SS1BTHID$ObjectsNameSuffix";
my $Help;
my $OSType;
my $FBLOption;
my $Toolchain = "CCS";                                    # Default Toolchin
my $KEIL;
my $IAR;
my $CCS;

sub BuildLibraries;


#-------------------------------------------------------------------------
# Parse the command line switches
#-------------------------------------------------------------------------
GetOptions('IAR'        => \$IAR,
           'CCS'        => \$CCS,
           'KEIL'       => \$KEIL,
           'sppserver'  => \$SPPServer,
           'sppclient'  => \$SPPClient,
           'sdpserver'  => \$SDPServer,
           'sdpclient'  => \$SDPClient,
           'lemaster'   => \$LEMaster,
           'leslave'    => \$LESlave,
           'gattserver' => \$GATTServer,
           'gattclient' => \$GATTClient,
           'mapserver'  => \$MAPServer,
           'mapclient'  => \$MAPClient,
           'pbapserver' => \$PBAPServer,
           'pbapclient' => \$PBAPClient,
           'hidhost'    => \$HIDHost,
           'hiddevice'  => \$HIDDevice,
           'sco'        => \$SCO,
           'help'       => \$Help
           );

if($KEIL) 
{
	$Toolchain = "KEIL";
}
elsif ($IAR)
{
	$Toolchain = "IAR";
}

		   
#-------------------------------------------------------------------------
# Display Help string if requested
#-------------------------------------------------------------------------
if($Help)
{
print <<HELP;
   fbl_iar.pl options (Note all options are optional):
      --IAR|CCS|KEIL - Specify toolchain (Default is $Toolchain)
      --sppserver  - Support SPP Server Role
      --sppclient  - Support SPP Client Role
      --goepserver - Support GOEP Server Role
      --goepclient - Support GOEP Client Role
      --sdpserver  - Support SDP Server Role
      --sdpclient  - Support SDP Client Role
      --lemaster   - Support LE Master Role
      --leslave    - Support LE Slave Role
      --gattserver - Support GATT Server Role
      --gattclient - Support GATT Client Role
      --mapserver  - Support MAP Server Role
      --mapclient  - Support MAP Client Role
      --pbapserver - Support PBAP Server Role
      --pbapclient - Support PBAP Client Role
      --hidhost    - Support HID Host Role
      --hiddevice  - Support HID Device Role
      --sco        - Support SCO (Audio)
HELP

   exit(0);
}

# Build the NoOS version of the libraries
print "No-OS Libraries\n";
print "-" x 80;
print "\n";
BuildLibraries("-");

exit(0);


#-------------------------------------------------------------------------------
# Subroutine: AddOption
# Arguments:
#    IsFirstOption - 1 if this is the first option for the object, 0 else 
#    OptionVal - option value (1 - option is support, 0- not supported)
#    OptionStr - option name (as apear in file name)
# Descriptions:  Generate the option (Y/N) of an FBL feature
#-------------------------------------------------------------------------------
sub AddFBLOption
{
	my ($IsFirstOption, $OptionVal, $OptionStr) = @_;
	if($IsFirstOption == 1)
	{
	  $FBLOption = "";
	}
	if($OptionVal == 1) 
	{
	  $FBLOption .= "_".$OptionStr."_y";
	}
	else 
	{
	  $FBLOption .= "_".$OptionStr."_n";
	}
}

#-------------------------------------------------------------------------------
# Subroutine: LinkObjects
# Arguments:
#    ProfName - profile name or "" in case of core bluetopia  
#    OutLib - target library name 
#    InObj  - list of input object files
# Descriptions:  Link object files to library and copy to dest lib
#-------------------------------------------------------------------------------
sub LinkObjects
{
   my ($ProfName, $OutLib, @InObj) = @_;
   my $Folder;
   my $WinFolder;
   my $ToolchainDir;
   my $Archiver;
   my $ArchiverCmd;
   my $ArchiverRes;
 
   
   unlink($OutLib);
    
   if($Toolchain eq "IAR")
   {
	  $OutLib .= ".a";
      $ToolchainDir = "ewarm";
	  $Archiver = "iarchive";
      $ArchiverCmd = $Archiver." --create @InObj -o $OutLib";
   }
   elsif($Toolchain eq "KEIL")
   {
	  $OutLib .= ".lib";
      $ToolchainDir = "rvmdk";
	  $Archiver = "armar";
      $ArchiverCmd = $Archiver. " --create $OutLib @InObj";
   }
   elsif($Toolchain = "CCS")
   {
 	  $OutLib .= ".lib";
      $ToolchainDir = "ccs";
	  $Archiver = "armar";
      $ArchiverCmd = $Archiver. " --create $OutLib @InObj";
  }
   else
   {
      print "ERROR:: unsupported toolcahin ($Toolchain)\n";
	  exit -1;
   }
   
   $Folder = "generated_libs";
   $WinFolder = "generated_libs";
   mkdir($Folder, 0700) unless (-d $Folder);
   
   if($ProfName ne "")
   {
	  $Folder    .= "/profiles";
	  mkdir($Folder, 0700) unless (-d $Folder);
	  $WinFolder .= "\\profiles";

	  $Folder    .= "/".$ProfName;
	  mkdir($Folder, 0700) unless (-d $Folder);
	  $WinFolder .= "\\". $ProfName;
   }
   else
   {
      print "\n";
      print "############################################################################\n";
      print "Running Flexible Build Library Utility.  Archiver for $Toolchain ($Archiver) must be in the \npath variable.\n";
      print "############################################################################\n\n";
	  $Folder    .= "/core";
	  mkdir($Folder, 0700) unless (-d $Folder);
	  $WinFolder .= "\\core";
   }

   $Folder .= "/" . $ToolchainDir;
   mkdir($Folder, 0700) unless (-d $Folder);
   $WinFolder .= "\\" . $ToolchainDir;
   
   if($OSType ne "-")
   {
      $Folder .= "/" . $OSType;
      mkdir($Folder, 0700) unless (-d $Folder);
      $WinFolder .= "\\" . $OSType;
   }
   print $ArchiverCmd;
   $ArchiverRes = `$ArchiverCmd`; # ARCHIVE Execution
   print $ArchiverRes. "\n";
  
   print "move $OutLib $WinFolder\n\n";
   `move $OutLib $WinFolder`;

}


#-------------------------------------------------------------------------------
# Subroutine: BuildLibraries
# Arguments:
#    OS Type - Name of directory to get objects (NoOS or RTOS)
# Descriptions:  Builds Flexible Build Library to use specifications.
#-------------------------------------------------------------------------------
sub BuildLibraries
{
   my $ObjDir;
   
   $OSType = shift or die "Usage";
   $ObjDir = "Objects\\";
   if($OSType ne "-")
   {
      $ObjDir .= $OSType . "\\";
   }
   @Objects     = ();
   @GATTObjects = ();
   @HIDObjects  = ();
   @MAPObjects  = ();
   @PBAPObjects = ();

   #-------------------------------------------------------------------------
   # Note if we have MAP support we need to also include the appropriate
   # GOEP support.
   #-------------------------------------------------------------------------
   if(($MAPServer == 1) || ($MAPClient == 1))
   {
      $GOEPServer = 1;
      $GOEPClient = 1;
   }

   #-------------------------------------------------------------------------
   # Note if we have PBAP support we need to also include the appropriate
   # GOEP support.
   #-------------------------------------------------------------------------
   if($PBAPServer == 1)
   {
      $GOEPServer = 1;
   }

   if($PBAPClient == 1)
   {
      $GOEPClient = 1;
   }

   #-------------------------------------------------------------------------
   # Note if we have GOEP support we need to also include the appropriate
   # SPP support.
   #-------------------------------------------------------------------------
   if($GOEPServer == 1)
   {
      $SPPServer = 1;
   }

   if($GOEPClient == 1)
   {
      $SPPClient = 1;
   }

   #-------------------------------------------------------------------------
   # Note if we are including SPP Server support we must have SDP Server
   # support.
   #-------------------------------------------------------------------------
   if($SPPServer == 1)
   {
      $SDPServer = 1;
   }

   
   #-------------------------------------------------------------------------
   # Change name of of core library in case of LE.
   #-------------------------------------------------------------------------
   if($LEMaster|$LESlave)
   {
      $BluetopiaLibrary = "Bluetopia_LE$ObjectsNameSuffix";
   }
   
   #-------------------------------------------------------------------------
   # Add the common objects
   #-------------------------------------------------------------------------
   push(@Objects, $ObjDir."HCIDrv".$ObjectsNameSuffix.".o");
   if($Toolchain eq "KEIL")
   {
      push(@Objects, $ObjDir."HCIComm".$ObjectsNameSuffix.".o");
      push(@Objects, $ObjDir."HCILL".$ObjectsNameSuffix.".o");
      #push(@Objects, $ObjDir."TWUART".$ObjectsNameSuffix.".o");
   }
   else
   {
      push(@Objects, $ObjDir."HCILL".$ObjectsNameSuffix.".o");
      push(@Objects, $ObjDir."HCIComm".$ObjectsNameSuffix.".o");
   }
   push(@Objects, $ObjDir."BSC".$ObjectsNameSuffix.".o");
   push(@Objects, $ObjDir."BTPSTMR".$ObjectsNameSuffix.".o");
   push(@Objects, $ObjDir."OTP".$ObjectsNameSuffix.".o");
   #if($Toolchain eq "KEIL")
   #{
   #   push(@Objects, $ObjDir."HCID".$ObjectsNameSuffix.".o");
   #}
   
   #-------------------------------------------------------------------------
   # Determine which SPP and RFCOMM objects are to be included
   #-------------------------------------------------------------------------
   
   AddFBLOption(1, $SPPClient, "Client");
   AddFBLOption(0, $SPPServer, "Server");
   push(@Objects, $ObjDir."RFCOMM".$FBLOption.$ObjectsNameSuffix.".o");
   push(@Objects, $ObjDir."SPP".$FBLOption.$ObjectsNameSuffix.".o");

   #-------------------------------------------------------------------------
   # Determine which GOEP objects are to be included
   #-------------------------------------------------------------------------
   AddFBLOption(1, $GOEPClient, "Client");
   AddFBLOption(0, $GOEPServer, "Server");
   push(@Objects, $ObjDir."GOEP".$FBLOption.$ObjectsNameSuffix.".o");

   #-------------------------------------------------------------------------
   # Determine which SDP Object is to be included
   #-------------------------------------------------------------------------
   AddFBLOption(1, $SDPClient, "Client");
   AddFBLOption(0, $SDPServer, "Server");
   push(@Objects, $ObjDir."SDP".$FBLOption.$ObjectsNameSuffix.".o");
 
   #-------------------------------------------------------------------------
   # Determine which HCI Object to include
   #-------------------------------------------------------------------------
   AddFBLOption(1, $LEMaster, "LEMaster");
   AddFBLOption(0, $LESlave, "LESlave");
   AddFBLOption(0, $SCO, "SCO");
 
   push(@Objects, $ObjDir."HCI".$FBLOption.$ObjectsNameSuffix.".o");

   #-------------------------------------------------------------------------
   # Determine which GAP Object to include
   #-------------------------------------------------------------------------
   AddFBLOption(1, ($LEMaster|$LESlave), "LE");
   push(@Objects, $ObjDir."GAP".$FBLOption.$ObjectsNameSuffix.".o");

   #-------------------------------------------------------------------------
   # Determine which GAPLE Object to include
   #-------------------------------------------------------------------------
   AddFBLOption(1, $LEMaster, "LEMaster");
   AddFBLOption(0, $LESlave, "LESlave");
   push(@Objects, $ObjDir."GAPLE".$FBLOption.$ObjectsNameSuffix.".o");

   #-------------------------------------------------------------------------
   # Determine which SCO Object to include
   #-------------------------------------------------------------------------
   AddFBLOption(1, $SCO, "Supported");
   push(@Objects, $ObjDir."SCO".$FBLOption.$ObjectsNameSuffix.".o");

   #-------------------------------------------------------------------------
   # Determine which L2CAP Object to include
   #-------------------------------------------------------------------------
   AddFBLOption(1, ($LEMaster|$LESlave), "LE");
   push(@Objects, $ObjDir."L2CAP".$FBLOption.$ObjectsNameSuffix.".o");

   #-------------------------------------------------------------------------
   # Determine which GATT Object to include
   #-------------------------------------------------------------------------
   if(($GATTServer == 1) || ($GATTClient == 1))
   {
     AddFBLOption(1, $GATTClient, "Client");
     AddFBLOption(0, $GATTServer, "Server");
     push(@GATTObjects, $ObjDir."SS1BTGAT".$FBLOption.$ObjectsNameSuffix.".o");
   }
   
   #-------------------------------------------------------------------------
   # Determine which HID Object to include
   #-------------------------------------------------------------------------
   if(($HIDHost == 1) || ($HIDDevice == 1))
   {
     AddFBLOption(1, $HIDHost, "Host");
     AddFBLOption(0, $HIDDevice, "Device");
     push(@HIDObjects, $ObjDir."SS1BTHID".$FBLOption.$ObjectsNameSuffix.".o");
   }
   #-------------------------------------------------------------------------
   # Determine which MAP Object to include
   #-------------------------------------------------------------------------
   if(($MAPClient == 1) || ($MAPServer == 1))
   {
     AddFBLOption(1, $MAPClient, "Client");
     AddFBLOption(0, $MAPServer, "Server");
     push(@MAPObjects, $ObjDir."SS1BTMAP".$FBLOption.$ObjectsNameSuffix.".o");
   }
   
   #-------------------------------------------------------------------------
   # Determine which PBAP Object to include
   #-------------------------------------------------------------------------
   if(($PBAPClient == 1) || ($PBAPServer == 1))
   {
     AddFBLOption(1, $PBAPClient, "Client");
     AddFBLOption(0, $PBAPServer, "Server");
     push(@PBAPObjects, $ObjDir."SS1BTPBA".$FBLOption.$ObjectsNameSuffix.".o");
   }
   
   #-------------------------------------------------------------------------
   # Create the library with the specified files
   #-------------------------------------------------------------------------
   LinkObjects("", $BluetopiaLibrary, @Objects);

   #-------------------------------------------------------------------------
   # Create the GATT library with the specified files if requested
   #-------------------------------------------------------------------------
   if(scalar(@GATTObjects) > 0)
   {
      LinkObjects("gatt", $GATTLibrary, @GATTObjects);
   }

   #-------------------------------------------------------------------------
   # Create the HID library with the specified files if requested
   #-------------------------------------------------------------------------
   if(scalar(@HIDObjects) > 0)
   {
      LinkObjects("hid", $HIDLibrary, @HIDObjects);
   }

   #-------------------------------------------------------------------------
   # Create the MAP library with the specified files if requested
   #-------------------------------------------------------------------------
   if(scalar(@MAPObjects) > 0)
   {
      LinkObjects("map", $MAPLibrary, @MAPObjects);
   }

   #-------------------------------------------------------------------------
   # Create the PBAP library with the specified files if requested
   #-------------------------------------------------------------------------
   if(scalar(@PBAPObjects) > 0)
   {
      LinkObjects("pbap", $PBAPLibrary, @PBAPObjects);
   }
}
