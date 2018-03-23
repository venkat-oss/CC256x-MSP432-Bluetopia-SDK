/*****< a3dpdemo_snk.c >*******************************************************/
/*      Copyright 2011 - 2014 Stonestreet One.                                */
/*      All Rights Reserved.                                                  */
/*                                                                            */
/*  A3DPDemo_SNK - Simple embedded application using A3DP (Sink Role) Profile.*/
/*                                                                            */
/*  Author:  Tim Cook                                                         */
/*                                                                            */
/*** MODIFICATION HISTORY *****************************************************/
/*                                                                            */
/*   dd/mm/yy  F. Lastname    Description of Modification                     */
/*   --------  -----------    ------------------------------------------------*/
/*   24/01/11  T. Cook        Initial creation.                               */
/*   22/07/13  M. Brown       Ported SPP demo to A3DP SNK demo.               */
/*   04/06/14  M. Brown       Added support for multiple A2DP sources.        */
/*   24/03/15  D. Keren       Add support for audio routing from the BT chip, */
/*                            CC256x, to the CS43L22 DAC, for music on the    */
/*                            STM3240G-EVAL.                                  */
/*   16/04/15  D. Horowitz    Added Display of FW version, DemoApp Version,   */
/*                            etc.                                            */
/******************************************************************************/
#include "Main.h"                /* Application Interface Abstraction.        */
#include "SS1BTPS.h"             /* Main SS1 Bluetooth Stack Header.          */
#include "SS1BTA2D.h"            /* SS1 A2DP Header.                          */
#include "SS1BTAUD.h"            /* SS1 Audio Subsystem Header.               */
#include "SS1BTAVC.h"            /* A/V Control Transport Header.             */
#include "SS1BTAVR.h"            /* A/V Remote Control Header.                */
#include "BTPSKRNL.h"            /* BTPS Kernel Header.                       */
#include "A3DPDemo_SNK.h"        /* Application Header.                       */
#include "HAL.h"                 /* Hardware Abstraction Layer Header.        */
#include "SS1BTVS.h"             /* Vendor Specific Prototypes/Constants.     */

#define MAX_SUPPORTED_COMMANDS                     (30)  /* Maximum number of */
                                                         /* user commands that*/
                                                         /* are supported by  */
                                                         /* this application. */

#define MAX_NUM_OF_PARAMETERS                      (5)   /* Denotes the max   */
                                                         /* number of         */
                                                         /* parameters a      */
                                                         /* command can have. */

#define MAX_INQUIRY_RESULTS                        (20)  /* Denotes the max   */
                                                         /* Denotes the max   */
                                                         /* number of inquiry */
                                                         /* results.          */

#define MAX_SUPPORTED_LINK_KEYS                    (1)   /* Max supported Link*/
                                                         /* keys.             */

#define DEFAULT_IO_CAPABILITY        (icNoInputNoOutput) /* Denotes the       */
                                                         /* default I/O       */
                                                         /* Capability that is*/
                                                         /* used with Secure  */
                                                         /* Simple Pairing.   */

#define DEFAULT_MITM_PROTECTION                  (FALSE) /* Denotes the       */
                                                         /* default value used*/
                                                         /* for Man in the    */
                                                         /* Middle (MITM)     */
                                                         /* protection used   */
                                                         /* with Secure Simple*/
                                                         /* Pairing.          */

#define CLASSIC_PIN_CODE                          "0000" /* PIN Code used for */
                                                         /* Classic Pairing.  */

#define NO_COMMAND_ERROR                           (-1)  /* Denotes that no   */
                                                         /* command was       */
                                                         /* specified to the  */
                                                         /* parser.           */

#define INVALID_COMMAND_ERROR                      (-2)  /* Denotes that the  */
                                                         /* Command does not  */
                                                         /* exist for         */
                                                         /* processing.       */

#define EXIT_CODE                                  (-3)  /* Denotes that the  */
                                                         /* Command specified */
                                                         /* was the Exit      */
                                                         /* Command.          */

#define FUNCTION_ERROR                             (-4)  /* Denotes that an   */
                                                         /* error occurred in */
                                                         /* execution of the  */
                                                         /* Command Function. */

#define TO_MANY_PARAMS                             (-5)  /* Denotes that there*/
                                                         /* are more          */
                                                         /* parameters then   */
                                                         /* will fit in the   */
                                                         /* UserCommand.      */

#define INVALID_PARAMETERS_ERROR                   (-6)  /* Denotes that an   */
                                                         /* error occurred due*/
                                                         /* to the fact that  */
                                                         /* one or more of the*/
                                                         /* required          */
                                                         /* parameters were   */
                                                         /* invalid.          */

#define UNABLE_TO_INITIALIZE_STACK                 (-7)  /* Denotes that an   */
                                                         /* error occurred    */
                                                         /* while Initializing*/
                                                         /* the Bluetooth     */
                                                         /* Protocol Stack.   */

#define INVALID_STACK_ID_ERROR                     (-8)  /* Denotes that an   */
                                                         /* occurred due to   */
                                                         /* attempted         */
                                                         /* execution of a    */
                                                         /* Command when a    */
                                                         /* Bluetooth Protocol*/
                                                         /* Stack has not been*/
                                                         /* opened.           */

#define UNABLE_TO_REGISTER_SERVER                  (-9)  /* Denotes that an   */
                                                         /* error occurred    */
                                                         /* when trying to    */
                                                         /* create a Serial   */
                                                         /* Port Server.      */

#define EXIT_MODE                                  (-10) /* Flags exit from   */
                                                         /* any Mode.         */

#define MAX_SOURCES                                (2)   /* Number of streams */
                                                         /* allowed to be     */
                                                         /* connected at once.*/

   /* Determine the Name we will use for this compilation.              */
#define APP_DEMO_NAME                        "A3DPDemo_SNK"

   /* The following represent the possible values of UI_Mode variable.  */
#define UI_MODE_IS_CLIENT                    (2)
#define UI_MODE_IS_SERVER                    (1)
#define UI_MODE_SELECT                       (0)
#define UI_MODE_IS_INVALID                   (-1)

   /* Define parameters for changing the Bluetooth ACL priority for     */
   /* better audio quality.  HCI Flow Spec command and DDIP command     */
#define HIGH_AUDIO_CONNECTION_PRIORITY       (1)
#define NORMAL_AUDIO_CONNECTION_PRIORITY     (0)

#define FLOW_SPEC_FLAGS                      (0)
#define FLOW_SPEC_FLOW_DIRECTION             (0x01)
#define FLOW_SPEC_SERVICE_TYPE_GUARANTEED    (0x02)
#define FLOW_SPEC_SERVICE_TYPE_BESTEFFORT    (0x02)
#define FLOW_SPEC_TOKEN_RATE                 (25000)
#define FLOW_SPEC_TOKEN_BUCKET_SIZE          (333)
#define FLOW_SPEC_PEAK_BANDWIDTH             (25000)
#define FLOW_SPEC_ACCESS_LATENCY             (13000)

#define BEST_EFFORT_HIGH_PRIORITY            (50)
#define BEST_EFFORT_NORMAL_PRIORITY          (20)
#define GUARANTEED_HIGH_PRIORITY             (90)
#define GUARANTEED_NORMAL_PRIORITY           (70)


   /* The following type definition represents the container type which */
   /* holds the mapping between Bluetooth devices (based on the BD_ADDR)*/
   /* and the Link Key (BD_ADDR <-> Link Key Mapping).                  */
typedef struct _tagLinkKeyInfo_t
{
   BD_ADDR_t  BD_ADDR;
   Link_Key_t LinkKey;
} LinkKeyInfo_t;

   /* The following type definition represents the structure which holds*/
   /* all information about the parameter, in particular the parameter  */
   /* as a string and the parameter as an unsigned int.                 */
typedef struct _tagParameter_t
{
   char     *strParam;
   SDWord_t  intParam;
} Parameter_t;

   /* The following type definition represents the structure which holds*/
   /* a list of parameters that are to be associated with a command The */
   /* NumberofParameters variable holds the value of the number of      */
   /* parameters in the list.                                           */
typedef struct _tagParameterList_t
{
   int         NumberofParameters;
   Parameter_t Params[MAX_NUM_OF_PARAMETERS];
} ParameterList_t;

   /* The following type definition represents the structure which holds*/
   /* the command and parameters to be executed.                        */
typedef struct _tagUserCommand_t
{
   char            *Command;
   ParameterList_t  Parameters;
} UserCommand_t;

   /* The following type definition represents the generic function     */
   /* pointer to be used by all commands that can be executed by the    */
   /* test program.                                                     */
typedef int (*CommandFunction_t)(ParameterList_t *TempParam);

   /* The following type definition represents the structure which holds*/
   /* information used in the interpretation and execution of Commands. */
typedef struct _tagCommandTable_t
{
   char              *CommandName;
   CommandFunction_t  CommandFunction;
} CommandTable_t;

   /* User to represent a structure to hold a BD_ADDR return from       */
   /* BD_ADDRToStr.                                                     */
typedef char BoardStr_t[15];

   /* The following structure holds status information about a send     */
   /* process.                                                          */
typedef struct _tagSend_Info_t
{
   DWord_t BytesToSend;
   DWord_t BytesSent;
} Send_Info_t;

   /* The following enumerated type represents the valid stream         */
   /* play/pause states that may be entered.                            */
typedef enum
{
   ssStopped,
   ssStarted,
   ssPending
} StreamState_t;

   /* The following enumerated type is used with the                    */
   /* SendRemoteControlCommand() function to specify the type of remote */
   /* control command to send to a connected Bluetooth device.          */
typedef enum
{
   rcPlay,
   rcPause,
   rcPlayPause,
   rcNext,
   rcBack,
   rcVolumeUp,
   rcVolumeDown
} RemoteControlCommand_t;

typedef struct _tagQueuedRemoteControlCommand_t
{
   BD_ADDR_t              BD_ADDR;
   RemoteControlCommand_t Command;
} QueuedRemoteControlCommand_t;

   /* The following MACRO is a utility MACRO that assigns the Audio/    */
   /* Video Remote Control Profile Bluetooth Universally Unique         */
   /* Identifier (AUDIO_VIDEO_REMOTE_CONTROL_PROFILE_UUID_16) to the    */
   /* specified UUID_16_t variable.  This MACRO accepts one parameter   */
   /* which is the UUID_16_t variable that is to receive the            */
   /* AUDIO_VIDEO_REMOTE_CONTROL_PROFILE_UUID_16 Constant value.        */
#ifndef SDP_ASSIGN_AUDIO_VIDEO_REMOTE_CONTROL_CONTROLLER_UUID_16

   #define SDP_ASSIGN_AUDIO_VIDEO_REMOTE_CONTROL_CONTROLLER_UUID_16(_x) ASSIGN_SDP_UUID_16((_x), 0x11, 0x0F)

#endif

   /* Internal Variables to this Module (Remember that all variables    */
   /* declared static are initialized to 0 automatically by the         */
   /* compiler as part of standard C/C++).                              */

static unsigned int        BluetoothStackID;        /* Variable which holds the Handle */
                                                    /* of the opened Bluetooth Protocol*/
                                                    /* Stack.                          */

static BD_ADDR_t           InquiryResultList[MAX_INQUIRY_RESULTS]; /* Variable which   */
                                                    /* contains the inquiry result     */
                                                    /* received from the most recently */
                                                    /* preformed inquiry.              */

static unsigned int        NumberofValidResponses;  /* Variable which holds the number */
                                                    /* of valid inquiry results within */
                                                    /* the inquiry results array.      */

static BD_ADDR_t           CurrentRemoteBD_ADDR;    /* Variable which holds the        */
                                                    /* current BD_ADDR of the device   */
                                                    /* which is currently pairing or   */
                                                    /* authenticating.                 */

static LinkKeyInfo_t       LinkKeyInfo[MAX_SUPPORTED_LINK_KEYS]; /* Variable holds     */
                                                    /* BD_ADDR <-> Link Keys for       */
                                                    /* pairing.                        */

static BD_ADDR_t           A2DPRemoteBD_ADDR;       /* Variable which holds the        */
                                                    /* BD_ADDR of the currently        */
                                                    /* connected A2DP SRC.             */

static GAP_IO_Capability_t IOCapability;            /* Variable which holds the        */
                                                    /* current I/O Capabilities that   */
                                                    /* are to be used for Secure Simple*/
                                                    /* Pairing.                        */

static Boolean_t           OOBSupport;              /* Variable which flags whether    */
                                                    /* or not Out of Band Secure Simple*/
                                                    /* Pairing exchange is supported.  */

static Boolean_t           MITMProtection;          /* Variable which flags whether or */
                                                    /* not Man in the Middle (MITM)    */
                                                    /* protection is to be requested   */
                                                    /* during a Secure Simple Pairing  */
                                                    /* procedure.                      */

static unsigned int        NumberCommands;          /* Variable which is used to hold  */
                                                    /* the number of Commands that are */
                                                    /* supported by this application.  */
                                                    /* Commands are added individually.*/

static BoardStr_t          Callback_BoardStr;       /* Holds a BD_ADDR string in the   */
                                                    /* Callbacks.                      */

static BoardStr_t          Function_BoardStr;       /* Holds a BD_ADDR string in the   */
                                                    /* various functions.              */

static CommandTable_t      CommandTable[MAX_SUPPORTED_COMMANDS]; /* Variable which is  */
                                                    /* used to hold the actual Commands*/
                                                    /* that are supported by this      */
                                                    /* application.                    */

static Boolean_t           AUDInitialized;          /* Variable which is used to signal*/
                                                    /* if the AUD layer has been       */
                                                    /* initialized.                    */

static Boolean_t           A3DPOpened;              /* Variable which is used hold if  */
                                                    /* an A3DP endpoint is currently   */
                                                    /* open.                           */

static Boolean_t           A3DPPlaying;             /* Variable which is used to hold  */
                                                    /* if an opened A3DP stream is     */
                                                    /* currently playing.              */

static StreamState_t       StreamState;             /* Variable which holds the current*/
                                                    /* stream state.                   */

static QueuedRemoteControlCommand_t QueuedCommand;  /* Stores a single AVRCP request to*/
                                                    /* stop the incoming A2DP source.  */

static Byte_t              NumConnected;            /* Number of sources connected.    */
                                                    /* Used to determine if we can     */
                                                    /* become connectable/discoverable.*/

static DWord_t             MaxBaudRate;             /* Variable stores the maximum     */
                                                    /* HCI UART baud rate supported by */
                                                    /* this platform.                  */

   /* The following table holds the supported Sink formats.             */
static BTPSCONST AUD_Stream_Format_t AudioSNKSupportedFormats[] =
{
   { 44100, 2, 0 },
   { 48000, 2, 0 },
   { 48000, 1, 0 },
   { 44100, 1, 0 }
} ;

#define NUM_SNK_SUPPORTED_FORMATS                     (sizeof(AudioSNKSupportedFormats)/sizeof(AUD_Stream_Format_t))

   /* The following string table is used to map HCI Version information */
   /* to an easily displayable version string.                          */
static BTPSCONST char *HCIVersionStrings[] =
{
   "1.0b",
   "1.1",
   "1.2",
   "2.0",
   "2.1",
   "3.0",
   "4.0",
   "4.1",
   "Unknown (greater 4.1)"
} ;

#define NUM_SUPPORTED_HCI_VERSIONS                    (sizeof(HCIVersionStrings)/sizeof(char *) - 1)

   /* The following string table is used to map the API I/O Capabilities*/
   /* values to an easily displayable string.                           */
static BTPSCONST char *IOCapabilitiesStrings[] =
{
   "Display Only",
   "Display Yes/No",
   "Keyboard Only",
   "No Input/Output"
} ;

   /* The following structure is used to hold information of the        */
   /* FIRMWARE version.                                                 */
typedef struct FW_Version_t
{
   Byte_t StatusResult;
   Byte_t HCI_VersionResult;
   Word_t HCI_RevisionResult;
   Byte_t LMP_VersionResult;
   Word_t Manufacturer_NameResult;
   Word_t LMP_SubversionResult;
} FW_Version;

   /* Internal function prototypes.                                     */
static void UserInterface(void);
static Boolean_t CommandLineInterpreter(char *Command);
static unsigned long StringToUnsignedInteger(char *StringInteger);
static char *StringParser(char *String);
static int CommandParser(UserCommand_t *TempCommand, char *Input);
static int CommandInterpreter(UserCommand_t *TempCommand);
static int AddCommand(char *CommandName, CommandFunction_t CommandFunction);
static CommandFunction_t FindCommand(char *Command);
static void ClearCommands(void);

static void BD_ADDRToStr(BD_ADDR_t Board_Address, BoardStr_t BoardStr);
static void DisplayIOCapabilities(void);
static void DisplayClassOfDevice(Class_of_Device_t Class_of_Device);
static void DisplayUsage(char *UsageString);
static void DisplayFunctionError(char *Function,int Status);
static void DisplayFunctionSuccess(char *Function);
static void DisplaySupportedFormats(AUD_Stream_Format_t *Formats, unsigned int NumberFormats);

static int OpenStack(HCI_DriverInformation_t *HCI_DriverInformation, BTPS_Initialization_t *BTPS_Initialization);
static int CloseStack(void);

static int SetDisc(void);
static int SetConnect(void);
static int SetPairable(void);
static int DeleteLinkKey(BD_ADDR_t BD_ADDR);

static void FormatEIRData(unsigned int BluetoothStackID, BD_ADDR_t BD_ADDR);

static int Initialize_Sink(void);

static int DisplayHelp(ParameterList_t *TempParam);
static void DisplayFWVersion (void);
static int SendRemoteControlCommand(RemoteControlCommand_t Command);
static void RemoteControlCommandAsync(unsigned int BluetoothStackID, unsigned long CallbackParameter);
static int QueueRemoteControlCommand(BD_ADDR_t BD_ADDR, RemoteControlCommand_t Command);
static int QueryMemory(ParameterList_t *TempParam);


static int Inquiry(ParameterList_t *TempParam);
static int DisplayInquiryList(ParameterList_t *TempParam);
static int ChangeSimplePairingParameters(ParameterList_t *TempParam);
static int EndPairing(ParameterList_t *TempParam);
static int SetLocalName(ParameterList_t *TempParam);
static int GetLocalName(ParameterList_t *TempParam);
static int GetRemoteName(ParameterList_t *TempParam);
static int SetClassOfDevice(ParameterList_t *TempParam);
static int GetClassOfDevice(ParameterList_t *TempParam);
static int Pair(ParameterList_t *TempParam);
static int PINCodeResponse(ParameterList_t *TempParam);
static int PassKeyResponse(ParameterList_t *TempParam);
static int UserConfirmationResponse(ParameterList_t *TempParam);
static int OpenA3DPEndpoint(ParameterList_t *TempParam);
static int CloseA3DPEndpoint(ParameterList_t *TempParam);
static int SetDiscoverabilityMode(ParameterList_t *TempParam);
static int SetConnectabilityMode(ParameterList_t *TempParam);
static int SetPairabilityMode(ParameterList_t *TempParam);
static int GetLocalAddress(ParameterList_t *TempParam);
static int SetBaudRate(ParameterList_t *TempParam);
static int RemotePlay(ParameterList_t *TempParam);
static int RemotePause(ParameterList_t *TempParam);
static int RemoteNext(ParameterList_t *TempParam);
static int RemotePrev(ParameterList_t *TempParam);
static int PcmLoopback(ParameterList_t *TempParam);

static int StartA3DPStream(BD_ADDR_t BD_ADDR);
static int StopA3DPStream(void);
static int OpenA3DPStream(BD_ADDR_t BD_ADDR);
static int CloseA3DPStream(void);
static int ReconfigureA3DPStream(AUD_Stream_Format_t *Format);
static void ChangeConnectionPriority(int high_normal, Word_t  ConnHandle);

   /* BTPS Callback function prototypes.                                */
static void BTPSAPI AUD_Event_Callback(unsigned int BluetoothStackID, AUD_Event_Data_t *AUD_Event_Data, unsigned long CallbackParameter);
static void BTPSAPI GAP_Event_Callback(unsigned int BluetoothStackID, GAP_Event_Data_t *GAP_Event_Data, unsigned long CallbackParameter);

   /* The following function is responsible for initializing the        */
   /* interface presented to the user.                                  */
static void UserInterface(void)
{
   ClearCommands();

   AddCommand("INQUIRY", Inquiry);
   AddCommand("DISPLAYINQUIRYLIST", DisplayInquiryList);
   AddCommand("PAIR", Pair);
   AddCommand("ENDPAIRING", EndPairing);
   AddCommand("PINCODERESPONSE", PINCodeResponse);
   AddCommand("PASSKEYRESPONSE", PassKeyResponse);
   AddCommand("USERCONFIRMATIONRESPONSE", UserConfirmationResponse);
   AddCommand("SETDISCOVERABILITYMODE", SetDiscoverabilityMode);
   AddCommand("SETCONNECTABILITYMODE", SetConnectabilityMode);
   AddCommand("SETPAIRABILITYMODE", SetPairabilityMode);
   AddCommand("CHANGESIMPLEPAIRINGPARAMETERS", ChangeSimplePairingParameters);
   AddCommand("GETLOCALADDRESS", GetLocalAddress);
   AddCommand("SETLOCALNAME", SetLocalName);
   AddCommand("GETLOCALNAME", GetLocalName);
   AddCommand("SETCLASSOFDEVICE", SetClassOfDevice);
   AddCommand("GETCLASSOFDEVICE", GetClassOfDevice);
   AddCommand("GETREMOTENAME", GetRemoteName);
   AddCommand("SETBAUDRATE", SetBaudRate);
   AddCommand("HELP", DisplayHelp);
   AddCommand("OPENSINK", OpenA3DPEndpoint);
   AddCommand("CLOSESINK", CloseA3DPEndpoint);
   AddCommand("REMOTEPLAY", RemotePlay);
   AddCommand("REMOTEPAUSE", RemotePause);
   AddCommand("REMOTENEXT", RemoteNext);
   AddCommand("REMOTEPREV", RemotePrev);
   AddCommand("PCMLOOPBACK", PcmLoopback);
   AddCommand("QUERYMEMORY", QueryMemory);

   /* Next display the available commands.                              */
   DisplayHelp(NULL);
}

   /* The following function is responsible for parsing user input      */
   /* and call appropriate command function.                            */
static Boolean_t CommandLineInterpreter(char *Command)
{
   int           Result = !EXIT_CODE;
   Boolean_t     ret_val = FALSE;
   UserCommand_t TempCommand;

   /* The string input by the user contains a value, now run the string */
   /* through the Command Parser.                                       */
   if(CommandParser(&TempCommand, Command) >= 0)
   {
      Display(("\r\n"));

      /* The Command was successfully parsed run the Command.           */
      Result = CommandInterpreter(&TempCommand);
      switch(Result)
      {
         case INVALID_COMMAND_ERROR:
            Display(("Invalid Command: %s.\r\n",TempCommand.Command));
            break;
         case FUNCTION_ERROR:
            Display(("Function Error.\r\n"));
            break;
         case EXIT_CODE:
            /* Set up the Selection Interface.                          */
            UserInterface();
            break;
      }

      /* Display a prompt.                                              */
      DisplayPrompt();

      ret_val = TRUE;
   }
   else
   {
      /* Display a prompt.                                              */
      DisplayPrompt();

      Display(("\r\nInvalid Command.\r\n"));
   }

   return(ret_val);
}

   /* The following function is responsible for converting number       */
   /* strings to there unsigned integer equivalent.  This function can  */
   /* handle leading and tailing white space, however it does not handle*/
   /* signed or comma delimited values.  This function takes as its     */
   /* input the string which is to be converted.  The function returns  */
   /* zero if an error occurs otherwise it returns the value parsed from*/
   /* the string passed as the input parameter.                         */
static unsigned long StringToUnsignedInteger(char *StringInteger)
{
   int           IsHex;
   unsigned int  Index;
   unsigned long ret_val = 0;

   /* Before proceeding make sure that the parameter that was passed as */
   /* an input appears to be at least semi-valid.                       */
   if((StringInteger) && (BTPS_StringLength(StringInteger)))
   {
      /* Initialize the variable.                                       */
      Index = 0;

      /* Next check to see if this is a hexadecimal number.             */
      if(BTPS_StringLength(StringInteger) > 2)
      {
         if((StringInteger[0] == '0') && ((StringInteger[1] == 'x') || (StringInteger[1] == 'X')))
         {
            IsHex = 1;

            /* Increment the String passed the Hexadecimal prefix.      */
            StringInteger += 2;
         }
         else
            IsHex = 0;
      }
      else
         IsHex = 0;

      /* Process the value differently depending on whether or not a    */
      /* Hexadecimal Number has been specified.                         */
      if(!IsHex)
      {
         /* Decimal Number has been specified.                          */
         while(1)
         {
            /* First check to make sure that this is a valid decimal    */
            /* digit.                                                   */
            if((StringInteger[Index] >= '0') && (StringInteger[Index] <= '9'))
            {
               /* This is a valid digit, add it to the value being      */
               /* built.                                                */
               ret_val += (StringInteger[Index] & 0xF);

               /* Determine if the next digit is valid.                 */
               if(((Index + 1) < BTPS_StringLength(StringInteger)) && (StringInteger[Index+1] >= '0') && (StringInteger[Index+1] <= '9'))
               {
                  /* The next digit is valid so multiply the current    */
                  /* return value by 10.                                */
                  ret_val *= 10;
               }
               else
               {
                  /* The next value is invalid so break out of the loop.*/
                  break;
               }
            }

            Index++;
         }
      }
      else
      {
         /* Hexadecimal Number has been specified.                      */
         while(1)
         {
            /* First check to make sure that this is a valid Hexadecimal*/
            /* digit.                                                   */
            if(((StringInteger[Index] >= '0') && (StringInteger[Index] <= '9')) || ((StringInteger[Index] >= 'a') && (StringInteger[Index] <= 'f')) || ((StringInteger[Index] >= 'A') && (StringInteger[Index] <= 'F')))
            {
               /* This is a valid digit, add it to the value being      */
               /* built.                                                */
               if((StringInteger[Index] >= '0') && (StringInteger[Index] <= '9'))
                  ret_val += (StringInteger[Index] & 0xF);
               else
               {
                  if((StringInteger[Index] >= 'a') && (StringInteger[Index] <= 'f'))
                     ret_val += (StringInteger[Index] - 'a' + 10);
                  else
                     ret_val += (StringInteger[Index] - 'A' + 10);
               }

               /* Determine if the next digit is valid.                 */
               if(((Index + 1) < BTPS_StringLength(StringInteger)) && (((StringInteger[Index+1] >= '0') && (StringInteger[Index+1] <= '9')) || ((StringInteger[Index+1] >= 'a') && (StringInteger[Index+1] <= 'f')) || ((StringInteger[Index+1] >= 'A') && (StringInteger[Index+1] <= 'F'))))
               {
                  /* The next digit is valid so multiply the current    */
                  /* return value by 16.                                */
                  ret_val *= 16;
               }
               else
               {
                  /* The next value is invalid so break out of the loop.*/
                  break;
               }
            }

            Index++;
         }
      }
   }

   return(ret_val);
}

   /* The following function is responsible for parsing strings into    */
   /* components.  The first parameter of this function is a pointer to */
   /* the String to be parsed.  This function will return the start of  */
   /* the string upon success and a NULL pointer on all errors.         */
static char *StringParser(char *String)
{
   int   Index;
   char *ret_val = NULL;

   /* Before proceeding make sure that the string passed in appears to  */
   /* be at least semi-valid.                                           */
   if((String) && (BTPS_StringLength(String)))
   {
      /* The string appears to be at least semi-valid.  Search for the  */
      /* first space character and replace it with a NULL terminating   */
      /* character.                                                     */
      for(Index=0, ret_val=String;Index < BTPS_StringLength(String);Index++)
      {
         /* Is this the space character.                                */
         if((String[Index] == ' ') || (String[Index] == '\r') || (String[Index] == '\n'))
         {
            /* This is the space character, replace it with a NULL      */
            /* terminating character and set the return value to the    */
            /* beginning character of the string.                       */
            String[Index] = '\0';
            break;
         }
      }
   }

   return(ret_val);
}

   /* This function is responsible for taking command strings and       */
   /* parsing them into a command, param1, and param2.  After parsing   */
   /* this string the data is stored into a UserCommand_t structure to  */
   /* be used by the interpreter.  The first parameter of this function */
   /* is the structure used to pass the parsed command string out of the*/
   /* function.  The second parameter of this function is the string    */
   /* that is parsed into the UserCommand structure.  Successful        */
   /* execution of this function is denoted by a return value of zero.  */
   /* Negative return values denote an error in the parsing of the      */
   /* string parameter.                                                 */
static int CommandParser(UserCommand_t *TempCommand, char *Input)
{
   int            ret_val;
   int            StringLength;
   char          *LastParameter;
   unsigned int   Count         = 0;

   /* Before proceeding make sure that the passed parameters appear to  */
   /* be at least semi-valid.                                           */
   if((TempCommand) && (Input) && (BTPS_StringLength(Input)))
   {
      /* First get the initial string length.                           */
      StringLength = BTPS_StringLength(Input);

      /* Retrieve the first token in the string, this should be the     */
      /* command.                                                       */
      TempCommand->Command = StringParser(Input);

      /* Flag that there are NO Parameters for this Command Parse.      */
      TempCommand->Parameters.NumberofParameters = 0;

       /* Check to see if there is a Command                            */
      if(TempCommand->Command)
      {
         /* Initialize the return value to zero to indicate success on  */
         /* commands with no parameters.                                */
         ret_val    = 0;

         /* Adjust the UserInput pointer and StringLength to remove the */
         /* Command from the data passed in before parsing the          */
         /* parameters.                                                 */
         Input        += BTPS_StringLength(TempCommand->Command)+1;
         StringLength  = BTPS_StringLength(Input);

         /* There was an available command, now parse out the parameters*/
         while((StringLength > 0) && ((LastParameter = StringParser(Input)) != NULL))
         {
            /* There is an available parameter, now check to see if     */
            /* there is room in the UserCommand to store the parameter  */
            if(Count < (sizeof(TempCommand->Parameters.Params)/sizeof(Parameter_t)))
            {
               /* Save the parameter as a string.                       */
               TempCommand->Parameters.Params[Count].strParam = LastParameter;

               /* Save the parameter as an unsigned int intParam will   */
               /* have a value of zero if an error has occurred.        */
               TempCommand->Parameters.Params[Count].intParam = StringToUnsignedInteger(LastParameter);

               Count++;
               Input        += BTPS_StringLength(LastParameter)+1;
               StringLength -= BTPS_StringLength(LastParameter)+1;

               ret_val = 0;
            }
            else
            {
               /* Be sure we exit out of the Loop.                      */
               StringLength = 0;

               ret_val      = TO_MANY_PARAMS;
            }
         }

         /* Set the number of parameters in the User Command to the     */
         /* number of found parameters                                  */
         TempCommand->Parameters.NumberofParameters = Count;
      }
      else
      {
         /* No command was specified                                    */
         ret_val = NO_COMMAND_ERROR;
      }
   }
   else
   {
      /* One or more of the passed parameters appear to be invalid.     */
      ret_val = INVALID_PARAMETERS_ERROR;
   }

   return(ret_val);
}

   /* This function is responsible for determining the command in which */
   /* the user entered and running the appropriate function associated  */
   /* with that command.  The first parameter of this function is a     */
   /* structure containing information about the command to be issued.  */
   /* This information includes the command name and multiple parameters*/
   /* which maybe be passed to the function to be executed.  Successful */
   /* execution of this function is denoted by a return value of zero.  */
   /* A negative return value implies that that command was not found   */
   /* and is invalid.                                                   */
static int CommandInterpreter(UserCommand_t *TempCommand)
{
   int               i;
   int               ret_val;
   CommandFunction_t CommandFunction;

   /* If the command is not found in the table return with an invalid   */
   /* command error                                                     */
   ret_val = INVALID_COMMAND_ERROR;

   /* Let's make sure that the data passed to us appears semi-valid.    */
   if((TempCommand) && (TempCommand->Command))
   {
      /* Now, let's make the Command string all upper case so that we   */
      /* compare against it.                                            */
      for(i=0;i<BTPS_StringLength(TempCommand->Command);i++)
      {
         if((TempCommand->Command[i] >= 'a') && (TempCommand->Command[i] <= 'z'))
            TempCommand->Command[i] -= ('a' - 'A');
      }

      /* Check to see if the command which was entered was exit.        */
      if(BTPS_MemCompare(TempCommand->Command, "QUIT", BTPS_StringLength("QUIT")) != 0)
      {
         /* The command entered is not exit so search for command in    */
         /* table.                                                      */
         if((CommandFunction = FindCommand(TempCommand->Command)) != NULL)
         {
            /* The command was found in the table so call the command.  */
            ret_val = (*CommandFunction)(&TempCommand->Parameters);
            if(!ret_val)
            {
               /* Return success to the caller.                         */
               ret_val = 0;
            }
            else
            {
               if((ret_val != EXIT_CODE) && (ret_val != EXIT_MODE))
                  ret_val = FUNCTION_ERROR;
            }
         }
      }
      else
      {
         /* The command entered is exit, set return value to EXIT_CODE  */
         /* and return.                                                 */
         ret_val = EXIT_CODE;
      }
   }
   else
      ret_val = INVALID_PARAMETERS_ERROR;

   return(ret_val);
}

   /* The following function is provided to allow a means to            */
   /* programmatically add Commands the Global (to this module) Command */
   /* Table.  The Command Table is simply a mapping of Command Name     */
   /* (NULL terminated ASCII string) to a command function.  This       */
   /* function returns zero if successful, or a non-zero value if the   */
   /* command could not be added to the list.                           */
static int AddCommand(char *CommandName, CommandFunction_t CommandFunction)
{
   int ret_val = 0;

   /* First, make sure that the parameters passed to us appear to be    */
   /* semi-valid.                                                       */
   if((CommandName) && (CommandFunction))
   {
      /* Next, make sure that we still have room in the Command Table   */
      /* to add commands.                                               */
      if(NumberCommands < MAX_SUPPORTED_COMMANDS)
      {
         /* Simply add the command data to the command table and        */
         /* increment the number of supported commands.                 */
         CommandTable[NumberCommands].CommandName       = CommandName;
         CommandTable[NumberCommands++].CommandFunction = CommandFunction;

         /* Return success to the caller.                               */
         ret_val                                        = 0;
      }
      else
         ret_val = 1;
   }
   else
      ret_val = 1;

   return(ret_val);
}

   /* The following function searches the Command Table for the         */
   /* specified Command.  If the Command is found, this function returns*/
   /* a NON-NULL Command Function Pointer.  If the command is not found */
   /* this function returns NULL.                                       */
static CommandFunction_t FindCommand(char *Command)
{
   unsigned int      Index;
   CommandFunction_t ret_val;

   /* First, make sure that the command specified is semi-valid.        */
   if(Command)
   {
      /* Now loop through each element in the table to see if there is  */
      /* a match.                                                       */
      for(Index=0,ret_val=NULL;((Index<NumberCommands) && (!ret_val));Index++)
      {
         if((BTPS_StringLength(CommandTable[Index].CommandName) == BTPS_StringLength(Command)) && (BTPS_MemCompare(Command, CommandTable[Index].CommandName, BTPS_StringLength(CommandTable[Index].CommandName)) == 0))
            ret_val = CommandTable[Index].CommandFunction;
      }
   }
   else
      ret_val = NULL;

   return(ret_val);
}

   /* The following function is provided to allow a means to clear out  */
   /* all available commands from the command table.                    */
static void ClearCommands(void)
{
   /* Simply flag that there are no commands present in the table.      */
   NumberCommands = 0;
}

   /* The following function is responsible for converting data of type */
   /* BD_ADDR to a string.  The first parameter of this function is the */
   /* BD_ADDR to be converted to a string.  The second parameter of this*/
   /* function is a pointer to the string in which the converted BD_ADDR*/
   /* is to be stored.                                                  */
static void BD_ADDRToStr(BD_ADDR_t Board_Address, BoardStr_t BoardStr)
{
   BTPS_SprintF((char *)BoardStr, "0x%02X%02X%02X%02X%02X%02X", Board_Address.BD_ADDR5, Board_Address.BD_ADDR4, Board_Address.BD_ADDR3, Board_Address.BD_ADDR2, Board_Address.BD_ADDR1, Board_Address.BD_ADDR0);
}

   /* Displays the current I/O Capabilities.                            */
static void DisplayIOCapabilities(void)
{
   Display(("I/O Capabilities: %s, MITM: %s.\r\n", IOCapabilitiesStrings[(unsigned int)IOCapability], MITMProtection?"TRUE":"FALSE"));
}

   /* Utility function to display a Class of Device Structure.          */
static void DisplayClassOfDevice(Class_of_Device_t Class_of_Device)
{
   Display(("Class of Device: 0x%02X%02X%02X.\r\n", Class_of_Device.Class_of_Device0, Class_of_Device.Class_of_Device1, Class_of_Device.Class_of_Device2));
}

   /* Displays a usage string..                                         */
static void DisplayUsage(char *UsageString)
{
   Display(("\nUsage: %s.\r\n", UsageString));
}

   /* Displays a function error.                                        */
static void DisplayFunctionError(char *Function, int Status)
{
   Display(("\n%s Failed: %d.\r\n", Function, Status));
}

static void DisplayFunctionSuccess(char *Function)
{
   Display(("\n%s success.\r\n", Function));
}

   /* The following function is responsible for opening the SS1         */
   /* Bluetooth Protocol Stack.  This function accepts a pre-populated  */
   /* HCI Driver Information structure that contains the HCI Driver     */
   /* Transport Information.  This function returns zero on successful  */
   /* execution and a negative value on all errors.                     */
static int OpenStack(HCI_DriverInformation_t *HCI_DriverInformation, BTPS_Initialization_t *BTPS_Initialization)
{
   int                        Result;
   int                        ret_val = 0;
   char                       BluetoothAddress[15];
   Byte_t                     Status;
   BD_ADDR_t                  BD_ADDR;
   HCI_Version_t              HCIVersion;
   unsigned long              ActiveFeatures;
   Class_of_Device_t          ClassOfDevice;
   L2CA_Link_Connect_Params_t L2CA_Link_Connect_Params;

   /* First check to see if the Stack has already been opened.          */
   if(!BluetoothStackID)
   {
      /* Next, makes sure that the Driver Information passed appears to */
      /* be semi-valid.                                                 */
      if((HCI_DriverInformation) && (BTPS_Initialization))
      {

         /* Initialize BTPSKRNL.                                        */
         BTPS_Init(BTPS_Initialization);

         Display(("\r\nOpenStack().\r\n"));

         /* Initialize the Stack                                        */
         Result = BSC_Initialize(HCI_DriverInformation, 0);

         /* Next, check the return value of the initialization to see if*/
         /* it was successful.                                          */
         if(Result > 0)
         {
            /* The Stack was initialized successfully, inform the user  */
            /* and set the return value of the initialization function  */
            /* to the Bluetooth Stack ID.                               */
            BluetoothStackID = Result;
            Display(("Bluetooth Stack ID: %d\r\n", BluetoothStackID));

            /* Attempt to enable the A3DP Sink Feature.                 */
            if((Result = BSC_EnableFeature(BluetoothStackID, BSC_FEATURE_A3DP_SINK)) != 0)
               DisplayFunctionError("BSC_EnableFeature()", Result);

            /* Initialize the default Secure Simple Pairing parameters. */
            IOCapability     = DEFAULT_IO_CAPABILITY;
            OOBSupport       = FALSE;
            MITMProtection   = DEFAULT_MITM_PROTECTION;

            if(!HCI_Version_Supported(BluetoothStackID, &HCIVersion))
            {
               Display(("Device Chipset: %s\r\n",
                (HCIVersion <= NUM_SUPPORTED_HCI_VERSIONS) ?
                    HCIVersionStrings[HCIVersion] : HCIVersionStrings[NUM_SUPPORTED_HCI_VERSIONS]));
            }

            /* Printing the BTPS version                                */
            Display(("BTPS Version  : %s \r\n", BTPS_VERSION_VERSION_STRING));
            /* Printing the FW version                                  */
            DisplayFWVersion();

            /* Printing the Demo Application name and version           */
            Display(("App Name      : %s \r\n", APP_DEMO_NAME));
            Display(("App Version   : %s \r\n", DEMO_APPLICATION_VERSION_STRING));

            /* Let's output the Bluetooth Device Address so that the    */
            /* user knows what the Device Address is.                   */
            if(!GAP_Query_Local_BD_ADDR(BluetoothStackID, &BD_ADDR))
            {
               BD_ADDRToStr(BD_ADDR, BluetoothAddress);

               Display(("BD_ADDR: %s\r\n", BluetoothAddress));
            }

            /* For the A3DP Sink board we will configure EIR data.      */
            FormatEIRData(BluetoothStackID, BD_ADDR);

            /* Go ahead and allow Master/Slave Role Switch.             */
            L2CA_Link_Connect_Params.L2CA_Link_Connect_Request_Config  = cqAllowRoleSwitch;
            L2CA_Link_Connect_Params.L2CA_Link_Connect_Response_Config = csMaintainCurrentRole;

            L2CA_Set_Link_Connection_Configuration(BluetoothStackID, &L2CA_Link_Connect_Params);

            if(HCI_Command_Supported(BluetoothStackID,
                HCI_SUPPORTED_COMMAND_WRITE_DEFAULT_LINK_POLICY_BIT_NUMBER) > 0)
            {
               HCI_Write_Default_Link_Policy_Settings(BluetoothStackID,
                (HCI_LINK_POLICY_SETTINGS_ENABLE_MASTER_SLAVE_SWITCH|HCI_LINK_POLICY_SETTINGS_ENABLE_SNIFF_MODE),
                 &Status);
            }

            /* Delete all Stored Link Keys.                             */
            BTPS_MemInitialize(&BD_ADDR, 0, sizeof(BD_ADDR));

            DeleteLinkKey(BD_ADDR);

            /* Verify that the Sink Role has been initialized           */
            /* successfully.                                            */
            if(((ret_val = BSC_QueryActiveFeatures(BluetoothStackID, &ActiveFeatures)) == 0) &&
                (ActiveFeatures & BSC_FEATURE_A3DP_SINK))
            {
               /* Attempt to initialize the sink.                       */
               if((ret_val = Initialize_Sink()) == 0)
               {
                  Display(("A3DP endpoint opened successfully.\r\n"));

                  /* Go ahead and set the Class of Device.              */
                  ASSIGN_CLASS_OF_DEVICE(ClassOfDevice, 0x24, 0x04, 0x04);
                  DisplayClassOfDevice(ClassOfDevice);
                  GAP_Set_Class_Of_Device(BluetoothStackID, ClassOfDevice);
               }
               else
                  Display(("Initialize AUD Sink: %d\r\n", ret_val));
            }
            else
            {
               Display(("Required A3DP Sink role not enabled.\r\n"));
            }
            /* Display the support Stream Formats.                      */
            DisplaySupportedFormats((AUD_Stream_Format_t *)AudioSNKSupportedFormats,
                                        NUM_SNK_SUPPORTED_FORMATS);

         }
         else
         {
            /* The Stack was NOT initialized successfully, inform the   */
            /* user and set the return value of the initialization      */
            /* function to an error.                                    */
            DisplayFunctionError("Stack Init", Result);

            BluetoothStackID = 0;

            ret_val          = UNABLE_TO_INITIALIZE_STACK;
         }
      }
      else
      {
         /* One or more of the necessary parameters are invalid.        */
         ret_val = INVALID_PARAMETERS_ERROR;
      }
   }

   return(ret_val);
}

   /* The following function is responsible for closing the SS1         */
   /* Bluetooth Protocol Stack.  This function requires that the        */
   /* Bluetooth Protocol stack previously have been initialized via the */
   /* OpenStack() function.  This function returns zero on successful   */
   /* execution and a negative value on all errors.                     */
static int CloseStack(void)
{
   int ret_val = 0;

   /* First check to see if the Stack has been opened.                  */
   if(BluetoothStackID)
   {
      /* Simply close the Stack                                         */
      BSC_Shutdown(BluetoothStackID);

      /* Free BTPSKRNL allocated memory.                                */
      BTPS_DeInit();

      Display(("Stack Shutdown.\r\n"));

      /* Flag that the Stack is no longer initialized.                  */
      BluetoothStackID = 0;

      /* Flag success to the caller.                                    */
      ret_val          = 0;
   }
   else
   {
      /* A valid Stack ID does not exist, inform to user.               */
      ret_val = UNABLE_TO_INITIALIZE_STACK;
   }

   return(ret_val);
}

   /* The following function is responsible for placing the Local       */
   /* Bluetooth Device into General Discoverability Mode.  Once in this */
   /* mode the Device will respond to Inquiry Scans from other Bluetooth*/
   /* Devices.  This function requires that a valid Bluetooth Stack ID  */
   /* exists before running.  This function returns zero on successful  */
   /* execution and a negative value if an error occurred.              */
static int SetDisc(void)
{
   int ret_val = 0;

   /* First, check that a valid Bluetooth Stack ID exists.              */
   if(BluetoothStackID)
   {
      /* A semi-valid Bluetooth Stack ID exists, now attempt to set the */
      /* attached Devices Discoverability Mode to General.              */
      ret_val = GAP_Set_Discoverability_Mode(BluetoothStackID, dmGeneralDiscoverableMode, 0);

      /* Next, check the return value of the GAP Set Discoverability    */
      /* Mode command for successful execution.                         */
      if(ret_val)
      {
         /* An error occurred while trying to set the Discoverability   */
         /* Mode of the Device.                                         */
         DisplayFunctionError("Set Discoverable Mode", ret_val);
      }
   }
   else
   {
      /* No valid Bluetooth Stack ID exists.                            */
      ret_val = INVALID_STACK_ID_ERROR;
   }

   return(ret_val);
}

   /* The following function is responsible for placing the Local       */
   /* Bluetooth Device into Connectable Mode.  Once in this mode the    */
   /* Device will respond to Page Scans from other Bluetooth Devices.   */
   /* This function requires that a valid Bluetooth Stack ID exists     */
   /* before running.  This function returns zero on success and a      */
   /* negative value if an error occurred.                              */
static int SetConnect(void)
{
   int ret_val = 0;

   /* First, check that a valid Bluetooth Stack ID exists.              */
   if(BluetoothStackID)
   {
      /* Attempt to set the attached Device to be Connectable.          */
      ret_val = GAP_Set_Connectability_Mode(BluetoothStackID, cmConnectableMode);

      /* Next, check the return value of the                            */
      /* GAP_Set_Connectability_Mode() function for successful          */
      /* execution.                                                     */
      if(ret_val)
      {
         /* An error occurred while trying to make the Device           */
         /* Connectable.                                                */
         DisplayFunctionError("Set Connectability Mode", ret_val);
      }
   }
   else
   {
      /* No valid Bluetooth Stack ID exists.                            */
      ret_val = INVALID_STACK_ID_ERROR;
   }

   return(ret_val);
}

   /* The following function is responsible for placing the local       */
   /* Bluetooth device into Pairable mode.  Once in this mode the device*/
   /* will response to pairing requests from other Bluetooth devices.   */
   /* This function returns zero on successful execution and a negative */
   /* value on all errors.                                              */
static int SetPairable(void)
{
   int Result;
   int ret_val = 0;

   /* First, check that a valid Bluetooth Stack ID exists.              */
   if(BluetoothStackID)
   {
      /* Attempt to set the attached device to be pairable.             */
      Result = GAP_Set_Pairability_Mode(BluetoothStackID, pmPairableMode_EnableSecureSimplePairing);

      /* Next, check the return value of the GAP Set Pairability Mode   */
      /* command for successful execution.                              */
      if(!Result)
      {
         /* The device has been set to pairable mode, now register an   */
         /* Authentication Callback to handle the Authentication events */
         /* if required.                                                */
         Result = GAP_Register_Remote_Authentication(BluetoothStackID, GAP_Event_Callback, (unsigned long)0);

         /* Next, check the return value of the GAP Register Remote     */
         /* Authentication command for successful execution.            */
         if(Result)
         {
            /* An error occurred while trying to execute this function. */
            DisplayFunctionError("Auth", Result);

            ret_val = Result;
         }
      }
      else
      {
         /* An error occurred while trying to make the device pairable. */
         DisplayFunctionError("Set Pairability Mode", Result);

         ret_val = Result;
      }
   }
   else
   {
      /* No valid Bluetooth Stack ID exists.                            */
      ret_val = INVALID_STACK_ID_ERROR;
   }

   return(ret_val);
}

   /* The following function is a utility function that exists to delete*/
   /* the specified Link Key from the Local Bluetooth Device.  If a NULL*/
   /* Bluetooth Device Address is specified, then all Link Keys will be */
   /* deleted.                                                          */
static int DeleteLinkKey(BD_ADDR_t BD_ADDR)
{
   int       Result;
   Byte_t    Status_Result;
   Word_t    Num_Keys_Deleted = 0;

   Result = HCI_Delete_Stored_Link_Key(BluetoothStackID, BD_ADDR, TRUE, &Status_Result, &Num_Keys_Deleted);

   /* Any stored link keys for the specified address (or all) have been */
   /* deleted from the chip.  Now, let's make sure that our stored Link */
   /* Key Array is in sync with these changes.                          */

   /* First check to see all Link Keys were deleted.                    */
   if(COMPARE_NULL_BD_ADDR(BD_ADDR))
      BTPS_MemInitialize(LinkKeyInfo, 0, sizeof(LinkKeyInfo));
   else
   {
      /* Individual Link Key.  Go ahead and see if know about the entry */
      /* in the list.                                                   */
      for(Result=0;(Result<sizeof(LinkKeyInfo)/sizeof(LinkKeyInfo_t));Result++)
      {
         if(COMPARE_BD_ADDR(BD_ADDR, LinkKeyInfo[Result].BD_ADDR))
         {
            BTPS_MemInitialize(&LinkKeyInfo[Result].BD_ADDR, 0, sizeof(BD_ADDR_t));

            break;
         }
      }
   }

   return(Result);
}

   /* The following function is a utility function which exists to      */
   /* format the EIR Data that is used by this application.             */
static void FormatEIRData(unsigned int BluetoothStackID, BD_ADDR_t BD_ADDR)
{
   Byte_t                           *TempPtr;
   Byte_t                            Status;
   char                              DeviceName[60];
   SByte_t                           TxPower;
   unsigned int                      Length;
   unsigned int                      StringLength;
   Extended_Inquiry_Response_Data_t  EIRData;

   /* First, check that valid Bluetooth Stack ID exists.                */
   if(BluetoothStackID)
   {
      /* Begin formatting the EIR Data.                                 */
      Length  = 0;
      TempPtr = EIRData.Extended_Inquiry_Response_Data;

      BTPS_MemInitialize(TempPtr, 0, EXTENDED_INQUIRY_RESPONSE_DATA_SIZE);

      /* Format the Inquiry Response Transmit Power Level.              */
      *TempPtr++ = 2;
      *TempPtr++ = HCI_EXTENDED_INQUIRY_RESPONSE_DATA_TYPE_TX_POWER_LEVEL;

      if((HCI_Read_Inquiry_Response_Transmit_Power_Level(BluetoothStackID, &Status, &TxPower)) || (Status))
         *TempPtr++ = 0;
      else
         *TempPtr++ = (Byte_t)TxPower;

      Length += 3;

      /* Format the Service Class List.                                 */
      *TempPtr++ = 1 + (UUID_16_SIZE<<1);
      *TempPtr++ = HCI_EXTENDED_INQUIRY_RESPONSE_DATA_TYPE_16_BIT_SERVICE_CLASS_UUID_COMPLETE;

      /* Assign the A2DP Sink UUID.                                     */
      SDP_ASSIGN_ADVANCED_AUDIO_DISTRIBUTION_AUDIO_SINK_UUID_16(*((UUID_16_t *)TempPtr));

      /* Swap the Endian-ness (above macro assigns in big endian        */
      /* format).                                                       */
      Status     = TempPtr[1];
      TempPtr[1] = TempPtr[0];
      TempPtr[0] = Status;

      TempPtr += UUID_16_SIZE;

      /* Assign the AVRCP UUID.                                         */
      SDP_ASSIGN_AUDIO_VIDEO_REMOTE_CONTROL_CONTROLLER_UUID_16(*((UUID_16_t *)TempPtr));

      /* Swap the Endian-ness (above macro assigns in big endian        */
      /* format).                                                       */
      Status     = TempPtr[1];
      TempPtr[1] = TempPtr[0];
      TempPtr[0] = Status;

      TempPtr += UUID_16_SIZE;

      /* Increment the length of the EIR data.                          */
      Length  += 2 + (UUID_16_SIZE<<1);

      /* Format the local device name.                                  */
      BTPS_SprintF(DeviceName, APP_DEMO_NAME);

      StringLength = BTPS_StringLength(DeviceName);
      if(StringLength <= (EXTENDED_INQUIRY_RESPONSE_DATA_MAXIMUM_SIZE - Length - 2))
      {
         *TempPtr++ = StringLength+1;
         *TempPtr++ = HCI_EXTENDED_INQUIRY_RESPONSE_DATA_TYPE_LOCAL_NAME_COMPLETE;
      }
      else
      {
         StringLength = EXTENDED_INQUIRY_RESPONSE_DATA_MAXIMUM_SIZE - Length - 2;
         *TempPtr++   = StringLength+1;
         *TempPtr++   = HCI_EXTENDED_INQUIRY_RESPONSE_DATA_TYPE_LOCAL_NAME_SHORTENED;
      }

      BTPS_MemCopy(TempPtr, DeviceName, StringLength);

      TempPtr += StringLength;
      Length  += StringLength+2;

      /* Also set the local name through the GAP Interface.             */
      GAP_Set_Local_Device_Name(BluetoothStackID, DeviceName);

      /* Write the Extended Inquiry Response Data.                      */
      GAP_Write_Extended_Inquiry_Information(BluetoothStackID, HCI_EXTENDED_INQUIRY_RESPONSE_FEC_REQUIRED, &EIRData);
   }
}

   /* The following function is a utility function which is used to     */
   /* initialize the audio layer.                                       */
static int Initialize_Sink(void)
{
   int ret_val;
   AUD_Initialization_Info_t                InitializationInfo;
   AUD_Stream_Initialization_Info_t         InitializationInfoSNK;
   AUD_Remote_Control_Initialization_Info_t InitializationInfoAVR;
   AUD_Remote_Control_Role_Info_t           RemoteControlRoleInfo;

   /* First, check to make sure that a valid Bluetooth Stack ID exists. */
   if(BluetoothStackID)
   {
      /* Next, check to make sure that the Audio Manager has not already*/
      /* been initialized.                                              */
      if(!AUDInitialized)
      {

         /* Set up our stream configuration parameters.                 */

#if defined(__SUPPORT_CC256X_PATCH__)

         /* Limit the older chips CC256XA to one concurrent stream.     */
         InitializationInfoSNK.InitializationFlags          = 0;
         InitializationInfoSNK.NumberConcurrentStreams      = 1;

#else

         /* Only support multiple sources on the CC256XB and above.     */
         InitializationInfoSNK.InitializationFlags          = AUD_STREAM_INITIALIZATION_FLAGS_NUMBER_CONCURRENT_STREAMS_PRESENT | AUD_STREAM_INITIALIZATION_FLAGS_GROUP_CONCURRENT_STREAMS;
         InitializationInfoSNK.NumberConcurrentStreams      = MAX_SOURCES;

#endif

         InitializationInfoSNK.EndpointSDPDescription       = "A3DP Sink";
         InitializationInfoSNK.NumberSupportedStreamFormats = NUM_SNK_SUPPORTED_FORMATS;

         /* Initialize the stream configuration supported format list.  */
         BTPS_MemCopy(InitializationInfoSNK.StreamFormat, AudioSNKSupportedFormats, sizeof(AudioSNKSupportedFormats));

         /* Set up the remote control role configuration.               */
         RemoteControlRoleInfo.SupportedFeaturesFlags       = SDP_AVRCP_SUPPORTED_FEATURES_CONTROLLER_CATEGORY_1;
         RemoteControlRoleInfo.ProviderName                 = "Texas Instruments";
         RemoteControlRoleInfo.ServiceName                  = "A3DP Sink";

         /* Set up the remaining AVRCP Controller configuration.        */
         InitializationInfoAVR.InitializationFlags          = 0;
         InitializationInfoAVR.TargetRoleInfo               = NULL;
         InitializationInfoAVR.ControllerRoleInfo           = &RemoteControlRoleInfo;
         InitializationInfoAVR.SupportedVersion             = apvVersion1_0;

         /* Set up the rest of the AUD initialization data.             */
         InitializationInfo.InitializationFlags             = 0;
         InitializationInfo.RemoteControlInitializationInfo = &InitializationInfoAVR;
         InitializationInfo.SRCInitializationInfo           = NULL;
         InitializationInfo.SNKInitializationInfo           = &InitializationInfoSNK;
         InitializationInfo.InitializationFlags             = AUD_INITIALIZATION_INFO_FLAGS_OVERRIDE_MEDIA_MTU;
         InitializationInfo.MediaMTU                        = 800;

         /* Initialize AUD now that all of its required structures have */
         /* been populated.                                             */
         ret_val = AUD_Initialize(BluetoothStackID, &InitializationInfo, AUD_Event_Callback, 0);

         if(!ret_val)
         {
            AUDInitialized = TRUE;

            /* Set the bitpool parameters to the mandatory values for an*/
            /* A2DP sink.                                               */
            AUD_Change_Media_Codec_Parameters(BluetoothStackID, astSNK, 2, 250);
         }
         else
            DisplayFunctionError("AUD_Initialize()", ret_val);
      }
      else
         ret_val = FUNCTION_ERROR;
   }
   else
      ret_val = FUNCTION_ERROR;

   return(ret_val);
}

   /* This function is responsible for taking a list of supported AUD   */
   /* formats and printing them out to the user.                        */
static void DisplaySupportedFormats(AUD_Stream_Format_t *Formats, unsigned int  NumberFormats)
{
   unsigned int Index;

   Display(("Supported formats:\r\n"));
   for(Index=0;Index<NumberFormats;Index++)
      Display(("   Frequency: %lu, Channels: %d, Flags: %lu\r\n", Formats[Index].SampleFrequency, Formats[Index].NumberChannels, Formats[Index].FormatFlags));
}

static void ChangeConnectionPriority(int high_normal, Word_t  ConnHandle)
{
   /* The HCI flow specifications parameters: 1 Flags=0x00, 1 For       */
   /* incoming=0x01, 1 Guaranteed=0x02, 4 Token Rate=25000, 4 Token     */
   /* Bucket Size=333, 4 Peak Bandwidth=25000, 4 Access Latency=13000 . */
   /* 0x00, 0x01, 0x02, 25000, 333, 25000, 13000                        */
   Byte_t Flags = FLOW_SPEC_FLAGS;
   Byte_t Flow_Direction = FLOW_SPEC_FLOW_DIRECTION;
   Byte_t Service_Type = FLOW_SPEC_SERVICE_TYPE_GUARANTEED;

   /* The rest are the defaults for A2DP.                               */
   DWord_t Token_Rate = FLOW_SPEC_TOKEN_RATE;
   DWord_t Token_Bucket_Size = FLOW_SPEC_TOKEN_BUCKET_SIZE;
   DWord_t Peak_Bandwidth = FLOW_SPEC_PEAK_BANDWIDTH;
   DWord_t Access_Latency = FLOW_SPEC_ACCESS_LATENCY;
   Byte_t StatusResult;

   /* For saving the connection handle at the change to high priority   */
   /* When the priority changed back to normal from close stream there  */
   /* is no connection handle available in the function call.           */
   static Word_t saved_ConnHandle = 0;

   if(ConnHandle)
   {
      /* If we got connection handle, from the called function, save it!*/
      saved_ConnHandle = ConnHandle;
   }
   else
   {
      if((0 == ConnHandle) && (0 != saved_ConnHandle))
      {
         /* If we got connection handle, from last called function,     */
         /* retrieve it!                                                */
         ConnHandle = saved_ConnHandle;
      }
   }

   if(HIGH_AUDIO_CONNECTION_PRIORITY == high_normal)
   {
      Display(("\r\n Send HCI Flow Spec for guaranteed ACL connection\r\n"));

      if(ConnHandle)
      {
         /* For A2DP streaming - Set the ACL to guaranteed with 90% to  */
         /* the ACL over scans priority Send Flow spec command for      */
         /* setting the ACL connection to guaranteed                    */
         HCI_Flow_Specification(BluetoothStackID, ConnHandle, Flags, Flow_Direction, Service_Type, Token_Rate, Token_Bucket_Size, Peak_Bandwidth, Access_Latency, &StatusResult);
      }
      else
      {
         Display(("\r\n No connection handle for sending HCI_Flow_Specification for high priority \r\n"));
      }

      Display(("\r\n Send HCI VS DDIP for ACL priority over scans\r\n"));

      /* Send Vendor DDIP command to change the ACL priority over scans */
      /* to 90%.                                                        */
      VS_Send_DDIP(BluetoothStackID, BEST_EFFORT_HIGH_PRIORITY, GUARANTEED_HIGH_PRIORITY);
   }
   else
   {
      /* Change back the priority to Best effort=20% and Guaranteed =   */
      /* 70% change back the ACL to Best Effort.                        */
      Service_Type = FLOW_SPEC_SERVICE_TYPE_BESTEFFORT;

      if(ConnHandle)
      {
         Display(("\r\n Send HCI Flow Spec for best effort ACL connection\r\n"));

         HCI_Flow_Specification(BluetoothStackID, ConnHandle, Flags, Flow_Direction, Service_Type, Token_Rate, Token_Bucket_Size, Peak_Bandwidth, Access_Latency, &StatusResult);
      }
      else
      {
         Display(("\r\n No connection handle for sending HCI_Flow_Specification for normal priority \r\n"));
      }

      Display(("\r\n Send HCI VS DDIP for ACL priority over scans\r\n"));

      /* Change back the priority to Best effort=20% and Guaranteed =   */
      /* 70%.                                                           */
      VS_Send_DDIP(BluetoothStackID, BEST_EFFORT_NORMAL_PRIORITY, GUARANTEED_NORMAL_PRIORITY);
   }
}

   /* This function is responsible for reconfiguring an already-open    */
   /* A3DP stream with the stream parameters.  This function is called  */
   /* as a result of either a stream being opened, or AUD passing a     */
   /* stream format change event to us.                                 */
   /* * NOTE * Currently, the BCLK is hardcoded at 3.087MHz and the SBC */
   /*          parameters are hardcoded.  If these need to be dynamic, a*/
   /*          change to AUD will be necessary to expose the SBC        */
   /*          parameters.                                              */
static int ReconfigureA3DPStream(AUD_Stream_Format_t *Format)
{
   int AudioFormat;
   int SBCFormat;
   int ret_val;

   AudioFormat = 0;
   ret_val     = 0;

   Display(("Status: Enable the audio codec.\r\n"));

   HAL_EnableAudioCodec(BluetoothStackID, aucA3DPSink, Format->SampleFrequency, Format->NumberChannels);

   /* Determine the incoming SBC frequency.                             */
   switch(Format->SampleFrequency)
   {
      case 44100:
         AudioFormat |= (AVRP_AUDIO_FORMAT_SBC_SAMPLE_RATE_44K1 | AVRP_AUDIO_FORMAT_PCM_SAMPLE_RATE_44K1);
         break;
      case 48000:
         AudioFormat |= (AVRP_AUDIO_FORMAT_SBC_SAMPLE_RATE_48K | AVRP_AUDIO_FORMAT_PCM_SAMPLE_RATE_48K);
         break;
      default:
         ret_val = BTPS_ERROR_INVALID_PARAMETER;
         break;
   }

   /* Only proceed if the sample rate is valid.                         */
   if(!ret_val)
   {
      if(Format->NumberChannels > 1)
         AudioFormat |= AVRP_AUDIO_FORMAT_SBC_MODE_JOINT_STEREO;
      else
         AudioFormat |= AVRP_AUDIO_FORMAT_SBC_MODE_MONO;

      /* Use defaults for the SBC format.                               */
      SBCFormat = (AVRP_SBC_FORMAT_ALLOCATION_METHOD_LOUDNESS | AVRP_SBC_FORMAT_BLOCK_LENGTH_16);

      ret_val = VS_A3DP_Sink_Codec_Configuration(BluetoothStackID, AudioFormat, SBCFormat);
   }

   return(ret_val);
}

   /* This function handles the AUD/GAP/VS commands required to open and*/
   /* configure an A3DP sink endpoint.  It is called after AUD confirms */
   /* that a stream has been opened by the source.                      */
static int OpenA3DPStream(BD_ADDR_t BD_ADDR)
{
   int                        ret_val;
   Word_t                     ConnHandle;
   AUD_Stream_Channel_Info_t  StreamChannelInfo;
   AUD_Stream_Configuration_t StreamConfigInfo;

   /* Verify that not stream is currently opened.                       */
   if(!A3DPOpened)
   {
      /* A3DP requires the CID of the sink device (in our case, the     */
      /* local CID). It also requires the ACL connection handle, and    */
      /* some other parameters.                                         */
      ret_val = AUD_Query_Stream_Channel_Information(BluetoothStackID, BD_ADDR, astSNK, &StreamChannelInfo);
      if(ret_val == 0)
      {
         ret_val = GAP_Query_Connection_Handle(BluetoothStackID, BD_ADDR, &ConnHandle);
         if(ret_val == 0)
         {
            if((ConnHandle > 0) && (ConnHandle < 8))
            {
               ret_val = VS_A3DP_Sink_Open_Stream(BluetoothStackID, ConnHandle, StreamChannelInfo.LocalCID);

               Display(("Status: VS_A3DP_Sink_Open_Stream(): %d\r\n", ret_val));
            }
            else
               ret_val = FUNCTION_ERROR;

            if(ret_val == 0)
            {
               AUD_Query_Stream_Configuration(BluetoothStackID, BD_ADDR, astSNK, &StreamConfigInfo);

               /* Change the ACL connection priority to high, for better*/
               /* audio quality with Bluetooth scans                    */
               ChangeConnectionPriority(HIGH_AUDIO_CONNECTION_PRIORITY, ConnHandle);

               /* Configure the A3DP stream.                            */
               ret_val = ReconfigureA3DPStream(&(StreamConfigInfo.StreamFormat));

               /* Flag that an A3DP stream is now open.                 */
               A3DPOpened = TRUE;
            }
         }
      }
   }
   else
      ret_val = FUNCTION_ERROR;

   return(ret_val);
}

   /* This function handles closing an A3DP stream cleanly, i.e. making */
   /* sure that the stream is stopped, then closing it.                 */
static int CloseA3DPStream(void)
{
   int ret_val;

   if(A3DPOpened)
   {
      if(A3DPPlaying)
      {
         StopA3DPStream();
      }

      /* Change the ACL connection priority back to normal, for better  */
      /* Bluetooth scans.                                               */
      ChangeConnectionPriority(NORMAL_AUDIO_CONNECTION_PRIORITY, 0);

      ret_val = VS_A3DP_Sink_Close_Stream(BluetoothStackID);

      Display(("Status: VS_A3DP_Sink_Close_Stream(): %d\r\n", ret_val));

      /* Disable the audio codec.                                       */
      HAL_DisableAudioCodec();

      /* Set to closed even if an error occurred.                       */
      A3DPOpened  = FALSE;

      if(StreamState != ssPending)
         StreamState = ssStopped;
   }
   else
      ret_val = FUNCTION_ERROR;

   return(ret_val);
}

   /* The following function is responsible for issuing the             */
   /* vendor-specific command and updating the application's internal   */
   /* state when starting an A3DP stream.                               */
static int StartA3DPStream(BD_ADDR_t BD_ADDR)
{
   int ret_val;

   if(!COMPARE_BD_ADDR(BD_ADDR, A2DPRemoteBD_ADDR))
   {
      /* Instruct the remote side to stop playing. However, we must do  */
      /* this via an asynchronous command because there is a chance we  */
      /* will exceed our stack space.                                   */

      if(!COMPARE_NULL_BD_ADDR(A2DPRemoteBD_ADDR) && (A3DPPlaying))
         QueueRemoteControlCommand(A2DPRemoteBD_ADDR, rcPause);

      /* We need to switch over the configuration to use the new CID.   */
      A2DPRemoteBD_ADDR = BD_ADDR;

      if(A3DPPlaying)
         StopA3DPStream();

      CloseA3DPStream();

      OpenA3DPStream(BD_ADDR);

      /* Reset the A3DPPlaying variable to maintain a proper state.     */
      A3DPPlaying = FALSE;
   }

   if(!A3DPPlaying)
   {
      ret_val = VS_A3DP_Sink_Start_Stream(BluetoothStackID);

      Display(("Status: VS_A3DP_Sink_Start_Stream(): %d\r\n", ret_val));

      if(ret_val == 0)
      {
         A3DPPlaying = TRUE;

         if(StreamState != ssPending)
            StreamState = ssStarted;
      }
   }
   else
      ret_val = FUNCTION_ERROR;

   return(ret_val);
}

   /* The following function is responsible for issuing the             */
   /* vendor-specific command and updating the application's internal   */
   /* state when stopping an A3DP stream.                               */
static int StopA3DPStream(void)
{
   int ret_val;

   if(A3DPPlaying)
   {
      ret_val = VS_A3DP_Sink_Stop_Stream(BluetoothStackID);

      Display(("Status: VS_A3DP_Sink_Stop_Stream: %d\r\n", ret_val));

      /* Reset regardless of return value.                              */
      A3DPPlaying = FALSE;

      if(StreamState != ssPending)
         StreamState = ssStopped;
   }
   else
      ret_val = FUNCTION_ERROR;

   return(ret_val);
}

   /* The following function is responsible for displaying all available*/
   /* commands to the user.  It always returns zero.                    */
static int DisplayHelp(ParameterList_t *TempParam)
{
   Display(("\r\n"));
   Display(("******************************************************************\r\n"));
   Display(("* Command Options: Inquiry, DisplayInquiryList, Pair,            *\r\n"));
   Display(("*                  EndPairing, PINCodeResponse, PassKeyResponse, *\r\n"));
   Display(("*                  UserConfirmationResponse,                     *\r\n"));
   Display(("*                  SetDiscoverabilityMode, SetConnectabilityMode,*\r\n"));
   Display(("*                  SetPairabilityMode, SetBaudRate               *\r\n"));
   Display(("*                  ChangeSimplePairingParameters,                *\r\n"));
   Display(("*                  GetLocalAddress, GetLocalName, SetLocalName,  *\r\n"));
   Display(("*                  GetClassOfDevice, SetClassOfDevice,           *\r\n"));
   Display(("*                  GetRemoteName, OpenSink, CloseSink,           *\r\n"));
   Display(("*                  RemotePlay, RemotePause, RemoteNext,          *\r\n"));
   Display(("*                  RemotePrev, Help                              *\r\n"));
   Display(("******************************************************************\r\n"));
   Display(("\r\n"));
   return(0);
}

   /* The following function is for displaying The FW Version by reading*/
   /* The Local version information form the FW.                        */
static void DisplayFWVersion (void)
{
    FW_Version FW_Version_Details;

    /* This function retrieves the Local Version Information of the FW. */
    HCI_Read_Local_Version_Information(BluetoothStackID, &FW_Version_Details.StatusResult, &FW_Version_Details.HCI_VersionResult, &FW_Version_Details.HCI_RevisionResult, &FW_Version_Details.LMP_VersionResult, &FW_Version_Details.Manufacturer_NameResult, &FW_Version_Details.LMP_SubversionResult);
    if (!FW_Version_Details.StatusResult)
    {
        /* This function prints The project type from firmware, Bits    */
        /* 10 to 14 (5 bits) from LMP_SubversionResult parameter.       */
        Display(("Project Type  : %d \r\n", ((FW_Version_Details.LMP_SubversionResult >> 10)) & 0x1F));
        /* This function prints The version of the firmware. The first  */
        /* number is the Major version, Bits 7 to 9 and 15 (4 bits) from*/
        /* LMP_SubversionResult parameter, the second number is the     */
        /* Minor Version, Bits 0 to 6 (7 bits) from LMP_SubversionResult*/
        /* parameter.                                                   */
        Display(("FW Version    : %d.%d \r\n", ((FW_Version_Details.LMP_SubversionResult >> 7) & 0x07) + ((FW_Version_Details.LMP_SubversionResult >> 12) & 0x08), FW_Version_Details.LMP_SubversionResult & 0x7F));
    }
    else
    {
        /* There was an error with HCI_Read_Local_Version_Information.  */
        /* Function.                                                    */
        DisplayFunctionError("HCI_Read_Local_Version_Information", FW_Version_Details.StatusResult);
    }
}

   /* The following function is responsible for querying the memory     */
   /* heap usage. This function will return zero on successful          */
   /* execution and a negative value on errors.                         */
static int QueryMemory(ParameterList_t *TempParam)
{
   BTPS_MemoryStatistics_t MemoryStatistics;
   int ret_val;

   /* Get current memory buffer usage                                   */
   ret_val = BTPS_QueryMemoryUsage(&MemoryStatistics, TRUE);
   if(!ret_val)
   {
      Display(("\r\n"));
      Display(("Heap Size:                %5d bytes\r\n", MemoryStatistics.HeapSize));
      Display(("Current Memory Usage:\r\n"));
      Display(("   Used:                  %5d bytes\r\n", MemoryStatistics.CurrentHeapUsed));
      Display(("   Free:                  %5d bytes\r\n", MemoryStatistics.HeapSize - MemoryStatistics.CurrentHeapUsed));
      Display(("Maximum Memory Usage:\r\n"));
      Display(("   Used:                  %5d bytes\r\n", MemoryStatistics.MaximumHeapUsed));
      Display(("   Free:                  %5d bytes\r\n", MemoryStatistics.HeapSize - MemoryStatistics.MaximumHeapUsed));
      Display(("Fragmentation:\r\n"));
      Display(("   Largest Free Fragment: %5d bytes\r\n", MemoryStatistics.LargestFreeFragment));
      Display(("   Free Fragment Count:   %5d\r\n",       MemoryStatistics.FreeFragmentCount));
   }
   else
   {
      Display(("Failed to get memory usage\r\n"));
   }

   return(ret_val);
}

   /* The following function is used to send the specified remote       */
   /* control to the currently connected remote control device.         */
static int SendRemoteControlCommand(RemoteControlCommand_t Command)
{
   int                               ret_val;
   Boolean_t                         ProcessCommand;
   AUD_Remote_Control_Command_Data_t RemoteCommand;

   /* Ensure that we have an open stream first.                         */
   if(A3DPOpened)
   {
      /* Initialize local variables.                                    */
      ret_val        = 0;
      ProcessCommand = TRUE;

      /* Initialize common members of the pass through command.         */
      /* * NOTE * State flag of FALSE specifies button down.            */
      RemoteCommand.MessageType                                            = amtPassThrough;
      RemoteCommand.MessageData.PassThroughCommandData.CommandType         = AVRCP_CTYPE_CONTROL;
      RemoteCommand.MessageData.PassThroughCommandData.SubunitType         = AVRCP_SUBUNIT_TYPE_PANEL;
      RemoteCommand.MessageData.PassThroughCommandData.SubunitID           = AVRCP_SUBUNIT_ID_INSTANCE_0;
      RemoteCommand.MessageData.PassThroughCommandData.OperationID         = (Byte_t)0;
      RemoteCommand.MessageData.PassThroughCommandData.StateFlag           = (Boolean_t)FALSE;
      RemoteCommand.MessageData.PassThroughCommandData.OperationDataLength = 0;
      RemoteCommand.MessageData.PassThroughCommandData.OperationData       = NULL;

      /* Determine the command and go ahead and build it.               */
      switch(Command)
      {
         case rcPlayPause:
            /* Verify that we do not have another play/pause pending.   */
            if(StreamState != ssPending)
            {
               if(StreamState == ssStarted)
                  RemoteCommand.MessageData.PassThroughCommandData.OperationID = (Byte_t)AVRCP_PASS_THROUGH_ID_PAUSE;
               else
                  RemoteCommand.MessageData.PassThroughCommandData.OperationID = (Byte_t)AVRCP_PASS_THROUGH_ID_PLAY;

               StreamState = ssPending;
            }
            else
            {
               /* Command already pending so don't process this request.*/
               ProcessCommand = FALSE;
            }
            break;
         case rcPlay:
            RemoteCommand.MessageData.PassThroughCommandData.OperationID = (Byte_t)AVRCP_PASS_THROUGH_ID_PLAY;
            break;
         case rcPause:
            RemoteCommand.MessageData.PassThroughCommandData.OperationID = (Byte_t)AVRCP_PASS_THROUGH_ID_PAUSE;
            break;
         case rcNext:
            RemoteCommand.MessageData.PassThroughCommandData.OperationID = (Byte_t)AVRCP_PASS_THROUGH_ID_FORWARD;
            break;
         case rcBack:
            RemoteCommand.MessageData.PassThroughCommandData.OperationID = (Byte_t)AVRCP_PASS_THROUGH_ID_BACKWARD;
            break;
         case rcVolumeUp:
            RemoteCommand.MessageData.PassThroughCommandData.OperationID = (Byte_t)AVRCP_PASS_THROUGH_ID_VOLUME_UP;
            break;
         case rcVolumeDown:
            RemoteCommand.MessageData.PassThroughCommandData.OperationID = (Byte_t)AVRCP_PASS_THROUGH_ID_VOLUME_DOWN;
            break;
         default:
            /* Unsupported - flag error.                                */
            ret_val = INVALID_PARAMETERS_ERROR;
            break;
      }

      /* Continue only if we have a command to process.                 */
      if(ProcessCommand)
      {
         /* Proceed only if the AVRCP command is supported.             */
         if(!ret_val)
            ret_val = AUD_Send_Remote_Control_Command(BluetoothStackID, A2DPRemoteBD_ADDR, &RemoteCommand, 0);

         if(ret_val > 0)
         {
            /* Now we must send the button-up command to finish up.     */
            RemoteCommand.MessageData.PassThroughCommandData.StateFlag = TRUE;
            ret_val = AUD_Send_Remote_Control_Command(BluetoothStackID, A2DPRemoteBD_ADDR, &RemoteCommand, 1000);

            if(ret_val > 0)
            {
               Display(("AVRCP command sent successfully.\r\n"));

               ret_val = 0;
            }
            else
            {
               DisplayFunctionError("AUD_Send_Remote_Control_Command()", ret_val);

               ret_val = FUNCTION_ERROR;
            }
         }
         else
         {
            DisplayFunctionError("AUD_Send_Remote_Control_Command()", ret_val);

            ret_val = FUNCTION_ERROR;
         }
      }
   }
   else
      ret_val = FUNCTION_ERROR;

   return(ret_val);
}

   /* The following function is an asynchronous callback to handle      */
   /* calling into the SendRemoteControlCommand function, in cases where*/
   /* we are too deep into the call stack to call it directly.          */
static void RemoteControlCommandAsync(unsigned int BluetoothStackID, unsigned long CallbackParameter)
{
   BD_ADDR_t PrevBD_ADDR = A2DPRemoteBD_ADDR;

   /* We need to cache the current A2DPRemoteBD_ADDR since it has been  */
   /* overwritten with the new source by now. Temporarily assign it to  */
   /* the old source and restore it when the command has been sent.     */

   A2DPRemoteBD_ADDR = QueuedCommand.BD_ADDR;
   SendRemoteControlCommand(QueuedCommand.Command);
   A2DPRemoteBD_ADDR = PrevBD_ADDR;
}

   /* The following function simply schedules an asynchronous callback  */
   /* to execute a remote control command, to reduce stack usage.       */
static int QueueRemoteControlCommand(BD_ADDR_t BD_ADDR, RemoteControlCommand_t Command)
{
   QueuedCommand.BD_ADDR = BD_ADDR;
   QueuedCommand.Command = Command;

   return((BSC_ScheduleAsynchronousCallback(BluetoothStackID, RemoteControlCommandAsync, 0) > 0) ? 0 : FUNCTION_ERROR);
}

   /* The following function is responsible for performing a General    */
   /* Inquiry for discovering Bluetooth Devices.  This function requires*/
   /* that a valid Bluetooth Stack ID exists before running.  This      */
   /* function returns zero is successful or a negative value if there  */
   /* was an error.                                                     */
static int Inquiry(ParameterList_t *TempParam)
{
   int Result;
   int ret_val = 0;

   /* First, check that valid Bluetooth Stack ID exists.                */
   if(BluetoothStackID)
   {
      /* Use the GAP_Perform_Inquiry() function to perform an Inquiry.  */
      /* The Inquiry will last 10 seconds or until 1 Bluetooth Device is*/
      /* found.  When the Inquiry Results become available the          */
      /* GAP_Event_Callback is called.                                  */
      Result = GAP_Perform_Inquiry(BluetoothStackID, itGeneralInquiry, 0, 0, 10, MAX_INQUIRY_RESULTS, GAP_Event_Callback, (unsigned long)NULL);

      /* Next, check to see if the GAP_Perform_Inquiry() function was   */
      /* successful.                                                    */
      if(!Result)
      {
         /* The Inquiry appears to have been sent successfully.         */
         /* Processing of the results returned from this command occurs */
         /* within the GAP_Event_Callback() function.                   */

         /* Flag that we have found NO Bluetooth Devices.               */
         NumberofValidResponses = 0;

         ret_val                = 0;
      }
      else
      {
         /* A error occurred while performing the Inquiry.              */
         DisplayFunctionError("Inquiry", Result);

         ret_val = Result;
      }
   }
   else
   {
      /* No valid Bluetooth Stack ID exists.                            */
      ret_val = INVALID_STACK_ID_ERROR;
   }

   return(ret_val);
}

   /* The following function is a utility function that exists to       */
   /* display the current Inquiry List (with Indexes). This is useful in*/
   /* case the user has forgotten what Inquiry Index a particular       */
   /* Bluetooth Device was located in.  This function returns zero on   */
   /* successful execution and a negative value on all errors.          */
static int DisplayInquiryList(ParameterList_t *TempParam)
{
   int          ret_val = 0;
   unsigned int Index;

   /* First, check that valid Bluetooth Stack ID exists.                */
   if(BluetoothStackID)
   {
      /* Simply display all of the items in the Inquiry List.           */
      Display(("Inquiry List: %d Devices%s\r\n\r\n", NumberofValidResponses, NumberofValidResponses?":":"."));

      for(Index=0;Index<NumberofValidResponses;Index++)
      {
         BD_ADDRToStr(InquiryResultList[Index], Function_BoardStr);

         Display((" Inquiry Result: %d, %s.\r\n", (Index+1), Function_BoardStr));
      }

      if(NumberofValidResponses)
         Display(("\r\n"));

      /* All finished, flag success to the caller.                      */
      ret_val = 0;
   }
   else
   {
      /* No valid Bluetooth Stack ID exists.                            */
      ret_val = INVALID_STACK_ID_ERROR;
   }

   return(ret_val);
}

   /* The following function is responsible for changing the Secure     */
   /* Simple Pairing Parameters that are exchanged during the Pairing   */
   /* procedure when Secure Simple Pairing (Security Level 4) is used.  */
   /* This function returns zero on successful execution and a negative */
   /* value on all errors.                                              */
static int ChangeSimplePairingParameters(ParameterList_t *TempParam)
{
   int ret_val = 0;

   /* First, check that valid Bluetooth Stack ID exists.                */
   if(BluetoothStackID)
   {
      /* Make sure that all of the parameters required for this function*/
      /* appear to be at least semi-valid.                              */
      if((TempParam) && (TempParam->NumberofParameters >= 2) && (TempParam->Params[0].intParam >= 0) && (TempParam->Params[0].intParam <= 3))
      {
         /* Parameters appear to be valid, map the specified parameters */
         /* into the API specific parameters.                           */
         if(TempParam->Params[0].intParam == 0)
            IOCapability = icDisplayOnly;
         else
         {
            if(TempParam->Params[0].intParam == 1)
               IOCapability = icDisplayYesNo;
            else
            {
               if(TempParam->Params[0].intParam == 2)
                  IOCapability = icKeyboardOnly;
               else
                  IOCapability = icNoInputNoOutput;
            }
         }

         /* Finally map the Man in the Middle (MITM) Protection value.  */
         MITMProtection = (Boolean_t)(TempParam->Params[1].intParam?TRUE:FALSE);

         /* Inform the user of the New I/O Capabilities.                */
         DisplayIOCapabilities();

         /* Flag success to the caller.                                 */
         ret_val = 0;
      }
      else
      {
         /* One or more of the necessary parameters is/are invalid.     */
         DisplayUsage("ChangeSimplePairingParameters [I/O Capability (0 = Display Only, 1 = Display Yes/No, 2 = Keyboard Only, 3 = No Input/Output)] [MITM Requirement (0 = No, 1 = Yes)]");

         ret_val = INVALID_PARAMETERS_ERROR;
      }
   }
   else
   {
      /* No valid Bluetooth Stack ID exists.                            */
      ret_val = INVALID_STACK_ID_ERROR;
   }

   return(ret_val);
}

   /* The following function is responsible for ending a previously     */
   /* initiated bonding session with a remote device.  This function    */
   /* returns zero on successful execution and a negative value on all  */
   /* errors.                                                           */
static int EndPairing(ParameterList_t *TempParam)
{
   int       Result;
   int       ret_val;

   /* First, check that valid Bluetooth Stack ID exists.                */
   if(BluetoothStackID)
   {
      /* Make sure that all of the parameters required for this function*/
      /* appear to be at least semi-valid.                              */
      if((TempParam) && (TempParam->NumberofParameters > 0) && (TempParam->Params[0].intParam) && (NumberofValidResponses) && (TempParam->Params[0].intParam <= NumberofValidResponses) && (!COMPARE_NULL_BD_ADDR(InquiryResultList[(TempParam->Params[0].intParam - 1)])))
      {
         /* Attempt to submit the command.                              */
         Result = GAP_End_Bonding(BluetoothStackID, InquiryResultList[(TempParam->Params[0].intParam - 1)]);

         /* Check the return value of the submitted command for success.*/
         if(!Result)
         {
            /* Display a message indicating that the End bonding was    */
            /* successfully submitted.                                  */
            DisplayFunctionSuccess("GAP_End_Bonding");

            /* Flag success to the caller.                              */
            ret_val = 0;

            /* Flag that there is no longer a current Authentication    */
            /* procedure in progress.                                   */
            ASSIGN_BD_ADDR(CurrentRemoteBD_ADDR, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
         }
         else
         {
            /* Display a message indicating that an error occurred while*/
            /* ending bonding.                                          */
            DisplayFunctionError("GAP_End_Bonding", Result);

            ret_val = FUNCTION_ERROR;
         }
      }
      else
      {
         /* One or more of the necessary parameters is/are invalid.     */
         DisplayUsage("EndPairing [Inquiry Index]");

         ret_val = INVALID_PARAMETERS_ERROR;
      }
   }
   else
   {
      /* No valid Bluetooth Stack ID exists.                            */
      ret_val = INVALID_STACK_ID_ERROR;
   }

   return(ret_val);
}

   /* The following function is responsible for setting the name of the */
   /* local Bluetooth Device to a specified name.  This function returns*/
   /* zero on successful execution and a negative value on all errors.  */
static int SetLocalName(ParameterList_t *TempParam)
{
   int Result;
   int ret_val = 0;

   /* First, check that valid Bluetooth Stack ID exists.                */
   if(BluetoothStackID)
   {
      /* Make sure that all of the parameters required for this function*/
      /* appear to be at least semi-valid.                              */
      if((TempParam) && (TempParam->NumberofParameters > 0) && (TempParam->Params[0].strParam))
      {
         /* Attempt to submit the command.                              */
         Result = GAP_Set_Local_Device_Name(BluetoothStackID, TempParam->Params[0].strParam);

         /* Check the return value of the submitted command for success.*/
         if(!Result)
         {
            /* Display a message indicating that the Device Name was    */
            /* successfully submitted.                                  */
            Display(("Local Device Name: %s.\r\n", TempParam->Params[0].strParam));

            /* Flag success to the caller.                              */
            ret_val = 0;
         }
         else
         {
            /* Display a message indicating that an error occurred while*/
            /* attempting to set the local Device Name.                 */
            DisplayFunctionError("GAP_Set_Local_Device_Name", Result);

            ret_val = FUNCTION_ERROR;
         }
      }
      else
      {
         /* One or more of the necessary parameters is/are invalid.     */
         DisplayUsage("SetLocalName [Local Name]");

         ret_val = INVALID_PARAMETERS_ERROR;
      }
   }
   else
   {
      /* No valid Bluetooth Stack ID exists.                            */
      ret_val = INVALID_STACK_ID_ERROR;
   }

   return(ret_val);
}

   /* The following function is responsible for querying the Bluetooth  */
   /* Device Name of the local Bluetooth Device.  This function returns */
   /* zero on successful execution and a negative value on all errors.  */
static int GetLocalName(ParameterList_t *TempParam)
{
   int  Result;
   int  ret_val;
   char *LocalName;

   /* First, check that valid Bluetooth Stack ID exists.                */
   if(BluetoothStackID)
   {
      /* Allocate a Buffer to hold the Local Name.                      */
      if((LocalName = BTPS_AllocateMemory(257)) != NULL)
      {
         /* Attempt to submit the command.                              */
         Result = GAP_Query_Local_Device_Name(BluetoothStackID, 257, (char *)LocalName);

         /* Check the return value of the submitted command for success.*/
         if(!Result)
         {
            Display(("Name of Local Device is: %s.\r\n", LocalName));

            /* Flag success to the caller.                              */
            ret_val = 0;
         }
         else
         {
            /* Display a message indicating that an error occurred while*/
            /* attempting to query the Local Device Name.               */
            Display(("GAP_Query_Local_Device_Name() Failure: %d.\r\n", Result));

            ret_val = FUNCTION_ERROR;
         }

         BTPS_FreeMemory(LocalName);
      }
      else
      {
         Display(("Failed to allocate buffer to hold Local Name.\r\n"));

         ret_val = FUNCTION_ERROR;
      }
   }
   else
   {
      /* No valid Bluetooth Stack ID exists.                            */
      ret_val = INVALID_STACK_ID_ERROR;
   }

   return(ret_val);
}

   /* The following function is responsible for querying the Bluetooth  */
   /* Device Name of the specified remote Bluetooth Device.  This       */
   /* function returns zero on successful execution and a negative value*/
   /* on all errors.                                                    */
static int GetRemoteName(ParameterList_t *TempParam)
{
   int       Result;
   int       ret_val;

   /* First, check that valid Bluetooth Stack ID exists.                */
   if(BluetoothStackID)
   {
      /* Make sure that all of the parameters required for this function*/
      /* appear to be at least semi-valid.                              */
      if((TempParam) && (TempParam->NumberofParameters > 0) && (TempParam->Params[0].intParam) && (NumberofValidResponses) && (TempParam->Params[0].intParam <= NumberofValidResponses) && (!COMPARE_NULL_BD_ADDR(InquiryResultList[(TempParam->Params[0].intParam - 1)])))
      {
         /* Attempt to submit the command.                              */
         Result = GAP_Query_Remote_Device_Name(BluetoothStackID, InquiryResultList[(TempParam->Params[0].intParam - 1)], GAP_Event_Callback, (unsigned long)0);

         /* Check the return value of the submitted command for success.*/
         if(!Result)
         {
            /* Display a message indicating that Remote Name request was*/
            /* initiated successfully.                                  */
            DisplayFunctionSuccess("GAP_Query_Remote_Device_Name");

            /* Flag success to the caller.                              */
            ret_val = 0;
         }
         else
         {
            /* Display a message indicating that an error occurred while*/
            /* initiating the Remote Name request.                      */
            DisplayFunctionError("GAP_Query_Remote_Device_Name", Result);

            ret_val = FUNCTION_ERROR;
         }
      }
      else
      {
         /* One or more of the necessary parameters is/are invalid.     */
         DisplayUsage("GetRemoteName [Inquiry Index]");

         ret_val = INVALID_PARAMETERS_ERROR;
      }
   }
   else
   {
      /* No valid Bluetooth Stack ID exists.                            */
      ret_val = INVALID_STACK_ID_ERROR;
   }

   return(ret_val);
}

   /* The following function is responsible for setting the Class of    */
   /* Device of the local Bluetooth Device to a Class Of Device value.  */
   /* This function returns zero on successful execution and a negative */
   /* value on all errors.                                              */
static int SetClassOfDevice(ParameterList_t *TempParam)
{
   int               Result;
   int               ret_val;
   Class_of_Device_t Class_of_Device;

   /* First, check that valid Bluetooth Stack ID exists.                */
   if(BluetoothStackID)
   {
      /* Make sure that all of the parameters required for this function*/
      /* appear to be at least semi-valid.                              */
      if((TempParam) && (TempParam->NumberofParameters > 0))
      {
         /* Attempt to submit the command.                              */
         ASSIGN_CLASS_OF_DEVICE(Class_of_Device, (Byte_t)((TempParam->Params[0].intParam) & 0xFF), (Byte_t)(((TempParam->Params[0].intParam) >> 8) & 0xFF), (Byte_t)(((TempParam->Params[0].intParam) >> 16) & 0xFF));

         Result = GAP_Set_Class_Of_Device(BluetoothStackID, Class_of_Device);

         /* Check the return value of the submitted command for success.*/
         if(!Result)
         {
            /* Display a message indicating that the Class of Device was*/
            /* successfully submitted.                                  */
            DisplayClassOfDevice(Class_of_Device);

            /* Flag success to the caller.                              */
            ret_val = 0;
         }
         else
         {
            /* Display a message indicating that an error occurred while*/
            /* attempting to set the local Class of Device.             */
            DisplayFunctionError("GAP_Set_Class_Of_Device", Result);

            ret_val = FUNCTION_ERROR;
         }
      }
      else
      {
         /* One or more of the necessary parameters is/are invalid.     */
         DisplayUsage("SetClassOfDevice [Class of Device]");

         ret_val = INVALID_PARAMETERS_ERROR;
      }
   }
   else
   {
      /* No valid Bluetooth Stack ID exists.                            */
      ret_val = INVALID_STACK_ID_ERROR;
   }

   return(ret_val);
}

   /* The following function is responsible for querying the Bluetooth  */
   /* Class of Device of the local Bluetooth Device.  This function     */
   /* returns zero on successful execution and a negative value on all  */
   /* errors.                                                           */
static int GetClassOfDevice(ParameterList_t *TempParam)
{
   int               Result;
   int               ret_val;
   Class_of_Device_t Class_of_Device;

   /* First, check that valid Bluetooth Stack ID exists.                */
   if(BluetoothStackID)
   {
      /* Attempt to submit the command.                                 */
      Result = GAP_Query_Class_Of_Device(BluetoothStackID, &Class_of_Device);

      /* Check the return value of the submitted command for success.   */
      if(!Result)
      {
         Display(("Local Class of Device is: 0x%02X%02X%02X.\r\n", Class_of_Device.Class_of_Device0, Class_of_Device.Class_of_Device1, Class_of_Device.Class_of_Device2));

         /* Flag success to the caller.                                 */
         ret_val = 0;
      }
      else
      {
         /* Display a message indicating that an error occurred while   */
         /* attempting to query the Local Class of Device.              */
         Display(("GAP_Query_Class_Of_Device() Failure: %d.\r\n", Result));

         ret_val = FUNCTION_ERROR;
      }
   }
   else
   {
      /* No valid Bluetooth Stack ID exists.                            */
      ret_val = INVALID_STACK_ID_ERROR;
   }

   return(ret_val);
}

   /* The following function is responsible for initiating bonding with */
   /* a remote device.  This function returns zero on successful        */
   /* execution and a negative value on all errors.                     */
static int Pair(ParameterList_t *TempParam)
{
   int                Result;
   int                ret_val;
   GAP_Bonding_Type_t BondingType;

   /* First, check that valid Bluetooth Stack ID exists.                */
   if(BluetoothStackID)
   {
      /* There are currently no active connections, make sure that all  */
      /* of the parameters required for this function appear to be at   */
      /* least semi-valid.                                              */
      if((TempParam) && (TempParam->NumberofParameters > 0) && (TempParam->Params[0].intParam) && (NumberofValidResponses) && (TempParam->Params[0].intParam <= NumberofValidResponses) && (!COMPARE_NULL_BD_ADDR(InquiryResultList[(TempParam->Params[0].intParam - 1)])))
      {
         /* Check to see if General Bonding was specified.              */
         if(TempParam->NumberofParameters > 1)
            BondingType = TempParam->Params[1].intParam?btGeneral:btDedicated;
         else
            BondingType = btDedicated;

         /* Before we submit the command to the stack, we need to make  */
         /* sure that we clear out any Link Key we have stored for the  */
         /* specified device.                                           */
         DeleteLinkKey(InquiryResultList[(TempParam->Params[0].intParam - 1)]);

         /* Attempt to submit the command.                              */
         Result = GAP_Initiate_Bonding(BluetoothStackID, InquiryResultList[(TempParam->Params[0].intParam - 1)], BondingType, GAP_Event_Callback, (unsigned long)0);

         /* Check the return value of the submitted command for success.*/
         if(!Result)
         {
            /* Display a message indicating that Bonding was initiated  */
            /* successfully.                                            */
            Display(("GAP_Initiate_Bonding(%s): Success.\r\n", (BondingType == btDedicated)?"Dedicated":"General"));

            /* Flag success to the caller.                              */
            ret_val = 0;
         }
         else
         {
            /* Display a message indicating that an error occurred while*/
            /* initiating bonding.                                      */
            DisplayFunctionError("GAP_Initiate_Bonding", Result);

            ret_val = FUNCTION_ERROR;
         }
      }
      else
      {
         /* One or more of the necessary parameters is/are invalid.     */
         DisplayUsage("Pair [Inquiry Index] [0 = Dedicated, 1 = General (optional)]");

         ret_val = INVALID_PARAMETERS_ERROR;
      }
   }
   else
   {
      /* No valid Bluetooth Stack ID exists.                            */
      ret_val = INVALID_STACK_ID_ERROR;
   }

   return(ret_val);
}

   /* The following function is responsible for issuing a GAP           */
   /* Authentication Response with a PIN Code value specified via the   */
   /* input parameter.  This function returns zero on successful        */
   /* execution and a negative value on all errors.                     */
static int PINCodeResponse(ParameterList_t *TempParam)
{
   int                              Result;
   int                              ret_val;
   PIN_Code_t                       PINCode;
   GAP_Authentication_Information_t GAP_Authentication_Information;

   /* First, check that valid Bluetooth Stack ID exists.                */
   if(BluetoothStackID)
   {
      /* First, check to see if there is an on-going Pairing operation  */
      /* active.                                                        */
      if(!COMPARE_NULL_BD_ADDR(CurrentRemoteBD_ADDR))
      {
         /* Make sure that all of the parameters required for this      */
         /* function appear to be at least semi-valid.                  */
         if((TempParam) && (TempParam->NumberofParameters > 0) && (TempParam->Params[0].strParam) && (BTPS_StringLength(TempParam->Params[0].strParam) > 0) && (BTPS_StringLength(TempParam->Params[0].strParam) <= sizeof(PIN_Code_t)))
         {
            /* Parameters appear to be valid, go ahead and convert the  */
            /* input parameter into a PIN Code.                         */

            /* Initialize the PIN code.                                 */
            ASSIGN_PIN_CODE(PINCode, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

            BTPS_MemCopy(&PINCode, TempParam->Params[0].strParam, BTPS_StringLength(TempParam->Params[0].strParam));

            /* Populate the response structure.                         */
            GAP_Authentication_Information.GAP_Authentication_Type      = atPINCode;
            GAP_Authentication_Information.Authentication_Data_Length   = (Byte_t)(BTPS_StringLength(TempParam->Params[0].strParam));
            GAP_Authentication_Information.Authentication_Data.PIN_Code = PINCode;

            /* Submit the Authentication Response.                      */
            Result = GAP_Authentication_Response(BluetoothStackID, CurrentRemoteBD_ADDR, &GAP_Authentication_Information);

            /* Check the return value for the submitted command for     */
            /* success.                                                 */
            if(!Result)
            {
               /* Operation was successful, inform the user.            */
               Display(("GAP_Authentication_Response(), Pin Code Response Success.\r\n"));

               /* Flag success to the caller.                           */
               ret_val = 0;
            }
            else
            {
               /* Inform the user that the Authentication Response was  */
               /* not successful.                                       */
               Display(("GAP_Authentication_Response() Failure: %d.\r\n", Result));

               ret_val = FUNCTION_ERROR;
            }

            /* Flag that there is no longer a current Authentication    */
            /* procedure in progress.                                   */
            ASSIGN_BD_ADDR(CurrentRemoteBD_ADDR, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
         }
         else
         {
            /* One or more of the necessary parameters is/are invalid.  */
            DisplayUsage("PINCodeResponse [PIN Code]");

            ret_val = INVALID_PARAMETERS_ERROR;
         }
      }
      else
      {
         /* There is not currently an on-going authentication operation,*/
         /* inform the user of this error condition.                    */
         Display(("PIN Code Authentication Response: Authentication not in progress.\r\n"));

         ret_val = FUNCTION_ERROR;
      }
   }
   else
   {
      /* No valid Bluetooth Stack ID exists.                            */
      ret_val = INVALID_STACK_ID_ERROR;
   }

   return(ret_val);
}

   /* The following function is responsible for issuing a GAP           */
   /* Authentication Response with a Pass Key value specified via the   */
   /* input parameter.  This function returns zero on successful        */
   /* execution and a negative value on all errors.                     */
static int PassKeyResponse(ParameterList_t *TempParam)
{
   int                              Result;
   int                              ret_val;
   GAP_Authentication_Information_t GAP_Authentication_Information;

   /* First, check that valid Bluetooth Stack ID exists.                */
   if(BluetoothStackID)
   {
      /* First, check to see if there is an on-going Pairing operation  */
      /* active.                                                        */
      if(!COMPARE_NULL_BD_ADDR(CurrentRemoteBD_ADDR))
      {
         /* Make sure that all of the parameters required for this      */
         /* function appear to be at least semi-valid.                  */
         if((TempParam) && (TempParam->NumberofParameters > 0) && (BTPS_StringLength(TempParam->Params[0].strParam) <= GAP_PASSKEY_MAXIMUM_NUMBER_OF_DIGITS))
         {
            /* Parameters appear to be valid, go ahead and populate the */
            /* response structure.                                      */
            GAP_Authentication_Information.GAP_Authentication_Type     = atPassKey;
            GAP_Authentication_Information.Authentication_Data_Length  = (Byte_t)(sizeof(DWord_t));
            GAP_Authentication_Information.Authentication_Data.Passkey = (DWord_t)(TempParam->Params[0].intParam);

            /* Submit the Authentication Response.                      */
            Result = GAP_Authentication_Response(BluetoothStackID, CurrentRemoteBD_ADDR, &GAP_Authentication_Information);

            /* Check the return value for the submitted command for     */
            /* success.                                                 */
            if(!Result)
            {
               /* Operation was successful, inform the user.            */
               DisplayFunctionSuccess("Passkey Response Success");

               /* Flag success to the caller.                           */
               ret_val = 0;
            }
            else
            {
               /* Inform the user that the Authentication Response was  */
               /* not successful.                                       */
               DisplayFunctionError("GAP_Authentication_Response", Result);

               ret_val = FUNCTION_ERROR;
            }

            /* Flag that there is no longer a current Authentication    */
            /* procedure in progress.                                   */
            ASSIGN_BD_ADDR(CurrentRemoteBD_ADDR, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
         }
         else
         {
            /* One or more of the necessary parameters is/are invalid.  */
            Display(("PassKeyResponse[Numeric Passkey(0 - 999999)].\r\n"));

            ret_val = INVALID_PARAMETERS_ERROR;
         }
      }
      else
      {
         /* There is not currently an on-going authentication operation,*/
         /* inform the user of this error condition.                    */
         Display(("Pass Key Authentication Response: Authentication not in progress.\r\n"));

         ret_val = FUNCTION_ERROR;
      }
   }
   else
   {
      /* No valid Bluetooth Stack ID exists.                            */
      ret_val = INVALID_STACK_ID_ERROR;
   }

   return(ret_val);
}

   /* The following function is responsible for issuing a GAP           */
   /* Authentication Response with a User Confirmation value specified  */
   /* via the input parameter.  This function returns zero on successful*/
   /* execution and a negative value on all errors.                     */
static int UserConfirmationResponse(ParameterList_t *TempParam)
{
   int                              Result;
   int                              ret_val;
   GAP_Authentication_Information_t GAP_Authentication_Information;

   /* First, check that valid Bluetooth Stack ID exists.                */
   if(BluetoothStackID)
   {
      /* First, check to see if there is an on-going Pairing operation  */
      /* active.                                                        */
      if(!COMPARE_NULL_BD_ADDR(CurrentRemoteBD_ADDR))
      {
         /* Make sure that all of the parameters required for this      */
         /* function appear to be at least semi-valid.                  */
         if((TempParam) && (TempParam->NumberofParameters > 0))
         {
            /* Parameters appear to be valid, go ahead and populate the */
            /* response structure.                                      */
            GAP_Authentication_Information.GAP_Authentication_Type          = atUserConfirmation;
            GAP_Authentication_Information.Authentication_Data_Length       = (Byte_t)(sizeof(Byte_t));
            GAP_Authentication_Information.Authentication_Data.Confirmation = (Boolean_t)(TempParam->Params[0].intParam?TRUE:FALSE);

            /* Submit the Authentication Response.                      */
            Result = GAP_Authentication_Response(BluetoothStackID, CurrentRemoteBD_ADDR, &GAP_Authentication_Information);

            /* Check the return value for the submitted command for     */
            /* success.                                                 */
            if(!Result)
            {
               /* Operation was successful, inform the user.            */
               DisplayFunctionSuccess("User Confirmation Response");

               /* Flag success to the caller.                           */
               ret_val = 0;
            }
            else
            {
               /* Inform the user that the Authentication Response was  */
               /* not successful.                                       */
               DisplayFunctionError("GAP_Authentication_Response", Result);

               ret_val = FUNCTION_ERROR;
            }

            /* Flag that there is no longer a current Authentication    */
            /* procedure in progress.                                   */
            ASSIGN_BD_ADDR(CurrentRemoteBD_ADDR, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
         }
         else
         {
            /* One or more of the necessary parameters is/are invalid.  */
            DisplayUsage("UserConfirmationResponse [Confirmation(0 = No, 1 = Yes)]");
            ret_val = INVALID_PARAMETERS_ERROR;
         }
      }
      else
      {
         /* There is not currently an on-going authentication operation,*/
         /* inform the user of this error condition.                    */
         Display(("User Confirmation Authentication Response: Authentication is not currently in progress.\r\n"));

         ret_val = FUNCTION_ERROR;
      }
   }
   else
   {
      /* No valid Bluetooth Stack ID exists.                            */
      ret_val = INVALID_STACK_ID_ERROR;
   }

   return(ret_val);
}

   /* The following function is responsible for initializing AUD if     */
   /* necessary, and initializing the A3DP subsystem.                   */
static int OpenA3DPEndpoint(ParameterList_t *TempParam)
{
   int           ret_val;
   unsigned long ActiveFeatures;

   /* Verify that not stream is currently opened.                       */
   if(!A3DPOpened)
   {
      /* Make sure that AUD is not already configured.                  */
      if(!AUDInitialized)
      {
         /* Verify that the Sink Role has been initialized successfully.*/
         if(((ret_val = BSC_QueryActiveFeatures(BluetoothStackID, &ActiveFeatures)) == 0) && (ActiveFeatures & BSC_FEATURE_A3DP_SINK))
         {
            /* Attempt to initialize the sink.                          */
            if((ret_val = Initialize_Sink()) == 0)
            {
               Display(("A3DP endpoint opened successfully.\r\n"));
            }
            else
            {
               Display(("Initialize AUD Sink: %d\r\n", ret_val));

               ret_val = FUNCTION_ERROR;
            }
         }
         else
         {
            Display(("Required A3DP Sink role not enabled.\r\n"));

            ret_val = FUNCTION_ERROR;
         }
      }
      else
      {
         Display(("Endpoint is already opened.\r\n"));

         ret_val = FUNCTION_ERROR;
      }
   }
   else
   {
      Display(("Endpoint is already opened.\r\n"));
      ret_val = FUNCTION_ERROR;
   }

   return(ret_val);
}

   /* The following function is responsible for cleaning up AUD and the */
   /* A3DP stream, if the stream is opened and/or playing.              */
static int CloseA3DPEndpoint(ParameterList_t *TempParam)
{
   int ret_val;

   /* Verify that AUD was initialized successfully.                     */
   if(AUDInitialized)
   {
      /* If a stream is open, and possibly playing, then handle this    */
      /* case.                                                          */
      if(A3DPOpened)
      {
         if(A3DPPlaying)
            StopA3DPStream();

         CloseA3DPStream();
      }

      AUD_Un_Initialize(BluetoothStackID);
      AUDInitialized = FALSE;
      ret_val        = 0;

      Display(("A3DP endpoint closed successfully.\r\n"));
   }
   else
   {
      Display(("A3DP endpoint is not opened yet.\r\n"));

      ret_val = FUNCTION_ERROR;
   }

   return(ret_val);
}

   /* The following function is responsible for setting the             */
   /* Discoverability Mode of the local device.  This function returns  */
   /* zero on successful execution and a negative value on all errors.  */
static int SetDiscoverabilityMode(ParameterList_t *TempParam)
{
   int                        Result;
   int                        ret_val;
   GAP_Discoverability_Mode_t DiscoverabilityMode;

   /* First, check that valid Bluetooth Stack ID exists.                */
   if(BluetoothStackID)
   {
      /* Make sure that all of the parameters required for this function*/
      /* appear to be at least semi-valid.                              */
      if((TempParam) && (TempParam->NumberofParameters > 0) && (TempParam->Params[0].intParam >= 0) && (TempParam->Params[0].intParam <= 2))
      {
         /* Parameters appear to be valid, map the specified parameters */
         /* into the API specific parameters.                           */
         if(TempParam->Params[0].intParam == 1)
            DiscoverabilityMode = dmLimitedDiscoverableMode;
         else
         {
            if(TempParam->Params[0].intParam == 2)
               DiscoverabilityMode = dmGeneralDiscoverableMode;
            else
               DiscoverabilityMode = dmNonDiscoverableMode;
         }

         /* Parameters mapped, now set the Discoverability Mode.        */
         Result = GAP_Set_Discoverability_Mode(BluetoothStackID, DiscoverabilityMode, (DiscoverabilityMode == dmLimitedDiscoverableMode)?60:0);

         /* Next, check the return value to see if the command was      */
         /* issued successfully.                                        */
         if(Result >= 0)
         {
            /* The Mode was changed successfully.                       */
            Display(("Discoverability: %s.\r\n", (DiscoverabilityMode == dmNonDiscoverableMode)?"Non":((DiscoverabilityMode == dmGeneralDiscoverableMode)?"General":"Limited")));

            /* Flag success to the caller.                              */
            ret_val = 0;
         }
         else
         {
            /* There was an error setting the Mode.                     */
            DisplayFunctionError("GAP_Set_Discoverability_Mode", Result);

            /* Flag that an error occurred while submitting the command.*/
            ret_val = FUNCTION_ERROR;
         }
      }
      else
      {
         /* One or more of the necessary parameters is/are invalid.     */
         DisplayUsage("SetDiscoverabilityMode [Mode(0 = Non Discoverable, 1 = Limited Discoverable, 2 = General Discoverable)]");

         ret_val = INVALID_PARAMETERS_ERROR;
      }
   }
   else
   {
      /* No valid Bluetooth Stack ID exists.                            */
      ret_val = INVALID_STACK_ID_ERROR;
   }

   return(ret_val);
}

   /* The following function is responsible for setting the             */
   /* Connectability Mode of the local device.  This function returns   */
   /* zero on successful execution and a negative value on all errors.  */
static int SetConnectabilityMode(ParameterList_t *TempParam)
{
   int                       Result;
   int                       ret_val;
   GAP_Connectability_Mode_t ConnectableMode;

   /* First, check that valid Bluetooth Stack ID exists.                */
   if(BluetoothStackID)
   {
      /* Make sure that all of the parameters required for this function*/
      /* appear to be at least semi-valid.                              */
      if((TempParam) && (TempParam->NumberofParameters > 0) && (TempParam->Params[0].intParam >= 0) && (TempParam->Params[0].intParam <= 1))
      {
         /* Parameters appear to be valid, map the specified parameters */
         /* into the API specific parameters.                           */
         if(TempParam->Params[0].intParam == 0)
            ConnectableMode = cmNonConnectableMode;
         else
            ConnectableMode = cmConnectableMode;

         /* Parameters mapped, now set the Connectability Mode.         */
         Result = GAP_Set_Connectability_Mode(BluetoothStackID, ConnectableMode);

         /* Next, check the return value to see if the command was      */
         /* issued successfully.                                        */
         if(Result >= 0)
         {
            /* The Mode was changed successfully.                       */
            Display(("Connectability Mode: %s.\r\n", (ConnectableMode == cmNonConnectableMode)?"Non Connectable":"Connectable"));

            /* Flag success to the caller.                              */
            ret_val = 0;
         }
         else
         {
            /* There was an error setting the Mode.                     */
            DisplayFunctionError("GAP_Set_Connectability_Mode", Result);

            /* Flag that an error occurred while submitting the command.*/
            ret_val = FUNCTION_ERROR;
         }
      }
      else
      {
         /* One or more of the necessary parameters is/are invalid.     */
         DisplayUsage("SetConnectabilityMode [(0 = NonConnectable, 1 = Connectable)]");

         ret_val = INVALID_PARAMETERS_ERROR;
      }
   }
   else
   {
      /* No valid Bluetooth Stack ID exists.                            */
      ret_val = INVALID_STACK_ID_ERROR;
   }

   return(ret_val);
}

   /* The following function is responsible for setting the Pairability */
   /* Mode of the local device.  This function returns zero on          */
   /* successful execution and a negative value on all errors.          */
static int SetPairabilityMode(ParameterList_t *TempParam)
{
   int                    Result;
   int                    ret_val;
   char                   *Mode;
   GAP_Pairability_Mode_t PairabilityMode;

   /* First, check that valid Bluetooth Stack ID exists.                */
   if(BluetoothStackID)
   {
      /* Make sure that all of the parameters required for this function*/
      /* appear to be at least semi-valid.                              */
      if((TempParam) && (TempParam->NumberofParameters > 0) && (TempParam->Params[0].intParam >= 0) && (TempParam->Params[0].intParam <= 2))
      {
         /* Parameters appear to be valid, map the specified parameters */
         /* into the API specific parameters.                           */
         if(TempParam->Params[0].intParam == 0)
         {
            PairabilityMode = pmNonPairableMode;
            Mode            = "pmNonPairableMode";
         }
         else
         {
            if(TempParam->Params[0].intParam == 1)
            {
               PairabilityMode = pmPairableMode;
               Mode            = "pmPairableMode";
            }
            else
            {
               PairabilityMode = pmPairableMode_EnableSecureSimplePairing;
               Mode            = "pmPairableMode_EnableSecureSimplePairing";
            }
         }

         /* Parameters mapped, now set the Pairability Mode.            */
         Result = GAP_Set_Pairability_Mode(BluetoothStackID, PairabilityMode);

         /* Next, check the return value to see if the command was      */
         /* issued successfully.                                        */
         if(Result >= 0)
         {
            /* The Mode was changed successfully.                       */
            Display(("Pairability Mode Changed to %s.\r\n", Mode));

            /* If Secure Simple Pairing has been enabled, inform the    */
            /* user of the current Secure Simple Pairing parameters.    */
            if(PairabilityMode == pmPairableMode_EnableSecureSimplePairing)
               DisplayIOCapabilities();

            /* Flag success to the caller.                              */
            ret_val = 0;
         }
         else
         {
            /* There was an error setting the Mode.                     */
            DisplayFunctionError("GAP_Set_Pairability_Mode", Result);

            /* Flag that an error occurred while submitting the command.*/
            ret_val = FUNCTION_ERROR;
         }
      }
      else
      {
         /* One or more of the necessary parameters is/are invalid.     */
         DisplayUsage("SetPairabilityMode [Mode (0 = Non Pairable, 1 = Pairable, 2 = Pairable (Secure Simple Pairing)]");

         ret_val = INVALID_PARAMETERS_ERROR;
      }
   }
   else
   {
      /* No valid Bluetooth Stack ID exists.                            */
      ret_val = INVALID_STACK_ID_ERROR;
   }

   return(ret_val);
}

   /* The following function is responsible for querying the Bluetooth  */
   /* Device Address of the local Bluetooth Device.  This function      */
   /* returns zero on successful execution and a negative value on all  */
   /* errors.                                                           */
static int GetLocalAddress(ParameterList_t *TempParam)
{
   int       Result;
   int       ret_val;
   BD_ADDR_t BD_ADDR;

   /* First, check that valid Bluetooth Stack ID exists.                */
   if(BluetoothStackID)
   {
      /* Attempt to submit the command.                                 */
      Result = GAP_Query_Local_BD_ADDR(BluetoothStackID, &BD_ADDR);

      /* Check the return value of the submitted command for success.   */
      if(!Result)
      {
         BD_ADDRToStr(BD_ADDR, Function_BoardStr);

         Display(("BD_ADDR of Local Device is: %s.\r\n", Function_BoardStr));

         /* Flag success to the caller.                                 */
         ret_val = 0;
      }
      else
      {
         /* Display a message indicating that an error occurred while   */
         /* attempting to query the Local Device Address.               */
         Display(("GAP_Query_Local_BD_ADDR() Failure: %d.\r\n", Result));

         ret_val = FUNCTION_ERROR;
      }
   }
   else
   {
      /* No valid Bluetooth Stack ID exists.                            */
      ret_val = INVALID_STACK_ID_ERROR;
   }

   return(ret_val);
}

   /* The following function is responsible for changing the baud rate  */
   /* which is used to communicate with the Bluetooth controller.       */
static int SetBaudRate(ParameterList_t *TempParam)
{
   int     ret_val;
   DWord_t BaudRate;

   /* First check to see if the parameters required for the execution of*/
   /* this function appear to be semi-valid.                            */
   if(BluetoothStackID)
   {
      /* Next check to see if the parameters required for the execution */
      /* of this function appear to be semi-valid.                      */
      if((TempParam) && (TempParam->NumberofParameters > 0) && (TempParam->Params[0].intParam))
      {
         /* Save the baud rate.                                         */
         BaudRate = (DWord_t)TempParam->Params[0].intParam;

         /* Check if the baud rate is less than the maximum.            */
         if(BaudRate <= MaxBaudRate)
         {
            /* The baud rate is less than the maximum, next write the   */
            /* command to the device.                                   */
            ret_val = VS_Update_UART_Baud_Rate(BluetoothStackID, BaudRate);
            if(!ret_val)
            {
               Display(("VS_Update_UART_Baud_Rate(%u): Success.\r\n", BaudRate));
            }
            else
            {
               /* Unable to write vendor specific command to chipset.   */
               Display(("VS_Update_UART_Baud_Rate(%u): Failure %d, %d.\r\n", BaudRate, ret_val));

               ret_val = FUNCTION_ERROR;
            }
         }
         else
         {
            Display(("Error: The specified baud rate (%u) is greater than the maximum supported by this platform (%u).\r\n", BaudRate, MaxBaudRate));

            ret_val = FUNCTION_ERROR;
         }
      }
      else
      {
         DisplayUsage("SetBaudRate [BaudRate]");

         ret_val = INVALID_PARAMETERS_ERROR;
      }
   }
   else
   {
      /* One or more of the necessary parameters are invalid.           */
      ret_val = INVALID_PARAMETERS_ERROR;
   }

   return(ret_val);
}

   /* This function is responsible for handling an AVRCP Play command   */
   /* issued by the user.                                               */
static int RemotePlay(ParameterList_t *TempParam)
{
   int ret_val;

   /* Ensure that we have an open stream first.                         */
   if(A3DPOpened)
      ret_val = SendRemoteControlCommand(rcPlay);
   else
   {
      Display(("A3DP Stream is not opened yet.\r\n"));

      ret_val = FUNCTION_ERROR;
   }

   return(ret_val);
}

   /* This function is responsible for handling an AVRCP Pause command  */
   /* issued by the user.                                               */
static int RemotePause(ParameterList_t *TempParam)
{
   int ret_val;

   /* Ensure that we have an open stream first.                         */
   if(A3DPOpened)
      ret_val = SendRemoteControlCommand(rcPause);
   else
   {
      Display(("A3DP Stream is not opened yet.\r\n"));

      ret_val = FUNCTION_ERROR;
   }

   return(ret_val);
}

   /* This function is responsible for handling an AVRCP Next command   */
   /* issued by the user.                                               */
static int RemoteNext(ParameterList_t *TempParam)
{
   int ret_val;

   /* Ensure that we have an open stream first.                         */
   if(A3DPOpened)
      ret_val = SendRemoteControlCommand(rcNext);
   else
   {
      Display(("A3DP Stream is not opened yet.\r\n"));

      ret_val = FUNCTION_ERROR;
   }

   return(ret_val);
}

   /* This function is responsible for handling an AVRCP Back command   */
   /* issued by the user.                                               */
static int RemotePrev(ParameterList_t *TempParam)
{
   int ret_val;

   /* Ensure that we have an open stream first.                         */
   if(A3DPOpened)
      ret_val = SendRemoteControlCommand(rcBack);
   else
   {
      Display(("A3DP Stream is not opened yet.\r\n"));

      ret_val = FUNCTION_ERROR;
   }

   return(ret_val);
}

   /* This function activate the CC256x PCM interface in a loopback     */
   /* mode.                                                             */
static int PcmLoopback(ParameterList_t *TempParam)
{
   int           ret_val = 0;
   unsigned long SamplingFrequency = 0;

   if((TempParam) && (TempParam->NumberofParameters >= 1))
   {
      SamplingFrequency = (unsigned long)TempParam->Params[0].intParam;

      Display(("PCM loopback with frequency = %d.\r\n", SamplingFrequency));

      if((SamplingFrequency == 48000) || (SamplingFrequency == 44100))
      {
         HAL_EnableAudioCodec(BluetoothStackID, aucLoopbackTest, SamplingFrequency, 2);

         VS_Set_PCM_Loopback(BluetoothStackID, TRUE);

         Display(("Status: Audio initialized.\r\n"));

         ret_val = 0;
      }
      else
      {
         Display(("Error: No valid PCM sampling frequency (48000 or 44100) given.\r\n"));
         DisplayUsage("PCMLoopback [SamplingFrequency]");
      }
   }
   else
   {
      Display(("Error: No PCM sampling frequency given.\r\n"));
      DisplayUsage("PCMLoopback [SamplingFrequency]");
   }

   return(ret_val);
}

   /*********************************************************************/
   /*                         Event Callbacks                           */
   /*********************************************************************/

   /* The following function is for the AUD Event Receive Data Callback.*/
   /* This function will be called whenever a Callback has been         */
   /* registered for the specified AUD Action that is associated with   */
   /* the Bluetooth Stack.  This function passes to the caller the AUD  */
   /* Event Data of the specified Event and the AUD Event Callback      */
   /* Parameter that was specified when this Callback was installed.    */
   /* The caller is free to use the contents of the AUD Event Data ONLY */
   /* in the context of this callback.  If the caller requires the Data */
   /* for a longer period of time, then the callback function MUST copy */
   /* the data into another Data Buffer.  This function is guaranteed   */
   /* NOT to be invoked more than once simultaneously for the specified */
   /* installed callback (i.e. this function DOES NOT have be           */
   /* reentrant). It Needs to be noted however, that if the same        */
   /* Callback is installed more than once, then the callbacks will be  */
   /* called serially.  Because of this, the processing in this function*/
   /* should be as efficient as possible.  It should also be noted that */
   /* this function is called in the Thread Context of a Thread that the*/
   /* User does NOT own.  Therefore, processing in this function should */
   /* be as efficient as possible (this argument holds anyway because   */
   /* other AUD Events will not be processed while this function call is*/
   /* outstanding).                                                     */
   /* * NOTE * This function MUST NOT Block and wait for events that can*/
   /*          only be satisfied by Receiving other AUD Events.  A      */
   /*          Deadlock WILL occur because NO AUD Event Callbacks will  */
   /*          be issued while this function is currently outstanding.  */
static void BTPSAPI AUD_Event_Callback(unsigned int BluetoothStackID, AUD_Event_Data_t *AUD_Event_Data, unsigned long CallbackParameter)
{
   Byte_t StatusResult;

   if((BluetoothStackID) && (AUD_Event_Data))
   {
      Display(("\r\n"));

      switch(AUD_Event_Data->Event_Data_Type)
      {
         case etAUD_Open_Request_Indication:
            BD_ADDRToStr(AUD_Event_Data->Event_Data.AUD_Open_Request_Indication_Data->BD_ADDR, Callback_BoardStr);
            Display(("etAUD_Open_Request_Indication\r\n"));
            Display(("BD_ADDR:               %s\r\n", Callback_BoardStr));
            Display(("ConnectionRequestType: %d\r\n", AUD_Event_Data->Event_Data.AUD_Open_Request_Indication_Data->ConnectionRequestType));
            break;
         case etAUD_Stream_Open_Indication:
            /* Occurs whenever the master device connects out to us.    */
            BD_ADDRToStr(AUD_Event_Data->Event_Data.AUD_Stream_Open_Indication_Data->BD_ADDR, Callback_BoardStr);
            Display(("etAUD_Stream_Open_Indication\r\n"));
            Display(("BD_ADDR:     %s\r\n", Callback_BoardStr));
            Display(("MediaMTU:    %d\r\n", AUD_Event_Data->Event_Data.AUD_Stream_Open_Indication_Data->MediaMTU));
            Display(("StreamType:  %s\r\n", (AUD_Event_Data->Event_Data.AUD_Stream_Open_Indication_Data->StreamType == astSRC) ? "SRC" : "SNK"));

            /* Attempt to become master (not critical if we don't).     */
            HCI_Switch_Role(BluetoothStackID, AUD_Event_Data->Event_Data.AUD_Stream_Open_Indication_Data->BD_ADDR, HCI_ROLE_SWITCH_BECOME_MASTER, &StatusResult);

            if(A3DPOpened)
            {
               /* Close and stop any active A3DP streams since we must  */
               /* set this up immediately before SBC data is sent.      */

               if(A3DPPlaying)
                  QueueRemoteControlCommand(A2DPRemoteBD_ADDR, rcPause);

               CloseA3DPStream();
            }

            /* Set this BD_ADDR as being active, and set up the stream. */
            A2DPRemoteBD_ADDR = AUD_Event_Data->Event_Data.AUD_Stream_Open_Indication_Data->BD_ADDR;
            OpenA3DPStream(AUD_Event_Data->Event_Data.AUD_Stream_Open_Indication_Data->BD_ADDR);

            NumConnected++;

            if(NumConnected == MAX_SOURCES)
            {
               /* Turn off connectability and discoverability until one */
               /* of the clients disconnects from us.                   */

               GAP_Set_Discoverability_Mode(BluetoothStackID, dmNonDiscoverableMode, 0);
               GAP_Set_Connectability_Mode(BluetoothStackID, cmNonConnectableMode);
            }

            break;
         case etAUD_Stream_Close_Indication:
            /* Occurs when the master either closes or disconnects.     */
            BD_ADDRToStr(AUD_Event_Data->Event_Data.AUD_Stream_Close_Indication_Data->BD_ADDR, Callback_BoardStr);
            Display(("etAUD_Stream_Close_Indication\r\n"));
            Display(("BD_ADDR:          %s\r\n", Callback_BoardStr));
            Display(("StreamType:       %d\r\n", AUD_Event_Data->Event_Data.AUD_Stream_Close_Indication_Data->StreamType));
            Display(("DisconnectReason: %d\r\n", AUD_Event_Data->Event_Data.AUD_Stream_Close_Indication_Data->DisconnectReason));

            if(COMPARE_BD_ADDR(AUD_Event_Data->Event_Data.AUD_Stream_Close_Indication_Data->BD_ADDR, A2DPRemoteBD_ADDR))
            {
               /* Check to see if this was a requested disconnect.      */
               if(AUD_Event_Data->Event_Data.AUD_Stream_Close_Indication_Data->DisconnectReason == adrRemoteDeviceDisconnect)
               {
                  /* Close the stream normally.                         */
                  ASSIGN_BD_ADDR(A2DPRemoteBD_ADDR, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
                  CloseA3DPStream();
               }
               else
               {
                  /* On link loss or timeout, the A3DP API seems to     */
                  /* return invalid connection handle error codes if we */
                  /* try to close the stream. So, instead we will       */
                  /* manually reset the state here.                     */
                  Display(("Resetting A3DP stream state.\r\n"));
                  A3DPOpened  = FALSE;
                  A3DPPlaying = FALSE;

                  /* Disable the audio codec.                           */
                  HAL_DisableAudioCodec();
               }

               /* Flag that the stream is stopped.                      */
               StreamState = ssStopped;
            }

            NumConnected--;

            /* Turn discoverability and connectability back on, since we*/
            /* have at least one free slot for a source.                */
            GAP_Set_Discoverability_Mode(BluetoothStackID, dmGeneralDiscoverableMode, 0);
            GAP_Set_Connectability_Mode(BluetoothStackID, cmConnectableMode);

            break;
         case etAUD_Remote_Control_Open_Indication:
            BD_ADDRToStr(AUD_Event_Data->Event_Data.AUD_Remote_Control_Open_Indication_Data->BD_ADDR, Callback_BoardStr);

            Display(("etAUD_Remote_Control_Open_Indication\r\n"));
            Display(("BD_ADDR:     %s\r\n", Callback_BoardStr));

            /* Change the stream state appropriately.                   */
            StreamState = A3DPPlaying?ssStarted:ssStopped;
            break;
         case etAUD_Remote_Control_Close_Indication:
            BD_ADDRToStr(AUD_Event_Data->Event_Data.AUD_Remote_Control_Close_Indication_Data->BD_ADDR, Callback_BoardStr);
            Display(("etAUD_Remote_Control_Close_Indication\r\n"));
            Display(("BD_ADDR:          %s\r\n", Callback_BoardStr));
            Display(("DisconnectReason: %d\r\n", AUD_Event_Data->Event_Data.AUD_Remote_Control_Close_Indication_Data->DisconnectReason));

            /* Change the stream state appropriately.                   */
            StreamState = A3DPPlaying?ssStarted:ssStopped;
            break;
         case etAUD_Remote_Control_Command_Indication:
            BD_ADDRToStr(AUD_Event_Data->Event_Data.AUD_Remote_Control_Command_Indication_Data->BD_ADDR, Callback_BoardStr);
            Display(("etAUD_Remote_Control_Command_Indication\r\n"));
            Display(("BD_ADDR:          %s\r\n", Callback_BoardStr));
            Display(("TransactionID:    %d\r\n", AUD_Event_Data->Event_Data.AUD_Remote_Control_Command_Indication_Data->TransactionID));
            break;
         case etAUD_Stream_State_Change_Indication:
            BD_ADDRToStr(AUD_Event_Data->Event_Data.AUD_Stream_State_Change_Indication_Data->BD_ADDR, Callback_BoardStr);
            /* Called whenever the master performs play/stop.           */
            Display(("etAUD_Stream_State_Change_Indication\r\n"));
            Display(("BD_ADDR:     %s\r\n", Callback_BoardStr));
            Display(("StreamType:  %s\r\n", (AUD_Event_Data->Event_Data.AUD_Stream_State_Change_Indication_Data->StreamType == astSRC) ? "SRC" : "SNK"));
            Display(("StreamState: %s\r\n", (AUD_Event_Data->Event_Data.AUD_Stream_State_Change_Indication_Data->StreamState == astStreamStarted) ? "Started" : "Suspended"));

            if(AUD_Event_Data->Event_Data.AUD_Stream_State_Change_Indication_Data->StreamState == astStreamStarted)
               StartA3DPStream(AUD_Event_Data->Event_Data.AUD_Stream_State_Change_Indication_Data->BD_ADDR);
            else
            {
               /* Only issue a stop message if this is the currently    */
               /* active remote A2DP source.                            */
               if(COMPARE_BD_ADDR(A2DPRemoteBD_ADDR, AUD_Event_Data->Event_Data.AUD_Stream_State_Change_Indication_Data->BD_ADDR))
                  StopA3DPStream();
            }
            break;
         case etAUD_Stream_Format_Change_Indication:
            BD_ADDRToStr(AUD_Event_Data->Event_Data.AUD_Stream_Format_Change_Indication_Data->BD_ADDR, Callback_BoardStr);
            Display(("etAUD_Stream_Format_Change_Indication\r\n"));
            Display(("BD_ADDR:     %s\r\n", Callback_BoardStr));
            Display(("StreamType:  %s\r\n",
                (AUD_Event_Data->Event_Data.AUD_Stream_Format_Change_Indication_Data->StreamType == astSRC) ? "SRC" : "SNK"));
            if((A3DPOpened) &&
                (COMPARE_BD_ADDR(A2DPRemoteBD_ADDR, AUD_Event_Data->Event_Data.AUD_Stream_Format_Change_Indication_Data->BD_ADDR)))
            {
               ReconfigureA3DPStream(&(AUD_Event_Data->Event_Data.AUD_Stream_Format_Change_Indication_Data->StreamFormat));
            }
            break;
         case etAUD_Encoded_Audio_Data_Indication:
            BD_ADDRToStr(AUD_Event_Data->Event_Data.AUD_Encoded_Audio_Data_Indication_Data->BD_ADDR, Callback_BoardStr);
            Display(("etAUD_Encoded_Audio_Data_Indication\r\n"));
            Display(("BD_ADDR:  %s\r\n", Callback_BoardStr));
            Display(("Length:   %d\r\n", AUD_Event_Data->Event_Data.AUD_Encoded_Audio_Data_Indication_Data->RawAudioDataFrameLength));
            break;
         case etAUD_Signalling_Channel_Open_Indication:
            BD_ADDRToStr(AUD_Event_Data->Event_Data.AUD_Signalling_Channel_Open_Indication_Data->BD_ADDR, Callback_BoardStr);
            Display(("etAUD_Signalling_Channel_Open_Indication\r\n"));
            Display(("BD_ADDR:  %s\r\n", Callback_BoardStr));
            break;
         case etAUD_Signalling_Channel_Close_Indication:
            BD_ADDRToStr(AUD_Event_Data->Event_Data.AUD_Signalling_Channel_Close_Indication_Data->BD_ADDR, Callback_BoardStr);
            Display(("etAUD_Signalling_Channel_Close_Indication\r\n"));
            Display(("BD_ADDR:  %s\r\n", Callback_BoardStr));
            Display(("DisconnectReason: %d\r\n", AUD_Event_Data->Event_Data.AUD_Signalling_Channel_Close_Indication_Data->DisconnectReason));
            break;
         case etAUD_Remote_Control_Command_Confirmation:
            BD_ADDRToStr(AUD_Event_Data->Event_Data.AUD_Remote_Control_Command_Confirmation_Data->BD_ADDR, Callback_BoardStr);
            Display(("etAUD_Remote_Control_Command_Confirmation\r\n"));
            Display(("BD_ADDR:            %s\r\n", Callback_BoardStr));
            Display(("TransactionID:      %d\r\n", AUD_Event_Data->Event_Data.AUD_Remote_Control_Command_Confirmation_Data->TransactionID));
            Display(("ConfirmationStatus: %d\r\n", AUD_Event_Data->Event_Data.AUD_Remote_Control_Command_Confirmation_Data->ConfirmationStatus));

            /* Check to see if this is a passthrough command.           */
            if((AUD_Event_Data->Event_Data.AUD_Remote_Control_Command_Confirmation_Data->RemoteControlResponseData.MessageType == amtPassThrough) && (AUD_Event_Data->Event_Data.AUD_Remote_Control_Command_Confirmation_Data->ConfirmationStatus == AUD_REMOTE_CONTROL_COMMAND_CONFIRMATION_STATUS_SUCCESS))
            {
               Display(("Pass Through Response Code: 0x%02X\r\n", AUD_Event_Data->Event_Data.AUD_Remote_Control_Command_Confirmation_Data->RemoteControlResponseData.MessageData.PassThroughResponseData.ResponseCode));

               /* Verify that the Pass through command was successful.  */
               if(AUD_Event_Data->Event_Data.AUD_Remote_Control_Command_Confirmation_Data->RemoteControlResponseData.MessageData.PassThroughResponseData.ResponseCode == AVRCP_RESPONSE_ACCEPTED)
               {
                  /* Check to see if this is a play or pause command.   */
                  if(AUD_Event_Data->Event_Data.AUD_Remote_Control_Command_Confirmation_Data->RemoteControlResponseData.MessageData.PassThroughResponseData.OperationID == AVRCP_PASS_THROUGH_ID_PAUSE)
                  {
                     Display(("Successfully paused stream\r\n"));

                     /* Go ahead and flag that streaming is being       */
                     /* stopped.  Even if the stream is not stopped by  */
                     /* the remote device for some number of seconds we */
                     /* need to do so here to preserve the play pause   */
                     /* state.                                          */
                     StreamState = ssStopped;
                  }
                  else
                  {
                     if(AUD_Event_Data->Event_Data.AUD_Remote_Control_Command_Confirmation_Data->RemoteControlResponseData.MessageData.PassThroughResponseData.OperationID == AVRCP_PASS_THROUGH_ID_PLAY)
                     {
                        Display(("Successfully started stream\r\n"));

                        /* Go ahead and flag that streaming is being    */
                        /* started.  Even if the stream is not started  */
                        /* by the remote device for some number of      */
                        /* seconds we need to do so here to preserve the*/
                        /* play pause state.                            */
                        StreamState = ssStarted;
                     }
                  }
               }
               else
               {
                  if(StreamState == ssPending)
                     StreamState = A3DPPlaying?ssStarted:ssStopped;
               }
            }
            else
            {
               if(StreamState == ssPending)
                  StreamState = A3DPPlaying?ssStarted:ssStopped;
            }
            break;
         default:
            Display(("Unhandled AUD event: %d\r\n", AUD_Event_Data->Event_Data_Type));
            break;
      }

      DisplayPrompt();
   }
}

   /* The following function is for the GAP Event Receive Data Callback.*/
   /* This function will be called whenever a Callback has been         */
   /* registered for the specified GAP Action that is associated with   */
   /* the Bluetooth Stack.  This function passes to the caller the GAP  */
   /* Event Data of the specified Event and the GAP Event Callback      */
   /* Parameter that was specified when this Callback was installed.    */
   /* The caller is free to use the contents of the GAP Event Data ONLY */
   /* in the context of this callback.  If the caller requires the Data */
   /* for a longer period of time, then the callback function MUST copy */
   /* the data into another Data Buffer.  This function is guaranteed   */
   /* NOT to be invoked more than once simultaneously for the specified */
   /* installed callback (i.e. this function DOES NOT have be           */
   /* reentrant). It Needs to be noted however, that if the same        */
   /* Callback is installed more than once, then the callbacks will be  */
   /* called serially.  Because of this, the processing in this function*/
   /* should be as efficient as possible.  It should also be noted that */
   /* this function is called in the Thread Context of a Thread that the*/
   /* User does NOT own.  Therefore, processing in this function should */
   /* be as efficient as possible (this argument holds anyway because   */
   /* other GAP Events will not be processed while this function call is*/
   /* outstanding).                                                     */
   /* * NOTE * This function MUST NOT Block and wait for events that can*/
   /*          only be satisfied by Receiving other GAP Events.  A      */
   /*          Deadlock WILL occur because NO GAP Event Callbacks will  */
   /*          be issued while this function is currently outstanding.  */
static void BTPSAPI GAP_Event_Callback(unsigned int BluetoothStackID, GAP_Event_Data_t *GAP_Event_Data,
                                        unsigned long CallbackParameter)
{
   int                               Result;
   int                               Index;
   BD_ADDR_t                         NULL_BD_ADDR;
   Boolean_t                         OOB_Data;
   Boolean_t                         MITM;
#ifdef __TI_A3DP_SNK_BOARD__
   PIN_Code_t                        PINCode;
#else
   GAP_Inquiry_Event_Data_t         *GAP_Inquiry_Event_Data;
#endif
   GAP_IO_Capability_t               RemoteIOCapability;
   GAP_Remote_Name_Event_Data_t     *GAP_Remote_Name_Event_Data;
   GAP_Authentication_Information_t  GAP_Authentication_Information;

   /* First, check to see if the required parameters appear to be       */
   /* semi-valid.                                                       */
   if((BluetoothStackID) && (GAP_Event_Data))
   {
      /* The parameters appear to be semi-valid, now check to see what  */
      /* type the incoming event is.                                    */
      switch(GAP_Event_Data->Event_Data_Type)
      {
         case etInquiry_Result:
            /* The GAP event received was of type Inquiry_Result.       */
            GAP_Inquiry_Event_Data = GAP_Event_Data->Event_Data.GAP_Inquiry_Event_Data;

            /* Next, Check to see if the inquiry event data received    */
            /* appears to be semi-valid.                                */
            if(GAP_Inquiry_Event_Data)
            {
               /* Now, check to see if the gap inquiry event data's     */
               /* inquiry data appears to be semi-valid.                */
               if(GAP_Inquiry_Event_Data->GAP_Inquiry_Data)
               {
                  Display(("\r\n"));

                  /* Display a list of all the devices found from       */
                  /* performing the inquiry.                            */
                  for(Index=0;(Index<GAP_Inquiry_Event_Data->Number_Devices) && (Index<MAX_INQUIRY_RESULTS);Index++)
                  {
                     InquiryResultList[Index] = GAP_Inquiry_Event_Data->GAP_Inquiry_Data[Index].BD_ADDR;
                     BD_ADDRToStr(GAP_Inquiry_Event_Data->GAP_Inquiry_Data[Index].BD_ADDR, Callback_BoardStr);

                     Display(("Result: %d,%s.\r\n", (Index+1), Callback_BoardStr));
                  }

                  NumberofValidResponses = GAP_Inquiry_Event_Data->Number_Devices;
               }
            }
            break;
         case etInquiry_Entry_Result:
            /* Next convert the BD_ADDR to a string.                    */
            BD_ADDRToStr(GAP_Event_Data->Event_Data.GAP_Inquiry_Entry_Event_Data->BD_ADDR, Callback_BoardStr);

            /* Display this GAP Inquiry Entry Result.                   */
            Display(("\r\n"));
            Display(("Inquiry Entry: %s.\r\n", Callback_BoardStr));
            break;
         case etAuthentication:
            /* An authentication event occurred, determine which type of*/
            /* authentication event occurred.                           */
            switch(GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->GAP_Authentication_Event_Type)
            {
               case atLinkKeyRequest:
                  BD_ADDRToStr(GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Remote_Device, Callback_BoardStr);
                  Display(("\r\n"));
                  Display(("atLinkKeyRequest: %s\r\n", Callback_BoardStr));

                  /* Setup the authentication information response      */
                  /* structure.                                         */
                  GAP_Authentication_Information.GAP_Authentication_Type    = atLinkKey;
                  GAP_Authentication_Information.Authentication_Data_Length = 0;

                  /* See if we have stored a Link Key for the specified */
                  /* device.                                            */
                  for(Index=0;Index<(sizeof(LinkKeyInfo)/sizeof(LinkKeyInfo_t));Index++)
                  {
                     if(COMPARE_BD_ADDR(LinkKeyInfo[Index].BD_ADDR, GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Remote_Device))
                     {
                        /* Link Key information stored, go ahead and    */
                        /* respond with the stored Link Key.            */
                        GAP_Authentication_Information.Authentication_Data_Length   = sizeof(Link_Key_t);
                        GAP_Authentication_Information.Authentication_Data.Link_Key = LinkKeyInfo[Index].LinkKey;

                        break;
                     }
                  }

                  /* Submit the authentication response.                */
                  Result = GAP_Authentication_Response(BluetoothStackID, GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Remote_Device, &GAP_Authentication_Information);

                  /* Check the result of the submitted command.         */
                  if(!Result)
                     DisplayFunctionSuccess("GAP_Authentication_Response");
                  else
                     DisplayFunctionError("GAP_Authentication_Response", Result);
                  break;
               case atPINCodeRequest:
                  /* A pin code request event occurred, first display   */
                  /* the BD_ADD of the remote device requesting the pin.*/
                  BD_ADDRToStr(GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Remote_Device, Callback_BoardStr);
                  Display(("\r\n"));
                  Display(("atPINCodeRequest: %s\r\n", Callback_BoardStr));

                  /* Note the current Remote BD_ADDR that is requesting */
                  /* the PIN Code.                                      */
                  CurrentRemoteBD_ADDR = GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Remote_Device;

                  /* Inform the user that they will need to respond with*/
                  /* a PIN Code Response.                               */
                  Display(("Respond with: PINCodeResponse\r\n"));

                  break;
               case atAuthenticationStatus:
                  /* An authentication status event occurred, display   */
                  /* all relevant information.                          */
                  BD_ADDRToStr(GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Remote_Device,
                                Callback_BoardStr);
                  Display(("\r\n"));
                  Display(("atAuthenticationStatus: %d for %s\r\n",
                    GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Authentication_Event_Data.Authentication_Status,
                    Callback_BoardStr));
                  /* Flag that there is no longer a current             */
                  /* Authentication procedure in progress.              */
                  ASSIGN_BD_ADDR(CurrentRemoteBD_ADDR, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);

                  /* If authentication failed delete the Link Key stored*/
                  /* for this device.                                   */
                  if(GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Authentication_Event_Data.Authentication_Status)
                  {
                     for(Index=0;Index<(sizeof(LinkKeyInfo)/sizeof(LinkKeyInfo_t));Index++)
                     {
                        if(COMPARE_BD_ADDR(LinkKeyInfo[Index].BD_ADDR,
                            GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Remote_Device))
                        {
                           BTPS_MemInitialize(&(LinkKeyInfo[Index]), 0, sizeof(LinkKeyInfo_t));
                           break;
                        }
                     }
                  }
                  break;
               case atLinkKeyCreation:
                  /* A link key creation event occurred, first display  */
                  /* the remote device that caused this event.          */
                  BD_ADDRToStr(GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Remote_Device, Callback_BoardStr);
                  Display(("\r\n"));
                  Display(("atLinkKeyCreation: %s\r\n", Callback_BoardStr));

                  /* Now store the link Key in either a free location OR*/
                  /* over the old key location.                         */
                  ASSIGN_BD_ADDR(NULL_BD_ADDR, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);

                  for(Index=0,Result=-1;Index<(sizeof(LinkKeyInfo)/sizeof(LinkKeyInfo_t));Index++)
                  {
                     if(COMPARE_BD_ADDR(LinkKeyInfo[Index].BD_ADDR,
                        GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Remote_Device))
                     {
                        break;
                     }
                     else
                     {
                        if((Result == (-1)) && (COMPARE_BD_ADDR(LinkKeyInfo[Index].BD_ADDR, NULL_BD_ADDR)))
                        {
                           Result = Index;
                        }
                     }
                  }

                  /* If we didn't find a match, see if we found an empty*/
                  /* location.                                          */
                  if(Index == (sizeof(LinkKeyInfo)/sizeof(LinkKeyInfo_t)))
                     Index = Result;

                  /* Check to see if we found a location to store the   */
                  /* Link Key information into.                         */
                  if(Index != (-1))
                  {
                     LinkKeyInfo[Index].BD_ADDR = GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Remote_Device;
                     LinkKeyInfo[Index].LinkKey = GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Authentication_Event_Data.Link_Key_Info.Link_Key;

                     Display(("Link Key Stored.\r\n"));
                  }
                  else
                     Display(("Link Key array full.\r\n"));
                  break;
               case atIOCapabilityRequest:
                  BD_ADDRToStr(GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Remote_Device, Callback_BoardStr);
                  Display(("\r\n"));
                  Display(("atIOCapabilityRequest: %s\r\n", Callback_BoardStr));

                  /* Setup the Authentication Information Response      */
                  /* structure.                                         */
                  GAP_Authentication_Information.GAP_Authentication_Type                                      = atIOCapabilities;
                  GAP_Authentication_Information.Authentication_Data_Length                                   = sizeof(GAP_IO_Capabilities_t);
                  GAP_Authentication_Information.Authentication_Data.IO_Capabilities.IO_Capability            = (GAP_IO_Capability_t)IOCapability;
                  GAP_Authentication_Information.Authentication_Data.IO_Capabilities.MITM_Protection_Required = MITMProtection;
                  GAP_Authentication_Information.Authentication_Data.IO_Capabilities.OOB_Data_Present         = OOBSupport;

                  /* Submit the Authentication Response.                */
                  Result = GAP_Authentication_Response(BluetoothStackID, GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Remote_Device, &GAP_Authentication_Information);

                  /* Check the result of the submitted command.         */
                  if(!Result)
                     DisplayFunctionSuccess("Auth");
                  else
                     DisplayFunctionError("Auth", Result);
                  break;
               case atIOCapabilityResponse:
                  BD_ADDRToStr(GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Remote_Device, Callback_BoardStr);
                  Display(("\r\n"));
                  Display(("atIOCapabilityResponse: %s\r\n", Callback_BoardStr));

                  RemoteIOCapability = GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Authentication_Event_Data.IO_Capabilities.IO_Capability;
                  MITM               = (Boolean_t)GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Authentication_Event_Data.IO_Capabilities.MITM_Protection_Required;
                  OOB_Data           = (Boolean_t)GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Authentication_Event_Data.IO_Capabilities.OOB_Data_Present;

                  Display(("Capabilities: %s%s%s\r\n", IOCapabilitiesStrings[RemoteIOCapability], ((MITM)?", MITM":""), ((OOB_Data)?", OOB Data":"")));
                  break;
               case atUserConfirmationRequest:
                  BD_ADDRToStr(GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Remote_Device, Callback_BoardStr);
                  Display(("\r\n"));
                  Display(("atUserConfirmationRequest: %s\r\n", Callback_BoardStr));

                  CurrentRemoteBD_ADDR = GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Remote_Device;

                  if(IOCapability != icDisplayYesNo)
                  {
                     /* Invoke JUST Works Process...                    */
                     GAP_Authentication_Information.GAP_Authentication_Type          = atUserConfirmation;
                     GAP_Authentication_Information.Authentication_Data_Length       = (Byte_t)sizeof(Byte_t);
                     GAP_Authentication_Information.Authentication_Data.Confirmation = TRUE;

                     /* Submit the Authentication Response.             */
                     Display(("\r\nAuto Accepting: %l\r\n", GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Authentication_Event_Data.Numeric_Value));

                     Result = GAP_Authentication_Response(BluetoothStackID, GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Remote_Device, &GAP_Authentication_Information);

                     if(!Result)
                        DisplayFunctionSuccess("GAP_Authentication_Response");
                     else
                        DisplayFunctionError("GAP_Authentication_Response", Result);

                     /* Flag that there is no longer a current          */
                     /* Authentication procedure in progress.           */
                     ASSIGN_BD_ADDR(CurrentRemoteBD_ADDR, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
                  }
                  else
                  {
                     Display(("User Confirmation: %l\r\n", (unsigned long)GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Authentication_Event_Data.Numeric_Value));

                     /* Inform the user that they will need to respond  */
                     /* with a PIN Code Response.                       */
                     Display(("Respond with: UserConfirmationResponse\r\n"));
                  }
                  break;
               case atPasskeyRequest:
                  BD_ADDRToStr(GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Remote_Device, Callback_BoardStr);
                  Display(("\r\n"));
                  Display(("atPasskeyRequest: %s\r\n", Callback_BoardStr));

                  /* Note the current Remote BD_ADDR that is requesting */
                  /* the Passkey.                                       */
                  CurrentRemoteBD_ADDR = GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Remote_Device;

                  /* Inform the user that they will need to respond with*/
                  /* a Passkey Response.                                */
                  Display(("Respond with: PassKeyResponse\r\n"));
                  break;
               case atRemoteOutOfBandDataRequest:
                  /* This application does not support OOB data so      */
                  /* respond with a data length of Zero to force a      */
                  /* negative reply.                                    */
                  GAP_Authentication_Information.GAP_Authentication_Type    = atOutOfBandData;
                  GAP_Authentication_Information.Authentication_Data_Length = 0;

                  Result = GAP_Authentication_Response(BluetoothStackID, GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Remote_Device, &GAP_Authentication_Information);
                  break;
               case atPasskeyNotification:
                  BD_ADDRToStr(GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Remote_Device, Callback_BoardStr);
                  Display(("\r\n"));
                  Display(("atPasskeyNotification: %s\r\n", Callback_BoardStr));

                  Display(("Passkey Value: %lu\r\n", GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Authentication_Event_Data.Numeric_Value));
                  break;
               case atKeypressNotification:
                  BD_ADDRToStr(GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Remote_Device, Callback_BoardStr);
                  Display(("\r\n"));
                  Display(("atKeypressNotification: %s\r\n", Callback_BoardStr));

                  Display(("Keypress: %d\r\n", (int)GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Authentication_Event_Data.Keypress_Type));
                  break;
               default:
                  Display(("Un-handled Auth. Event.\r\n"));
                  break;
            }
            break;
         case etRemote_Name_Result:
            /* Bluetooth Stack has responded to a previously issued     */
            /* Remote Name Request that was issued.                     */
            GAP_Remote_Name_Event_Data = GAP_Event_Data->Event_Data.GAP_Remote_Name_Event_Data;
            if(GAP_Remote_Name_Event_Data)
            {
               /* Inform the user of the Result.                        */
               BD_ADDRToStr(GAP_Remote_Name_Event_Data->Remote_Device, Callback_BoardStr);

               Display(("\r\n"));
               Display(("BD_ADDR: %s.\r\n", Callback_BoardStr));

               if(GAP_Remote_Name_Event_Data->Remote_Name)
                  Display(("Name: %s.\r\n", GAP_Remote_Name_Event_Data->Remote_Name));
               else
                  Display(("Name: NULL.\r\n"));
            }
            break;
         case etEncryption_Change_Result:
            BD_ADDRToStr(GAP_Event_Data->Event_Data.GAP_Encryption_Mode_Event_Data->Remote_Device, Callback_BoardStr);
            Display(("\r\netEncryption_Change_Result for %s, Status: 0x%02X, Mode: %s.\r\n", Callback_BoardStr,
                                                                                             GAP_Event_Data->Event_Data.GAP_Encryption_Mode_Event_Data->Encryption_Change_Status,
                                                                                             ((GAP_Event_Data->Event_Data.GAP_Encryption_Mode_Event_Data->Encryption_Mode == emDisabled)?"Disabled": "Enabled")));
            break;
         default:
            /* An unknown/unexpected GAP event was received.            */
            Display(("\r\nUnknown Event: %d.\r\n", GAP_Event_Data->Event_Data_Type));
            break;
      }
   }
   else
   {
      /* There was an error with one or more of the input parameters.   */
      Display(("\r\n"));
      Display(("Null Event\r\n"));
   }

   DisplayPrompt();
}

   /* The following function is used to initialize the application      */
   /* instance.  This function should open the stack and prepare to     */
   /* execute commands based on user input.  The first parameter passed */
   /* to this function is the HCI Driver Information that will be used  */
   /* when opening the stack and the second parameter is used to pass   */
   /* parameters to BTPS_Init.  This function returns the               */
   /* BluetoothStackID returned from BSC_Initialize on success or a     */
   /* negative error code (of the form APPLICATION_ERROR_XXX).          */
int InitializeApplication(HCI_DriverInformation_t *HCI_DriverInformation, BTPS_Initialization_t *BTPS_Initialization)
{
   int ret_val = APPLICATION_ERROR_UNABLE_TO_OPEN_STACK;

   /* Initialize some defaults.                                         */
   NumberofValidResponses = 0;

   /* Next, makes sure that the Driver Information passed appears to be */
   /* semi-valid.                                                       */
   if((HCI_DriverInformation) && (BTPS_Initialization))
   {
      /* Try to Open the stack and check if it was successful.          */
      if(!OpenStack(HCI_DriverInformation, BTPS_Initialization))
      {
         /* The stack was opened successfully.  Now set some defaults.  */

         /* First, attempt to set the Device to be Connectable.         */
         ret_val = SetConnect();

         /* Next, check to see if the Device was successfully made      */
         /* Connectable.                                                */
         if(!ret_val)
         {
            /* Now that the device is Connectable attempt to make it    */
            /* Discoverable.                                            */
            ret_val = SetDisc();

            /* Next, check to see if the Device was successfully made   */
            /* Discoverable.                                            */
            if(!ret_val)
            {
               /* Now that the device is discoverable attempt to make it*/
               /* pairable.                                             */
               ret_val = SetPairable();
               if(!ret_val)
               {
                  /* Save the maximum supported baud rate.              */
                  MaxBaudRate = (DWord_t)(HCI_DriverInformation->DriverInformation.COMMDriverInformation.BaudRate);

                  /* Set up the Selection Interface.                    */
                  UserInterface();

                  /* Display the first command prompt.                  */
                  DisplayPrompt();

                  /* Return success to the caller.                      */
                  ret_val = (int)BluetoothStackID;
               }
               else
                  DisplayFunctionError("SetPairable", ret_val);
            }
            else
               DisplayFunctionError("SetDisc", ret_val);
         }
         else
            DisplayFunctionError("SetDisc", ret_val);

         /* In some error occurred then close the stack.                */
         if(ret_val < 0)
         {
            /* Close the Bluetooth Stack.                               */
            CloseStack();
         }
      }
      else
      {
         /* There was an error while attempting to open the Stack.      */
         Display(("Unable to open the stack.\r\n"));
      }
   }
   else
      ret_val = APPLICATION_ERROR_INVALID_PARAMETERS;

   return(ret_val);
}

   /* Displays the application's prompt.                                */
void DisplayPrompt(void)
{
   Display(("\r\nA3DP+SNK>"));
}

   /* The following function is used to process a command line string.  */
   /* This function takes as it's only parameter the command line string*/
   /* to be parsed and returns TRUE if a command was parsed and executed*/
   /* or FALSE otherwise.                                               */
Boolean_t ProcessCommandLine(char *String)
{
   return(CommandLineInterpreter(String));
}
