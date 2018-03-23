/*****< hfpdemo_hf.c >*********************************************************/
/*      Copyright 2003 - 2014 Stonestreet One.                                */
/*      All Rights Reserved.                                                  */
/*                                                                            */
/*      Copyright 2015 Texas Instruments Incorporated.                        */
/*      All Rights Reserved.                                                  */
/*                                                                            */
/*  HFPDEMO_HF - Simple embedded application using HFRE v1.6 Profile.         */
/*                                                                            */
/*  Author:  Rory Sledge                                                      */
/*                                                                            */
/*** MODIFICATION HISTORY *****************************************************/
/*                                                                            */
/*   mm/dd/yy  F. Lastname    Description of Modification                     */
/*   --------  -----------    ------------------------------------------------*/
/*   01/03/03  R. Sledge       Initial creation.                              */
/*   06/22/12  T. Cook         Ported to embedded.                            */
/*   03/03/15  D. Horowitz     Adding Demo Application version.               */
/*   03/22/15  D. Keren        Adding Audio with the CC256x PCM               */
/*   04/07/15  D. Horowitz     Adding 3Way Calling support.                   */
/*   06/11/15  D. Keren        Adding two function, for two phones connection */
/*                             scenario. One to switch the audio between      */
/*                             two active calls in the two phones. Second to  */
/*                             answer waiting call during active call and     */
/*                             switch the audio between the two phones.       */
/******************************************************************************/
#include <stdio.h>         /* Included for sscanf.                            */
#include "Main.h"          /* Application Interface Abstraction.              */
#include "HFPDemo_HF.h"    /* Application Header.                             */
#include "SS1BTPS.h"       /* Includes for the SS1 Bluetooth Protocol Stack.  */
#include "SS1BTHFR.h"      /* Bluetooth HFRE API Prototypes/Constants.        */
#include "SS1BTVS.h"       /* Vendor Specific Prototypes/Constants.           */
#include "BTPSKRNL.h"      /* BTPS Kernel Header.                             */
#include "HAL.h"           /* Hardware Abstraction Layer Header.              */

#define MAX_SUPPORTED_COMMANDS                     (38)  /* Denotes the       */
                                                         /* maximum number of */
                                                         /* User Commands that*/
                                                         /* are supported by  */
                                                         /* this application. */

#define MAX_NUM_OF_PARAMETERS                       (6)  /* Denotes the max   */
                                                         /* number of         */
                                                         /* parameters a      */
                                                         /* command can have. */

#define MAX_COMMAND_LENGTH                         (64)  /* Denotes the max   */
                                                         /* buffer size used  */
                                                         /* for user commands */
                                                         /* input via the     */
                                                         /* UserInterface.    */

#define MAX_INQUIRY_RESULTS                        (25)  /* Denotes the max   */
                                                         /* number of inquiry */
                                                         /* results.          */

#define DEFAULT_IO_CAPABILITY       (icNoInputNoOutput)  /* Denotes the       */
                                                         /* default I/O       */
                                                         /* Capability that is*/
                                                         /* used with Secure  */
                                                         /* Simple Pairing.   */

#define MAXIMUM_SUPPORTED_HFRE_CONNECTIONS          (2)  /* Denotes the       */
                                                         /* Maximum available */
                                                         /* connections of    */
                                                         /* the demoApp.      */

#define HFP_SERVER_PORT                             (1)  /* Denotes the number*/
                                                         /* of the Server Port*/
                                                         /* that will be used */
                                                         /* by the SPP to open*/
                                                         /* the RFCOMM port.  */

#define DEFAULT_MITM_PROTECTION                  (FALSE) /* Denotes the       */
                                                         /* default value used*/
                                                         /* for Man in the    */
                                                         /* Middle (MITM)     */
                                                         /* protection used   */
                                                         /* with Secure Simple*/
                                                         /* Pairing.          */

#define NO_COMMAND_ERROR                           (-1)  /* Denotes that no   */
                                                         /* command was spec. */
                                                         /* to the parser     */

#define INVALID_COMMAND_ERROR                      (-2)  /* Denotes that the  */
                                                         /* Command does not  */
                                                         /* exist for process.*/

#define EXIT_CODE                                  (-3)  /* Denotes that the  */
                                                         /* command is to     */
                                                         /* exit.             */

#define FUNCTION_ERROR                             (-4)  /* Denotes that an   */
                                                         /* error occurred in */
                                                         /* execution of the  */
                                                         /* function.         */

#define TO_MANY_PARAMS                             (-5)  /* Denotes that there*/
                                                         /* are more          */
                                                         /* parameters then   */
                                                         /* will fit in the   */
                                                         /* UserCommand.      */

#define INVALID_PARAMETERS_ERROR                   (-6)  /* Denotes that an   */
                                                         /* error occurred due*/
                                                         /* to the fact that  */
                                                         /* one or more of the*/
                                                         /* required params.  */
                                                         /* were invalid.     */

#define UNABLE_TO_INITIALIZE_STACK                 (-7)  /* Denotes that an   */
                                                         /* error occurred    */
                                                         /* while initializing*/
                                                         /* the stack.        */

#define INVALID_STACK_ID_ERROR                     (-8)  /* Denotes that an   */
                                                         /* error occurred due*/
                                                         /* to attempted      */
                                                         /* execution of a    */
                                                         /* command without a */
                                                         /* valid Bluetooth   */
                                                         /* Stack ID.         */

#define UNABLE_TO_REGISTER_SERVER                  (-9)  /* Denotes that an   */
                                                         /* error occurred    */
                                                         /* when trying to    */
                                                         /* create a Serial   */
                                                         /* Port Server.      */

#define MAXIMUM_HFRE_SUPPORTED_CONNECTIONS_REACHED (-10) /* Denotes that an   */
                                                         /* error occurred    */
                                                         /* when trying to    */
                                                         /* create more Serial*/
                                                         /* Port Server than  */
                                                         /* is allowed.       */

#define TWO_HFRE_VOICES_ARE_WORKING_IN_PARALLEL    (-11) /* Denotes that an   */
                                                         /* error occurred    */
                                                         /* when trying switch*/
                                                         /* between calls when*/
                                                         /* two voices are    */
                                                         /* working in        */
                                                         /* parallel.         */

#define CVSD_FREQUENCY                             (8000)/* 8KHz sampling     */
                                                         /* frequency in      */
                                                         /* simple SCO/eSCO   */
                                                         /* with CVSD air     */
                                                         /* encoding         */

#define WBS_FREQUENCY                             (16000)/* 16KHz sampling    */
                                                         /* frequency in WBS  */
                                                         /* eSCO              */

#define FIRST_EMPTY_SLOT                            (0)  /* Denoted the value */
                                                         /* of an empty memory*/
                                                         /* cell in the portID*/
                                                         /* Array.            */

#define HFRE_SUPPORTED_FEATURES                    (HFRE_CLI_SUPPORTED_BIT | \
                                                    HFRE_CALL_WAITING_THREE_WAY_CALLING_SUPPORTED_BIT | \
                                                    HFRE_HF_ENHANCED_CALL_STATUS_SUPPORTED_BIT | \
                                                    HFRE_HF_SOUND_ENHANCEMENT_SUPPORTED_BIT | \
                                                    HFRE_HF_VOICE_RECOGNITION_SUPPORTED_BIT | \
                                                    HFRE_HF_ENHANCED_CALL_CONTROL_SUPPORTED_BIT | \
                                                    HFRE_HF_CODEC_NEGOTIATION_SUPPORTED_BIT)

   /* The following converts an ASCII character to an integer value.    */
#define ToInt(_x)                                  (((_x) > 0x39)?((_x)-0x37):((_x)-0x30))

   /* Determine the Name we will use for this compilation.              */
#define APP_DEMO_NAME                              "HFPDemo_HF"

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
typedef char BoardStr_t[16];

   /* The following structure for is used to hold a list of information */
   /* on all paired devices.                                            */
typedef struct _tagServerPortInfo_t
{
   unsigned int  HFServerPortID;
   Byte_t        SupportWBS;
   Boolean_t     IsConnected;
   Boolean_t     IsCallSetupInProgress;
   Boolean_t     IsInActiveCall;
   Boolean_t     IsCallOnHold;
   Boolean_t     IsInActiveSCO;
   Boolean_t     IsInActiveAudio;
   Boolean_t     RemoteSupportedFeaturesValid;
   unsigned long RemoteSupportedFeatures;
   BD_ADDR_t     RemoteBD_ADDR;
} ServerPortInfo_t;

   /* Internal Variables to this Module (Remember that all variables    */
   /* declared static are initialized to 0 automatically by the         */
   /* compiler as part of standard C/C++).                              */

static unsigned int        BluetoothStackID;        /* Variable which holds the Handle */
                                                    /* of the opened Bluetooth Protocol*/
                                                    /* Stack.                          */

static DWord_t             HFServerSDPHandle;       /* Variable used to hold the Serial*/
                                                    /* Port Service Record of the      */
                                                    /* Serial Port Server SDP Service  */
                                                    /* Record.                         */

static ServerPortInfo_t    HFServerPortDATA[MAXIMUM_SUPPORTED_HFRE_CONNECTIONS];
                                                    /* Variable which Holds the Server */
                                                    /* Server Port ID, the BD_ADDR and */
                                                    /* if this port is in used.        */

static int                 NumberofactiveRFCOMM;    /* Variable which contains the     */
                                                    /* number of active RFCOMM ports   */
                                                    /* and limit the user to the       */
                                                    /* maximum connection allowed.     */

static char                NumberOfCurrentConnections; /* Variable which contains      */
                                                    /* the number of active HFP        */
                                                    /* connections.                    */

static unsigned int        WBS_Connected_Port_ID;   /* Variable which flags whether    */
                                                    /* or not WBS is enabled on one of */
                                                    /* the connections or not.         */
                                                    /* *Note* Due to Hardware          */
                                                    /* limitations - only one WBS can  */
                                                    /* be established in current time  */

static BD_ADDR_t           InquiryResultList[MAX_INQUIRY_RESULTS];  /* Variable which  */
                                                    /* contains the inquiry result     */
                                                    /* received from the most recently */
                                                    /* preformed inquiry.              */

static unsigned int        NumberofValidResponses;  /* Variable which holds the number */
                                                    /* of valid inquiry results within */
                                                    /* the inquiry results array.      */

static LinkKeyInfo_t       LinkKeyInfo[MAXIMUM_SUPPORTED_HFRE_CONNECTIONS*3];
                                                    /* Variable which holds the list   */
                                                    /* of BD_ADDR <-> Link Keys for    */
                                                    /* pairing.                        */

static BD_ADDR_t           CurrentRemoteBD_ADDR;    /* Variable which holds the        */
                                                    /* current BD_ADDR of the device   */
                                                    /* which is currently pairing or   */
                                                    /* authenticating.                 */

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

static CommandTable_t      CommandTable[MAX_SUPPORTED_COMMANDS]; /* Variable which is  */
                                                    /* used to hold the actual Commands*/
                                                    /* that are supported by this      */
                                                    /* application.                    */

static int                 lastAudioFreq;           /* Integer value for check before  */
                                                    /* Audio init, if the frequency did*/
                                                    /* change and do un-init before    */
                                                    /* init.                           */

   /* The following string table is used to map HCI Version information */
   /* to an easily displayable version string.                          */
static char *HCIVersionStrings[] =
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

#define NUM_SUPPORTED_HCI_VERSIONS              (sizeof(HCIVersionStrings)/sizeof(char *) - 1)

   /* The following string table is used to map the API I/O Capabilities*/
   /* values to an easily displayable string.                           */
static char *IOCapabilitiesStrings[] =
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

   /* Command line functions                                            */
static unsigned long StringToUnsignedInteger(char *StringInteger);
static char *StringParser(char *String);
static int CommandParser(UserCommand_t *TempCommand, char *UserInput);
static int CommandInterpreter(UserCommand_t *TempCommand);
static int AddCommand(char *CommandName, CommandFunction_t CommandFunction);
static CommandFunction_t FindCommand(char *Command);
static void ClearCommands(void);

static void BD_ADDRToStr(BD_ADDR_t Board_Address, char *BoardStr);
static void DisplayFunctionError(char *Function, int Status);
static void DisplayUsageHoldingMultipartyCall(void);
static int DisplayFunctionChecker(char *Function, int Status);
static int  DisplayHelp(ParameterList_t *TempParam);
static void DisplayFWVersion (void);
static int QueryActiveConnections(ParameterList_t *TempParam);
static void DisplayConnectionStatus(int ServerPortIndex);
static int DisableWBS(int ServerPortIndex);
static void SendAvailableCodecs(unsigned int Index);
static unsigned int NumberOfAnsweredCalls(void);

static int OpenStack(HCI_DriverInformation_t *HCI_DriverInformation, BTPS_Initialization_t *BTPS_Initialization);
static int CloseStack(void);

static int SetDisc(void);
static int SetConnect(void);
static int SetPairable(void);
static int DeleteLinkKey(BD_ADDR_t BD_ADDR);

static int HFRESetupAudioConnection(unsigned int ServerPortInfoIndex);
static int HFREReleaseAudioConnection(unsigned int ServerPortInfoIndex);

static int Inquiry(ParameterList_t *TempParam);
static int DisplayInquiryList(ParameterList_t *TempParam);
static int SetDiscoverabilityMode(ParameterList_t *TempParam);
static int SetConnectabilityMode(ParameterList_t *TempParam);
static int SetPairabilityMode(ParameterList_t *TempParam);
static int ChangeSimplePairingParameters(ParameterList_t *TempParam);
static int Pair(ParameterList_t *TempParam);
static int EndPairing(ParameterList_t *TempParam);
static int PINCodeResponse(ParameterList_t *TempParam);
static int PassKeyResponse(ParameterList_t *TempParam);
static int UserConfirmationResponse(ParameterList_t *TempParam);
static int GetLocalAddress(ParameterList_t *TempParam);
static int SetLocalName(ParameterList_t *TempParam);
static int GetLocalName(ParameterList_t *TempParam);
static int SetClassOfDevice(ParameterList_t *TempParam);
static int GetClassOfDevice(ParameterList_t *TempParam);
static int GetRemoteName(ParameterList_t *TempParam);

static int FindHFActivePortEntry(void);
static int FindHFServerPortEntry(unsigned int HFServerPort);
static int OpenHFServer(ParameterList_t *TempParam);
static int CloseHFServer(ParameterList_t *TempParam);
static int ManageAudioConnection(ParameterList_t *TempParam);
static int AnswerIncomingCall(ParameterList_t *TempParam);
static int HangUpCall(ParameterList_t *TempParam);
static int PcmLoopback(ParameterList_t *TempParam);
static int CallHoldMultipartyHandlingIntToENUMSwitch(SDWord_t UserInput, HFRE_Call_Hold_Multiparty_Handling_Type_t *CallHoldMultipartyHandling);
static int SearchForServerPortIndexByUserInputs(ParameterList_t *TempParam, int *ServerPortIndex);
static void OpenServiceLevelConnectionIndication(HFRE_Event_Data_t *HFREEventData, int *ServerPortIndex);
static void ControlIndicatorStatusIndicationCallParameter(HFRE_Event_Data_t *HFREEventData, int *ServerPortIndex, int *Result);
static void ControlIndicatorStatusIndicationCallSetupParameter(HFRE_Event_Data_t *HFREEventData, int *ServerPortIndex);
static void ControlIndicatorStatusIndicationCallHeldParameter(HFRE_Event_Data_t *HFREEventData, int *ServerPortIndex);
static void ClosePortIndication(int *ServerPortIndex);
static void AudioConnectionIndication(HFRE_Event_Data_t *HFREEventData, int *ServerPortIndex, Word_t *ConnectionHandle, int *Result);
static void AudioDisconnectionIndication(unsigned int ServerPortInfoIndex);
static void CodecSelectRequestIndication(HFRE_Event_Data_t *HFREEventData, int *ServerPortIndex, Word_t *ConnectionHandle, BoardStr_t *BoardStr, int *Result);

static int PlaceCallOnHold(ParameterList_t *TempParam);
static int HoldingMultipartyCall(ParameterList_t *TempParam);
static int HoldingMultiPhonesCall(ParameterList_t *TempParam);
static int SwitchAudioFor2PhonesWithActiveCall(ParameterList_t *TempParam);
static int AnswerSeconePhoneIncomingCallAndSwitchAudio(ParameterList_t *TempParam);
static int QueryMemory(ParameterList_t *TempParam);

   /* Callback Function Prototypes.                                     */
static void BTPSAPI GAP_Event_Callback(unsigned int BluetoothStackID, GAP_Event_Data_t *GAPEventData, unsigned long CallbackParameter);
static void BTPSAPI HFRE_Event_Callback(unsigned int BluetoothStackID, HFRE_Event_Data_t *HFRE_Event_Data, unsigned long CallbackParameter);

   /* This function is responsible for taking the users input and do the*/
   /* appropriate thing with it.  First, this function get a string of  */
   /* user input, parse the user input in to command and parameters, and*/
   /* finally executing the command or display an error message if the  */
   /* input is corrupt.                                                 */
static void UserInterface(void)
{
   /* First Clear all of the commands in the Command Table.             */
   ClearCommands();

   /* Now add all of the commands that are associated with the HID      */
   /* Device unit to the Command Table.                                 */
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
   AddCommand("OPENHFSERVER", OpenHFServer);
   AddCommand("CLOSEHFSERVER", CloseHFServer);
   AddCommand("PLACECALLONHOLD", PlaceCallOnHold);
   AddCommand("HOLDINGMULTIPARTYCALL", HoldingMultipartyCall);
   AddCommand("HOLDINGMULTIPHONESCALL", HoldingMultiPhonesCall);
   AddCommand("2PHONESCALLAUDIOSWITCH", SwitchAudioFor2PhonesWithActiveCall);
   AddCommand("2PHONESANSWERCALL", AnswerSeconePhoneIncomingCallAndSwitchAudio);
   AddCommand("MANAGEAUDIO", ManageAudioConnection);
   AddCommand("ANSWERCALL", AnswerIncomingCall);
   AddCommand("HANGUPCALL", HangUpCall);
   AddCommand("QUERYACTIVECONNECTIONS", QueryActiveConnections);
   AddCommand("QUERYMEMORY", QueryMemory);
   AddCommand("PCMLOOPBACK", PcmLoopback);
   AddCommand("HELP", DisplayHelp);
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
            break;
      }

      /* Display a prompt.                                              */
      DisplayPrompt();

      ret_val = TRUE;
   }
   else
      Display(("\r\nInvalid Command.\r\n"));

   return(ret_val);
}

   /* The following function is responsible for converting number       */
   /* strings to their unsigned integer equivalent.  This function can  */
   /* handle leading and tailing white space, however it does not handle*/
   /* signed or comma delimited values.  This function takes as its     */
   /* input the string which is to be converted.  The function returns  */
   /* zero if an error occurs otherwise it returns the value parsed from*/
   /* the string passed as the input parameter.                         */
static unsigned long StringToUnsignedInteger(char *StringInteger)
{
   int           IsHex;
   unsigned long Index;
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
static int CommandParser(UserCommand_t *TempCommand, char *UserInput)
{
   int            ret_val;
   int            StringLength;
   char          *LastParameter;
   unsigned int   Count         = 0;

   /* Before proceeding make sure that the passed parameters appear to  */
   /* be at least semi-valid.                                           */
   if((TempCommand) && (UserInput) && (BTPS_StringLength(UserInput)))
   {
      /* First get the initial string length.                           */
      StringLength = BTPS_StringLength(UserInput);

      /* Retrieve the first token in the string, this should be the     */
      /* command.                                                       */
      TempCommand->Command = StringParser(UserInput);

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
         UserInput    += BTPS_StringLength(TempCommand->Command)+1;
         StringLength  = BTPS_StringLength(UserInput);

         /* There was an available command, now parse out the parameters*/
         while((StringLength > 0) && ((LastParameter = StringParser(UserInput)) != NULL))
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
               UserInput    += BTPS_StringLength(LastParameter)+1;
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
            if(!((*CommandFunction)(&TempCommand->Parameters)))
            {
               /* Return success to the caller.                         */
               ret_val = 0;
            }
            else
               ret_val = FUNCTION_ERROR;
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
   int ret_val;

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
         if(BTPS_MemCompare(Command, CommandTable[Index].CommandName, BTPS_StringLength(CommandTable[Index].CommandName)) == 0)
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

   /* Displays a function error message.                                */
static void DisplayFunctionError(char *Function, int Status)
{
   Display(("Error - %s returned %d.\r\n", Function, Status));
}

   /* The following function displays the available command of the      */
   /* function HoldingMultipartyCall.                                   */
static void DisplayUsageHoldingMultipartyCall(void)
{
   Display(("\r\nHoldingMultipartyCal Usage: [Command] [Index - Optional] [Port ID]\r\n"));
   Display(("   [0 = Release All Held Calls]\r\n"));
   Display(("   [1 = Release All Active Calls Accept Waiting Call, \r\n          if no active call with Audio on second phone]\r\n"));
   Display(("   [2 = Place All Active Calls On Hold Accept The Other, \r\n          if no active call with Audio on second phone]\r\n"));
   Display(("   [3 = Add A Held Call To Conversation]\r\n"));
   Display(("   [4 = Connect Two Calls And Disconnect]\r\n"));
   Display(("   [5 = Release Specified Call Index], [Index]\r\n"));
   Display(("   [6 = Private Consultation Mode], [Index], \r\n          if no active call with Audio on second phone\r\n"));
}

   /* The following function check if the Status parameter include      */
   /* error result or not, and display it with the Function name.       */
static int DisplayFunctionChecker(char *Function, int Status)
{
   /* Now check to see if the command was submitted successfully.       */
   if(Status == 0)
   {
      /* The function was submitted successfully.                       */
      Display(("%s(): Function Successful.\r\n", Function));
   }
   else
   {
      /* There was an error submitting the function.                    */
      Display(("%s() Failure: %d.\r\n", Function, Status));
   }

   return(Status);
}

   /* The following function displays the status of the connection      */
   /* The only parameter for this function is the Port Index for the    */
   /* connection.                                                       */
static void DisplayConnectionStatus(int ServerPortIndex)
{
   Display(("ServerPortID          : %u.\r\n", HFServerPortDATA[ServerPortIndex].HFServerPortID));
   Display(("IsConnected           : %s.\r\n", (HFServerPortDATA[ServerPortIndex].IsConnected)?"TRUE":"FALSE"));
   Display(("IsCallSetupInProgress : %s.\r\n", (HFServerPortDATA[ServerPortIndex].IsCallSetupInProgress)?"TRUE":"FALSE"));
   Display(("IsCallOnHold          : %s.\r\n", (HFServerPortDATA[ServerPortIndex].IsCallOnHold)?"TRUE":"FALSE"));
   Display(("IsInActiveCall        : %s.\r\n", (HFServerPortDATA[ServerPortIndex].IsInActiveCall)?"TRUE":"FALSE"));
   Display(("IsInActiveSCO         : %s.\r\n", (HFServerPortDATA[ServerPortIndex].IsInActiveSCO)?"TRUE":"FALSE"));
   Display(("IsInActiveAudio       : %s.\r\n", (HFServerPortDATA[ServerPortIndex].IsInActiveAudio)?"TRUE":"FALSE"));
}

   /* The following function is responsible for redisplaying the Menu   */
   /* options to the user.  This function returns zero on successful    */
   /* execution or a negative value on all errors.                      */
static int DisplayHelp(ParameterList_t *TempParam)
{
   Display(("\r\n"));
   Display(("*******************************************************************\r\n"));
   Display(("* Command Options: Inquiry, DisplayInquiryList, Pair,             *\r\n"));
   Display(("*                  EndPairing, PINCodeResponse, PassKeyResponse,  *\r\n"));
   Display(("*                  UserConfirmationResponse,                      *\r\n"));
   Display(("*                  SetDiscoverabilityMode, SetConnectabilityMode, *\r\n"));
   Display(("*                  SetPairabilityMode,                            *\r\n"));
   Display(("*                  ChangeSimplePairingParameters,                 *\r\n"));
   Display(("*                  GetLocalAddress, GetLocalName, SetLocalName,   *\r\n"));
   Display(("*                  GetClassOfDevice, SetClassOfDevice,            *\r\n"));
   Display(("*                  GetRemoteName, OpenHFServer, CloseHFServer     *\r\n"));
   Display(("*                  ManageAudio, AnswerCall, HangUpCall,           *\r\n"));
   Display(("*                  PlaceCallOnHold, HoldingMultipartyCall         *\r\n"));
   Display(("*                  HoldingMultiPhonesCall, 2PhonesCallAudioSwitch,*\r\n"));
   Display(("*                  2PhonesAnswerCall, QueryActiveConnections,     *\r\n"));
   Display(("*                  Help                                           *\r\n"));
   Display(("*******************************************************************\r\n"));

   return(0);
}

   /* The following function is for displaying The FW Version by reading*/
   /* The Local version information form the FW.                        */
static void DisplayFWVersion (void)
{
   FW_Version FW_Version_Details;

   /* This function retrieves the Local Version Information of the FW.  */
   HCI_Read_Local_Version_Information(BluetoothStackID, &FW_Version_Details.StatusResult, &FW_Version_Details.HCI_VersionResult, &FW_Version_Details.HCI_RevisionResult, &FW_Version_Details.LMP_VersionResult, &FW_Version_Details.Manufacturer_NameResult, &FW_Version_Details.LMP_SubversionResult);

   if(!FW_Version_Details.StatusResult)
   {
      /* This function prints The project type from firmware, Bits 10 to*/
      /* 14 (5 bits) from LMP_SubversionResult parameter.               */
      Display(("Project Type  : %d \r\n", ((FW_Version_Details.LMP_SubversionResult >> 10)) & 0x1F));

      /* This function prints The version of the firmware.  The first   */
      /* number is the Major version, Bits 7 to 9 and 15 (4 bits) from  */
      /* LMP_SubversionResult parameter, the second number is the Minor */
      /* Version, Bits 0 to 6 (7 bits) from LMP_SubversionResult        */
      /* parameter.                                                     */
      Display(("FW Version    : %d.%d \r\n", ((FW_Version_Details.LMP_SubversionResult >> 7) & 0x07) + ((FW_Version_Details.LMP_SubversionResult >> 12) & 0x08), FW_Version_Details.LMP_SubversionResult & 0x7F));
   }
   else
   {
      /* There was an error with HCI_Read_Local_Version_Information.    */
      /* Function.                                                      */
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

   /* The following function is responsible for querying all portIDs    */
   /* that are in active connection. This function will return zero     */
   /* on successful execution and a negative value on errors.           */
static int QueryActiveConnections(ParameterList_t *TempParam)
{
   int       ret_val = 0;
   int       index;
   char      BluetoothAddress[16];
   Boolean_t Found_Active = 0;

   if((TempParam) && (TempParam->NumberofParameters == 1) && ((TempParam->Params[0].intParam == 0) || (TempParam->Params[0].intParam == 1)))
   {
      /* Searching in all available open ports                          */
      for(index = 0; index < MAXIMUM_SUPPORTED_HFRE_CONNECTIONS; index++)
      {
         /* Checking if this port is connected to a remote audio gateway*/
         if(HFServerPortDATA[index].IsConnected == TRUE)
         {
            /* if port is in use, display basic information             */
            Display(("\r\nPortID:  %u\r\n", HFServerPortDATA[index].HFServerPortID));

            BD_ADDRToStr(HFServerPortDATA[index].RemoteBD_ADDR, BluetoothAddress);
            Display(("BD_ADDR: %s\r\n", BluetoothAddress));

            /* A flag the stores if connected port was found            */
            Found_Active = 1;

            /* Checking if the user want detailed information           */
            if (TempParam->Params[0].intParam == 1)
            {
               DisplayConnectionStatus(index);
            }
         }
      }

      /* Check if the search results got an active connection           */
      if(Found_Active == 0)
      {
         Display(("No Active connections were found.\r\n"));
      }
   }
   else
   {
      Display(("QueryActiveConnections Usage: Detailed Data[1 = Enable , 0 = Disable] - Optional\r\n"));
   }

   return(ret_val);
}

   /* The following function is responsible for opening the SS1         */
   /* Bluetooth Protocol Stack.  This function accepts a pre-populated  */
   /* HCI Driver Information structure that contains the HCI Driver     */
   /* Transport Information.  This function returns zero on successful  */
   /* execution and a negative value on all errors.                     */
static int OpenStack(HCI_DriverInformation_t *HCI_DriverInformation, BTPS_Initialization_t *BTPS_Initialization)
{
   int                        Result;
   int                        ret_val;
   char                       BluetoothAddress[16];
   Byte_t                     Status;
   BD_ADDR_t                  BD_ADDR;
   HCI_Version_t              HCIVersion;
   Class_of_Device_t          Class_of_Device;
   L2CA_Link_Connect_Params_t L2CA_Link_Connect_Params;

   /* First check to see if the Stack has already been opened.          */
   if(!BluetoothStackID)
   {
      /* Next, makes sure that the Driver Information passed appears to */
      /* be semi-valid.                                                 */
      if((HCI_DriverInformation) && (BTPS_Initialization))
      {
         /* Initialize BTPSKNRL.                                        */
         BTPS_Init((void *)BTPS_Initialization);

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

            ret_val = 0;

            /* Attempt to enable the WBS feature.                       */
            Result = BSC_EnableFeature(BluetoothStackID, BSC_FEATURE_WIDE_BAND_SPEECH);
            if(!Result)
            {
               Display(("WBS Support initialized.\r\n"));
            }
            else
            {
               Display(("WBS Support not initialized %d.\r\n", Result));
            }

            /* Initialize the default Secure Simple Pairing parameters. */
            IOCapability   = DEFAULT_IO_CAPABILITY;
            OOBSupport     = FALSE;
            MITMProtection = DEFAULT_MITM_PROTECTION;

            if(!HCI_Version_Supported(BluetoothStackID, &HCIVersion))
               Display(("Device Chipset: %s\r\n", (HCIVersion <= NUM_SUPPORTED_HCI_VERSIONS)?HCIVersionStrings[HCIVersion]:HCIVersionStrings[NUM_SUPPORTED_HCI_VERSIONS]));

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

               Display(("LOCAL BD_ADDR: %s\r\n", BluetoothAddress));
            }

            /* Go ahead and allow Master/Slave Role Switch.             */
            L2CA_Link_Connect_Params.L2CA_Link_Connect_Request_Config  = cqAllowRoleSwitch;
            L2CA_Link_Connect_Params.L2CA_Link_Connect_Response_Config = csMaintainCurrentRole;

            L2CA_Set_Link_Connection_Configuration(BluetoothStackID, &L2CA_Link_Connect_Params);

            if(HCI_Command_Supported(BluetoothStackID, HCI_SUPPORTED_COMMAND_WRITE_DEFAULT_LINK_POLICY_BIT_NUMBER) > 0)
            {
               HCI_Write_Default_Link_Policy_Settings(BluetoothStackID, HCI_LINK_POLICY_SETTINGS_ENABLE_MASTER_SLAVE_SWITCH | HCI_LINK_POLICY_SETTINGS_ENABLE_SNIFF_MODE , &Status);
            }

            /* Delete all Stored Link Keys.                             */
            ASSIGN_BD_ADDR(BD_ADDR, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);

            DeleteLinkKey(BD_ADDR);

            /* Set the Class of Device.                                 */
            ASSIGN_CLASS_OF_DEVICE(Class_of_Device, 0x00, 0x00, 0x00);
            SET_MAJOR_DEVICE_CLASS(Class_of_Device, HCI_LMP_CLASS_OF_DEVICE_MAJOR_DEVICE_CLASS_AUDIO_VIDEO);
            SET_MINOR_DEVICE_CLASS(Class_of_Device, HCI_LMP_CLASS_OF_DEVICE_MINOR_DEVICE_CLASS_AUDIO_VIDEO_HANDS_FREE);

            /* Write out the Class of Device.                           */
            GAP_Set_Class_Of_Device(BluetoothStackID, Class_of_Device);

            /* Set the Local Device Name.                               */
            GAP_Set_Local_Device_Name(BluetoothStackID, APP_DEMO_NAME);
         }
         else
         {
            /* The Stack was NOT initialized successfully, inform the   */
            /* user and set the return value of the initialization      */
            /* function to an error.                                    */
            if(HCI_DriverInformation->DriverType == hdtUSB)
               Display(("Stack Initialization on USB Failed: %d.\r\n", Result));
            else
               Display(("Stack Initialization on Port %d %d (%s) Failed: %d.\r\n", HCI_DriverInformation->DriverInformation.COMMDriverInformation.COMPortNumber, HCI_DriverInformation->DriverInformation.COMMDriverInformation.BaudRate, ((HCI_DriverInformation->DriverInformation.COMMDriverInformation.Protocol == cpBCSP)?"BCSP":"UART"), Result));

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
   else
   {
      /* A valid Stack ID already exists, inform to user.               */
      Display(("Stack Already Initialized.\r\n"));

      ret_val = 0;
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
   int ret_val;

   /* First check to see if the Stack has been opened.                  */
   if(BluetoothStackID)
   {
      /* Simply close the Stack                                         */
      BSC_Shutdown(BluetoothStackID);

      Display(("Stack Shutdown Successfully.\r\n"));

      /* Flag that the Stack is no longer initialized.                  */
      BluetoothStackID = 0;

      /* Flag success to the caller.                                    */
      ret_val          = 0;
   }
   else
   {
      /* A valid Stack ID does not exist, inform to user.               */
      Display(("Stack not Initialized.\r\n"));

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
   int ret_val;

   /* First, check that a valid Bluetooth Stack ID exists.              */
   if(BluetoothStackID)
   {
      /* A semi-valid Bluetooth Stack ID exists, now attempt to set the */
      /* attached Devices Discoverability Mode to General.              */
      ret_val = GAP_Set_Discoverability_Mode(BluetoothStackID, dmGeneralDiscoverableMode, 0);

      /* Next, check the return value of the GAP Set Discoverability    */
      /* Mode command for successful execution.                         */
      if(!ret_val)
      {
         /* The command appears to have been successful.  The attached  */
         /* Device is now in General Discoverability Mode.              */
         Display(("GAP_Set_Discoverability_Mode(dmGeneralDiscoverable, 0).\r\n"));
      }
      else
      {
         /* An error occurred while trying to set the Discoverability   */
         /* Mode of the Device.                                         */
         Display(("Set Discoverable Mode Command Error : %d.\r\n", ret_val));
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
   int ret_val;

   /* First, check that a valid Bluetooth Stack ID exists.              */
   if(BluetoothStackID)
   {
      /* Attempt to set the attached Device to be Connectable.          */
      ret_val = GAP_Set_Connectability_Mode(BluetoothStackID, cmConnectableMode);

      /* Next, check the return value of the                            */
      /* GAP_Set_Connectability_Mode() function for successful          */
      /* execution.                                                     */
      if(!ret_val)
      {
         /* The command appears to have been successful.  The attached  */
         /* Device is now in Connectable Mode.                          */
         Display(("GAP_Set_Connectability_Mode(cmConnectable).\r\n"));
      }
      else
      {
         /* An error occurred while trying to make the Device           */
         /* Connectable.                                                */
         Display(("Set Connectability Mode Command Error : %d.\r\n", ret_val));
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
   int ret_val;

   /* First, check that a valid Bluetooth Stack ID exists.              */
   if(BluetoothStackID)
   {
      /* Attempt to set the attached device to be pairable.             */
      ret_val = GAP_Set_Pairability_Mode(BluetoothStackID, pmPairableMode);

      /* Next, check the return value of the GAP Set Pairability mode   */
      /* command for successful execution.                              */
      if(!ret_val)
      {
         /* The command appears to have been successful.  The attached  */
         /* device is now in pairable mode.                             */
         Display(("GAP_Set_Pairability_Mode(pmPairableMode).\r\n"));

         /* The device has been set to pairable mode, now register an   */
         /* Authentication Callback to handle the Authentication events */
         /* if required.                                                */
         ret_val = GAP_Register_Remote_Authentication(BluetoothStackID, GAP_Event_Callback, (unsigned long)0);

         /* Next, check the return value of the GAP Register Remote     */
         /* Authentication command for successful execution.            */
         if(!ret_val)
         {
            /* The command appears to have been successful.             */
            Display(("GAP_Register_Remote_Authentication() Success.\r\n"));
         }
         else
         {
            /* An error occurred while trying to execute this function. */
            Display(("GAP_Register_Remote_Authentication() Failure: %d\r\n", ret_val));
         }
      }
      else
      {
         /* An error occurred while trying to make the device pairable. */
         Display(("Set Pairability Mode Command Error : %d.\r\n", ret_val));
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
   BD_ADDR_t NULL_BD_ADDR;

   Result = HCI_Delete_Stored_Link_Key(BluetoothStackID, BD_ADDR, TRUE, &Status_Result, &Num_Keys_Deleted);
   if(Result)
      Display(("Deleting Stored Link Key(s) FAILED!\r\n"));

   /* Any stored link keys for the specified address (or all) have been */
   /* deleted from the chip.  Now, let's make sure that our stored Link */
   /* Key Array is in sync with these changes.                          */

   /* First check to see all Link Keys were deleted.                    */
   ASSIGN_BD_ADDR(NULL_BD_ADDR, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);

   if(COMPARE_BD_ADDR(BD_ADDR, NULL_BD_ADDR))
      BTPS_MemInitialize(LinkKeyInfo, 0, sizeof(LinkKeyInfo));
   else
   {
      /* Individual Link Key.  Go ahead and see if we know about the    */
      /* entry in the list.                                             */
      for(Result=0;(Result<sizeof(LinkKeyInfo)/sizeof(LinkKeyInfo_t));Result++)
      {
         if(COMPARE_BD_ADDR(BD_ADDR, LinkKeyInfo[Result].BD_ADDR))
         {
            LinkKeyInfo[Result].BD_ADDR = NULL_BD_ADDR;

            break;
         }
      }
   }

   return(Result);
}

   /* The following function is responsible for Setting up an Audio     */
   /* Connection.  This function returns zero on successful execution   */
   /* and a negative value on all errors.                               */
static int HFRESetupAudioConnection(unsigned int ServerPortInfoIndex)
{
   int ret_val;

   /* Submit the command.                                               */
   ret_val = HFRE_Setup_Audio_Connection(BluetoothStackID, HFServerPortDATA[ServerPortInfoIndex].HFServerPortID);

   return(DisplayFunctionChecker("HFRE_Setup_Audio_Connection", ret_val));
}

   /* The following function is responsible for releasing an existing   */
   /* audio connection.  This function returns zero on successful       */
   /* execution and a negative value on all errors.                     */
static int HFREReleaseAudioConnection(unsigned int ServerPortInfoIndex)
{
   int ret_val;

   /* The Port ID appears to be a semi-valid value.  Now submit the     */
   /* command.                                                          */
   ret_val = HFRE_Release_Audio_Connection(BluetoothStackID, HFServerPortDATA[ServerPortInfoIndex].HFServerPortID);

   /* Now check to see if the command was submitted successfully.       */
   if(!ret_val)
   {
      /* The function was submitted successfully.                       */
      Display(("HFRE_Release_Audio_Connection: Function Successful.\r\n"));

      /* Use the audio disconnection indication function to clean up the*/
      /* audio connection.                                              */
      AudioDisconnectionIndication(ServerPortInfoIndex);
   }
   else
   {
      /* There was an error submitting the function.                    */
      Display(("HFRE_Release_Audio_Connection() Failure: %d.\r\n", ret_val));
   }

   return(ret_val);
}

   /* The following function is responsible for performing a General    */
   /* Inquiry for discovering Bluetooth Devices.  This function requires*/
   /* that a valid Bluetooth Stack ID exists before running.  This      */
   /* function returns zero is successful or a negative value if there  */
   /* was an error.                                                     */
static int Inquiry(ParameterList_t *TempParam)
{
   int ret_val;

   /* First, check that valid Bluetooth Stack ID exists.                */
   if(BluetoothStackID)
   {
      /* Use the GAP_Perform_Inquiry() function to perform an Inquiry.  */
      /* The Inquiry will last the specified amount of time or until the*/
      /* specified number of Bluetooth Device are found.  When the      */
      /* Inquiry Results become available the GAP_Event_Callback is     */
      /* called.                                                        */
      ret_val = GAP_Perform_Inquiry(BluetoothStackID, itGeneralInquiry, 0, 0, 10, MAX_INQUIRY_RESULTS, GAP_Event_Callback, 0);

      /* Next, check to see if the GAP_Perform_Inquiry() function was   */
      /* successful.                                                    */
      if(!ret_val)
      {
         /* The Inquiry appears to have been sent successfully.         */
         /* Processing of the results returned from this command occurs */
         /* within the GAP_Event_Callback() function.                   */
         Display(("Return Value is %d GAP_Perform_Inquiry() SUCCESS.\r\n", ret_val));
         NumberofValidResponses = 0;
      }
      else
      {
         /* A error occurred while performing the Inquiry.              */
         Display(("Return Value is %d GAP_Perform_Inquiry() FAILURE.\r\n", ret_val));
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
   /* display the current Inquiry List (with Indexes).  This is useful  */
   /* in case the user has forgotten what Inquiry Index a particular    */
   /* Bluetooth Device was located in.  This function returns zero on   */
   /* successful execution and a negative value on all errors.          */
static int DisplayInquiryList(ParameterList_t *TempParam)
{
   int          ret_val;
   char         BoardStr[16];
   unsigned int Index;

   /* First, check that valid Bluetooth Stack ID exists.                */
   if(BluetoothStackID)
   {
      /* Simply display all of the items in the Inquiry List.           */
      Display(("Inquiry List: %d Devices%s\r\n\r\n", NumberofValidResponses, NumberofValidResponses?":":"."));

      for(Index=0;Index<NumberofValidResponses;Index++)
      {
         BD_ADDRToStr(InquiryResultList[Index], BoardStr);

         Display((" Inquiry Result: %d, %s.\r\n", (Index+1), BoardStr));
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
            Display(("Discoverability Mode successfully set to: %s Discoverable.\r\n", (DiscoverabilityMode == dmNonDiscoverableMode)?"Non":((DiscoverabilityMode == dmGeneralDiscoverableMode)?"General":"Limited")));

            /* Flag success to the caller.                              */
            ret_val = 0;
         }
         else
         {
            /* There was an error setting the Mode.                     */
            Display(("GAP_Set_Discoverability_Mode() Failure: %d.\r\n", Result));

            /* Flag that an error occurred while submitting the command.*/
            ret_val = FUNCTION_ERROR;
         }
      }
      else
      {
         /* One or more of the necessary parameters is/are invalid.     */
         Display(("Usage: SetDiscoverabilityMode [Mode (0 = Non Discoverable, 1 = Limited Discoverable, 2 = General Discoverable)].\r\n"));

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
            Display(("Connectability Mode successfully set to: %s.\r\n", (ConnectableMode == cmNonConnectableMode)?"Non Connectable":"Connectable"));

            /* Flag success to the caller.                              */
            ret_val = 0;
         }
         else
         {
            /* There was an error setting the Mode.                     */
            Display(("GAP_Set_Connectability_Mode() Failure: %d.\r\n", Result));

            /* Flag that an error occurred while submitting the command.*/
            ret_val = FUNCTION_ERROR;
         }
      }
      else
      {
         /* One or more of the necessary parameters is/are invalid.     */
         Display(("Usage: SetConnectabilityMode [Mode (0 = Non Connectable, 1 = Connectable)].\r\n"));

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
            PairabilityMode = pmNonPairableMode;
         else
         {
            if(TempParam->Params[0].intParam == 1)
               PairabilityMode = pmPairableMode;
            else
               PairabilityMode = pmPairableMode_EnableSecureSimplePairing;
         }

         /* Parameters mapped, now set the Pairability Mode.            */
         Result = GAP_Set_Pairability_Mode(BluetoothStackID, PairabilityMode);

         /* Next, check the return value to see if the command was      */
         /* issued successfully.                                        */
         if(Result >= 0)
         {
            /* The Mode was changed successfully.                       */
            Display(("Pairability Mode successfully set to: %s.\r\n", (PairabilityMode == pmNonPairableMode)?"Non Pairable":((PairabilityMode == pmPairableMode)?"Pairable":"Pairable (Secure Simple Pairing)")));

            /* If Secure Simple Pairing has been enabled, inform the    */
            /* user of the current Secure Simple Pairing parameters.    */
            if(PairabilityMode == pmPairableMode_EnableSecureSimplePairing)
               Display(("Current I/O Capabilities: %s, MITM Protection: %s.\r\n", IOCapabilitiesStrings[(unsigned int)IOCapability], MITMProtection?"TRUE":"FALSE"));

            /* Flag success to the caller.                              */
            ret_val = 0;
         }
         else
         {
            /* There was an error setting the Mode.                     */
            Display(("GAP_Set_Pairability_Mode() Failure: %d.\r\n", Result));

            /* Flag that an error occurred while submitting the command.*/
            ret_val = FUNCTION_ERROR;
         }
      }
      else
      {
         /* One or more of the necessary parameters is/are invalid.     */
         Display(("Usage: SetPairabilityMode [Mode (0 = Non Pairable, 1 = Pairable, 2 = Pairable (Secure Simple Pairing)].\r\n"));

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

   /* The following function is responsible for changing the Secure     */
   /* Simple Pairing Parameters that are exchanged during the Pairing   */
   /* procedure when Secure Simple Pairing (Security Level 4) is used.  */
   /* This function returns zero on successful execution and a negative */
   /* value on all errors.                                              */
static int ChangeSimplePairingParameters(ParameterList_t *TempParam)
{
   int ret_val;

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
         Display(("Current I/O Capabilities: %s, MITM Protection: %s.\r\n", IOCapabilitiesStrings[(unsigned int)IOCapability], MITMProtection?"TRUE":"FALSE"));

         /* Flag success to the caller.                                 */
         ret_val = 0;
      }
      else
      {
         /* One or more of the necessary parameters is/are invalid.     */
         Display(("Usage: ChangeSimplePairingParameters [I/O Capability (0 = Display Only, 1 = Display Yes/No, 2 = Keyboard Only, 3 = No Input/Output)] [MITM Requirement (0 = No, 1 = Yes)].\r\n"));

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

   /* The following function is responsible for initiating bonding with */
   /* a remote device.  This function returns zero on successful        */
   /* execution and a negative value on all errors.                     */
static int Pair(ParameterList_t *TempParam)
{
   int                Result;
   int                ret_val;
   BD_ADDR_t          NullADDR;
   GAP_Bonding_Type_t BondingType;

   ASSIGN_BD_ADDR(NullADDR, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);

   /* First, check that valid Bluetooth Stack ID exists.                */
   if(BluetoothStackID)
   {
     /* There is no currently active connection, make sure that all of  */
     /* the parameters required for this function appear to be at least */
     /* semi-valid.                                                     */
     if((TempParam) && (TempParam->NumberofParameters > 0) && (TempParam->Params[0].intParam) && (NumberofValidResponses) && (TempParam->Params[0].intParam <= NumberofValidResponses) && (!COMPARE_BD_ADDR(InquiryResultList[(TempParam->Params[0].intParam - 1)], NullADDR)))
     {
        /* Check to see if General Bonding was specified.               */
        if(TempParam->NumberofParameters > 1)
           BondingType = TempParam->Params[1].intParam?btGeneral:btDedicated;
        else
           BondingType = btDedicated;

        /* Before we submit the command to the stack, we need to make   */
        /* sure that we clear out any Link Key we have stored for the   */
        /* specified device.                                            */
        DeleteLinkKey(InquiryResultList[(TempParam->Params[0].intParam - 1)]);

        /* Attempt to submit the command.                               */
        Result = GAP_Initiate_Bonding(BluetoothStackID, InquiryResultList[(TempParam->Params[0].intParam - 1)], BondingType, GAP_Event_Callback, (unsigned long)0);

        /* Check the return value of the submitted command for success. */
        if(!Result)
        {
           /* Display a message indicating that Bonding was initiated   */
           /* successfully.                                             */
           Display(("GAP_Initiate_Bonding (%s): Function Successful.\r\n", (BondingType == btDedicated)?"Dedicated":"General"));

           /* Flag success to the caller.                               */
           ret_val = 0;
        }
        else
        {
           /* Display a message indicating that an error occurred while */
           /* initiating bonding.                                       */
           Display(("GAP_Initiate_Bonding() Failure: %d.\r\n", Result));

           ret_val = FUNCTION_ERROR;
        }
     }
     else
     {
        /* One or more of the necessary parameters is/are invalid.      */
        Display(("Usage: Pair [Inquiry Index] [Bonding Type (0 = Dedicated, 1 = General) (optional).\r\n"));

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
   BD_ADDR_t NullADDR;

   ASSIGN_BD_ADDR(NullADDR, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);

   /* First, check that valid Bluetooth Stack ID exists.                */
   if(BluetoothStackID)
   {
      /* Make sure that all of the parameters required for this function*/
      /* appear to be at least semi-valid.                              */
      if((TempParam) && (TempParam->NumberofParameters > 0) && (TempParam->Params[0].intParam) && (NumberofValidResponses) && (TempParam->Params[0].intParam <= NumberofValidResponses) && (!COMPARE_BD_ADDR(InquiryResultList[(TempParam->Params[0].intParam - 1)], NullADDR)))
      {
         /* Attempt to submit the command.                              */
         Result = GAP_End_Bonding(BluetoothStackID, InquiryResultList[(TempParam->Params[0].intParam - 1)]);

         /* Check the return value of the submitted command for success.*/
         if(!Result)
         {
            /* Display a message indicating that the End bonding was    */
            /* successfully submitted.                                  */
            Display(("GAP_End_Bonding: Function Successful.\r\n"));



            /* Flag that there is no longer a current Authentication    */
            /* procedure in progress.                                   */
            for (ret_val = 0; (ret_val < MAXIMUM_SUPPORTED_HFRE_CONNECTIONS); ret_val ++)
            {
                if (COMPARE_BD_ADDR(InquiryResultList[(TempParam->Params[0].intParam - 1)], HFServerPortDATA[ret_val].RemoteBD_ADDR))
                {
                    ASSIGN_BD_ADDR(HFServerPortDATA[ret_val].RemoteBD_ADDR, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
                    break;
                }
            }

            /* Flag success to the caller.                              */
            ret_val = 0;
         }
         else
         {
            /* Display a message indicating that an error occurred while*/
            /* ending bonding.                                          */
            Display(("GAP_End_Bonding() Failure: %d.\r\n", Result));

            ret_val = FUNCTION_ERROR;
         }
      }
      else
      {
         /* One or more of the necessary parameters is/are invalid.     */
         Display(("Usage: EndPairing [Inquiry Index].\r\n"));

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
   BD_ADDR_t                        NullADDR;
   PIN_Code_t                       PINCode;
   GAP_Authentication_Information_t GAP_Authentication_Information;

   ASSIGN_BD_ADDR(NullADDR, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);

   /* First, check that valid Bluetooth Stack ID exists.                */
   if(BluetoothStackID)
   {
      /* First, check to see if there is an on-going Pairing operation  */
      /* active.                                                        */
      if(!COMPARE_BD_ADDR(CurrentRemoteBD_ADDR, NullADDR))
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
            Display(("Usage: PINCodeResponse [PIN Code].\r\n"));

            ret_val = INVALID_PARAMETERS_ERROR;
         }
      }
      else
      {
         /* There is not currently an on-going authentication operation,*/
         /* inform the user of this error condition.                    */
         Display(("Unable to issue PIN Code Authentication Response: Authentication is not currently in progress.\r\n"));

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
   BD_ADDR_t                        NullADDR;
   GAP_Authentication_Information_t GAP_Authentication_Information;

   ASSIGN_BD_ADDR(NullADDR, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);

   /* First, check that valid Bluetooth Stack ID exists.                */
   if(BluetoothStackID)
   {
      /* First, check to see if there is an on-going Pairing operation  */
      /* active.                                                        */
      if(!COMPARE_BD_ADDR(CurrentRemoteBD_ADDR, NullADDR))
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
               Display(("GAP_Authentication_Response(), Passkey Response Success.\r\n"));

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
            Display(("Usage: PassKeyResponse [Numeric Passkey (0 - 999999)].\r\n"));

            ret_val = INVALID_PARAMETERS_ERROR;
         }
      }
      else
      {
         /* There is not currently an on-going authentication operation,*/
         /* inform the user of this error condition.                    */
         Display(("Unable to issue Pass Key Authentication Response: Authentication is not currently in progress.\r\n"));

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
   BD_ADDR_t                        NullADDR;
   GAP_Authentication_Information_t GAP_Authentication_Information;

   ASSIGN_BD_ADDR(NullADDR, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);

   /* First, check that valid Bluetooth Stack ID exists.                */
   if(BluetoothStackID)
   {
      /* First, check to see if there is an on-going Pairing operation  */
      /* active.                                                        */
      if(!COMPARE_BD_ADDR(CurrentRemoteBD_ADDR, NullADDR))
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
               Display(("GAP_Authentication_Response(), User Confirmation Response Success.\r\n"));

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
            Display(("Usage: UserConfirmationResponse [Confirmation (0 = No, 1 = Yes)].\r\n"));

            ret_val = INVALID_PARAMETERS_ERROR;
         }
      }
      else
      {
         /* There is not currently an on-going authentication operation,*/
         /* inform the user of this error condition.                    */
         Display(("Unable to issue User Confirmation Authentication Response: Authentication is not currently in progress.\r\n"));

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
   /* Device Address of the local Bluetooth Device.  This function      */
   /* returns zero on successful execution and a negative value on all  */
   /* errors.                                                           */
static int GetLocalAddress(ParameterList_t *TempParam)
{
   int       Result;
   int       ret_val;
   char      BoardStr[16];
   BD_ADDR_t BD_ADDR;

   /* First, check that valid Bluetooth Stack ID exists.                */
   if(BluetoothStackID)
   {
      /* Attempt to submit the command.                                 */
      Result = GAP_Query_Local_BD_ADDR(BluetoothStackID, &BD_ADDR);

      /* Check the return value of the submitted command for success.   */
      if(!Result)
      {
         BD_ADDRToStr(BD_ADDR, BoardStr);

         Display(("BD_ADDR of Local Device is: %s.\r\n", BoardStr));

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

   /* The following function is responsible for setting the name of the */
   /* local Bluetooth Device to a specified name.  This function returns*/
   /* zero on successful execution and a negative value on all errors.  */
static int SetLocalName(ParameterList_t *TempParam)
{
   int Result;
   int ret_val;

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
            Display(("Local Device Name set to: %s.\r\n", TempParam->Params[0].strParam));

            /* Flag success to the caller.                              */
            ret_val = 0;
         }
         else
         {
            /* Display a message indicating that an error occurred while*/
            /* attempting to set the local Device Name.                 */
            Display(("GAP_Set_Local_Device_Name() Failure: %d.\r\n", Result));

            ret_val = FUNCTION_ERROR;
         }
      }
      else
      {
         /* One or more of the necessary parameters is/are invalid.     */
         Display(("Usage: SetLocalName [Local Name (no spaces allowed)].\r\n"));

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
   char LocalName[257];

   /* First, check that valid Bluetooth Stack ID exists.                */
   if(BluetoothStackID)
   {
      /* Attempt to submit the command.                                 */
      Result = GAP_Query_Local_Device_Name(BluetoothStackID, sizeof(LocalName), (char *)LocalName);

      /* Check the return value of the submitted command for success.   */
      if(!Result)
      {
         Display(("Name of Local Device is: %s.\r\n", LocalName));

         /* Flag success to the caller.                                 */
         ret_val = 0;
      }
      else
      {
         /* Display a message indicating that an error occurred while   */
         /* attempting to query the Local Device Name.                  */
         Display(("GAP_Query_Local_Device_Name() Failure: %d.\r\n", Result));

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
            Display(("Set Class of Device to 0x%02X%02X%02X.\r\n", Class_of_Device.Class_of_Device0, Class_of_Device.Class_of_Device1, Class_of_Device.Class_of_Device2));

            /* Flag success to the caller.                              */
            ret_val = 0;
         }
         else
         {
            /* Display a message indicating that an error occurred while*/
            /* attempting to set the local Class of Device.             */
            Display(("GAP_Set_Class_Of_Device() Failure: %d.\r\n", Result));

            ret_val = FUNCTION_ERROR;
         }
      }
      else
      {
         /* One or more of the necessary parameters is/are invalid.     */
         Display(("Usage: SetClassOfDevice [Class of Device].\r\n"));

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

   /* The following function is responsible for querying the Bluetooth  */
   /* Device Name of the specified remote Bluetooth Device.  This       */
   /* function returns zero on successful execution and a negative value*/
   /* on all errors.                                                    */
static int GetRemoteName(ParameterList_t *TempParam)
{
   int       Result;
   int       ret_val;
   BD_ADDR_t NullADDR;

   ASSIGN_BD_ADDR(NullADDR, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);

   /* First, check that valid Bluetooth Stack ID exists.                */
   if(BluetoothStackID)
   {
      /* Make sure that all of the parameters required for this function*/
      /* appear to be at least semi-valid.                              */
      if((TempParam) && (TempParam->NumberofParameters > 0) && (TempParam->Params[0].intParam) && (NumberofValidResponses) && (TempParam->Params[0].intParam <= NumberofValidResponses) && (!COMPARE_BD_ADDR(InquiryResultList[(TempParam->Params[0].intParam - 1)], NullADDR)))
      {
         /* Attempt to submit the command.                              */
         Result = GAP_Query_Remote_Device_Name(BluetoothStackID, InquiryResultList[(TempParam->Params[0].intParam - 1)], GAP_Event_Callback, (unsigned long)0);

         /* Check the return value of the submitted command for success.*/
         if(!Result)
         {
            /* Display a message indicating that Remote Name request was*/
            /* initiated successfully.                                  */
            Display(("GAP_Query_Remote_Device_Name: Function Successful.\r\n"));

            /* Flag success to the caller.                              */
            ret_val = 0;
         }
         else
         {
            /* Display a message indicating that an error occurred while*/
            /* initiating the Remote Name request.                      */
            Display(("GAP_Query_Remote_Device_Name() Failure: %d.\r\n", Result));

            ret_val = FUNCTION_ERROR;
         }
      }
      else
      {
         /* One or more of the necessary parameters is/are invalid.     */
         Display(("Usage: GetRemoteName [Inquiry Index].\r\n"));

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

   /* The following function is responsible for searching for a specific*/
   /* open port. This function returns the index of the opened port if  */
   /* successful, or a negative return value if an error occurred.      */
static int FindHFServerPortEntry(unsigned int HFServerPort)
{
   int ret_val;

   /* Searching in all available open ports                             */
   for(ret_val = 0; (ret_val < MAXIMUM_SUPPORTED_HFRE_CONNECTIONS); ret_val++)
   {
      /* Check and return if the PortID was found                       */
      if(HFServerPort == HFServerPortDATA[ret_val].HFServerPortID)
      {
         return(ret_val);
      }
   }

   return(INVALID_PARAMETERS_ERROR);
}

   /* The following function is responsible for searching the first open*/
   /* port that is in active or hold connection. This function returns  */
   /* index of  the active connection, or a negative return value if an */
   /* error occurred.                                                   */
static int FindHFActivePortEntry(void)
{
   int ret_val;
   int Index;
   int MoreThanOneConnection = 0;

   /* Searching in all available open ports                             */
   for(Index = 0; (Index < MAXIMUM_SUPPORTED_HFRE_CONNECTIONS); Index++)
   {
      /* Check and return if the PortID exists                          */
      if(HFServerPortDATA[Index].HFServerPortID)
      {
         /* Check and return the index of the active call connection    */
         if(HFServerPortDATA[Index].IsInActiveCall == TRUE)
         {
            return(Index);
         }
         else
         {
            /* Check if there is call on hold or call waiting and sum   */
            /* the amount of this connections                           */
            if((HFServerPortDATA[Index].IsConnected == TRUE) && ((HFServerPortDATA[Index].IsCallOnHold == TRUE) || (HFServerPortDATA[Index].IsCallSetupInProgress == TRUE)))
            {
               ret_val = Index;

               MoreThanOneConnection++;
            }
         }
      }
   }

   /* Check if there is only one connection in on hold call or waiting  */
   /* call and return its index                                         */
   if(MoreThanOneConnection == 1)
   {
      return (ret_val);
   }
   else
   {
      return(INVALID_PARAMETERS_ERROR);
   }
}

   /* The following function is responsible for disabling WBS and       */
   /* sending all the available codecs to all connected connections the */
   /* function return 0 for success, or a negative return value if an   */
   /* error occurred.                                                   */
static int DisableWBS(int ServerPortIndex)
{
   int          Result;
   unsigned int Index;

   /* Check if the active WBS port id is the same as the port id of the */
   /* received index                                                    */
   if (WBS_Connected_Port_ID == HFServerPortDATA[ServerPortIndex].HFServerPortID)
   {
      if((Result = VS_WBS_Disassociate(BluetoothStackID)) < 0)
      {
         DisplayFunctionError("VS_WBS_Disassociate()", Result);
      }
      else
      {
         /* After DisableWBS was successful, clear the active WBS port  */
         /* id and send the available codecs to all connected devices   */
         WBS_Connected_Port_ID = 0;

         /* We have closed our WBS connection, now send our supported   */
         /* codecs to all connected remote devices in order to notify   */
         /* them the WBS codec is now available.                        */
         for(Index = 0; Index < MAXIMUM_SUPPORTED_HFRE_CONNECTIONS; Index++)
         {
            SendAvailableCodecs(Index);
         }
      }
   }
   else
   {
      Display(("WBS isn't enabled on port id: %u.\r\n",HFServerPortDATA[ServerPortIndex].HFServerPortID));
   }

   return (Result);
}

   /* The following sends the available codecs to the remote device at  */
   /* the specified index.                                              */
static void SendAvailableCodecs(unsigned int Index)
{
   int           Result;
   unsigned char AvailableCodecs[2];
   unsigned int  NumAvailableCodecs;

   if((HFServerPortDATA[Index].IsConnected) && (HFServerPortDATA[Index].RemoteSupportedFeaturesValid) && (HFServerPortDATA[Index].RemoteSupportedFeatures & HFRE_AG_CODEC_NEGOTIATION_SUPPORTED_BIT))
   {
      AvailableCodecs[0] = HFRE_CVSD_CODEC_ID;
      NumAvailableCodecs = 1;

      /* Check if there is an active WBS connection.                    */
      if(WBS_Connected_Port_ID == 0)
      {
         /* There is not an active WBS connection, include the WBS codec*/
         /* in our available codecs.                                    */
         AvailableCodecs[NumAvailableCodecs] = HFRE_MSBC_CODEC_ID;
         NumAvailableCodecs += 1;
      }

      Result = HFRE_Send_Available_Codecs(BluetoothStackID, HFServerPortDATA[Index].HFServerPortID, NumAvailableCodecs, AvailableCodecs);

      if(Result < 0)
         DisplayFunctionError("HFRE_Send_Available_Codecs", Result);
   }
}

   /* The following functions returns the number of answered calls, i.e.*/
   /* the number of active calls plus the number of calls on hold.      */
static unsigned int NumberOfAnsweredCalls(void)
{
   unsigned int ret_val;
   unsigned int Index;

   /* Initialize our return value to zero answered calls.               */
   ret_val = 0;

   /* Loop through our server ports and count the number of answered    */
   /* calls.                                                            */
   for(Index = 0; Index < MAXIMUM_SUPPORTED_HFRE_CONNECTIONS; Index++)
   {
      /* Check if this server port is connected.                        */
      if(HFServerPortDATA[Index].IsConnected)
      {
         /* This server port is connected, check if there is an active  */
         /* call.                                                       */
         if(HFServerPortDATA[Index].IsInActiveCall)
         {
            /* There is an active call, increment the number of answered*/
            /* calls return variable.                                   */
            ret_val++;
         }

         /* This server port is connected, check if there is a call on  */
         /* hold.                                                       */
         if(HFServerPortDATA[Index].IsCallOnHold)
         {
            /* There is a call on hold, increment the number of answered*/
            /* calls return variable.                                   */
            ret_val++;
         }
      }
   }

   return(ret_val);
}

   /* The following function is responsible for opening a Serial Port   */
   /* Server on the Local Device. Each time this function is called it  */
   /* opens the same Serial Port Server on several RFCOMM Channels.     */
   /* This function returns zero if successful, or a negative return    */
   /* value if an error occurred.                                       */
static int OpenHFServer(ParameterList_t *TempParam)
{
   int  ret_val = 0;
   int  Result;
   char ServiceName[20];
   int  EmptyArraySlot;
   int  PortNumber;

   /* First check to see if a valid Bluetooth Stack ID exists.          */
   if(BluetoothStackID)
   {
     /* Checks if the amount of active RFCOMM is lower than the maximum */
     /* defined                                                         */
     if (NumberofactiveRFCOMM < MAXIMUM_SUPPORTED_HFRE_CONNECTIONS)
     {
        /* Now try and open an Audio Gateway Server Port.               */
        PortNumber = HFRE_Open_HandsFree_Server_Port(BluetoothStackID, HFP_SERVER_PORT, HFRE_SUPPORTED_FEATURES, 0, NULL, HFRE_Event_Callback, (unsigned long)0);

        Display(("Port Number: %d\r\n",PortNumber));

        /* Check to see if the call was executed successfully.          */
        if(PortNumber > 0)
        {
           /* The Server was successfully opened.  Save the returned    */
           /* result as the Current Server Port ID because it will be   */
           /* used by later function calls.                             */
           EmptyArraySlot = FindHFServerPortEntry(FIRST_EMPTY_SLOT);

           if(EmptyArraySlot >= 0)
           {
               HFServerPortDATA[EmptyArraySlot].HFServerPortID = (unsigned int)PortNumber;

               /* After successful opening of RFCOMM increase the       */
               /* counter of active RFCOMM                              */
               NumberofactiveRFCOMM++;
               Display(("HFRE_Open_HandsFree_Server_Port: Function Successful.\r\n"));

               /* Check if a SDP record is already exists, if not make  */
               /* one                                                   */
               if (HFServerSDPHandle == 0)
               {
                   /* The Server was opened successfully, now register a*/
                   /* SDP Record indicating that an Audio Gateway Server*/
                   /* exists.  Do this by first creating a Service Name.*/
                   BTPS_SprintF(ServiceName, "HandsFree");

                   /* Now that a Service Name has been created try and  */
                   /* Register the SDP Record.                          */
                   Result = HFRE_Register_HandsFree_SDP_Record(BluetoothStackID, HFServerPortDATA[EmptyArraySlot].HFServerPortID, ServiceName, &HFServerSDPHandle);

                   /* Check the result of the above function call for   */
                   /* success.                                          */
                   if(!Result)
                   {
                      /* Display a message indicating that the SDP      */
                      /* Record for the Audio Gateway Server was        */
                      /* registered successfully.                       */
                      Display(("HFRE_Register_HandsFree_SDP_Record: Function Successful.\r\n"));

                      ret_val = Result;
                   }
                   else
                   {
                      /* Display an error message and make sure the     */
                      /* Audio Gateway Server SDP Handle is invalid, and*/
                      /* close the Audio Gateway Server Port we just    */
                      /* opened because we weren't able to register a   */
                      /* SDP Record.                                    */
                      Display(("HFRE_Register_HandsFree_SDP_Record: Function Failure. %d\r\n", Result));

                      HFServerSDPHandle = 0;

                      ret_val = Result;

                      /* Now try and close the opened Port.             */
                      Result = HFRE_Close_Server_Port(BluetoothStackID, HFServerPortDATA[EmptyArraySlot].HFServerPortID);

                      HFServerPortDATA[EmptyArraySlot].HFServerPortID = 0;

                      /* Next check the return value of the issued      */
                      /* command see if it was successful.              */
                      ret_val = DisplayFunctionChecker("HFRE_Close_Server_Port", Result);
                   }
               }
           }
           else
           {
                Display(("Error: All available connection ports are opened.\r\n"));

                ret_val = EmptyArraySlot;
           }
        }
        else
        {
           Display(("Unable to open server on: %d, Error = %d.\r\n", PortNumber, ret_val));

           ret_val = UNABLE_TO_REGISTER_SERVER;
        }
     }
     else
     {
         Display(("Error: Unable to open port server, Maximum open ports has been reached.\r\n"));

         ret_val = MAXIMUM_HFRE_SUPPORTED_CONNECTIONS_REACHED;
     }
   }
   else
   {
      /* No valid Bluetooth Stack ID exists.                            */
      ret_val = INVALID_STACK_ID_ERROR;
   }

   return(ret_val);
}

   /* The following function is responsible for closing a specific      */
   /* Serial Port Server that was previously opened via a successful    */
   /* call to the OpenServer() function. only when all ports are closed,*/
   /* this function will delete the SDP record This function returns    */
   /* zero if successful or a negative return error code if there was   */
   /* an error.                                                         */
static int CloseHFServer(ParameterList_t *TempParam)
{
   int     ret_val = 0;
   int     ServerPortIndex;

   /* First check to see if a valid Bluetooth Stack ID exists.          */
   if(BluetoothStackID)
   {

      /* Make sure that all of the parameters required for this function*/
      /* appear to be at least semi-valid.                              */
      if((TempParam) && (TempParam->NumberofParameters > 0) && (TempParam->Params[0].intParam))
      {

          /* If a Serial Port Server is already opened, then simply     */
          /* close it.                                                  */
          if((ServerPortIndex = FindHFServerPortEntry((unsigned int)(TempParam->Params[0].intParam))) >= 0)
          {
             /* If there is an SDP Service Record associated with the   */
             /* Serial Port Server and it is the last RFCOMM port opened*/
             /* then we need to remove it from the SDP Database.        */
             if(NumberofactiveRFCOMM == 1)
             {
                HFRE_Un_Register_SDP_Record(BluetoothStackID, SerialPortID, HFServerSDPHandle);

                /* Flag that there is no longer an SDP Serial Port      */
                /* Server Record.                                       */
                HFServerSDPHandle = 0;
             }

             /* Finally close the Serial Port Server.                   */
             ret_val = HFRE_Close_Server_Port(BluetoothStackID, HFServerPortDATA[ServerPortIndex].HFServerPortID);

             if(ret_val < 0)
             {
                Display(("HFP_Close_Server_Port failed with %d\r\n", ret_val));

                ret_val = FUNCTION_ERROR;
             }
             else
             {
                NumberofactiveRFCOMM--;
                /* Flag that the HF server port is no longer open and   */
                /* clean all its parameters.                            */
                BTPS_MemInitialize(HFServerPortDATA, 0, sizeof(HFServerPortDATA));
                ret_val                                             = 0;

                Display(("Server Closed.\r\n"));
             }
          }
          else
          {
             Display(("NO Server open.\r\n"));

             ret_val = INVALID_PARAMETERS_ERROR;
          }
      }
      else
      {
         Display(("Usage: CloseHFServer [Port Number]\r\n"));
      }
   }
   else
   {
      /* No valid Bluetooth Stack ID exists.                            */
      ret_val = INVALID_STACK_ID_ERROR;
   }

   return(ret_val);
}

   /* The following function is responsible for setting up or releasing */
   /* an audio connection.  This function returns zero on successful    */
   /* execution and a negative value on all errors.                     */
static int ManageAudioConnection(ParameterList_t *TempParam)
{
   int ret_val;
   int ServerPortIndex;

   /* First check to see if a valid Bluetooth Stack ID exists.          */
   if(BluetoothStackID)
   {
      ServerPortIndex = -1;

      if(TempParam->NumberofParameters == 2)
      {
         ServerPortIndex = FindHFServerPortEntry((unsigned int)(TempParam->Params[1].intParam));
      }

      if(ServerPortIndex >= 0)
      {
         /* Check to see if this is a request to setup an audio         */
         /* connection or disconnect an audio connection.               */
         if(TempParam->Params[0].intParam)
         {
            /* This is a request to setup an audio connection, call the */
            /* Setup Audio Connection function.                         */
            ret_val = HFRESetupAudioConnection(ServerPortIndex);

            HFServerPortDATA[ServerPortIndex].IsInActiveSCO = TRUE;
         }
         else
         {
            /* This is a request to disconnect an audio connection, call*/
            /* the Release Audio Connection function.                   */
            ret_val = HFREReleaseAudioConnection(ServerPortIndex);
         }
      }
      else
      {
         /* A required parameter is invalid.                            */
         Display(("Usage: ManageAudio [Release = 0, Setup = 1] [PortID Number]\r\n"));

         ret_val = INVALID_PARAMETERS_ERROR;
      }
   }
   else
   {
      /* No valid Bluetooth Stack ID exists.                            */
      Display(("Stack ID Invalid.\r\n"));

      ret_val = INVALID_STACK_ID_ERROR;
   }

   return(ret_val);
}

   /* The following function is responsible for sending the command to  */
   /* Answer an Incoming Call on the Remote Audio Gateway, This function*/
   /* Can answer without any parameter if the HF device has only one    */
   /* connection available, if not it needs the port ID parameter. This */
   /* function returns zero on successful execution and a negative value*/
   /* on all errors.                                                    */
static int AnswerIncomingCall(ParameterList_t *TempParam)
{
   int     Result;
   int     ret_val;
   int     ServerPortIndex;

   /* First, check that valid Bluetooth Stack ID exists.                */
   if(BluetoothStackID)
   {
      /* Now check to make sure that the Port ID appears to be          */
      /* semi-valid.                                                    */
      if(((NumberOfCurrentConnections == 1)  && (TempParam->NumberofParameters == 0 )) || ((TempParam) && (TempParam->NumberofParameters == 1 ) && (ServerPortIndex = FindHFServerPortEntry((unsigned int)(TempParam->Params[0].intParam))) >= 0))
      {
         /* If number of connections is one we do not need any input    */
         /* from the user, we only need the entry index.                */
         if ((NumberOfCurrentConnections == 1) && (TempParam->NumberofParameters == 0 ))
         {
            ServerPortIndex = FindHFActivePortEntry();
         }
         /* The Port ID appears to be a semi-valid value.  Now submit   */
         /* the command.                                                */
         Result  = HFRE_Answer_Incoming_Call(BluetoothStackID, HFServerPortDATA[ServerPortIndex].HFServerPortID);

         /* Set the return value of this function equal to the Result of*/
         /* the function call.                                          */
         /* Now check to see if the command was submitted successfully. */
         ret_val = DisplayFunctionChecker("HFRE_Answer_Incoming_Call", Result);

      }
      else
      {
         /* One or more of the parameters are invalid.                  */
         Display(("Answer Incoming Call Usage: [Port ID].\r\n"));

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


   /* The following function is responsible for sending the command to  */
   /* HangUp ongoing calls or reject incoming calls on the Remote Audio */
   /* Gateway, This function Can answer without any parameter if the HF */
   /* device has only one connection available, if not it needs the port*/
   /* ID parameter.  This function returns zero on successful execution */
   /* and a negative value on all errors.                               */
static int HangUpCall(ParameterList_t *TempParam)
{
   int     Result;
   int     ret_val;
   int     ServerPortIndex;
   /* First, check that valid Bluetooth Stack ID exists.                */
   if(BluetoothStackID)
   {
      /* Now check to make sure that the Port ID appears to be          */
      /* semi-valid.                                                    */
      if((NumberOfCurrentConnections == 1) || ((TempParam) && (TempParam->NumberofParameters == 1 )
        && (ServerPortIndex = FindHFServerPortEntry((unsigned int)(TempParam->Params[0].intParam))) >= 0))
      {
         /* If number of connections is one we do not need any input    */
         /* from the user, we only need the entry index.                */
         if (NumberOfCurrentConnections == 1)
         {
            ServerPortIndex = FindHFActivePortEntry();
         }
         /* The Port ID appears to be a semi-valid value.  Now submit   */
         /* the command.                                                */
         Result  = HFRE_Hang_Up_Call(BluetoothStackID, HFServerPortDATA[ServerPortIndex].HFServerPortID);

         /* Set the return value of this function equal to the Result of*/
         /* the function call.                                          */
         /* Now check to see if the command was submitted successfully. */
         ret_val = DisplayFunctionChecker("HFRE_Hang_Up_Call", Result);
      }
      else
      {
         /* One or more of the parameters are invalid.                  */
         Display(("Hang Up Call Usage: [Port ID].\r\n"));

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

      if((SamplingFrequency == WBS_FREQUENCY) || (SamplingFrequency == CVSD_FREQUENCY))
      {
         HAL_EnableAudioCodec(BluetoothStackID, aucLoopbackTest, SamplingFrequency, 1);

         VS_Set_PCM_Loopback(BluetoothStackID, TRUE);

         Display(("Status: Audio initialized.\r\n"));

         ret_val = 0;
      }
      else
      {
         Display(("Error: No valid PCM sampling frequency (%d or %d) given.\r\n", WBS_FREQUENCY, CVSD_FREQUENCY));

         Display(("Usage: PCMLoopback [SamplingFrequency]"));
      }
   }
   else
   {
      Display(("Error: No PCM sampling frequency given.\r\n"));

      Display(("Usage: PCMLoopback [SamplingFrequency]"));
   }

   return(ret_val);
}

   /* The following function is responsible for formatting the user     */
   /* input into an ENUM parameter.  This function returns zero on      */
   /* successful execution and a negative value on all errors.          */
static int CallHoldMultipartyHandlingIntToENUMSwitch (SDWord_t UserInput, HFRE_Call_Hold_Multiparty_Handling_Type_t *CallHoldMultipartyHandling)
{
    int ret_val = 0;
    switch (UserInput)
    {
         case 0:
                 *CallHoldMultipartyHandling = chReleaseAllHeldCalls;
                 break;
         case 1:
                 *CallHoldMultipartyHandling = chReleaseAllActiveCallsAcceptWaitingCall;
                 break;
         case 2:
                 *CallHoldMultipartyHandling = chPlaceAllActiveCallsOnHoldAcceptTheOther;
                 break;
         case 3:
                 *CallHoldMultipartyHandling = chAddAHeldCallToConversation;
                 break;
         case 4:
                 *CallHoldMultipartyHandling = chConnectTwoCallsAndDisconnect;
                 break;
         case 5:
                 *CallHoldMultipartyHandling = chReleaseSpecifiedCallIndex;
                 break;
         case 6:
                 *CallHoldMultipartyHandling = chPrivateConsultationMode;
                 break;
         default:
                 ret_val = INVALID_PARAMETERS_ERROR;
                 break;
    }
    return (ret_val);
}


   /* The following function is responsible for searching an index of a */
   /* port entry according to the user input. when using only one       */
   /* connection, the user doesn't need to enter port ID parameter.     */
   /* This function returns zero on successful execution and a negative */
   /* value on all errors.                                              */
static int SearchForServerPortIndexByUserInputs(ParameterList_t *TempParam, int *ServerPortIndex)
{
    int ret_val = 0;

    /* Now check to make sure that the Port ID appears to be semi-valid.*/
    if (TempParam)
    {
        if((TempParam->Params[0].intParam >= 0) || (TempParam->Params[0].intParam <= 4))
        {
            if((NumberOfCurrentConnections == 1) && (TempParam->NumberofParameters == 1))
            {
                *ServerPortIndex = FindHFActivePortEntry();
            }
            else if (TempParam->NumberofParameters == 2)
            {
                *ServerPortIndex = FindHFServerPortEntry((unsigned int)(TempParam->Params[1].intParam));
            }
            else
            {
                ret_val = INVALID_PARAMETERS_ERROR;
            }
        }
        else if (((TempParam->Params[0].intParam >= 5) || (TempParam->Params[0].intParam <= 6))
          && (TempParam->Params[1].intParam))
        {
            if((NumberOfCurrentConnections == 1) && (TempParam->NumberofParameters == 2))
            {
                *ServerPortIndex = FindHFActivePortEntry();
            }
            else if (TempParam->NumberofParameters == 3)
            {
                *ServerPortIndex = FindHFServerPortEntry((unsigned int)(TempParam->Params[2].intParam));
            }
            else
            {
                ret_val = INVALID_PARAMETERS_ERROR;
            }
        }
        else
        {
            ret_val = INVALID_PARAMETERS_ERROR;
        }
    }
    else
    {
        ret_val = INVALID_PARAMETERS_ERROR;
    }
    return (ret_val);
}


   /* Utility function for HFRE_Open_Service_Level_Connection_Indication*/
   /* event in HFRE_Event_Callback.                                     */
static void OpenServiceLevelConnectionIndication(HFRE_Event_Data_t *HFREEventData, int *ServerPortIndex)
{
   Byte_t StatusResult;

   /* Check if Caller ID is supported by the remote device.             */
   if (HFREEventData->Event_Data.HFRE_Open_Service_Level_Connection_Indication_Data->RemoteSupportedFeatures & HFRE_AG_ENHANCED_CALL_STATUS_SUPPORTED_BIT)
   {
      /* Enabled Caller ID information.                                 */
      HFRE_Enable_Remote_Call_Line_Identification_Notification(BluetoothStackID, HFREEventData->Event_Data.HFRE_Open_Service_Level_Connection_Indication_Data->HFREPortID, TRUE);
      Display(("HFRE Enable Call Line Identification\r\n"));
   }

   /* Check if three way calling is supported by the remote device.     */
   if (HFREEventData->Event_Data.HFRE_Open_Service_Level_Connection_Indication_Data->RemoteSupportedFeatures & HFRE_AG_ENHANCED_CALL_STATUS_SUPPORTED_BIT)
   {
      /* Enable Call waiting notification                               */
      HFRE_Enable_Remote_Call_Waiting_Notification(BluetoothStackID, HFREEventData->Event_Data.HFRE_Open_Service_Level_Connection_Indication_Data->HFREPortID, TRUE);
      Display(("HFRE Enable Remote Call Waiting Notification\r\n"));
   }

   HFServerPortDATA[*ServerPortIndex].IsConnected = TRUE;

   /* Save the supported features information.                          */
   HFServerPortDATA[*ServerPortIndex].RemoteSupportedFeaturesValid = HFREEventData->Event_Data.HFRE_Open_Service_Level_Connection_Indication_Data->RemoteSupportedFeaturesValid;
   HFServerPortDATA[*ServerPortIndex].RemoteSupportedFeatures      = HFREEventData->Event_Data.HFRE_Open_Service_Level_Connection_Indication_Data->RemoteSupportedFeatures;

   /* Try to be the Master of the link if possible.                     */
   HCI_Switch_Role(BluetoothStackID, HFServerPortDATA[*ServerPortIndex].RemoteBD_ADDR, 0, &StatusResult);
   Display(("StatusResult %d\r\n",StatusResult));
   DisplayConnectionStatus(*ServerPortIndex);

   /* Send our available codecs to this device.                         */
   SendAvailableCodecs(*ServerPortIndex);
}

   /* Utility function for HFRE_Control_Indicator_Status_Indication     */
   /* event in HFRE_Event_Callback. This function will run when the     */
   /* event parameter is "CALL".                                        */
static void ControlIndicatorStatusIndicationCallParameter(HFRE_Event_Data_t *HFREEventData, int *ServerPortIndex, int *Result)
{
   /* When CurrentIndicatorValue is false, that means that the call     */
   /* (Waiting Call/Call On Hold/active call) was disconnected.         */
   if(HFREEventData->Event_Data.HFRE_Control_Indicator_Status_Indication_Data->HFREControlIndicatorEntry.Control_Indicator_Data.ControlIndicatorBooleanType.CurrentIndicatorValue == FALSE)
   {
      /* Flag that we are no longer in an active call or holding call.  */
      /* Note that the specification doesn't clearly differentiate      */
      /* between a held or active call, a held call is also an active   */
      /* call, so if we receive this event there is neither an active   */
      /* (from the point of view of this app), or a held call.          */
      HFServerPortDATA[*ServerPortIndex].IsInActiveCall = FALSE;
      HFServerPortDATA[*ServerPortIndex].IsCallOnHold = FALSE;

      /* Checks if there are no answered calls but still a waiting call */
      /* so the system will automatically answer the waiting call.      */
      if(NumberOfAnsweredCalls() == 0)
      {
         if(HFServerPortDATA[*ServerPortIndex].IsCallSetupInProgress)
         {
            /* Set the return value of this function equal to the Result*/
            /* of the function call.                                    */
            *Result = HFRE_Answer_Incoming_Call(BluetoothStackID, HFREEventData->Event_Data.HFRE_Control_Indicator_Status_Indication_Data->HFREPortID);
         }
      }
   }
   /* When CurrentIndicatorValue is true, that means that the call was  */
   /* connected.                                                        */
   else
   {
      HFServerPortDATA[*ServerPortIndex].IsInActiveCall = TRUE;
   }
}

   /* Utility function for HFRE_Control_Indicator_Status_Indication     */
   /* event in HFRE_Event_Callback.  This function will run when the    */
   /* event parameter is "CALLSETUP".                                   */
static void ControlIndicatorStatusIndicationCallSetupParameter(HFRE_Event_Data_t *HFREEventData, int *ServerPortIndex)
{
   /* CALLSETUP = 0 - not currently in call set up, means that the call */
   /* was hung up.                                                      */
   if(HFREEventData->Event_Data.HFRE_Control_Indicator_Status_Indication_Data->HFREControlIndicatorEntry.Control_Indicator_Data.ControlIndicatorRangeType.CurrentIndicatorValue == 0)
   {
      HFServerPortDATA[*ServerPortIndex].IsCallSetupInProgress = FALSE;
   }
   /* CALLSETUP > 0: call setup procedure is ongoing.                   */
   else
   {
      HFServerPortDATA[*ServerPortIndex].IsCallSetupInProgress = TRUE;
   }
}

   /* Utility function for HFRE_Control_Indicator_Status_Indication     */
   /* event in HFRE_Event_Callback.  This function will run when the    */
   /* event parameter is "CALLHELD".                                    */
static void ControlIndicatorStatusIndicationCallHeldParameter(HFRE_Event_Data_t *HFREEventData, int *ServerPortIndex)
{
   /* Check the event parameter CALLHELD to act accordingly.            */
   switch(HFREEventData->Event_Data.HFRE_Control_Indicator_Status_Indication_Data->HFREControlIndicatorEntry.Control_Indicator_Data.ControlIndicatorRangeType.CurrentIndicatorValue)
   {
      case 0:
         HFServerPortDATA[*ServerPortIndex].IsInActiveCall = TRUE;
         HFServerPortDATA[*ServerPortIndex].IsCallOnHold = FALSE;
         break;
      case 1:
         HFServerPortDATA[*ServerPortIndex].IsInActiveCall = TRUE;
         HFServerPortDATA[*ServerPortIndex].IsCallOnHold = TRUE;
         break;
      case 2:
         HFServerPortDATA[*ServerPortIndex].IsInActiveCall = FALSE;
         HFServerPortDATA[*ServerPortIndex].IsCallOnHold = TRUE;
         break;
   }
}

   /* Utility function for HFRE_Close_Port_Indication event in          */
   /* HFRE_Event_Callback.                                              */
static void ClosePortIndication(int *ServerPortIndex)
{
   /* Flag that we are no longer connected.                             */
   HFServerPortDATA[*ServerPortIndex].IsConnected = FALSE;

   if(HFServerPortDATA[*ServerPortIndex].IsInActiveAudio)
   {
      /* Disable the audio codec.                                       */
      HAL_DisableAudioCodec();
   }

   DisableWBS(*ServerPortIndex);

   NumberOfCurrentConnections--;

   Display(("\r\nNumber of active connections: %d\r\n", NumberOfCurrentConnections ));
   Display(("WBS_Connected_Port_ID: %d\r\n", WBS_Connected_Port_ID));
}

   /* Utility function for HFRE_Audio_Connection_Indication event in    */
   /* HFRE_Event_Callback.  *** Note *** This event is invoked usually  */
   /* after the SCO/eSCO is already connected and the voice call is     */
   /* already active with audio.                                        */
static void AudioConnectionIndication(HFRE_Event_Data_t *HFREEventData, int *ServerPortIndex, Word_t *ConnectionHandle, int *Result)
{
   int          SecondPortIndex;
   unsigned int Index;

   /* Set the flag for active SCO for this server index                 */
   HFServerPortDATA[*ServerPortIndex].IsInActiveSCO = TRUE;

   /* Route the audio to this port index just if the second is not      */
   /* active                                                            */
   SecondPortIndex = (*ServerPortIndex) ? 0 : 1;

   if(HFServerPortDATA[SecondPortIndex].IsInActiveAudio == TRUE)
   {
      /* if the second port is in active SCO with audio- Don't route    */
      /* this one also                                                  */
      Display((" Second SCO/eSCO channel opened with handle=0x%x", HFREEventData->Event_Data.HFRE_Audio_Connection_Indication_Data->SCO_Connection_Handle));
      Display((" Audio already active on port index %d - return without routing the audio\r\n", SecondPortIndex));
      return;
   }
   else
   {
      if(HFServerPortDATA[*ServerPortIndex].IsInActiveAudio == TRUE)
      {
         /* if the port is in active SCO with audio- Don't route again  */
         Display((" Audio already active on this port index %d - return without re-routing the audio\r\n", SecondPortIndex));
         return;
      }
   }

   /* Check if WBS already enabled by HFP 1.6 CODEC negotiation command.*/
   /* Also check if no other WBS is already connected and streaming     */
   if((HFServerPortDATA[*ServerPortIndex].SupportWBS) && (!WBS_Connected_Port_ID))
   {
      if((*Result = GAP_Query_Connection_Handle(BluetoothStackID, HFServerPortDATA[*ServerPortIndex].RemoteBD_ADDR, ConnectionHandle)) == 0)
      {
         lastAudioFreq = WBS_FREQUENCY;

         /* Enable the audio codec.                                     */
         HAL_EnableAudioCodec(BluetoothStackID, aucHFP_HSP, WBS_FREQUENCY, 1);

         HFServerPortDATA[*ServerPortIndex].IsInActiveAudio = TRUE;

         Display((" Setup WBS with Audio for ACL handle 0x%04X \r\n", *ConnectionHandle));

         /* Issue the WBS Associate.                                    */
         if((*Result = VS_WBS_Associate(BluetoothStackID, *ConnectionHandle)) < 0)
         {
            DisplayFunctionError("VS_WBS_Associate", *Result);
         }
         else
         {
            WBS_Connected_Port_ID = HFREEventData->Event_Data.HFRE_Audio_Connection_Indication_Data->HFREPortID;

            /* We have opened a WBS connection, now send our supported  */
            /* codecs to all connected remote devices, except the one   */
            /* WBS is active on, in order to notify them that WBS is no */
            /* longer available.                                        */
            for(Index = 0; Index < MAXIMUM_SUPPORTED_HFRE_CONNECTIONS; Index++)
            {
               if(Index != (*ServerPortIndex))
                  SendAvailableCodecs(Index);
            }
         }
      }
   }
   /* Start CVSD 8KHz Audio just if the WBS flag is FALSE               */
   else
   {
      if(!(HFServerPortDATA[*ServerPortIndex].SupportWBS))
      {
         if((lastAudioFreq) && (lastAudioFreq != CVSD_FREQUENCY))
         {
            /* Disable the WBS functionality by sending the controller  */
            /* WBS associate with DISABLE                               */
            if((*Result = VS_WBS_Disassociate(BluetoothStackID)) < 0)
            {
               DisplayFunctionError("VS_WBS_Disassociate", *Result);
            }
         }

         lastAudioFreq = CVSD_FREQUENCY;

         /* Enable the audio codec, note that if it is already enabled  */
         /* that this will simply change the frequency.                 */
         HAL_EnableAudioCodec(BluetoothStackID, aucHFP_HSP, CVSD_FREQUENCY, 1);

         HFServerPortDATA[*ServerPortIndex].IsInActiveAudio = TRUE;

         Display((" Setup CVSD with Audio for ACL handle 0x%04X \r\n", *ConnectionHandle));

         if(WBS_Connected_Port_ID == HFREEventData->Event_Data.HFRE_Audio_Connection_Indication_Data->HFREPortID)
         {
            WBS_Connected_Port_ID = 0;
         }
      }
   }
}

   /* Utility function for HFRE_Audio_Disconnection_Indication event in */
   /* HFRE_Event_Callback.                                              */
static void AudioDisconnectionIndication(unsigned int ServerPortInfoIndex)
{
   HFServerPortDATA[ServerPortInfoIndex].IsInActiveSCO = FALSE;

   if(HFServerPortDATA[ServerPortInfoIndex].IsInActiveAudio == TRUE)
   {
      /* Disable the audio codec.                                       */
      HAL_DisableAudioCodec();

      Display(("Audio codec disabled.\r\n"));

      HFServerPortDATA[ServerPortInfoIndex].IsInActiveAudio = FALSE;

      if(WBS_Connected_Port_ID == HFServerPortDATA[ServerPortInfoIndex].HFServerPortID)
      {
         DisableWBS(ServerPortInfoIndex);
      }

      DisplayConnectionStatus(ServerPortInfoIndex);
   }
}

   /* Utility function for handling the event-                          */
   /* HFRE_Codec_Select_Request_Indication, event in                    */
   /* HFRE_Event_Callback.  The function also route the audio with the  */
   /* right frequency if no audio call is ongoing.  If audio call is    */
   /* ongoing just send back the selected CODEC to the AG In HFP1.6 with*/
   /* WBS, this event invoked before the SCO/eSCO is opened, then the   */
   /* stack selects or send to the AG the right parameters to the       */
   /* SCO/eSCO channel.                                                 */
static void CodecSelectRequestIndication(HFRE_Event_Data_t *HFREEventData, int *ServerPortIndex, Word_t *ConnectionHandle, BoardStr_t *BoardStr, int *Result)
{
   unsigned int  SelectedCodecID;
   unsigned long ActiveFeatures;
   Boolean_t     ActiveCallWithAudioOngoing = 0;

   /* * NOTE * Here is where the AG suggests a Codec to use.  Codec ID 1*/
   /*          is for CVSD and CodecID 2 is for mSBC. If anything other */
   /*          than a 1 or 2 is received then default to CVSD.          */
   SelectedCodecID = HFREEventData->Event_Data.HFRE_Codec_Select_Indication_Data->CodecID;

   if(SelectedCodecID != HFRE_MSBC_CODEC_ID)
   {
      /* Set the default CODEC supported if no mSBC.                    */
      SelectedCodecID = HFRE_CVSD_CODEC_ID;
   }

   Display(("SelectedCodecID %d\r\n",SelectedCodecID));

   /* Check if we have already active call with audio.                  */
   if((HFServerPortDATA[0].IsInActiveAudio) || (HFServerPortDATA[1].IsInActiveAudio))
   {
      ActiveCallWithAudioOngoing = 1;
   }

   /* Check to see if mSBC is being requested.                          */
   if(SelectedCodecID == HFRE_MSBC_CODEC_ID)
   {
      /* Check if no audio now.                                         */
      if(ActiveCallWithAudioOngoing == 0)
      {
         *Result = GAP_Query_Connection_Handle(BluetoothStackID, HFServerPortDATA[*ServerPortIndex].RemoteBD_ADDR, ConnectionHandle);

         /* Query the connection handle of the currently connected AG.  */
         if((*Result) == 0)
         {
            /* Verify that WBS is currently supported.                  */
            if(((*Result = BSC_QueryActiveFeatures(BluetoothStackID, &ActiveFeatures)) == 0) && (ActiveFeatures & BSC_FEATURE_WIDE_BAND_SPEECH))
            {
               BD_ADDRToStr(HFServerPortDATA[*ServerPortIndex].RemoteBD_ADDR, *BoardStr);
               Display(("ConnectionHandle %d for %s.\r\n", *ConnectionHandle, *BoardStr));

               /* Enabled WBS, Configure the Codec for 16 KHz.          */
               HFServerPortDATA[*ServerPortIndex].SupportWBS = 1;

               lastAudioFreq = WBS_FREQUENCY;

               /* Enable the audio codec.                               */
               HAL_EnableAudioCodec(BluetoothStackID, aucHFP_HSP, WBS_FREQUENCY, 1);

               HFServerPortDATA[*ServerPortIndex].IsInActiveAudio = TRUE;

               Display((" Setup WBS, initialize codec for HFP port index %d\r\n", *ServerPortIndex));

               /* Issue the WBS Associate.                              */
               if((*Result = VS_WBS_Associate(BluetoothStackID, *ConnectionHandle)) < 0)
               {
                  DisplayFunctionError("VS_WBS_Associate", *Result);
               }
               else
               {
                  /* Set the active WBS port ID.                        */
                  WBS_Connected_Port_ID = HFREEventData->Event_Data.HFRE_Codec_Select_Indication_Data->HFREPortID;
               }
            }
            else
            {
               if(*Result)
               {
                  DisplayFunctionError("BSC_QueryActiveFeatures()", *Result);
                  return;
               }

               Display((" WBS Feature is not enabled, set the selected CODEC to CVSD \r\n"));
               HFServerPortDATA[*ServerPortIndex].SupportWBS = 0;
               SelectedCodecID = HFRE_CVSD_CODEC_ID;
            }
         }
         else /* if(0 == (*Result)), result of querying the connection handle from the GAP is not 0 = failed */
         {
            DisplayFunctionError("GAP_Query_Connection_Handle", *Result);
            return;
         }
      }
   }
   else /* if(SelectedCodecID == HFRE_MSBC_CODEC_ID) */
   {
      /* Check if no audio now.                                         */
      if (ActiveCallWithAudioOngoing == 0)
      {
         HFServerPortDATA[*ServerPortIndex].SupportWBS = 0;

         if((lastAudioFreq) && (lastAudioFreq != CVSD_FREQUENCY))
         {
            /* Disable the WBS functionality by sending the controller  */
            /* WBS associate with DISABLE.                              */
            if((*Result = VS_WBS_Disassociate(BluetoothStackID)) < 0)
            {
               DisplayFunctionError("VS_WBS_Disassociate", *Result);
            }
         }

         lastAudioFreq = CVSD_FREQUENCY;

         Display(("HFRE_Send_Select_Codec: CVSD\r\n"));

         HAL_EnableAudioCodec(BluetoothStackID, aucHFP_HSP, CVSD_FREQUENCY, 1);

         HFServerPortDATA[*ServerPortIndex].IsInActiveAudio = TRUE;

         Display((" Setup CVSD for HFP port index %d\r\n", *ServerPortIndex));
      }
   }

   /* Send to the AG which codec to select.                             */
   HFRE_Send_Select_Codec(BluetoothStackID, HFREEventData->Event_Data.HFRE_Codec_Select_Indication_Data->HFREPortID, SelectedCodecID);

   Display(("HFRE_Send_Select_Codec: %d\r\n", SelectedCodecID));
   Display(("WBS_Connected_Port_ID:  %d\r\n", WBS_Connected_Port_ID));
}

   /* The following function is responsible for sending the command to  */
   /* Hold a connected Call on the Remote Audio Gateway.  This function */
   /* returns zero on successful execution and a negative value on all  */
   /* errors.                                                           */
static int PlaceCallOnHold(ParameterList_t *TempParam)
{
   int Result;
   int ret_val;
   int ServerPortIndex;

   /* First, check that valid Bluetooth Stack ID exists.                */
   if(BluetoothStackID)
   {
      /* Now check to make sure that the Port ID appears to be          */
      /* semi-valid.                                                    */
      if(TempParam)
      {
         ServerPortIndex = -1;

         /* Determine the server port index.  Note that this command can*/
         /* work without any parameters if the HF device has only one   */
         /* connection available, if not it needs the port ID parameter.*/
         if((NumberOfAnsweredCalls() == 1) && (TempParam->NumberofParameters == 0))
         {
            ServerPortIndex = FindHFActivePortEntry();
         }
         else
         {
            if(TempParam->NumberofParameters == 1)
            {
               ServerPortIndex = FindHFServerPortEntry((unsigned int)(TempParam->Params[0].intParam));
            }
         }

         /* Checks if the server index is valid and that there is active*/
         /* call or call on hold                                        */
         if((ServerPortIndex >= 0) && ((HFServerPortDATA[ServerPortIndex].IsInActiveCall) || (HFServerPortDATA[ServerPortIndex].IsCallOnHold)))
         {
            /* Send the command "Place All Active Calls On Hold Accept  */
            /* The Other" to the remote audio gateway.                  */
            Result = HFRE_Send_Call_Holding_Multiparty_Selection(BluetoothStackID, HFServerPortDATA[ServerPortIndex].HFServerPortID, chPlaceAllActiveCallsOnHoldAcceptTheOther, 0);

            /* Set the return value of this function equal to the Result*/
            /* of the function call.  Now check to see if the command   */
            /* was submitted successfully.                              */
            ret_val = DisplayFunctionChecker("HFRE_Send_Call_Holding_Multiparty_Selection", Result);

            /* Check if the function finished successfully.             */
            if(ret_val == 0)
            {
               if((HFServerPortDATA[ServerPortIndex].IsInActiveCall == TRUE) &&
                  (HFServerPortDATA[ServerPortIndex].IsCallOnHold == FALSE) &&
                  (HFServerPortDATA[ServerPortIndex].IsCallSetupInProgress == FALSE) &&
                  (HFServerPortDATA[ServerPortIndex].IsInActiveSCO == TRUE))
               {
                  ret_val = HFREReleaseAudioConnection(ServerPortIndex);
               }
            }
         }
         else
         {
            ret_val = INVALID_PARAMETERS_ERROR;
         }
      }
      else
      {
         ret_val = INVALID_PARAMETERS_ERROR;
      }

      if(ret_val != 0)
      {
         /* The required parameter is invalid.                          */
         Display(("Usage: PlaceCallOnHold [PortID Number (optional if only one connection)].\r\n"));
      }
   }
   else
   {
      /* No valid Bluetooth Stack ID exists.                            */
      ret_val = INVALID_STACK_ID_ERROR;
   }

   return(ret_val);
}

   /* The following function is responsible for sending the             */
   /* HoldingMultipartyCall command to a waiting Call on the Remote     */
   /* Audio Gateway.  This function returns zero on successful execution*/
   /* and a negative value on all errors.                               */
   /* * NOTE * Using this function can cause undesirable effects if not */
   /*          used carefully - mostly used in order to dismiss the     */
   /*          waiting call on the secondary device.                    */
static int HoldingMultipartyCall(ParameterList_t *TempParam)
{
   int                                       Result;
   int                                       ret_val;
   int                                       ServerPortIndex;
   HFRE_Call_Hold_Multiparty_Handling_Type_t CallHoldMultipartyHandling;

   /* First, check that valid Bluetooth Stack ID exists.                */
   if(BluetoothStackID)
   {
      /* Now check to make sure that the Port ID appears to be          */
      /* semi-valid.                                                    */
      ret_val = SearchForServerPortIndexByUserInputs(TempParam, &ServerPortIndex);

      if((ServerPortIndex >= 0) && (ret_val == 0))
      {
         /* Format the user input into ENUM parameter                   */
         ret_val = CallHoldMultipartyHandlingIntToENUMSwitch(TempParam->Params[0].intParam, &CallHoldMultipartyHandling);

         /* The Port ID appears to be a semi-valid value.  Now submit   */
         /* the command.                                                */
         if(ret_val == 0)
         {
            /* Send the place-call-on-hold-request.  Note that we will  */
            /* wait for the CALLHELD indicator from the AG before       */
            /* updating our IsCallOnHold state variable; we do this is  */
            /* because the call isn't actually on hold until the AG has */
            /* changed the call's state.                                */
            Result = HFRE_Send_Call_Holding_Multiparty_Selection(BluetoothStackID, HFServerPortDATA[ServerPortIndex].HFServerPortID, CallHoldMultipartyHandling, TempParam->Params[1].intParam);

            /* Set the return value of this function equal to the Result*/
            /* of the function call.  Now check to see if the command   */
            /* was submitted successfully.                              */
            ret_val = DisplayFunctionChecker("HFRE_Send_Call_Holding_Multiparty_Selection", Result);
         }
      }
      else
      {
         /* The required parameter is invalid.                          */
         DisplayUsageHoldingMultipartyCall();

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

   /* The following function switches the audio between two phones with */
   /* two active calls.  When the first call answered and the SCO/eSCO  */
   /* channel is connected, also the PCM audio from the BT device is    */
   /* routed to the evaluation board CODEC and the MIC is routed to ADC */
   /* and then to the BT PCM. If the other phone also answer an incoming*/
   /* call and open another SCO/eSCO channel, the audio stays with the  */
   /* first phone that started his active call.  When calling this      */
   /* function, first check that there are two phones with active calls,*/
   /* then check also that there are no more calls in the phone (on hold*/
   /* or waiting). After the checks, send to the phone with the audio   */
   /* active call, to hold the call and release the SCO/eSCO channel.   */
   /* After the SCO/eSCO released, uninitialize the audio routings.     */
   /* Then wait 10ms and disconnect the SCO/eSCO of the second phone for*/
   /* connect again with audio routing.  This function returns zero on  */
   /* successful execution and a negative value on all errors.          */
static int SwitchAudioFor2PhonesWithActiveCall(ParameterList_t *TempParam)
{
   int          ret_val;
   unsigned int Index;
   unsigned int CurrentActiveIndex;
   unsigned int NextActiveIndex;
   unsigned int NumOngoing;

   if(BluetoothStackID)
   {
      ret_val = 0;

      if(HFServerPortDATA[0].IsInActiveAudio == TRUE)
      {
         CurrentActiveIndex = 0;
         NextActiveIndex    = 1;
      }
      else
      {
         if(HFServerPortDATA[1].IsInActiveAudio == TRUE)
         {
            CurrentActiveIndex = 1;
            NextActiveIndex    = 0;
         }
         else
         {
            Display(("Error: No active calls.\r\n"));

            ret_val = FUNCTION_ERROR;
         }
      }

      /* Make sure that each phone has 1 and only 1 ongoing call.       */
      for(Index = 0; (Index < MAXIMUM_SUPPORTED_HFRE_CONNECTIONS) && (ret_val == 0); Index++)
      {
         /* Check if this server port is connected.                     */
         if(HFServerPortDATA[Index].IsConnected)
         {
            NumOngoing = 0;

            if(HFServerPortDATA[Index].IsInActiveCall == TRUE)
               NumOngoing++;

            if(HFServerPortDATA[Index].IsCallOnHold == TRUE)
               NumOngoing++;

            if(NumOngoing == 0)
            {
               Display(("Error: There is not an ongoing call on server port ID %u.\r\n", HFServerPortDATA[Index].HFServerPortID));

               ret_val = FUNCTION_ERROR;
            }
            else
            {
               if(NumOngoing > 1)
               {
                  Display(("Error: There is more than one ongoing call on server port ID %u.\r\n", HFServerPortDATA[Index].HFServerPortID));

                  ret_val = FUNCTION_ERROR;
               }
            }
         }
      }

      /* Check in what HFP index the active call with audio exist       */
      if(ret_val == 0)
      {
         /* Check if only one active call in the specific server index  */
         if((HFServerPortDATA[CurrentActiveIndex].IsCallSetupInProgress == FALSE) && (HFServerPortDATA[CurrentActiveIndex].IsCallOnHold == FALSE))
         {
            /* Hold the current active call and disconnect the SCO/eSCO,*/
            /* for the switch For Holding the call- Send the command    */
            /* "Place All Active Calls On Hold Accept The Other" to the */
            /* remote audio gateway.                                    */
            /* * NOTE * Some phones don't support being placed into hold*/
            /*          when they have a single call and therefore this */
            /*          request may fail.  If this occurs it's not an   */
            /*          issue because the audio connection will be      */
            /*          closed in the code below so audio from that call*/
            /*          won't be heard.                                 */
            ret_val = HFRE_Send_Call_Holding_Multiparty_Selection(BluetoothStackID, HFServerPortDATA[CurrentActiveIndex].HFServerPortID, chPlaceAllActiveCallsOnHoldAcceptTheOther, 0);

            if(ret_val)
            {
               DisplayFunctionError("HFRE_Send_Call_Holding_Multiparty_Selection()", ret_val);
            }
            else
            {
               Display(("Hold command sent successfully for HFP server port ID %u.\r\n", HFServerPortDATA[CurrentActiveIndex].HFServerPortID));
            }

            /* Disconnect the SCO/eSCO from the currently active index. */
            ret_val = HFREReleaseAudioConnection(CurrentActiveIndex);

            if(ret_val)
            {
               DisplayFunctionError("HFREReleaseAudioConnection()", ret_val);
            }

            /* Check if need to reconnect the SCO/eSCO of the second    */
            /* phone                                                    */
            if(HFServerPortDATA[NextActiveIndex].IsInActiveSCO == TRUE)
            {
               /* Disconnect the SCO/eSCO also for the next active      */
               /* index.                                                */
               ret_val = HFREReleaseAudioConnection(NextActiveIndex);

               if(ret_val)
               {
                  DisplayFunctionError("HFREReleaseAudioConnection()", ret_val);
               }
            }

            /* If all the above succeeded                               */
            if(!ret_val)
            {
               /* Check if the next active device's call is on hold.    */
               if(HFServerPortDATA[NextActiveIndex].IsCallOnHold == TRUE)
               {
                  /* Its call is on hold, take it off hold.             */
                  ret_val = HFRE_Send_Call_Holding_Multiparty_Selection(BluetoothStackID, HFServerPortDATA[NextActiveIndex].HFServerPortID, chPlaceAllActiveCallsOnHoldAcceptTheOther, 0);

                  ret_val = DisplayFunctionChecker("HFRE_Send_Call_Holding_Multiparty_Selection", ret_val);
               }

               if(ret_val == 0)
               {
                  /* Setup the SCO/eSCO audio connection.               */
                  ret_val = HFRESetupAudioConnection(NextActiveIndex);
               }
            }
         }
         else
         {
            Display(("Error: Phone with server port ID %u with active call and another call waiting/on=hold, can't perform the switch.\r\n", HFServerPortDATA[CurrentActiveIndex].HFServerPortID));
         }
      }
   }
   else
   {
      ret_val = FUNCTION_ERROR;
   }

   return (ret_val);
}

   /* The following function switches the audio between two phones, that*/
   /* one in active call with audio and the second with waiting call.   */
   /* The waiting call is answered and the audio is routed to the new   */
   /* active call.  When the first call answered and the SCO/eSCO       */
   /* channel is connected, also the PCM audio from the BT device is    */
   /* routed to the local audio codec and the MIC is routed to ADC and  */
   /* then to the BT PCM. If the other phone has an incoming call the   */
   /* audio will stays with the first phone that started his active call*/
   /* even if the user will answer the call.  When calling this         */
   /* function, first check that there are two phones with active and   */
   /* waiting calls, then check also that there are no more calls in the*/
   /* phone (on hold or waiting or active). After the checks, send to   */
   /* the phone with the audio active call, to hold the call and release*/
   /* the SCO/eSCO channel.  After the SCO/eSCO released, uninitialize  */
   /* the audio routings.  Then wait 10ms and disconnect the SCO/eSCO of*/
   /* the second phone for connect again with audio routing after       */
   /* answering the waiting call.  This function returns zero on        */
   /* successful execution and a negative value on all errors.          */
static int AnswerSeconePhoneIncomingCallAndSwitchAudio(ParameterList_t *TempParam)
{
   int          ret_val = 0;
   unsigned int CurrentActiveIndex;
   unsigned int NextActiveIndex;

   if(BluetoothStackID)
   {
      if(!(((HFServerPortDATA[0].IsCallSetupInProgress == TRUE) && (HFServerPortDATA[1].IsInActiveCall == TRUE))|| ((HFServerPortDATA[1].IsCallSetupInProgress == TRUE) && (HFServerPortDATA[0].IsInActiveCall == TRUE))))
      {
         Display(("Error: No waiting and active call on 2 phones.\r\n"));

         return(FUNCTION_ERROR);
      }

      if(HFServerPortDATA[0].IsInActiveAudio == TRUE)
      {
         CurrentActiveIndex = 0;
         NextActiveIndex    = 1;
      }
      else
      {
         if(HFServerPortDATA[1].IsInActiveAudio == TRUE)
         {
            CurrentActiveIndex = 1;
            NextActiveIndex    = 0;
         }
         else
         {
            Display(("Error: No active calls.\r\n"));

            ret_val = FUNCTION_ERROR;
         }
      }

      /* Check in what HFP index the active call with audio exist.      */
      if(ret_val == 0)
      {
         /* Check if only one active call in the specific server index. */
         if((HFServerPortDATA[CurrentActiveIndex].IsCallSetupInProgress == FALSE) && (HFServerPortDATA[CurrentActiveIndex].IsCallOnHold == FALSE))
         {
            /* Hold the current active call and disconnect the SCO/eSCO,*/
            /* for the switch For Holding the call- Send the command    */
            /* "Place All Active Calls On Hold Accept The Other" to the */
            /* remote audio gateway.                                    */
            /* * NOTE * Some phones don't support being placed into hold*/
            /*          when they have a single call and therefore this */
            /*          request may fail.  If this occurs it's not an   */
            /*          issue because the audio connection will be      */
            /*          closed in the code below so audio from that call*/
            /*          won't be heard.                                 */
            ret_val = HFRE_Send_Call_Holding_Multiparty_Selection(BluetoothStackID, HFServerPortDATA[CurrentActiveIndex].HFServerPortID, chPlaceAllActiveCallsOnHoldAcceptTheOther, 0);

            if(ret_val)
            {
               DisplayFunctionError("HFRE_Send_Call_Holding_Multiparty_Selection()", ret_val);
            }
            else
            {
               Display(("Hold command sent successfully, for HFP server port ID %u.\r\n", HFServerPortDATA[CurrentActiveIndex].HFServerPortID));
            }

            /* Disconnect the SCO/eSCO from currently active index.     */
            ret_val = HFREReleaseAudioConnection(CurrentActiveIndex);

            if(ret_val)
            {
               DisplayFunctionError("HFREReleaseAudioConnection()", ret_val);
            }

            /* Check if need to reconnect the SCO/eSCO of the second    */
            /* phone                                                    */
            if(HFServerPortDATA[NextActiveIndex].IsInActiveSCO == TRUE)
            {
               /* Disconnect the SCO/eSCO also from the next active     */
               /* index.                                                */
               ret_val = HFREReleaseAudioConnection(NextActiveIndex);

               if(ret_val)
               {
                  DisplayFunctionError("HFREReleaseAudioConnection()", ret_val);
               }
            }

            /* If all the above succeeded                               */
            if(!ret_val)
            {
               /* Answer the incoming call.                             */
               ret_val = HFRE_Answer_Incoming_Call(BluetoothStackID, HFServerPortDATA[NextActiveIndex].HFServerPortID);

               if(ret_val)
               {
                  DisplayFunctionError("HFRE_Answer_Incoming_Call()", ret_val);
               }
               else
               {
                  Display(("Answer call sent successfully, for HFP server port ID %u.\r\n", HFServerPortDATA[NextActiveIndex].HFServerPortID));
               }
            }
         }
         else
         {
            Display(("Error: Phone with server port ID %u with active call and another call waiting/on=hold, can't perform the switch.\r\n", HFServerPortDATA[CurrentActiveIndex].HFServerPortID));
         }
      }
   }
   else
   {
      ret_val = FUNCTION_ERROR;
   }

   return (ret_val);
}

   /* The following function has the following responsibilities: 1) When*/
   /* using only one connection this function responsible for sending   */
   /* the HoldingMultipartyCall command to a waiting call on the Remote */
   /* Audio Gateway.  2) When using two connection and one connection   */
   /* has waiting call or call in hold and an active connection this    */
   /* function responsible for sending the HoldingMultipartycall command*/
   /* so the user can change the active call in this phone but he can't */
   /* return to the second phone without releasing one of those calls.  */
   /* 3) When using two connections, one is in active call and the other*/
   /* is in waiting call or in on hold call, this function responsible  */
   /* for answering and switching between the active call (Which phone  */
   /* has the WBS active), you can enable CVSD in the Second line using */
   /* PlaceCallOnHold (Port ID of the call on hold) *Note* When using   */
   /* PlaceCallOnHold, you won't be able to switch between the calls.   */
   /* This function returns zero on successful execution and a negative */
   /* value on all errors.                                              */
static int HoldingMultiPhonesCall(ParameterList_t *TempParam)
{
   int                                       Result = 0;
   int                                       ret_val = 0;
   int                                       ServerPortIndex;
   int                                       WBSConnectedPortIndex;
   HFRE_Call_Hold_Multiparty_Handling_Type_t CallHoldMultipartyHandling;

   /* First, check that valid Bluetooth Stack ID exists.                */
   if(BluetoothStackID)
   {
      /* Now check to make sure that the Port ID appears to be          */
      /* semi-valid.                                                    */
      ret_val = SearchForServerPortIndexByUserInputs(TempParam, &ServerPortIndex);

      /* Check if the port index valid                                  */
      if((ServerPortIndex >= 0) && (ret_val == 0))
      {
         /* Checks if the remote audio gateway is in active call and    */
         /* second call is waiting or has a call on hold                */
         if((HFServerPortDATA[ServerPortIndex].IsInActiveCall == TRUE) && ((HFServerPortDATA[ServerPortIndex].IsCallSetupInProgress == TRUE) || (HFServerPortDATA[ServerPortIndex].IsCallOnHold == TRUE)))
         {
            /* Format the user input into ENUM parameter                */
            ret_val = CallHoldMultipartyHandlingIntToENUMSwitch(TempParam->Params[0].intParam, &CallHoldMultipartyHandling);

            Display(("One of the remote audio gateway has Waiting/Holding Call and active call\r\n"
                     "in order to work with secondary audio gateway release one of the calls\r\n"));

            /* Send the command - with the parameter selected in the    */
            /* switch to the remote audio gateway                       */
            if(ret_val == 0)
            {
               /* Send the place-call-on-hold-request.  Note that we    */
               /* will wait for the CALLHELD indicator from the AG      */
               /* before updating our IsCallOnHold state variable; we do*/
               /* this is because the call isn't actually on hold until */
               /* the AG has changed the call's state.                  */
               Result = HFRE_Send_Call_Holding_Multiparty_Selection(BluetoothStackID, HFServerPortDATA[ServerPortIndex].HFServerPortID, CallHoldMultipartyHandling, TempParam->Params[1].intParam);

               /* Set the return value of this function equal to the    */
               /* Result of the function call.  Now check to see if the */
               /* command was submitted successfully.                   */
               ret_val = DisplayFunctionChecker("HFRE_Send_Call_Holding_Multiparty_Selection", Result);
            }
         }
         else
         {
            /* Search for the index of the WBS active port              */
            WBSConnectedPortIndex = FindHFServerPortEntry(WBS_Connected_Port_ID);

            /* Check if at least one of the AG devices is not in active */
            /* call                                                     */
            if((HFServerPortDATA[ServerPortIndex].IsInActiveCall == FALSE) || ((WBSConnectedPortIndex == INVALID_PARAMETERS_ERROR) || (HFServerPortDATA[WBSConnectedPortIndex].IsInActiveCall == FALSE)))
            {
               /* Check if the AG is in active call and has call on hold*/
               /* or waiting call.                                      */
               if((HFServerPortDATA[ServerPortIndex].IsInActiveCall == TRUE) && ((HFServerPortDATA[ServerPortIndex].IsCallSetupInProgress == TRUE) || (HFServerPortDATA[ServerPortIndex].IsCallOnHold == TRUE)))
               {
                  Display(("Please Disconnect One of the calls in the second phone\r\n"));
               }

               /* Check if one the AG is in call on hold or waiting     */
               /* call, and if the second AG is in another port index.  */
               else if((WBSConnectedPortIndex != ServerPortIndex) && ((WBSConnectedPortIndex == INVALID_PARAMETERS_ERROR) ||
                       ((HFServerPortDATA[WBSConnectedPortIndex].IsInActiveCall == TRUE) &&
                       ((HFServerPortDATA[ServerPortIndex].IsCallOnHold == TRUE) || (HFServerPortDATA[ServerPortIndex].IsCallSetupInProgress == TRUE)))))
               {
                  if(WBSConnectedPortIndex != INVALID_PARAMETERS_ERROR)
                  {
                     /* Send the command "Place All Active Calls On Hold*/
                     /* Accept The Other" to the remote audio gateway.  */
                     Result = HFRE_Send_Call_Holding_Multiparty_Selection(BluetoothStackID, HFServerPortDATA[WBSConnectedPortIndex].HFServerPortID, chPlaceAllActiveCallsOnHoldAcceptTheOther, TempParam->Params[1].intParam);

                     /* Set the return value of this function equal to  */
                     /* the Result of the function call.  Now check to  */
                     /* see if the command was submitted successfully.  */
                     ret_val = DisplayFunctionChecker("HFRE_Send_Call_Holding_Multiparty_Selection", Result);
                  }

                  if(Result == 0)
                  {
                     /* Check if there a call on hold, if there isn't it*/
                     /* means that there is waiting call, so there is   */
                     /* need for answering it when there is call on     */
                     /* hold, there is need to release it               */
                     if(HFServerPortDATA[ServerPortIndex].IsCallOnHold == FALSE)
                     {
                        Display(("Call is waiting on the other line.\r\n"));

                        /* Answer the waiting call.                     */
                        Result = HFRE_Answer_Incoming_Call(BluetoothStackID, HFServerPortDATA[ServerPortIndex].HFServerPortID);

                        /* Set the return value of this function equal  */
                        /* to the Result of the function call.  Now     */
                        /* check to see if the command was submitted    */
                        /* successfully.                                */
                        ret_val = DisplayFunctionChecker("HFRE_Answer_Incoming_Call", Result);
                     }
                     else
                     {
                        Display(("Call is on hold in the other line\r\n"));

                        /* Send the command "Place All Active Calls On  */
                        /* Hold Accept The Other" to the remote audio   */
                        /* gateway.  Note that we will wait for the     */
                        /* CALLHELD indicator from the AG before        */
                        /* updating our IsCallOnHold state variable; we */
                        /* do this is because the call isn't actually on*/
                        /* hold until the AG has changed the call's     */
                        /* state.                                       */
                        Result = HFRE_Send_Call_Holding_Multiparty_Selection(BluetoothStackID, HFServerPortDATA[ServerPortIndex].HFServerPortID, chPlaceAllActiveCallsOnHoldAcceptTheOther, TempParam->Params[1].intParam);

                        /* Set the return value of this function equal  */
                        /* to the Result of the function call.  Now     */
                        /* check to see if the command was submitted    */
                        /* successfully.                                */
                        ret_val = DisplayFunctionChecker("HFRE_Send_Call_Holding_Multiparty_Selection", Result);
                     }
                  }
               }
               else
               {
                  ret_val = INVALID_PARAMETERS_ERROR;
               }
            }
            else
            {
               Display(("Two voice connections are working in parallel,\r\n"
                        "close one of them in order to use this function.\r\n"));

               ret_val = TWO_HFRE_VOICES_ARE_WORKING_IN_PARALLEL;
            }
         }
      }
      else
      {
         ret_val = INVALID_PARAMETERS_ERROR;
      }
   }
   else
   {
      /* No valid Bluetooth Stack ID exists.                            */
      ret_val = INVALID_STACK_ID_ERROR;
   }

   if(ret_val && (ret_val != TWO_HFRE_VOICES_ARE_WORKING_IN_PARALLEL) )
   {
      /* The required parameter is invalid.                             */
      DisplayUsageHoldingMultipartyCall();
   }

   return (ret_val);
}

   /*********************************************************************/
   /*                         Event Callbacks                           */
   /*********************************************************************/

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
   /* installed callback (i.e.  this function DOES NOT have be          */
   /* reentrant).  It Needs to be noted however, that if the same       */
   /* Callback is installed more than once, then the callbacks will be  */
   /* called serially.  Because of this, the processing in this function*/
   /* should be as efficient as possible.  It should also be noted that */
   /* this function is called in the Thread Context of a Thread that the*/
   /* User does NOT own.  Therefore, processing in this function should */
   /* be as efficient as possible (this argument holds anyway because   */
   /* other GAP Events will not be processed while this function call is*/
   /* outstanding).                                                     */
   /* * NOTE * This function MUST NOT Block and wait for events that    */
   /*          can only be satisfied by Receiving other GAP Events.  A  */
   /*          Deadlock WILL occur because NO GAP Event Callbacks will  */
   /*          be issued while this function is currently outstanding.  */
static void BTPSAPI GAP_Event_Callback(unsigned int BluetoothStackID, GAP_Event_Data_t *GAP_Event_Data, unsigned long CallbackParameter)
{
   int                               Result;
   int                               Index;
   char                              BoardStr[16];
   BD_ADDR_t                         NULL_BD_ADDR;
   Boolean_t                         OOB_Data;
   Boolean_t                         MITM;
   GAP_IO_Capability_t               RemoteIOCapability;
   GAP_Inquiry_Event_Data_t         *GAP_Inquiry_Event_Data;
   GAP_Remote_Name_Event_Data_t     *GAP_Remote_Name_Event_Data;
   GAP_Authentication_Information_t  GAP_Authentication_Information;

   /* First, check to see if the required parameters appear to be       */
   /* semi-valid.                                                       */
   if((BluetoothStackID) && (GAP_Event_Data))
   {
      Display(("\r\n"));

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
               /* The inquiry event data received appears to be         */
               /* semi-valid.                                           */
               Display(("GAP_Inquiry_Result: %d Found.\r\n", GAP_Inquiry_Event_Data->Number_Devices));

               /* Now, check to see if the gap inquiry event data's     */
               /* inquiry data appears to be semi-valid.                */
               if(GAP_Inquiry_Event_Data->GAP_Inquiry_Data)
               {
                  /* Display a list of all the devices found from       */
                  /* performing the inquiry.                            */
                  for(Index=0;(Index<GAP_Inquiry_Event_Data->Number_Devices) && (Index<MAX_INQUIRY_RESULTS);Index++)
                  {
                     InquiryResultList[Index] = GAP_Inquiry_Event_Data->GAP_Inquiry_Data[Index].BD_ADDR;
                     BD_ADDRToStr(GAP_Inquiry_Event_Data->GAP_Inquiry_Data[Index].BD_ADDR, BoardStr);

                     Display(("GAP Inquiry Result: %d, %s.\r\n", (Index+1), BoardStr));
                  }

                  NumberofValidResponses = GAP_Inquiry_Event_Data->Number_Devices;
               }
            }
            break;
         case etInquiry_Entry_Result:
            /* Next convert the BD_ADDR to a string.                    */
            BD_ADDRToStr(GAP_Event_Data->Event_Data.GAP_Inquiry_Entry_Event_Data->BD_ADDR, BoardStr);

            /* Display this GAP Inquiry Entry Result.                   */
            Display(("GAP Inquiry Entry Result: %s.\r\n", BoardStr));
            break;
         case etAuthentication:
            /* An authentication event occurred, determine which type of*/
            /* authentication event occurred.                           */
            switch(GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->GAP_Authentication_Event_Type)
            {
               case atLinkKeyRequest:
                  BD_ADDRToStr(GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Remote_Device, BoardStr);
                  Display(("atLinkKeyRequest: %s\r\n", BoardStr));

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
                     Display(("GAP_Authentication_Response() Success.\r\n"));
                  else
                     Display(("GAP_Authentication_Response() Failure: %d.\r\n", Result));
                  break;
               case atPINCodeRequest:
                  /* A pin code request event occurred, first display   */
                  /* the BD_ADD of the remote device requesting the pin.*/
                  BD_ADDRToStr(GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Remote_Device, BoardStr);
                  Display(("atPINCodeRequest: %s\r\n", BoardStr));

                  /* Note the current Remote BD_ADDR that is requesting */
                  /* the PIN Code.                                      */
                  CurrentRemoteBD_ADDR = GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Remote_Device;
                  /* Inform the user that they will need to respond with*/
                  /* a PIN Code Response.                               */
                  Display(("\r\nRespond with the command: PINCodeResponse\r\n"));
                  break;
               case atAuthenticationStatus:
                  /* An authentication status event occurred, display   */
                  /* all relevant information.                          */
                  BD_ADDRToStr(GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Remote_Device, BoardStr);
                  Display(("atAuthenticationStatus: %d Board: %s\r\n", GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Authentication_Event_Data.Authentication_Status, BoardStr));

                  /* Flag that there is no longer a current             */
                  /* Authentication procedure in progress.              */
                  ASSIGN_BD_ADDR(CurrentRemoteBD_ADDR, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
                  break;
               case atLinkKeyCreation:
                  /* A link key creation event occurred, first display  */
                  /* the remote device that caused this event.          */
                  BD_ADDRToStr(GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Remote_Device, BoardStr);
                  Display(("atLinkKeyCreation: %s\r\n", BoardStr));

                  /* The BD_ADDR of the remote device has been displayed*/
                  /* now display the link key being created.            */
                  Display(("Link Key: 0x"));

                  for(Index = 0;Index<sizeof(Link_Key_t);Index++)
                     Display(("%02X", ((Byte_t *)(&(GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Authentication_Event_Data.Link_Key_Info.Link_Key)))[Index]));

                  Display(("\r\n"));

                  /* Now store the link Key in either a free location OR*/
                  /* over the old key location.                         */
                  ASSIGN_BD_ADDR(NULL_BD_ADDR, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);

                  for(Index=0,Result=-1;Index<(sizeof(LinkKeyInfo)/sizeof(LinkKeyInfo_t));Index++)
                  {
                     if(COMPARE_BD_ADDR(LinkKeyInfo[Index].BD_ADDR, GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Remote_Device))
                        break;
                     else
                     {
                        if((Result == (-1)) && (COMPARE_BD_ADDR(LinkKeyInfo[Index].BD_ADDR, NULL_BD_ADDR)))
                           Result = Index;
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

                     Display(("Link Key Stored locally.\r\n"));
                  }
                  else
                     Display(("Link Key NOT Stored locally: Link Key array is full.\r\n"));
                  break;
               case atIOCapabilityRequest:
                  BD_ADDRToStr(GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Remote_Device, BoardStr);
                  Display(("atIOCapabilityRequest: %s\r\n", BoardStr));

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
                  /* Check the result of the submitted command.         */
                  if(!Result)
                     Display(("GAP_Authentication_Response() Success.\r\n"));
                  else
                     Display(("GAP_Authentication_Response() Failure: %d.\r\n", Result));
                  break;
               case atIOCapabilityResponse:
                  BD_ADDRToStr(GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Remote_Device, BoardStr);
                  Display(("atIOCapabilityResponse: %s\r\n", BoardStr));

                  RemoteIOCapability = GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Authentication_Event_Data.IO_Capabilities.IO_Capability;
                  MITM               = (Boolean_t)GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Authentication_Event_Data.IO_Capabilities.MITM_Protection_Required;
                  OOB_Data           = (Boolean_t)GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Authentication_Event_Data.IO_Capabilities.OOB_Data_Present;

                  Display(("Remote Capabilities: %s%s%s\r\n", IOCapabilitiesStrings[RemoteIOCapability], ((MITM)?", MITM":""), ((OOB_Data)?", OOB Data":"")));
                  break;
               case atUserConfirmationRequest:
                  BD_ADDRToStr(GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Remote_Device, BoardStr);
                  Display(("atUserConfirmationRequest: %s\r\n", BoardStr));

                  CurrentRemoteBD_ADDR = GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Remote_Device;

                  if(IOCapability != icDisplayYesNo)
                  {
                     /* Invoke JUST Works Process...                    */
                     GAP_Authentication_Information.GAP_Authentication_Type          = atUserConfirmation;
                     GAP_Authentication_Information.Authentication_Data_Length       = (Byte_t)sizeof(Byte_t);
                     GAP_Authentication_Information.Authentication_Data.Confirmation = TRUE;

                     /* Submit the Authentication Response.             */
                     Display(("\r\nAuto Accepting: %u\r\n", GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Authentication_Event_Data.Numeric_Value));

                     Result = GAP_Authentication_Response(BluetoothStackID, GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Remote_Device, &GAP_Authentication_Information);

                     if(!Result)
                        Display(("GAP_Authentication_Response() Success.\r\n"));
                     else
                        Display(("GAP_Authentication_Response() Failure: %d.\r\n", Result));

                     /* Flag that there is no longer a current          */
                     /* Authentication procedure in progress.           */
                     ASSIGN_BD_ADDR(CurrentRemoteBD_ADDR, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
                  }
                  else
                  {
                     Display(("User Confirmation: %u\r\n", GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Authentication_Event_Data.Numeric_Value));

                     /* Inform the user that they will need to respond  */
                     /* with a PIN Code Response.                       */
                     Display(("\r\nRespond with the command: UserConfirmationResponse\r\n"));
                  }
                  break;
               case atPasskeyRequest:
                  BD_ADDRToStr(GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Remote_Device, BoardStr);
                  Display(("atPasskeyRequest: %s\r\n", BoardStr));

                  /* Note the current Remote BD_ADDR that is requesting */
                  /* the Passkey.                                       */
                  CurrentRemoteBD_ADDR = GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Remote_Device;

                  /* Inform the user that they will need to respond with*/
                  /* a Passkey Response.                                */
                  Display(("\r\nRespond with the command: PassKeyResponse\r\n"));
                  break;
               case atRemoteOutOfBandDataRequest:
                  BD_ADDRToStr(GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Remote_Device, BoardStr);
                  Display(("atRemoteOutOfBandDataRequest: %s\r\n", BoardStr));

                  /* This application does not support OOB data so      */
                  /* respond with a data length of Zero to force a      */
                  /* negative reply.                                    */
                  GAP_Authentication_Information.GAP_Authentication_Type    = atOutOfBandData;
                  GAP_Authentication_Information.Authentication_Data_Length = 0;

                  Result = GAP_Authentication_Response(BluetoothStackID, GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Remote_Device, &GAP_Authentication_Information);

                  if(!Result)
                     Display(("GAP_Authentication_Response() Success.\r\n"));
                  else
                     Display(("GAP_Authentication_Response() Failure: %d.\r\n", Result));
                  break;
               case atPasskeyNotification:
                  BD_ADDRToStr(GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Remote_Device, BoardStr);
                  Display(("atPasskeyNotification: %s\r\n", BoardStr));

                  Display(("Passkey Value: %u\r\n", GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Authentication_Event_Data.Numeric_Value));
                  break;
               case atKeypressNotification:
                  BD_ADDRToStr(GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Remote_Device, BoardStr);
                  Display(("atKeypressNotification: %s\r\n", BoardStr));

                  Display(("Keypress: %d\r\n", (int)GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Authentication_Event_Data.Keypress_Type));
                  break;
               default:
                  Display(("Un-handled GAP Authentication Event.\r\n"));
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
               BD_ADDRToStr(GAP_Remote_Name_Event_Data->Remote_Device, BoardStr);

               Display(("GAP Remote Name Result: BD_ADDR: %s.\r\n", BoardStr));

               if(GAP_Remote_Name_Event_Data->Remote_Name)
                  Display(("GAP Remote Name Result: %s.\r\n", GAP_Remote_Name_Event_Data->Remote_Name));
               else
                  Display(("GAP Remote Name Result: NULL.\r\n"));
            }
            break;
         default:
            /* An unknown/unexpected GAP event was received.            */
            Display(("Unknown/Unhandled GAP Event: %d.\n", GAP_Event_Data->Event_Data_Type));
            break;
      }

      DisplayPrompt();
   }
   else
   {
      /* There was an error with one or more of the input parameters.   */
      Display(("\r\n"));

      Display(("GAP Callback Data: Event_Data = NULL.\r\n"));

      DisplayPrompt();
   }
}

   /* The following function is for an HFRE Event Callback.  This       */
   /* function will be called whenever a HFRE Event occurs that is      */
   /* associated with the Bluetooth Stack.  This function passes to the */
   /* caller the HFRE Event Data that occurred and the HFRE Event       */
   /* Callback Parameter that was specified when this Callback was      */
   /* installed.  The caller is free to use the contents of the HFRE    */
   /* Event Data ONLY in the context of this callback.  If the caller   */
   /* requires the Data for a longer period of time, then the callback  */
   /* function MUST copy the data into another Data Buffer.  This       */
   /* function is guaranteed NOT to be invoked more than once           */
   /* simultaneously for the specified installed callback (i.e.  this   */
   /* function DOES NOT have be reentrant).  It Needs to be noted       */
   /* however, that if the same Callback is installed more than once,   */
   /* then the callbacks will be called serially.  Because of this, the */
   /* processing in this function should be as efficient as possible.   */
   /* It should also be noted that this function is called in the Thread*/
   /* Context of a Thread that the User does NOT own.  Therefore,       */
   /* processing in this function should be as efficient as possible    */
   /* (this argument holds anyway because another HFRE Event will not be*/
   /* processed while this function call is outstanding).               */
   /* * NOTE * This function MUST NOT Block and wait for events that    */
   /*          can only be satisfied by Receiving HFP Event Packets.  A */
   /*          Deadlock WILL occur because NO HFP Event Callbacks will  */
   /*          be issued while this function is currently outstanding.  */
static void BTPSAPI HFRE_Event_Callback(unsigned int BluetoothStackID, HFRE_Event_Data_t *HFREEventData, unsigned long CallbackParameter)
{
   int          Result;
   int          ServerPortIndex;
   unsigned int TempHFServerPortID;
   Word_t       ConnectionHandle;
   BoardStr_t   BoardStr;

   /* First, check to see if the required parameters appear to be       */
   /* semi-valid.                                                       */
   if(HFREEventData != NULL)
   {
      /* Save the server port index.  Note that we can access the       */
      /* HFREPortID regardless of the event because it is the first     */
      /* member of all HFRE event data structures.                      */
      ServerPortIndex = FindHFServerPortEntry(HFREEventData->Event_Data.HFRE_Open_Port_Indication_Data->HFREPortID);

      /* The parameters appear to be semi-valid, now check to see what  */
      /* type the incoming event is.                                    */
      switch(HFREEventData->Event_Data_Type)
      {
         case etHFRE_Open_Port_Indication:
            /* A Client has connected to the Server, display the BD_ADDR*/
            /* of the connecting device.                                */
            BD_ADDRToStr(HFREEventData->Event_Data.HFRE_Open_Port_Indication_Data->BD_ADDR, BoardStr);
            Display(("\r\nHFRE Open Port Indication, ID: 0x%04X, Board: %s.\r\n", HFREEventData->Event_Data.HFRE_Open_Port_Indication_Data->HFREPortID, BoardStr));
            HFServerPortDATA[ServerPortIndex].RemoteBD_ADDR = HFREEventData->Event_Data.HFRE_Open_Port_Indication_Data->BD_ADDR;

            NumberOfCurrentConnections++;
            Display(("\r\nNumber of active connections: %d\r\n", NumberOfCurrentConnections ));

            break;
         case etHFRE_Open_Service_Level_Connection_Indication:
            /* A Open Service Level Indication was received, display    */
            /* relevant information.                                    */
            Display(("\r\nHFRE Open Service Level Connection Indication, ID: 0x%04X\r\n", HFREEventData->Event_Data.HFRE_Open_Service_Level_Connection_Indication_Data->HFREPortID));
            Display(("RemoteSupportedFeaturesValid                     : %s\r\n", (HFREEventData->Event_Data.HFRE_Open_Service_Level_Connection_Indication_Data->RemoteSupportedFeaturesValid)?"TRUE":"FALSE"));
            Display(("RemoteSupportedFeatures                          : 0x%08lX\r\n", HFREEventData->Event_Data.HFRE_Open_Service_Level_Connection_Indication_Data->RemoteSupportedFeatures));
            Display(("RemoteCallHoldMultipartySupport                  : 0x%08lX\r\n", HFREEventData->Event_Data.HFRE_Open_Service_Level_Connection_Indication_Data->RemoteCallHoldMultipartySupport));

            OpenServiceLevelConnectionIndication(HFREEventData, &ServerPortIndex);

            break;
         case etHFRE_Control_Indicator_Status_Indication:
            /* A Control Indicator Status Indication was received,      */
            /* display all relevant information.                        */
            switch(HFREEventData->Event_Data.HFRE_Control_Indicator_Status_Indication_Data->HFREControlIndicatorEntry.ControlIndicatorType)
            {
               case ciBoolean:
                  Display(("\r\nHFRE Control Indicator Status Indication, ID: 0x%04X\r\nDescription: %s\r\nValue      : %s.\r\n",  HFREEventData->Event_Data.HFRE_Control_Indicator_Status_Indication_Data->HFREPortID,
                                                                                                                                   HFREEventData->Event_Data.HFRE_Control_Indicator_Status_Indication_Data->HFREControlIndicatorEntry.IndicatorDescription,
                                                                                                                                  (HFREEventData->Event_Data.HFRE_Control_Indicator_Status_Indication_Data->HFREControlIndicatorEntry.Control_Indicator_Data.ControlIndicatorBooleanType.CurrentIndicatorValue)?"TRUE":"FALSE"));
                  /* Checks if the event parameter description is "CALL"*/
                  if(BTPS_MemCompare(HFREEventData->Event_Data.HFRE_Control_Indicator_Status_Indication_Data->HFREControlIndicatorEntry.IndicatorDescription, "CALL", (BTPS_StringLength(HFREEventData->Event_Data.HFRE_Control_Indicator_Status_Indication_Data->HFREControlIndicatorEntry.IndicatorDescription))) == 0)
                  {
                     ControlIndicatorStatusIndicationCallParameter(HFREEventData, &ServerPortIndex, &Result);
                  }
                  break;
               case ciRange:
                  Display(("\r\nHFRE Control Indicator Status Indication, ID: 0x%04X\r\nDescription: %s\r\nValue      : %u\r\n", HFREEventData->Event_Data.HFRE_Control_Indicator_Status_Indication_Data->HFREPortID,
                                                                                                                                 HFREEventData->Event_Data.HFRE_Control_Indicator_Status_Indication_Data->HFREControlIndicatorEntry.IndicatorDescription,
                                                                                                                                 HFREEventData->Event_Data.HFRE_Control_Indicator_Status_Indication_Data->HFREControlIndicatorEntry.Control_Indicator_Data.ControlIndicatorRangeType.CurrentIndicatorValue));

                  /* Checks if the event parameter description is       */
                  /* "CALLSETUP"                                        */
                  if(0 == BTPS_MemCompare(HFREEventData->Event_Data.HFRE_Control_Indicator_Status_Indication_Data->HFREControlIndicatorEntry.IndicatorDescription,
                        "CALLSETUP", (BTPS_StringLength(HFREEventData->Event_Data.HFRE_Control_Indicator_Status_Indication_Data->HFREControlIndicatorEntry.IndicatorDescription))))
                  {
                      ControlIndicatorStatusIndicationCallSetupParameter(HFREEventData, &ServerPortIndex);
                  }
                  /* Checks if the event parameter description is       */
                  /* "CALLHELD"                                         */
                  else if(0 == BTPS_MemCompare(HFREEventData->Event_Data.HFRE_Control_Indicator_Status_Indication_Data->HFREControlIndicatorEntry.IndicatorDescription,
                        "CALLHELD", (BTPS_StringLength(HFREEventData->Event_Data.HFRE_Control_Indicator_Status_Indication_Data->HFREControlIndicatorEntry.IndicatorDescription))))
                  {
                      ControlIndicatorStatusIndicationCallHeldParameter(HFREEventData, &ServerPortIndex);
                  }
                  break;
            }
            DisplayConnectionStatus(ServerPortIndex);
            Display(("WBS_Connected_Port_ID: %d\r\n", WBS_Connected_Port_ID));
            Display(("NumberOfAnsweredCalls: %d\r\n", NumberOfAnsweredCalls()));
            break;
         case etHFRE_Control_Indicator_Status_Confirmation:
            /* A Control Indicator Status Confirmation was received,    */
            /* display all relevant information.                        */
            switch(HFREEventData->Event_Data.HFRE_Control_Indicator_Status_Confirmation_Data->HFREControlIndicatorEntry.ControlIndicatorType)
            {
               case ciBoolean:
                  Display(("\r\nHFRE Control Indicator Status Confirmation, ID: 0x%04X\r\nDescription: %s\r\nValue     : %s.\r\n", HFREEventData->Event_Data.HFRE_Control_Indicator_Status_Confirmation_Data->HFREPortID,
                                                                                                                      HFREEventData->Event_Data.HFRE_Control_Indicator_Status_Confirmation_Data->HFREControlIndicatorEntry.IndicatorDescription,
                                                                                                                      (HFREEventData->Event_Data.HFRE_Control_Indicator_Status_Confirmation_Data->HFREControlIndicatorEntry.Control_Indicator_Data.ControlIndicatorBooleanType.CurrentIndicatorValue)?"TRUE":"FALSE"));
                  break;
               case ciRange:
                  Display(("\r\nHFRE Control Indicator Status Confirmation, ID: 0x%04X\r\nDescription: %s\r\nValue     : %u.\r\n", HFREEventData->Event_Data.HFRE_Control_Indicator_Status_Confirmation_Data->HFREPortID,
                                                                                                                      HFREEventData->Event_Data.HFRE_Control_Indicator_Status_Confirmation_Data->HFREControlIndicatorEntry.IndicatorDescription,
                                                                                                                      HFREEventData->Event_Data.HFRE_Control_Indicator_Status_Confirmation_Data->HFREControlIndicatorEntry.Control_Indicator_Data.ControlIndicatorRangeType.CurrentIndicatorValue));
                  break;
            }
            DisplayConnectionStatus(ServerPortIndex);
            break;
         case etHFRE_Call_Hold_Multiparty_Support_Confirmation:
            /* A Call Hold and Multiparty Support Confirmation was      */
            /* received, display all relevant information.              */
            Display(("\r\nHFRE Call Hold Multiparty Support Confirmation, ID: 0x%04X\r\nSupport Mask: 0x%08lX.\r\n", HFREEventData->Event_Data.HFRE_Call_Hold_Multiparty_Support_Confirmation_Data->HFREPortID,
                                                                                                               HFREEventData->Event_Data.HFRE_Call_Hold_Multiparty_Support_Confirmation_Data->CallHoldSupportMask));
            DisplayConnectionStatus(ServerPortIndex);
            break;
         case etHFRE_Call_Waiting_Notification_Indication:
            /* A Call Waiting Notification Indication was received,     */
            /* display all relevant information.                        */
            Display(("\r\nHFRE Call Waiting Notification Indication, ID: 0x%04X\r\nPhone Number %s.\r\n", HFREEventData->Event_Data.HFRE_Call_Waiting_Notification_Indication_Data->HFREPortID,
                                                                                                    (HFREEventData->Event_Data.HFRE_Call_Waiting_Notification_Indication_Data->PhoneNumber)?HFREEventData->Event_Data.HFRE_Call_Waiting_Notification_Indication_Data->PhoneNumber:"<None>"));
            break;
         case etHFRE_Call_Line_Identification_Notification_Indication:
            /* A Call Line Identification Notification Indication was   */
            /* received, display all relevant information.              */
            Display(("\r\nHFRE Call Line Identification Notification Indication, ID: 0x%04X\r\nPhone Number %s.\r\n", HFREEventData->Event_Data.HFRE_Call_Line_Identification_Notification_Indication_Data->HFREPortID,
                                                                                                                      HFREEventData->Event_Data.HFRE_Call_Line_Identification_Notification_Indication_Data->PhoneNumber));
            break;
         case etHFRE_Ring_Indication:
            /* A Ring Indication was received, display all relevant     */
            /* information.                                             */
            Display(("\r\nHFRE Ring Indication, ID: 0x%04X.\r\n", HFREEventData->Event_Data.HFRE_Ring_Indication_Data->HFREPortID));
            break;
         case etHFRE_InBand_Ring_Tone_Setting_Indication:
            /* An InBand Ring Tone Setting Indication was received,     */
            /* display all relevant information.                        */
            Display(("\r\nHFRE InBand Ring Tone Setting Indication, ID: 0x%04X\r\nEnabled: %s.\r\n", HFREEventData->Event_Data.HFRE_InBand_Ring_Tone_Setting_Indication_Data->HFREPortID,
                                                                                                   (HFREEventData->Event_Data.HFRE_InBand_Ring_Tone_Setting_Indication_Data->Enabled)?"TRUE":"FALSE"));
            break;
         case etHFRE_Voice_Tag_Request_Indication:
            /* A Voice Tag Request Indication was received, display all */
            /* relevant information.                                    */
            Display(("\r\nHFRE Voice Tag Request Indication, ID: 0x%04X.\r\n", HFREEventData->Event_Data.HFRE_Voice_Tag_Request_Indication_Data->HFREPortID));
            break;
         case etHFRE_Voice_Tag_Request_Confirmation:
            /* A Voice Tag Request Confirmation was received, display   */
            /* all relevant information.                                */
            if(HFREEventData->Event_Data.HFRE_Voice_Tag_Request_Confirmation_Data->PhoneNumber)
            {
               Display(("\r\nHFRE Voice Tag Request Confirmation, ID: 0x%04X\r\nPhone Number %s.\r\n", HFREEventData->Event_Data.HFRE_Voice_Tag_Request_Confirmation_Data->HFREPortID,
                                                                                                       HFREEventData->Event_Data.HFRE_Voice_Tag_Request_Confirmation_Data->PhoneNumber));
            }
            else
            {
               Display(("\r\nHFRE Voice Tag Request Confirmation, ID: 0x%04X\r\nRequest Rejected.\r\n", HFREEventData->Event_Data.HFRE_Voice_Tag_Request_Confirmation_Data->HFREPortID));
            }
            break;
         case etHFRE_Close_Port_Indication:
            /* A Close Port Indication was received, display all        */
            /* relevant information.                                    */
            Display(("\r\nHFRE Close Port Indication, ID: 0x%04X\r\nStatus: 0x%04X.\r\n", HFREEventData->Event_Data.HFRE_Close_Port_Indication_Data->HFREPortID,
                                                                                          HFREEventData->Event_Data.HFRE_Close_Port_Indication_Data->PortCloseStatus));

            ClosePortIndication(&ServerPortIndex);

            /* Clear all data associated with this port excluding the   */
            /* port ID.                                                 */
            TempHFServerPortID = HFServerPortDATA[ServerPortIndex].HFServerPortID;

            BTPS_MemInitialize(&HFServerPortDATA[ServerPortIndex], 0, sizeof(HFServerPortDATA[ServerPortIndex]));

            HFServerPortDATA[ServerPortIndex].HFServerPortID = TempHFServerPortID;

            break;
         case etHFRE_Audio_Connection_Indication:
            /* An Audio Connection Indication was received, display all */
            /* relevant information.                                    */
            Display(("\r\nHFRE Audio Connection Indication, SCO/eSCO handle: 0x%x HFP ID: 0x%04X, Status: 0x%04X.\r\n",
                     HFREEventData->Event_Data.HFRE_Audio_Connection_Indication_Data->SCO_Connection_Handle,
                     HFREEventData->Event_Data.HFRE_Audio_Connection_Indication_Data->HFREPortID,
                     HFREEventData->Event_Data.HFRE_Audio_Connection_Indication_Data->AudioConnectionOpenStatus));

            AudioConnectionIndication(HFREEventData, &ServerPortIndex, &ConnectionHandle, &Result);
            DisplayConnectionStatus(ServerPortIndex);
            break;
         case etHFRE_Audio_Disconnection_Indication:
            /* An Audio Disconnection Indication was received, display  */
            /* all relevant information.                                */
            Display(("\r\nHFRE Audio Disconnection Indication\r\nID: 0x%04X.\r\n", HFREEventData->Event_Data.HFRE_Audio_Disconnection_Indication_Data->HFREPortID));

            AudioDisconnectionIndication(ServerPortIndex);

            break;
         case etHFRE_Subscriber_Number_Information_Indication:
            Display(("\r\nHFRE Subscriber Number Information Indication, ID: 0x%04X.\r\n", HFREEventData->Event_Data.HFRE_Subscriber_Number_Information_Indication_Data->HFREPortID));
            break;
         case etHFRE_Subscriber_Number_Information_Confirmation:
            Display(("\r\nHFRE Subscriber Number Information Confirmation, ID: 0x%04X.\r\n", HFREEventData->Event_Data.HFRE_Subscriber_Number_Information_Confirmation_Data->HFREPortID));

            Display(("+CNUM: SvcType: %d \r\nFormat: %d \r\nNum: %s\r\n", HFREEventData->Event_Data.HFRE_Subscriber_Number_Information_Confirmation_Data->ServiceType,
                                                                  HFREEventData->Event_Data.HFRE_Subscriber_Number_Information_Confirmation_Data->NumberFormat,
                                                                  HFREEventData->Event_Data.HFRE_Subscriber_Number_Information_Confirmation_Data->PhoneNumber));
            break;
         case etHFRE_Response_Hold_Status_Confirmation:
            Display(("\r\nHFRE Response Hold Status Confirmation, ID: 0x%04X \r\nCallState: %d.\r\n", HFREEventData->Event_Data.HFRE_Response_Hold_Status_Confirmation_Data->HFREPortID,
                                                                                                      HFREEventData->Event_Data.HFRE_Response_Hold_Status_Confirmation_Data->CallState));
            break;
         case etHFRE_Incoming_Call_State_Indication:
            Display(("\r\nHFRE Incoming Call State Indication, ID: 0x%04X \r\nCallState: %d.\r\n", HFREEventData->Event_Data.HFRE_Incoming_Call_State_Indication_Data->HFREPortID,
                                                                                                   HFREEventData->Event_Data.HFRE_Incoming_Call_State_Indication_Data->CallState));
            break;
         case etHFRE_Incoming_Call_State_Confirmation:
            Display(("\r\nHFRE Incoming Call State Confirmation, ID: 0x%04X \r\nCallState: %d.\r\n", HFREEventData->Event_Data.HFRE_Incoming_Call_State_Confirmation_Data->HFREPortID,
                                                                                                     HFREEventData->Event_Data.HFRE_Incoming_Call_State_Confirmation_Data->CallState));
            break;
         case etHFRE_Command_Result:
            /* An Command Confirmation was received, display the        */
            /* relevant information.                                    */
            Display(("\r\nHFRE Command Result, ID: 0x%04X \r\nType %d \r\nCode %d.\r\n", HFREEventData->Event_Data.HFRE_Command_Result_Data->HFREPortID,
                                                                                         HFREEventData->Event_Data.HFRE_Command_Result_Data->ResultType,
                                                                                         HFREEventData->Event_Data.HFRE_Command_Result_Data->ResultValue));
            break;
         case etHFRE_Codec_Select_Request_Indication:
            Display(("\r\n\r\n"
                     "etHFRE_Codec_Select_Indication:\r\n"
                     "  PortID:   0x%04X\r\n"
                     "  Codec ID: %d\r\n\r\n",
                     HFREEventData->Event_Data.HFRE_Codec_Select_Indication_Data->HFREPortID,
                     HFREEventData->Event_Data.HFRE_Codec_Select_Indication_Data->CodecID));

            CodecSelectRequestIndication(HFREEventData, &ServerPortIndex, &ConnectionHandle, &BoardStr, &Result);
            break;
         default:
            /* An unknown/unexpected HFRE event was received.           */
            Display(("\r\nUnknown HFRE Event Received: %d.\r\n", HFREEventData->Event_Data_Type));
            break;
      }

      DisplayPrompt();
   }
   else
   {
      /* There was an error with one or more of the input parameters.   */
      Display(("\r\nHFRE callback data: Event_Data = NULL.\r\n"));
   }
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
   int  ret_val = APPLICATION_ERROR_UNABLE_TO_OPEN_STACK;

   /* Next, makes sure that the Driver Information passed appears to be */
   /* semi-valid.                                                       */
   if((HCI_DriverInformation) && (BTPS_Initialization))
   {
      /* Try to Open the stack and check if it was successful.          */
      if(!OpenStack(HCI_DriverInformation, BTPS_Initialization))
      {
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
                  /* Set up the Selection Interface.                    */
                  UserInterface();

                  /* Initialize the Server Data Array in order to be    */
                  /* able to save portID and SDP handles and Initialize */
                  /* the LinkKeyInfo array.                             */
                  BTPS_MemInitialize(HFServerPortDATA, 0, sizeof(HFServerPortDATA));
                  BTPS_MemInitialize(LinkKeyInfo, 0, sizeof(LinkKeyInfo));

                  /* Display a list of available commands.              */
                  DisplayHelp(NULL);

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
            DisplayFunctionError("SetConnect", ret_val);

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
   Display(("\r\nHFP HF>"));
}

   /* The following function is used to process a command line string.  */
   /* This function takes as it's only parameter the command line string*/
   /* to be parsed and returns TRUE if a command was parsed and executed*/
   /* or FALSE otherwise.                                               */
Boolean_t ProcessCommandLine(char *String)
{
   return(CommandLineInterpreter(String));
}

