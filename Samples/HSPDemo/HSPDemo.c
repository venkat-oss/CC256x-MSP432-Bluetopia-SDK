/*****< HDSETDemo.c >**********************************************************/
/*      Copyright 2003 - 2014 Stonestreet One.                                */
/*      All Rights Reserved.                                                  */
/*                                                                            */
/*      Copyright 2015 Texas Instruments Incorporated.                        */
/*      All Rights Reserved.                                                  */
/*                                                                            */
/*   HDSETDemo - Simple embedded application                                  */
/*                  displaying Headset Profile Implementation.                */
/*  Author:  x0073106                                                         */
/*                                                                            */
/*** MODIFICATION HISTORY *****************************************************/
/*                                                                            */
/*   mm/dd/yy  F. Lastname    Description of Modification                     */
/*   --------  -----------    ------------------------------------------------*/
/*   03/10/15  x0073106       Initial creation.                               */
/*   03/03/15  D. Horowitz    Adding Demo Application version.                */
/******************************************************************************/
#include <stdio.h>         /* Included for sscanf.                            */
#include "Main.h"          /* Application Interface Abstraction.              */
#include "HSPDemo.h"       /* Application Header.                             */
#include "SS1BTPS.h"       /* Includes for the SS1 Bluetooth Protocol Stack.  */
#include "SS1BTHDS.h"      /* Bluetooth HFRE API Prototypes/Constants.        */
#include "SS1BTVS.h"       /* Vendor Specific Prototypes/Constants.           */
#include "BTPSKRNL.h"      /* BTPS Kernel Header.                             */
#include "HAL.h"           /* Hardware Abstraction Layer Header.              */

#define MAX_SUPPORTED_COMMANDS                     (32)  /* Denotes the       */
                                                         /* maximum number of */
                                                         /* User Commands that*/
                                                         /* are supported by  */
                                                         /* this application. */

#define MAX_NUM_OF_PARAMETERS                       (6)  /* Denotes the max   */
                                                         /* number of         */
                                                         /* parameters a      */
                                                         /* command can have. */

#define LOCAL_SERVER_CHANNEL_ID                     (1)  /* Denotes the Server*/
                                                         /* Port ID to use    */
                                                         /* when opening      */
                                                         /* Headset/Audio     */
                                                         /* Gateway Server    */
                                                         /* Ports.            */

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

#define DEFAULT_MITM_PROTECTION                 (FALSE)  /* Denotes the       */
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

#define INDENT_LENGTH                               (3)  /* Denotes the number*/
                                                         /* of character      */
                                                         /* spaces to be used */
                                                         /* for indenting when*/
                                                         /* displaying SDP    */
                                                         /* Data Elements.    */

#define EXIT_MODE                                 (-10)  /* Flags exit from   */
                                                         /* any Mode.         */

#define CVSD_FREQUENCY                           (8000)  /* 8KHz sampling     */
                                                         /* rate.             */

   /* The following represent the possible values of UI_Mode variable.  */
#define UI_MODE_IS_CLIENT      (2)
#define UI_MODE_IS_SERVER      (1)
#define UI_MODE_SELECT         (0)
#define UI_MODE_IS_INVALID     (-1)

   /* The following converts an ASCII character to an integer value.    */
#define ToInt(_x)                                  (((_x) > 0x39)?((_x)-0x37):((_x)-0x30))

   /* Determine the Name we will use for this compilation.              */
#define APP_DEMO_NAME                          "HSPDemo"


    /* The following type definition represents the Connection Status   */
    /* of an Active Connection (Headset or Audio Gateway).              */
typedef enum
{
    csConnected,
    csAudioConnected,
    csDisconnected
} ConnectionState_t;


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


   /* The following type definition represents the container type which */
   /* holds the mapping between Profile UUIDs and Profile Names (UUID   */
   /* <-> Name).                                                        */
typedef struct _tagUUIDInfo_t
{
   char       *Name;
   UUID_128_t  UUID;
} UUIDInfo_t;


   /* User to represent a structure to hold a BD_ADDR return from       */
   /* BD_ADDRToStr.                                                     */
typedef char BoardStr_t[16];

   /* Internal Variables to this Module (Remember that all variables    */
   /* declared static are initialized to 0 automatically by the         */
   /* compiler as part of standard C/C++).                              */

static unsigned int       HDSClientID;             /* Variable used to hold the       */
                                                   /* Headset/Audio Gateway Client ID */
                                                   /* of the currently opened Remote  */
                                                   /* Headset/Audio Gateway Server    */
                                                   /* (or Local Headset/Audio Gateway */
                                                   /* Client).                        */

static unsigned int       HDSServerID;             /* Variable used to hold the       */
                                                   /* Headset/Audio Gateway Server ID */
                                                   /* of the currently opened Headset */
                                                   /* or Audio Gateway Server.        */

static DWord_t            HDSServerSDPHandle;      /* Variable used to hold the       */
                                                   /* Headset/Audio Gateway SDP       */
                                                   /* Service Record of the Headset or */
                                                   /* Audio Gateway Server SDP Service*/
                                                   /* Record.                         */

static Boolean_t          ServerConnected;         /* Variable which contains the     */
                                                   /* current connection state of the */
                                                   /* HFRE Server.                    */


static ConnectionState_t   CurrentConnectionState;  /* Variable used to hold the       */
                                                    /* current connection status state.*/


static int                 UI_Mode;                 /* Holds the UI Mode.              */

static unsigned int        BluetoothStackID;        /* Variable which holds the Handle */
                                                    /* of the opened Bluetooth Protocol*/
                                                    /* Stack.                          */

static BD_ADDR_t           InquiryResultList[MAX_INQUIRY_RESULTS];  /* Variable which  */
                                                    /* contains the inquiry result     */
                                                    /* received from the most recently */
                                                    /* preformed inquiry.              */

static unsigned int        NumberofValidResponses;  /* Variable which holds the number */
                                                    /* of valid inquiry results within */
                                                    /* the inquiry results array.      */

static LinkKeyInfo_t       LinkKeyInfo[1];          /* Variable which holds the list of*/
                                                    /* BD_ADDR <-> Link Keys for       */
                                                    /* pairing.                        */

static BD_ADDR_t           CurrentRemoteBD_ADDR;    /* Variable which holds the        */
                                                    /* current BD_ADDR of the device   */
                                                    /* which is currently pairing or   */
                                                    /* authenticating.                 */

static BD_ADDR_t           ConnectedBD_ADDR;        /* Variable which holds the        */
                                                    /* current BD_ADDR of the currently*/
                                                    /* connected AG.                   */

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

static UUIDInfo_t UUIDTable[] =
{
   { "L2CAP",                 { 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB } },
   { "Advanced Audio",        { 0x00, 0x00, 0x11, 0x0D, 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB } },
   { "A/V Remote Control",    { 0x00, 0x00, 0x11, 0x0E, 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB } },
   { "Basic Imaging",         { 0x00, 0x00, 0x11, 0x1A, 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB } },
   { "Basic Printing",        { 0x00, 0x00, 0x11, 0x22, 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB } },
   { "Dial-up Networking",    { 0x00, 0x00, 0x11, 0x03, 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB } },
   { "FAX",                   { 0x00, 0x00, 0x11, 0x11, 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB } },
   { "File Transfer",         { 0x00, 0x00, 0x11, 0x06, 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB } },
   { "Hard Copy Cable Repl.", { 0x00, 0x00, 0x11, 0x25, 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB } },
   { "Health Device",         { 0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB } },
   { "Headset",               { 0x00, 0x00, 0x11, 0x08, 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB } },
   { "Audio gateway",         { 0x00, 0x00, 0x11, 0x12, 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB } },
   { "HID",                   { 0x00, 0x00, 0x11, 0x24, 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB } },
   { "LAN Access",            { 0x00, 0x00, 0x11, 0x02, 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB } },
   { "Message Access",        { 0x00, 0x00, 0x11, 0x34, 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB } },
   { "Object Push",           { 0x00, 0x00, 0x11, 0x05, 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB } },
   { "Personal Area Network", { 0x00, 0x00, 0x00, 0x0F, 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB } },
   { "Phonebook Access",      { 0x00, 0x00, 0x11, 0x30, 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB } },
   { "SIM Access",            { 0x00, 0x00, 0x11, 0x2D, 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB } },
   { "Serial Port",           { 0x00, 0x00, 0x11, 0x01, 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB } },
   { "IrSYNC",                { 0x00, 0x00, 0x11, 0x04, 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB } }
} ;

#define NUM_UUIDS                               (sizeof(UUIDTable)/sizeof(UUIDInfo_t))


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
static Boolean_t CommandLineInterpreter(char *Command);

static unsigned long StringToUnsignedInteger(char *StringInteger);
static char *StringParser(char *String);
static int CommandParser(UserCommand_t *TempCommand, char *UserInput);
static int CommandInterpreter(UserCommand_t *TempCommand);
static int AddCommand(char *CommandName, CommandFunction_t CommandFunction);
static CommandFunction_t FindCommand(char *Command);
static void ClearCommands(void);

static void BD_ADDRToStr(BD_ADDR_t Board_Address, char *BoardStr);
static void DisplayFunctionError(char *Function, int Status);

static int DisplayHelp(ParameterList_t *TempParam);
static void DisplayFWVersion (void);

static int OpenStack(HCI_DriverInformation_t *HCI_DriverInformation, BTPS_Initialization_t *BTPS_Initialization);
static int CloseStack(void);

static int SetDisc(void);
static int SetConnect(void);
static int SetPairable(void);
static int DeleteLinkKey(BD_ADDR_t BD_ADDR);



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

static int OpenRemoteServer(ParameterList_t *TempParam);
static int CloseRemoteServer(ParameterList_t *TempParam);
static int PressButton(ParameterList_t *TempParam);
static int ChangeSpeakerGain(ParameterList_t *TempParam);
static int ChangeMicrophoneGain(ParameterList_t *TempParam);
static int RingIndication(ParameterList_t *TempParam);

static int ManageAudioConnection(ParameterList_t *TempParam);
static int SetupAudioConnection(void);
static int ReleaseAudioConnection(void);


static int OpenServer(ParameterList_t *TempParam);
static int CloseServer(ParameterList_t *TempParam);


static int HeadsetMode(ParameterList_t *TempParam);
static int AudioGatewayMode(ParameterList_t *TempParam);

static int ServiceDiscovery(ParameterList_t *TempParam);
static void DisplaySDPAttributeResponse(SDP_Service_Attribute_Response_Data_t *SDPServiceAttributeResponse, unsigned int InitLevel);
static void DisplaySDPSearchAttributeResponse(SDP_Service_Search_Attribute_Response_Data_t *SDPServiceSearchAttributeResponse);
static void DisplayDataElement(SDP_Data_Element_t *SDPDataElement, unsigned int Level);
static int QueryMemory(ParameterList_t *TempParam);


   /* Callback Function Prototypes.                                     */
static void BTPSAPI GAP_Event_Callback(unsigned int BluetoothStackID, GAP_Event_Data_t *GAPEventData, unsigned long CallbackParameter);
static void BTPSAPI SDP_Event_Callback(unsigned int BluetoothStackID, unsigned int SDPRequestID, SDP_Response_Data_t *SDP_Response_Data, unsigned long CallbackParameter);
static void BTPSAPI HDSET_Event_Callback(unsigned int BluetoothStackID, HDSET_Event_Data_t *HDSET_Event_Data, unsigned long CallbackParameter);

   /* This function is responsible for taking the users input and do the*/
   /* appropriate thing with it.  First, this function get a string of  */
   /* user input, parse the user input in to command and parameters, and*/
   /* finally executing the command or display an error message if the  */
   /* input is corrupt.                                                 */
static void UserInterface_Server(void)
{
    /* Next display the available commands.                             */
    DisplayHelp(NULL);

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
   AddCommand("SERVICEDISCOVERY", ServiceDiscovery);
   AddCommand("OPENSERVER", OpenServer);
   AddCommand("CLOSESERVER", CloseServer);
   AddCommand("PRESSBUTTON", PressButton);
   AddCommand("CHANGESPEAKERGAIN", ChangeSpeakerGain);
   AddCommand("CHANGEMICROPHONEGAIN", ChangeMicrophoneGain);
   AddCommand("HELP", DisplayHelp);
   AddCommand("QUERYMEMORY", QueryMemory);
}

static void UserInterface_Client(void)
{
     /* Next display the available commands.                            */
     DisplayHelp(NULL);

    /* First Clear all of the commands in the Command Table.            */
    ClearCommands();

    /* Now add all of the commands that are associated with the HID     */
    /* Device unit to the Command Table.                                */
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
    AddCommand("SERVICEDISCOVERY", ServiceDiscovery);
    AddCommand("OPENCLIENT", OpenRemoteServer);
    AddCommand("CLOSECLIENT", CloseRemoteServer);
    AddCommand("RINGINDICATION", RingIndication);
    AddCommand("CHANGESPEAKERGAIN", ChangeSpeakerGain);
    AddCommand("CHANGEMICROPHONEGAIN", ChangeMicrophoneGain);
    AddCommand("MANAGEAUDIO", ManageAudioConnection);
    AddCommand("HELP", DisplayHelp);
    AddCommand("QUERYMEMORY", QueryMemory);
}


   /* The following function is responsible for choosing the user       */
   /* interface to present to the user.                                 */
static void UserInterface_Selection(void)
{
         /* Next display the available commands.                        */
    DisplayHelp(NULL);

    ClearCommands();

    AddCommand("HEADSET", HeadsetMode);
    AddCommand("AUDIOGATEWAY", AudioGatewayMode);
    AddCommand("HELP", DisplayHelp);
    AddCommand("QUERYMEMORY", QueryMemory);
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
             /* If the user has request to exit we might as well go     */
             /* ahead an close any Remote Audio Gateway Server that we  */
             /* have open.                                              */
             if(HDSClientID)
                CloseRemoteServer(NULL);

             /* Close the Local Headset Server that we have open.       */
             /* CloseServer();                                          */

            /* Restart the User Interface Selection.                    */
            UI_Mode = UI_MODE_SELECT;

            /* Set up the Selection Interface.                          */
            UserInterface_Selection();
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
        /* There was an error with HCI_Read_Local_Version_Information.  */
        /* Function.                                                    */
        DisplayFunctionError("HCI_Read_Local_Version_Information", FW_Version_Details.StatusResult);
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
      if(HCI_DriverInformation)
      {
         /* Initialize BTPSKNRL.                                        */
         BTPS_Init((void *)BTPS_Initialization);

         Display(("\r\nOpenStack().\r\n"));

         /* Initialize the Stack.                                       */
         Result = BSC_Initialize(HCI_DriverInformation, 0);

         /* Next, check the return value of the initialization to see if*/
         /* it was successful.                                          */
         if(Result > 0)
         {
            /* The Stack was initialized successfully, inform the user  */
            /* and set the return value of the initialization function  */
            /* to the Bluetooth Stack ID.                               */
            if(HCI_DriverInformation->DriverType == hdtUSB)
               Display(("Stack Initialization on USB Successful.\r\n"));
            else
               Display(("Stack Initialization on Port %d %ld (%s) Successful.\r\n", HCI_DriverInformation->DriverInformation.COMMDriverInformation.COMPortNumber, HCI_DriverInformation->DriverInformation.COMMDriverInformation.BaudRate, ((HCI_DriverInformation->DriverInformation.COMMDriverInformation.Protocol == cpBCSP)?"BCSP":"UART")));

            BluetoothStackID = Result;
            Display(("Bluetooth Stack ID: %d\r\n", BluetoothStackID));

            ret_val          = 0;

            /* Initialize the default Secure Simple Pairing parameters. */
            IOCapability     = DEFAULT_IO_CAPABILITY;
            OOBSupport       = FALSE;
            MITMProtection   = DEFAULT_MITM_PROTECTION;

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
               HCI_Write_Default_Link_Policy_Settings(BluetoothStackID, HCI_LINK_POLICY_SETTINGS_ENABLE_MASTER_SLAVE_SWITCH|HCI_LINK_POLICY_SETTINGS_ENABLE_SNIFF_MODE, &Status);

            /* Delete all Stored Link Keys.                             */
            ASSIGN_BD_ADDR(BD_ADDR, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);

            DeleteLinkKey(BD_ADDR);

            /* Set the Class of Device.                                 */
            ASSIGN_CLASS_OF_DEVICE(Class_of_Device, 0x00, 0x00, 0x00);
            SET_MAJOR_DEVICE_CLASS(Class_of_Device, HCI_LMP_CLASS_OF_DEVICE_MAJOR_DEVICE_CLASS_AUDIO_VIDEO);
            SET_MINOR_DEVICE_CLASS(Class_of_Device, HCI_LMP_CLASS_OF_DEVICE_MINOR_DEVICE_CLASS_AUDIO_VIDEO_HANDS_FREE);

            /* Write out the Class of Device.                           */
            GAP_Set_Class_Of_Device(BluetoothStackID, Class_of_Device);

            /* Write out the Class of Device.                           */
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
               Display(("Stack Initialization on Port %d %ld (%s) Failed: %d.\r\n", HCI_DriverInformation->DriverInformation.COMMDriverInformation.COMPortNumber, HCI_DriverInformation->DriverInformation.COMMDriverInformation.BaudRate, ((HCI_DriverInformation->DriverInformation.COMMDriverInformation.Protocol == cpBCSP)?"BCSP":"UART"), Result));

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
      /* Individual Link Key.  Go ahead and see if know about the entry */
      /* in the list.                                                   */
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

   /* The following function is responsible for displaying the current  */
   /* Command Options for either Serial Port Client or Serial Port      */
   /* Server.  The input parameter to this function is completely       */
   /* ignored, and only needs to be passed in because all Commands that */
   /* can be entered at the Prompt pass in the parsed information.  This*/
   /* function displays the current Command Options that are available  */
   /* and always returns zero.                                          */
static int DisplayHelp(ParameterList_t *TempParam)
{
  if(UI_Mode == UI_MODE_IS_CLIENT)
  {
     Display(("\r\n"));
        Display(("******************************************************************\r\n"));
        Display(("* Command Options: Inquiry, DisplayInquiryList, Pair,            *\r\n"));
        Display(("*                  EndPairing, PINCodeResponse, PassKeyResponse, *\r\n"));
        Display(("*                  UserConfirmationResponse,                     *\r\n"));
        Display(("*                  SetDiscoverabilityMode, SetConnectabilityMode,*\r\n"));
        Display(("*                  SetPairabilityMode,                           *\r\n"));
        Display(("*                  ChangeSimplePairingParameters,                *\r\n"));
        Display(("*                  GetLocalAddress, GetLocalName, SetLocalName,  *\r\n"));
        Display(("*                  GetClassOfDevice, SetClassOfDevice,           *\r\n"));
        Display(("*                  GetRemoteName, ServiceDiscovery,              *\r\n"));
        Display(("*                  OpenClient, CloseClient,                      *\r\n"));
        Display(("*                  RingIndication, ChangeSpeakerGain,            *\r\n"));
        Display(("*                  ChangeMicrophoneGain,                         *\r\n"));
        Display(("*                  ManageAudio,                                  *\r\n"));
        Display(("*                  Help, Quit                                    *\r\n"));
        Display(("******************************************************************\r\n"));
  }
  else
  {
     if(UI_Mode == UI_MODE_IS_SERVER)
     {
        Display(("\r\n"));
        Display(("******************************************************************\r\n"));
        Display(("* Command Options: Inquiry, DisplayInquiryList, Pair,            *\r\n"));
        Display(("*                  EndPairing, PINCodeResponse, PassKeyResponse, *\r\n"));
        Display(("*                  UserConfirmationResponse,                     *\r\n"));
        Display(("*                  SetDiscoverabilityMode, SetConnectabilityMode,*\r\n"));
        Display(("*                  SetPairabilityMode,                           *\r\n"));
        Display(("*                  ChangeSimplePairingParameters,                *\r\n"));
        Display(("*                  GetLocalAddress, GetLocalName, SetLocalName,  *\r\n"));
        Display(("*                  GetClassOfDevice, SetClassOfDevice,           *\r\n"));
        Display(("*                  GetRemoteName, ServiceDiscovery,              *\r\n"));
        Display(("*                  OpenServer, CloseServer,                      *\r\n"));
        Display(("*                  PressButton, ChangeSpeakerGain,               *\r\n"));
        Display(("*                  ChangeMicrophoneGain,                         *\r\n"));
        Display(("*                  Help, Quit                                    *\r\n"));
        Display(("******************************************************************\r\n"));
     }
     else
     {
        Display(("\r\n"));
        Display(("******************************************************************\r\n"));
        Display(("* Command Options: Headset, AudioGateway, Help                   *\r\n"));
        Display(("******************************************************************\r\n"));
     }
  }

  return(0);
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
      /* Next, make sure that we are not already connected.             */
      if(COMPARE_NULL_BD_ADDR(ConnectedBD_ADDR))
      {
         /* There is no currently active connection, make sure that all */
         /* of the parameters required for this function appear to be at*/
         /* least semi-valid.                                           */
         if((TempParam) && (TempParam->NumberofParameters > 0) && (TempParam->Params[0].intParam) && (NumberofValidResponses) && (TempParam->Params[0].intParam <= NumberofValidResponses) && (!COMPARE_BD_ADDR(InquiryResultList[(TempParam->Params[0].intParam - 1)], NullADDR)))
         {
            /* Check to see if General Bonding was specified.           */
            if(TempParam->NumberofParameters > 1)
               BondingType = TempParam->Params[1].intParam?btGeneral:btDedicated;
            else
               BondingType = btDedicated;

            /* Before we submit the command to the stack, we need to    */
            /* make sure that we clear out any Link Key we have stored  */
            /* for the specified device.                                */
            DeleteLinkKey(InquiryResultList[(TempParam->Params[0].intParam - 1)]);

            /* Attempt to submit the command.                           */
            Result = GAP_Initiate_Bonding(BluetoothStackID, InquiryResultList[(TempParam->Params[0].intParam - 1)], BondingType, GAP_Event_Callback, (unsigned long)0);

            /* Check the return value of the submitted command for      */
            /* success.                                                 */
            if(!Result)
            {
               /* Display a message indicating that Bonding was         */
               /* initiated successfully.                               */
               Display(("GAP_Initiate_Bonding (%s): Function Successful.\r\n", (BondingType == btDedicated)?"Dedicated":"General"));

               /* Flag success to the caller.                           */
               ret_val = 0;
            }
            else
            {
               /* Display a message indicating that an error occurred   */
               /* while initiating bonding.                             */
               Display(("GAP_Initiate_Bonding() Failure: %d.\r\n", Result));

               ret_val = FUNCTION_ERROR;
            }
         }
         else
         {
            /* One or more of the necessary parameters is/are invalid.  */
            Display(("Usage: Pair [Inquiry Index] [Bonding Type (0 = Dedicated, 1 = General) (optional).\r\n"));

            ret_val = INVALID_PARAMETERS_ERROR;
         }
      }
      else
      {
         /* Display an error to the user describing that Pairing can    */
         /* only occur when we are not connected.                       */
         Display(("The Pair command can only be issued when not already connected.\r\n"));

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


   /* The following function is responsible for issuing a Service Search*/
   /* Attribute Request to a Remote SDP Server.  This function returns  */
   /* zero if successful and a negative value if an error occurred.     */
static int ServiceDiscovery(ParameterList_t *TempParam)
{
   int                           Result;
   int                           ret_val;
   int                           Index;
   BD_ADDR_t         NullADDR;
   SDP_UUID_Entry_t              SDPUUIDEntry;
   SDP_Attribute_ID_List_Entry_t AttributeID;

   ASSIGN_BD_ADDR(NullADDR, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);

   /* First, check that valid Bluetooth Stack ID exists.                */
   if(BluetoothStackID)
   {
      /* Now let's make sure that all of the parameters required for    */
      /* this function appear to be at least semi-valid.                */
      if((TempParam) && (TempParam->NumberofParameters > 1) && (((TempParam->Params[1].intParam) && (TempParam->Params[1].intParam <= NUM_UUIDS)) || ((!TempParam->Params[1].intParam) && (TempParam->NumberofParameters > 2))) && (TempParam->Params[0].intParam) && (NumberofValidResponses) && (TempParam->Params[0].intParam <= NumberofValidResponses) && (!COMPARE_BD_ADDR(InquiryResultList[(TempParam->Params[0].intParam - 1)], NullADDR)))
      {
         /* OK, parameters appear to be semi-valid, now let's attempt to*/
         /* issue the SDP Service Attribute Request.                    */
         if(!TempParam->Params[1].intParam)
         {
            /* First let's build the UUID 32 value(s).                  */
            SDPUUIDEntry.SDP_Data_Element_Type = deUUID_32;

            ASSIGN_SDP_UUID_32(SDPUUIDEntry.UUID_Value.UUID_32, ((TempParam->Params[2].intParam & 0xFF000000) >> 24), ((TempParam->Params[2].intParam & 0x00FF0000) >> 16), ((TempParam->Params[2].intParam & 0x0000FF00) >> 8), (TempParam->Params[2].intParam & 0x000000FF));
         }
         else
         {
            SDPUUIDEntry.SDP_Data_Element_Type = deUUID_128;

            SDPUUIDEntry.UUID_Value.UUID_128   = UUIDTable[TempParam->Params[1].intParam - 1].UUID;
         }

         AttributeID.Attribute_Range    = (Boolean_t)TRUE;
         AttributeID.Start_Attribute_ID = 0;
         AttributeID.End_Attribute_ID   = 65335;

         /* Finally submit the SDP Request.                             */
         Result = SDP_Service_Search_Attribute_Request(BluetoothStackID, InquiryResultList[(TempParam->Params[0].intParam - 1)], 1, &SDPUUIDEntry, 1, &AttributeID, SDP_Event_Callback, (unsigned long)0);

         if(Result > 0)
         {
            /* The SDP Request was submitted successfully.              */
            Display(("SDP_Service_Search_Attribute_Request(%s) Success.\r\n", TempParam->Params[1].intParam?UUIDTable[TempParam->Params[1].intParam - 1].Name:"Manual"));

            /* Flag success to the caller.                              */
            ret_val = 0;
         }
         else
         {
            /* There was an error submitting the SDP Request.           */
            Display(("SDP_Service_Search_Attribute_Request(%s) Failure: %d.\r\n", TempParam->Params[1].intParam?UUIDTable[TempParam->Params[1].intParam - 1].Name:"Manual", Result));

            /* Flag success to the caller.                              */
            ret_val = 0;
         }
      }
      else
      {
         /* One or more of the necessary parameters is/are invalid.     */
         Display(("Usage: SERVICEDISCOVERY [Inquiry Index] [Profile Index] [16/32 bit UUID (Manual only)].\r\n"));
         Display(("\r\n   Profile Index:\r\n"));
         Display(("       0) Manual (MUST specify 16/32 bit UUID)\r\n"));
         for(Index=0;Index<NUM_UUIDS;Index++)
            Display(("      %2d) %s\r\n", Index + 1, UUIDTable[Index].Name));
         Display(("\r\n"));

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

   /* The following function is responsible for Displaying the contents */
   /* of an SDP Service Attribute Response to the display.              */
static void DisplaySDPAttributeResponse(SDP_Service_Attribute_Response_Data_t *SDPServiceAttributeResponse, unsigned int InitLevel)
{
   int Index;

   /* First, check to make sure that there were Attributes returned.    */
   if(SDPServiceAttributeResponse->Number_Attribute_Values)
   {
      /* Loop through all returned SDP Attribute Values.                */
      for(Index = 0; Index < SDPServiceAttributeResponse->Number_Attribute_Values; Index++)
      {
         /* First Print the Attribute ID that was returned.             */
         Display(("Attribute ID 0x%04X\r\n", SDPServiceAttributeResponse->SDP_Service_Attribute_Value_Data[Index].Attribute_ID));

         /* Now Print out all of the SDP Data Elements that were        */
         /* returned that are associated with the SDP Attribute.        */
         DisplayDataElement(SDPServiceAttributeResponse->SDP_Service_Attribute_Value_Data[Index].SDP_Data_Element, (InitLevel + 1));
      }
   }
   else
      Display(("No SDP Attributes Found.\r\n"));
}

   /* The following function is responsible for displaying the contents */
   /* of an SDP Service Search Attribute Response to the display.       */
static void DisplaySDPSearchAttributeResponse(SDP_Service_Search_Attribute_Response_Data_t *SDPServiceSearchAttributeResponse)
{
   int Index;

   /* First, check to see if Service Records were returned.             */
   if(SDPServiceSearchAttributeResponse->Number_Service_Records)
   {
      /* Loop through all returned SDP Service Records.                 */
      for(Index = 0; Index < SDPServiceSearchAttributeResponse->Number_Service_Records; Index++)
      {
         /* First display the number of SDP Service Records we are      */
         /* currently processing.                                       */
         Display(("Service Record: %u:\r\n", (Index + 1)));

         /* Call Display SDPAttributeResponse for all SDP Service       */
         /* Records received.                                           */
         DisplaySDPAttributeResponse(&(SDPServiceSearchAttributeResponse->SDP_Service_Attribute_Response_Data[Index]), 1);
      }
   }
   else
      Display(("No SDP Service Records Found.\r\n"));
}

   /* The following function is responsible for actually displaying an  */
   /* individual SDP Data Element to the Display.  The Level Parameter  */
   /* is used in conjunction with the defined INDENT_LENGTH constant to */
   /* make readability easier when displaying Data Element Sequences    */
   /* and Data Element Alternatives.  This function will recursively    */
   /* call itself to display the contents of Data Element Sequences and */
   /* Data Element Alternatives when it finds these Data Types (and     */
   /* increments the Indent Level accordingly).                         */
static void DisplayDataElement(SDP_Data_Element_t *SDPDataElement, unsigned int Level)
{
   unsigned int Index;
   char         Buffer[256];

   switch(SDPDataElement->SDP_Data_Element_Type)
   {
      case deNIL:
         /* Display the NIL Type.                                       */
         Display(("%*s Type: NIL\r\n", (Level*INDENT_LENGTH), ""));
         break;
      case deNULL:
         /* Display the NULL Type.                                      */
         Display(("%*s Type: NULL\r\n", (Level*INDENT_LENGTH), ""));
         break;
      case deUnsignedInteger1Byte:
         /* Display the Unsigned Integer (1 Byte) Type.                 */
         Display(("Type: Unsigned Int = 0x%02X\r\n", SDPDataElement->SDP_Data_Element.UnsignedInteger1Byte));
         break;
      case deUnsignedInteger2Bytes:
         /* Display the Unsigned Integer (2 Bytes) Type.                */
         Display(("Type: Unsigned Int = 0x%04X\r\n", SDPDataElement->SDP_Data_Element.UnsignedInteger2Bytes));
         break;
      case deUnsignedInteger4Bytes:
         /* Display the Unsigned Integer (4 Bytes) Type.                */
         Display(("Type: Unsigned Int = 0x%08X\r\n", (unsigned int)SDPDataElement->SDP_Data_Element.UnsignedInteger4Bytes));
         break;
      case deUnsignedInteger8Bytes:
         /* Display the Unsigned Integer (8 Bytes) Type.                */
         Display(("Type: Unsigned Int = 0x%02X%02X%02X%02X%02X%02X%02X%02X\r\n", SDPDataElement->SDP_Data_Element.UnsignedInteger8Bytes[7],
                                                                                 SDPDataElement->SDP_Data_Element.UnsignedInteger8Bytes[6],
                                                                                 SDPDataElement->SDP_Data_Element.UnsignedInteger8Bytes[5],
                                                                                 SDPDataElement->SDP_Data_Element.UnsignedInteger8Bytes[4],
                                                                                 SDPDataElement->SDP_Data_Element.UnsignedInteger8Bytes[3],
                                                                                 SDPDataElement->SDP_Data_Element.UnsignedInteger8Bytes[2],
                                                                                 SDPDataElement->SDP_Data_Element.UnsignedInteger8Bytes[1],
                                                                                 SDPDataElement->SDP_Data_Element.UnsignedInteger8Bytes[0]));
         break;
      case deUnsignedInteger16Bytes:
         /* Display the Unsigned Integer (16 Bytes) Type.               */
         Display(("Type: Unsigned Int = 0x%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X\r\n", SDPDataElement->SDP_Data_Element.UnsignedInteger16Bytes[15],
                                                                                                                   SDPDataElement->SDP_Data_Element.UnsignedInteger16Bytes[14],
                                                                                                                   SDPDataElement->SDP_Data_Element.UnsignedInteger16Bytes[13],
                                                                                                                   SDPDataElement->SDP_Data_Element.UnsignedInteger16Bytes[12],
                                                                                                                   SDPDataElement->SDP_Data_Element.UnsignedInteger16Bytes[11],
                                                                                                                   SDPDataElement->SDP_Data_Element.UnsignedInteger16Bytes[10],
                                                                                                                   SDPDataElement->SDP_Data_Element.UnsignedInteger16Bytes[9],
                                                                                                                   SDPDataElement->SDP_Data_Element.UnsignedInteger16Bytes[8],
                                                                                                                   SDPDataElement->SDP_Data_Element.UnsignedInteger16Bytes[7],
                                                                                                                   SDPDataElement->SDP_Data_Element.UnsignedInteger16Bytes[6],
                                                                                                                   SDPDataElement->SDP_Data_Element.UnsignedInteger16Bytes[5],
                                                                                                                   SDPDataElement->SDP_Data_Element.UnsignedInteger16Bytes[4],
                                                                                                                   SDPDataElement->SDP_Data_Element.UnsignedInteger16Bytes[3],
                                                                                                                   SDPDataElement->SDP_Data_Element.UnsignedInteger16Bytes[2],
                                                                                                                   SDPDataElement->SDP_Data_Element.UnsignedInteger16Bytes[1],
                                                                                                                   SDPDataElement->SDP_Data_Element.UnsignedInteger16Bytes[0]));
         break;
      case deSignedInteger1Byte:
         /* Display the Signed Integer (1 Byte) Type.                   */
         Display(("Type: Signed Int = 0x%02X\r\n", SDPDataElement->SDP_Data_Element.SignedInteger1Byte));
         break;
      case deSignedInteger2Bytes:
         /* Display the Signed Integer (2 Bytes) Type.                  */
         Display(("Type: Signed Int = 0x%04X\r\n", SDPDataElement->SDP_Data_Element.SignedInteger2Bytes));
         break;
      case deSignedInteger4Bytes:
         /* Display the Signed Integer (4 Bytes) Type.                  */
         Display(("Type: Signed Int = 0x%08X\r\n", (unsigned int)SDPDataElement->SDP_Data_Element.SignedInteger4Bytes));
         break;
      case deSignedInteger8Bytes:
         /* Display the Signed Integer (8 Bytes) Type.                  */
         Display(("Type: Signed Int = 0x%02X%02X%02X%02X%02X%02X%02X%02X\r\n", SDPDataElement->SDP_Data_Element.SignedInteger8Bytes[7],
                                                                                 SDPDataElement->SDP_Data_Element.SignedInteger8Bytes[6],
                                                                                 SDPDataElement->SDP_Data_Element.SignedInteger8Bytes[5],
                                                                                 SDPDataElement->SDP_Data_Element.SignedInteger8Bytes[4],
                                                                                 SDPDataElement->SDP_Data_Element.SignedInteger8Bytes[3],
                                                                                 SDPDataElement->SDP_Data_Element.SignedInteger8Bytes[2],
                                                                                 SDPDataElement->SDP_Data_Element.SignedInteger8Bytes[1],
                                                                                 SDPDataElement->SDP_Data_Element.SignedInteger8Bytes[0]));
         break;
      case deSignedInteger16Bytes:
         /* Display the Signed Integer (16 Bytes) Type.                 */
         Display(("Type: Signed Int = 0x%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X\r\n", SDPDataElement->SDP_Data_Element.SignedInteger16Bytes[15],
                                                                                                                 SDPDataElement->SDP_Data_Element.SignedInteger16Bytes[14],
                                                                                                                 SDPDataElement->SDP_Data_Element.SignedInteger16Bytes[13],
                                                                                                                 SDPDataElement->SDP_Data_Element.SignedInteger16Bytes[12],
                                                                                                                 SDPDataElement->SDP_Data_Element.SignedInteger16Bytes[11],
                                                                                                                 SDPDataElement->SDP_Data_Element.SignedInteger16Bytes[10],
                                                                                                                 SDPDataElement->SDP_Data_Element.SignedInteger16Bytes[9],
                                                                                                                 SDPDataElement->SDP_Data_Element.SignedInteger16Bytes[8],
                                                                                                                 SDPDataElement->SDP_Data_Element.SignedInteger16Bytes[7],
                                                                                                                 SDPDataElement->SDP_Data_Element.SignedInteger16Bytes[6],
                                                                                                                 SDPDataElement->SDP_Data_Element.SignedInteger16Bytes[5],
                                                                                                                 SDPDataElement->SDP_Data_Element.SignedInteger16Bytes[4],
                                                                                                                 SDPDataElement->SDP_Data_Element.SignedInteger16Bytes[3],
                                                                                                                 SDPDataElement->SDP_Data_Element.SignedInteger16Bytes[2],
                                                                                                                 SDPDataElement->SDP_Data_Element.SignedInteger16Bytes[1],
                                                                                                                 SDPDataElement->SDP_Data_Element.SignedInteger16Bytes[0]));
         break;
      case deTextString:
         /* First retrieve the Length of the Text String so that we can */
         /* copy the Data into our Buffer.                              */
         Index = (SDPDataElement->SDP_Data_Element_Length < sizeof(Buffer))?SDPDataElement->SDP_Data_Element_Length:(sizeof(Buffer)-1);

         /* Copy the Text String into the Buffer and then NULL terminate*/
         /* it.                                                         */
         BTPS_MemCopy(Buffer, SDPDataElement->SDP_Data_Element.TextString, Index);
         Buffer[Index] = '\0';

         Display(("Type: Text String = %s\r\n", Buffer));
         break;
      case deBoolean:
         Display(("Type: Boolean = %s\r\n", (SDPDataElement->SDP_Data_Element.Boolean)?"TRUE":"FALSE"));
         break;
      case deURL:
         /* First retrieve the Length of the URL String so that we can  */
         /* copy the Data into our Buffer.                              */
         Index = (SDPDataElement->SDP_Data_Element_Length < sizeof(Buffer))?SDPDataElement->SDP_Data_Element_Length:(sizeof(Buffer)-1);

         /* Copy the URL String into the Buffer and then NULL terminate */
         /* it.                                                         */
         BTPS_MemCopy(Buffer, SDPDataElement->SDP_Data_Element.URL, Index);
         Buffer[Index] = '\0';

         Display(("Type: URL = %s\r\n", Buffer));
         break;
      case deUUID_16:
         Display(("Type: UUID_16 = 0x%02X%02X\r\n", SDPDataElement->SDP_Data_Element.UUID_16.UUID_Byte0,
                                                                                 SDPDataElement->SDP_Data_Element.UUID_16.UUID_Byte1));
         break;
      case deUUID_32:
         Display(("Type: UUID_32 = 0x%02X%02X%02X%02X\r\n", SDPDataElement->SDP_Data_Element.UUID_32.UUID_Byte0,
                                                                                         SDPDataElement->SDP_Data_Element.UUID_32.UUID_Byte1,
                                                                                         SDPDataElement->SDP_Data_Element.UUID_32.UUID_Byte2,
                                                                                         SDPDataElement->SDP_Data_Element.UUID_32.UUID_Byte3));
         break;
      case deUUID_128:
         Display(("Type: UUID_128 = 0x%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X\r\n", SDPDataElement->SDP_Data_Element.UUID_128.UUID_Byte0,
                                                                                                               SDPDataElement->SDP_Data_Element.UUID_128.UUID_Byte1,
                                                                                                               SDPDataElement->SDP_Data_Element.UUID_128.UUID_Byte2,
                                                                                                               SDPDataElement->SDP_Data_Element.UUID_128.UUID_Byte3,
                                                                                                               SDPDataElement->SDP_Data_Element.UUID_128.UUID_Byte4,
                                                                                                               SDPDataElement->SDP_Data_Element.UUID_128.UUID_Byte5,
                                                                                                               SDPDataElement->SDP_Data_Element.UUID_128.UUID_Byte6,
                                                                                                               SDPDataElement->SDP_Data_Element.UUID_128.UUID_Byte7,
                                                                                                               SDPDataElement->SDP_Data_Element.UUID_128.UUID_Byte8,
                                                                                                               SDPDataElement->SDP_Data_Element.UUID_128.UUID_Byte9,
                                                                                                               SDPDataElement->SDP_Data_Element.UUID_128.UUID_Byte10,
                                                                                                               SDPDataElement->SDP_Data_Element.UUID_128.UUID_Byte11,
                                                                                                               SDPDataElement->SDP_Data_Element.UUID_128.UUID_Byte12,
                                                                                                               SDPDataElement->SDP_Data_Element.UUID_128.UUID_Byte13,
                                                                                                               SDPDataElement->SDP_Data_Element.UUID_128.UUID_Byte14,
                                                                                                               SDPDataElement->SDP_Data_Element.UUID_128.UUID_Byte15));
         break;
      case deSequence:
         /* Display that this is a SDP Data Element Sequence.           */
         Display(("Type: Data Element Sequence\r\n"));

         /* Loop through each of the SDP Data Elements in the SDP Data  */
         /* Element Sequence.                                           */
         for(Index = 0; Index < SDPDataElement->SDP_Data_Element_Length; Index++)
         {
            /* Call this function again for each of the SDP Data        */
            /* Elements in this SDP Data Element Sequence.              */
            DisplayDataElement(&(SDPDataElement->SDP_Data_Element.SDP_Data_Element_Sequence[Index]), (Level + 1));
         }
         break;
      case deAlternative:
         /* Display that this is a SDP Data Element Alternative.        */
         Display(("%*s Type: Data Element Alternative\r\n", (Level*INDENT_LENGTH), ""));

         /* Loop through each of the SDP Data Elements in the SDP Data  */
         /* Element Alternative.                                        */
         for(Index = 0; Index < SDPDataElement->SDP_Data_Element_Length; Index++)
         {
            /* Call this function again for each of the SDP Data        */
            /* Elements in this SDP Data Element Alternative.           */
            DisplayDataElement(&(SDPDataElement->SDP_Data_Element.SDP_Data_Element_Alternative[Index]), (Level + 1));
         }
         break;
      default:
         Display(("%*s Unknown SDP Data Element Type\r\n", (Level*INDENT_LENGTH), ""));
         break;
   }
}

   /* The following function is for the SDP Event Receive Data Callback.*/
   /* This function will be called whenever a Callback has been         */
   /* registered for the specified SDP Action that is associated with   */
   /* the Bluetooth Stack.  This function passes to the caller the SDP  */
   /* Request ID of the SDP Request, the SDP Response Event Data of the */
   /* specified Response Event and the SDP Response Event Callback      */
   /* Parameter that was specified when this Callback was installed.    */
   /* The caller is free to use the contents of the SDP Event Data ONLY */
   /* in the context of this callback.  If the caller requires the Data */
   /* for a longer period of time, then the callback function MUST copy */
   /* the data into another Data Buffer.  This function is guaranteed   */
   /* NOT to be invoked more than once simultaneously for the specified */
   /* installed callback (i.e. this function DOES NOT have be           */
   /* reentrant).  It Needs to be noted however, that if the same       */
   /* Callback is installed more than once, then the callbacks will be  */
   /* called serially.  Because of this, the processing in this function*/
   /* should be as efficient as possible.  It should also be noted that */
   /* this function is called in the Thread Context of a Thread that the*/
   /* User does NOT own.  Therefore, processing in this function should */
   /* be as efficient as possible (this argument holds anyway because   */
   /* other SDP Events will not be processed while this function call is*/
   /* outstanding).                                                     */
   /* * NOTE * This function MUST NOT Block and wait for events that    */
   /*          can only be satisfied by Receiving other SDP Events.  A  */
   /*          Deadlock WILL occur because NO SDP Event Callbacks will  */
   /*          be issued while this function is currently outstanding.  */
static void BTPSAPI SDP_Event_Callback(unsigned int BluetoothStackID, unsigned int SDPRequestID, SDP_Response_Data_t *SDP_Response_Data, unsigned long CallbackParameter)
{
   int Index;

   /* First, check to see if the required parameters appear to be       */
   /* semi-valid.                                                       */
   if((SDP_Response_Data != NULL) && (BluetoothStackID))
   {
      /* The parameters appear to be semi-valid, now check to see what  */
      /* type the incoming Event is.                                    */
      switch(SDP_Response_Data->SDP_Response_Data_Type)
      {
         case rdTimeout:
            /* A SDP Timeout was received, display a message indicating */
            /* this.                                                    */
            Display(("\r\n"));
            Display(("SDP Timeout Received (Size = 0x%04X).\r\n", sizeof(SDP_Response_Data_t)));
            break;
         case rdConnectionError:
            /* A SDP Connection Error was received, display a message   */
            /* indicating this.                                         */
            Display(("\r\n"));
            Display(("SDP Connection Error Received (Size = 0x%04X).\r\n", sizeof(SDP_Response_Data_t)));
            break;
         case rdErrorResponse:
            /* A SDP error response was received, display all relevant  */
            /* information regarding this event.                        */
            Display(("\r\n"));
            Display(("SDP Error Response Received (Size = 0x%04X) - Error Code: %d.\r\n", sizeof(SDP_Response_Data_t), SDP_Response_Data->SDP_Response_Data.SDP_Error_Response_Data.Error_Code));
            break;
         case rdServiceSearchResponse:
            /* A SDP Service Search Response was received, display all  */
            /* relevant information regarding this event                */
            Display(("\r\n"));
            Display(("SDP Service Search Response Received (Size = 0x%04X) - Record Count: %d\r\n", sizeof(SDP_Response_Data_t), SDP_Response_Data->SDP_Response_Data.SDP_Service_Search_Response_Data.Total_Service_Record_Count));

            /* First, check to see if any SDP Service Records were      */
            /* found.                                                   */
            if(SDP_Response_Data->SDP_Response_Data.SDP_Service_Search_Response_Data.Total_Service_Record_Count)
            {
               Display(("Record Handles:\r\n"));

               for(Index = 0; (Word_t)Index < SDP_Response_Data->SDP_Response_Data.SDP_Service_Search_Response_Data.Total_Service_Record_Count; Index++)
               {
                  Display(("Record %u: 0x%08X\r\n", (Index + 1), (unsigned int)SDP_Response_Data->SDP_Response_Data.SDP_Service_Search_Response_Data.Service_Record_List[Index]));
               }
            }
            else
               Display(("No SDP Service Records Found.\r\n"));
            break;
         case rdServiceAttributeResponse:
            /* A SDP Service Attribute Response was received, display   */
            /* all relevant information regarding this event            */
            Display(("\r\n"));
            Display(("SDP Service Attribute Response Received (Size = 0x%04X)\r\n", sizeof(SDP_Response_Data_t)));

            DisplaySDPAttributeResponse(&SDP_Response_Data->SDP_Response_Data.SDP_Service_Attribute_Response_Data, 0);
            break;
         case rdServiceSearchAttributeResponse:
            /* A SDP Service Search Attribute Response was received,    */
            /* display all relevant information regarding this event    */
            Display(("\r\n"));
            Display(("SDP Service Search Attribute Response Received (Size = 0x%04X)\r\n", sizeof(SDP_Response_Data_t)));

            DisplaySDPSearchAttributeResponse(&SDP_Response_Data->SDP_Response_Data.SDP_Service_Search_Attribute_Response_Data);
            break;
         default:
            /* An unknown/unexpected SDP event was received.            */
            Display(("\r\n"));
            Display(("Unknown SDP Event.\r\n"));
            break;
      }
   }
   else
   {
      /* There was an error with one or more of the input parameters.   */
      Display(("\r\n"));
      Display(("SDP callback data: Response_Data = NULL.\r\n"));
   }

   DisplayPrompt();
}

    /* The following function is responsible for opening a Headset or   */
    /* Audio Gateway Server on the Local Device.  This function opens   */
    /* the Headset or Audio Gateway Server on RFCOMM on Channel 1. This */
    /* function returns zero if successful, or a negative return value  */
    /* if an error occurred.                                            */
static int OpenServer(ParameterList_t *TempParam)
{
    int               ret_val;
    char              ServiceName[64];
    Class_of_Device_t ClassOfDevice;

    /* First check to see if a valid Bluetooth Stack ID exists.         */
    if(BluetoothStackID)
    {
       /* First, let's figure out if we are to open a Headset or an     */
       /* Audio Gateway Server.                                         */
       if(UI_Mode == UI_MODE_IS_SERVER)
       {
          /* Simply attempt to open a Headset Server, on RFCOMM Server  */
          /* Port 1.                                                    */
          ret_val = HDSET_Open_Headset_Server_Port(BluetoothStackID, LOCAL_SERVER_CHANNEL_ID, TRUE, HDSET_Event_Callback, (unsigned long)0);

          /* If the Open was successful, then note the Headset Server   */
          /* ID.                                                        */
          if(ret_val > 0)
          {
             /* Note the Headset Server ID of the opened Headset Server.*/
             HDSServerID = ret_val;

             /* Now lets make sure that the Major and Minor Device Class*/
             /* is set correctly.                                       */
             if(!GAP_Query_Class_Of_Device(BluetoothStackID, &ClassOfDevice))
             {
                SET_MAJOR_DEVICE_CLASS(ClassOfDevice, HCI_LMP_CLASS_OF_DEVICE_MAJOR_DEVICE_CLASS_AUDIO_VIDEO);
                SET_MINOR_DEVICE_CLASS(ClassOfDevice, HCI_LMP_CLASS_OF_DEVICE_MINOR_DEVICE_CLASS_AUDIO_VIDEO_HEADSET);

                /* Write out the Class of Device.                       */
                GAP_Set_Class_Of_Device(BluetoothStackID, ClassOfDevice);
             }

             /* The Server was opened successfully, now register a SDP  */
             /* Record indicating that a Headset Server exists.  Do this*/
             /* by first creating a Service Name.                       */
             sprintf(ServiceName, "Headset Server Port %u", LOCAL_SERVER_CHANNEL_ID);

             /* Now that a Service Name has been created try to Register*/
             /* the SDP Record.                                         */
             ret_val = HDSET_Register_Headset_SDP_Record(BluetoothStackID, HDSServerID, ServiceName, &HDSServerSDPHandle);

             /* If there was an error creating the Headset Server's SDP */
             /* Service Record then go ahead an close down the server an*/
             /* flag an error.                                          */
             if(ret_val < 0)
             {
                Display(("Unable to Register Headset Server SDP Record, Error = %d.\r\n", ret_val));

                HDSET_Close_Server_Port(BluetoothStackID, HDSServerID);

                /* Flag that there is no longer a Headset Server Open.  */
                HDSServerID            = 0;

                /* Flag that the Connection State is not connected.     */
                CurrentConnectionState = csDisconnected;

                ret_val                = UNABLE_TO_REGISTER_SERVER;
             }
             else
             {
                /* Simply flag to the user that everything initialized  */
                /* correctly.                                           */
                Display(("Headset Server Opened (Channel = %u, ID = %d).\r\n", LOCAL_SERVER_CHANNEL_ID, HDSServerID));

                /* Flag success to the caller.                          */
                ret_val = 0;
             }
          }
          else
          {
             Display(("Unable to Open Headset Server, Error = %d.\r\n", ret_val));

             ret_val = UNABLE_TO_REGISTER_SERVER;
          }
       }
       else
       {
          /* Simply attempt to open an Audio Gateway Server, on RFCOMM  */
          /* Server Port 1.                                             */
          ret_val = HDSET_Open_Audio_Gateway_Server_Port(BluetoothStackID, LOCAL_SERVER_CHANNEL_ID, HDSET_Event_Callback, (unsigned long)0);

          /* If the Open was successful, then note the Audio Gateway    */
          /* Server ID.                                                 */
          if(ret_val > 0)
          {
             /* Note the Audio Gateway Server ID of the opened Audio    */
             /* Gateway Server.                                         */
             HDSServerID = ret_val;

             /* The Server was opened successfully, now register a SDP  */
             /* Record indicating that an Audio Gateway Server exists.  */
             /* Do this by first creating a Service Name.               */
             sprintf(ServiceName, "Audio Gateway Server Port %u", LOCAL_SERVER_CHANNEL_ID);

             /* Now that a Service Name has been created try to Register*/
             /* the SDP Record.                                         */
             ret_val = HDSET_Register_Audio_Gateway_SDP_Record(BluetoothStackID, HDSServerID, ServiceName, &HDSServerSDPHandle);

             /* If there was an error creating the Audio Gateway        */
             /* Server's SDP Service Record then go ahead an close down */
             /* the server an flag an error.                            */
             if(ret_val < 0)
             {
                Display(("Unable to Register Audio Gateway Server SDP Record, Error = %d.\r\n", ret_val));

                HDSET_Close_Server_Port(BluetoothStackID, HDSServerID);

                /* Flag that there is no longer an Audio Gateway Server */
                /* Open.                                                */
                HDSServerID            = 0;

                /* Flag that the Connection State is not connected.     */
                CurrentConnectionState = csDisconnected;

                ret_val                = UNABLE_TO_REGISTER_SERVER;
             }
             else
             {
                /* Simply flag to the user that everything initialized  */
                /* correctly.                                           */
                Display(("Audio Gateway Server Opened (Channel = %u).\r\n", LOCAL_SERVER_CHANNEL_ID));

                /* Flag success to the caller.                          */
                ret_val = 0;
             }
          }
          else
          {
             Display(("Unable to Open Audio Gateway Server, Error = %d.\r\n", ret_val));

             ret_val = UNABLE_TO_REGISTER_SERVER;
          }
       }
    }
    else
    {
       /* No valid Bluetooth Stack ID exists.                           */
       ret_val = INVALID_STACK_ID_ERROR;
    }

    return(ret_val);
}

    /* The following function is responsible for closing a Headset or   */
    /* Audio Gateway Server that was previously opened via a successful */
    /* call to the OpenServer() function.  This function returns zero if*/
    /* successful or a negative return error code if there was an error.*/
static int CloseServer(ParameterList_t *TempParam)
{
    int ret_val;

    /* First check to see if a valid Bluetooth Stack ID exists.         */
    if(BluetoothStackID)
    {
       /* If a Headset or Audio Gateway Server is already opened, then  */
       /* simply close it.                                              */
       if(HDSServerID)
       {
          /* If there is a Headset/Audio Gateway SDP Service Record     */
          /* associated with the Headset/Audio Gateway Server then we   */
          /* need to remove it from the SDP Database.                   */
          if(HDSServerSDPHandle)
          {
             HDSET_Un_Register_SDP_Record(BluetoothStackID, HDSServerID, HDSServerSDPHandle);

             /* Flag that there is no longer an SDP Headset/Audio       */
             /* Gateway Server Record.                                  */
             HDSServerSDPHandle = 0;
          }

          /* Close the Server based on what type of Server that was     */
          /* opened.                                                    */
          if(UI_Mode == UI_MODE_IS_SERVER)
          {
             /* Close the Headset Server.                               */
             ret_val    = HDSET_Close_Server_Port(BluetoothStackID, HDSServerID);

             /* Flag that the Connection State is not connected.        */
             CurrentConnectionState = csDisconnected;

             Display(("Headset Server Closed.\r\n"));
          }
          else
          {
             /* Close the Audio Gateway Server.                         */
             ret_val = HDSET_Close_Server_Port(BluetoothStackID, HDSServerID);

             /* Flag that the Connection State is not connected.        */
             CurrentConnectionState = csDisconnected;

             Display(("Audio Gateway Server Closed.\r\n"));
          }

          if(ret_val < 0)
             ret_val = INVALID_PARAMETERS_ERROR;
          else
             ret_val = 0;

          /* Flag that there is no Headset/Audio Gateway Server         */
          /* currently open.                                            */
          HDSServerID = 0;
          ServerConnected = FALSE;
       }
       else
       {
          if(UI_Mode == UI_MODE_IS_SERVER)
             Display(("There is NO Headset Server currently open.\r\n"));
          else
             Display(("There is NO Audio Gateway Server currently open.\r\n"));

          ret_val = INVALID_PARAMETERS_ERROR;
       }
    }
    else
    {
       /* No valid Bluetooth Stack ID exists.                           */
       ret_val = INVALID_STACK_ID_ERROR;
    }

    return(ret_val);
}




     /* The following function is responsible for initiating a          */
     /* connection with a Remote Headset or Audio Gateway Server.  This */
     /* function returns zero if successful and a negative value if an  */
     /* error occurred.                                                 */
static int OpenRemoteServer(ParameterList_t *TempParam)
{
    int            ret_val;
    int            Result;
    BD_ADDR_t          NullADDR;
    Class_of_Device_t ClassOfDevice;

    ASSIGN_BD_ADDR(NullADDR, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);

    /* First, check that valid Bluetooth Stack ID exists.               */
    if(BluetoothStackID)
    {
        /* Next, let's make sure that there is not an Headset/Audio     */
        /* Gateway Client already open.                                 */
        if(!HDSClientID)
        {
            /* Next, let's make sure that the user has specified a      */
            /* Remote Bluetooth Device to open.                         */

            /* Next, let's determine if the user has specified a valid  */
            /* Bluetooth Device Address to connect with.                */
            if((TempParam) && (TempParam->NumberofParameters > 1) && (TempParam->Params[0].intParam) && (NumberofValidResponses) && (TempParam->Params[0].intParam <= NumberofValidResponses) && (!COMPARE_BD_ADDR(InquiryResultList[(TempParam->Params[0].intParam - 1)], NullADDR)) && (TempParam->Params[1].intParam))
            {
                /* Now let's attempt to open the Remote Headset or Audio*/
                /* Gateway Server.                                      */
                if(UI_Mode == UI_MODE_IS_SERVER)
                {
                Result = HDSET_Open_Remote_Audio_Gateway_Port(BluetoothStackID, InquiryResultList[(TempParam->Params[0].intParam - 1)], TempParam->Params[1].intParam, TRUE, HDSET_Event_Callback, (unsigned long)0);

                 if(Result > 0)
                 {
                    /* Inform the user that the call to open the Remote */
                    /* Audio Gateway Server was successful.             */
                    Display(("HDSET_Open_Remote_Audio_Gateway_Port() was successful.\r\n"));

                    /* Note the Headset Client ID.                      */
                    HDSClientID = Result;

                    /* Next, let's make sure the Class of Device is set */
                    /* correctly.                                       */
                    if(!GAP_Query_Class_Of_Device(BluetoothStackID, &ClassOfDevice))
                    {
                       SET_MAJOR_DEVICE_CLASS(ClassOfDevice, HCI_LMP_CLASS_OF_DEVICE_MAJOR_DEVICE_CLASS_AUDIO_VIDEO);
                       SET_MINOR_DEVICE_CLASS(ClassOfDevice, HCI_LMP_CLASS_OF_DEVICE_MINOR_DEVICE_CLASS_AUDIO_VIDEO_HEADSET);

                       /* Write out the Class of Device.                */
                       GAP_Set_Class_Of_Device(BluetoothStackID, ClassOfDevice);
                    }

                    /* Flag success to the caller.                      */
                    ret_val     = 0;
                 }
                 else
                 {
                    /* Inform the user that the call to Open the Remote */
                    /* Audio Gateway Server failed.                     */
                    Display(("Error calling HDSET_Open_Remote_Audio_Gateway_Port(): %d.\r\n", Result));

                    /* One or more of the necessary parameters is/are   */
                    /* invalid.                                         */
                    ret_val = INVALID_PARAMETERS_ERROR;
                 }
            }
            else
            {
                Result = HDSET_Open_Remote_Headset_Port(BluetoothStackID, InquiryResultList[(TempParam->Params[0].intParam - 1)], TempParam->Params[1].intParam, FALSE, HDSET_Event_Callback, (unsigned long)0);

                if(Result > 0)
                {
                    /* Inform the user that the call to open the Remote */
                    /* Headset Server was successful.                   */
                    Display(("HDSET_Open_Remote_Headset_Port() was successful.\r\n"));

                    /* Note the Audio Gateway Client ID.                */
                    HDSClientID = Result;

                    /* Flag success to the caller.                      */
                    ret_val     = 0;
                 }
                 else
                 {
                    /* Inform the user that the call to Open the Remote */
                    /* Headset Server failed.                           */
                    Display(("Error calling HDSET_Open_Remote_Headset_Port(): %d.\r\n", Result));

                    /* One or more of the necessary parameters is/are   */
                    /* invalid.                                         */
                    ret_val = INVALID_PARAMETERS_ERROR;
                 }
              }
            }
            else
            {
                Display(("Usage: Open [Inquiry Index] [RFCOMM Server Port].\r\n"));

                /* One or more of the necessary parameters is/are       */
                /* invalid.                                             */
                ret_val = INVALID_PARAMETERS_ERROR;
            }
        }
        else
        {
            if(UI_Mode == UI_MODE_IS_SERVER)
                Display(("Unable to open Headset Client.\r\nHeadset Client is already open.\r\n"));
            else
                Display(("Unable to open Audio Gateway Client.\r\nAudio Gateway Client is already open.\r\n"));

            ret_val = INVALID_PARAMETERS_ERROR;
        }
    }
    else
    {
        /* No valid Bluetooth Stack ID exists.                          */
        ret_val = INVALID_STACK_ID_ERROR;
    }

 return(ret_val);
}

     /* The following function is responsible for terminating a         */
     /* connection with a Remote Headset or Audio Gateway Server.  This */
     /* function returns zero if successful and a negative value if an  */
     /* error occurred.                                                 */
static int CloseRemoteServer(ParameterList_t *TempParam)
{
    int ret_val;

    /* First, check that valid Bluetooth Stack ID exists.               */
    if(BluetoothStackID)
    {
        /* Next, let's make sure that a Remote Headset or Audio Gateway */
        /* Server is indeed Open.                                       */
        if(HDSClientID)
        {
           /* Now let's attempt to close the Remote Headset or Audio    */
           /* Gateway Server.                                           */
           if(UI_Mode == UI_MODE_IS_SERVER)
           {
              Display(("Headset Client closed successfully.\r\n"));
              /* Simply close the Headset Client.                       */
              HDSET_Close_Port(BluetoothStackID, HDSClientID);
              /* Flag that there is no longer a Audio Gateway Client    */
              /* connected.                                             */
              HDSClientID            = 0;
                      /* Flag that the Connection State is not          */
                      /* connected.                                     */
              CurrentConnectionState = csDisconnected;
                      /* Flag success to the caller.                    */
              ret_val                = 0;
            }
            else
            {
                Display(("Audio Gateway Client closed successfully.\r\n"));
                /* Simply close the Audio Gateway Client.               */
                HDSET_Close_Port(BluetoothStackID, HDSClientID);
                /* Flag that there is no longer a Headset Client        */
                /* connected.                                           */
                HDSClientID          = 0;

                /* Flag that the Connection State is not connected.     */
                CurrentConnectionState = csDisconnected;

                /* Flag success to the caller.                          */
                ret_val              = 0;
            }
        }
        else
        {
            /* Display an error to the user informing them that no      */
            /* Headset or Audio Gateway Client is open.                 */
            if(UI_Mode == UI_MODE_IS_SERVER)
            Display(("You must open a Headset Client before you can close a Headset Client.\r\n"));
            else
            Display(("You must open an Audio Gateway Client before you can close an Audio Gateway Client.\r\n"));

            ret_val = INVALID_PARAMETERS_ERROR;
        }
    }
    else
    {
        /* No valid Bluetooth Stack ID exists.                          */
        ret_val = INVALID_STACK_ID_ERROR;
    }

 return(ret_val);
}

   /* The following function is responsible for issuing a Press Button  */
   /* Command for a Headset (either Accept or End a Call).  This        */
   /* function returns zero if successful or a negative return error    */
   /* code if an error occurs.                                          */
static int PressButton(ParameterList_t *TempParam)
{
   int Result;
   int ret_val;

   /* First, check that valid Bluetooth Stack ID exists.                */
   if(BluetoothStackID)
   {
      /* Next, let's make sure that we are operating in Headset Mode.   */
      if(UI_Mode == UI_MODE_IS_SERVER)
      {
         /* Next, let's make sure that there is an connection active.   */
         if(((ServerConnected)?HDSServerID:HDSClientID))
         {
            /* OK, we are a Headset, and there is an active connection  */
            /* so attempt to issue the Button Press.                    */
            Result = HDSET_Send_Button_Press(BluetoothStackID, ((ServerConnected)?HDSServerID:HDSClientID));

            if(Result >= 0)
            {
               /* Send Button Press was issued successfully so flag     */
               /* success to the user.                                  */
               Display(("HDSET_Send_Button_Press() Success.\r\n"));

               /* Flag success to the caller.                           */
               ret_val = 0;
            }
            else
            {
               /* Send Button Press was issued unsuccessfully so flag an*/
               /* error to the user.                                    */
               Display(("HDSET_Send_Button_Press() Failure: %d.\r\n", Result));

               /* Flag an error to the caller.                          */
               ret_val = FUNCTION_ERROR;
            }
         }
         else
         {
            /* No Connection is present so flag an error to the user.   */
            Display(("You must have an active connection to use this command.\r\n"));

            ret_val = FUNCTION_ERROR;
         }
      }
      else
      {
         /* One or more of the necessary parameters is/are invalid.     */
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

   /* The following function is responsible for sending a Speaker gain  */
   /* (Volume) Change Command to the Remote Connection.  This function  */
   /* returns zero if successful or a negative return error code if an  */
   /* error occurs.                                                     */
static int ChangeSpeakerGain(ParameterList_t *TempParam)
{
   int Result;
   int ret_val;

   /* First, check that valid Bluetooth Stack ID exists.                */
   if(BluetoothStackID)
   {
      /* Next, let's make sure that there is an connection active.      */
      if(((ServerConnected)?HDSServerID:HDSClientID))
      {
         /* Next make sure that valid parameters were specified.        */
        if((TempParam) && (TempParam->NumberofParameters > 0) && (TempParam->Params[0].intParam >= HDSET_SPEAKER_GAIN_MINIMUM) && (TempParam->Params[0].intParam <= HDSET_SPEAKER_GAIN_MAXIMUM))
        {
            Result = HDSET_Set_Speaker_Gain(BluetoothStackID, ((ServerConnected)?HDSServerID:HDSClientID), TempParam->Params[0].intParam);

            if(Result >= 0)
            {
               /* Speaker Gain was issued successfully so flag success  */
               /* to the user.                                          */
               Display(("HDSET_Set_Speaker_Gain() Success.\r\n"));

               /* Flag success to the caller.                           */
               ret_val = 0;
            }
            else
            {
               /* Speaker Gain was issued unsuccessfully so flag an     */
               /* error to the user.                                    */
               Display(("HDSET_Set_Speaker_Gain() Failure: %d.\r\n", Result));

               /* Flag an error to the caller.                          */
               ret_val = FUNCTION_ERROR;
            }
         }
         else
         {
            /* One or more of the necessary parameters is/are invalid.  */
            Display(("Usage: ChangeSpeakerGain [Volume Value (0-15)].\r\n"));

            ret_val = INVALID_PARAMETERS_ERROR;
         }
      }
      else
      {
         /* No Connection is present so flag an error to the user.      */
         Display(("You must have an active connection to use this command.\r\n"));

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

    /* The following function is responsible for sending a Ring         */
    /* Indication to the Remote connected Headset.  This function       */
    /* returns zero if successful or a negative return error code if an */
    /* error occurs.                                                    */
static int RingIndication(ParameterList_t *TempParam)
{
    int Result;
    int ret_val;

    /* First, check that valid Bluetooth Stack ID exists.               */
    if(BluetoothStackID)
    {
        /* Next, let's make sure that we are operating in Audio Gateway */
        /* Mode.                                                        */
        if(UI_Mode == UI_MODE_IS_CLIENT)
        {
            /* Next, let's make sure that there is an connection active.*/
            if(((ServerConnected)?HDSServerID:HDSClientID))
            {
                Result = HDSET_Ring_Indication(BluetoothStackID, ((ServerConnected)?HDSServerID:HDSClientID));
                if(Result >= 0)
                {
                   /* Ring Indication was issued successfully so flag   */
                   /* success to the user.                              */
                   Display(("HDSET_Ring_Indication() Success.\r\n"));

                   /* Flag success to the caller.                       */
                   ret_val = 0;
                }
                else
                {
                   /* Ring Indication was issued unsuccessfully so flag */
                   /* an error to the user.                             */
                   Display(("HDSET_Ring_Indication() Failure: %d.\r\n", Result));

                   /* Flag an error to the caller.                      */
                   ret_val = FUNCTION_ERROR;
                }
            }
            else
            {
                /* No Connection is present so flag an error to the     */
                /* user.                                                */
                Display(("You must have an active connection to use this command.\r\n"));
                ret_val = FUNCTION_ERROR;
             }
        }
        else
        {
             /* One or more of the necessary parameters is/are invalid. */
            ret_val = INVALID_PARAMETERS_ERROR;
        }
     }
     else
     {
        /* No valid Bluetooth Stack ID exists.                          */
        ret_val = INVALID_STACK_ID_ERROR;
     }

     return(ret_val);
}

    /* The following function is responsible for sending a Change       */
    /* Microphone Gain Command to the Remote Connection.  This function */
    /* returns zero if successful or a negative return error code if an */
    /* error occurs.                                                    */
static int ChangeMicrophoneGain(ParameterList_t *TempParam)
{
    int Result;
    int ret_val;

    /* First, check that valid Bluetooth Stack ID exists.               */
    if(BluetoothStackID)
    {
        /* Next, let's make sure that there is an connection active.    */
        if(((ServerConnected)?HDSServerID:HDSClientID))
        {
            /* Next make sure that valid parameters were specified.     */
            if((TempParam) && (TempParam->NumberofParameters > 0) && (TempParam->Params[0].intParam >= HDSET_MICROPHONE_GAIN_MINIMUM) && (TempParam->Params[0].intParam <= HDSET_MICROPHONE_GAIN_MAXIMUM))
            {
                Result = HDSET_Set_Microphone_Gain(BluetoothStackID, ((ServerConnected)?HDSServerID:HDSClientID), TempParam->Params[0].intParam);

                if(Result >= 0)
                {
                   /* Microphone Gain was issued successfully so flag   */
                   /* success to the user.                              */
                   Display(("HDSET_Set_Microphone_Gain() Success.\r\n"));

                   /* Flag success to the caller.                       */
                   ret_val = 0;
                }
                else
                {
                   /* Microphone Gain was issued unsuccessfully so flag */
                   /* an error to the user.                             */
                   Display(("HDSET_Set_Microphone_Gain() Failure: %d.\r\n", Result));

                   /* Flag an error to the caller.                      */
                   ret_val = FUNCTION_ERROR;
                }
            }
            else
            {
                /* One or more of the necessary parameters is/are       */
                /* invalid.                                             */
                Display(("Usage: ChangeMicrophoneGain [Gain Value (0-15)].\r\n"));

                ret_val = INVALID_PARAMETERS_ERROR;
            }
        }
        else
        {
            /* No Connection is present so flag an error to the user.   */
            Display(("You must have an active connection to use this command.\r\n"));

            ret_val = FUNCTION_ERROR;
        }
    }
    else
    {
        /* No valid Bluetooth Stack ID exists.                          */
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

   /* First check to see if a valid Bluetooth Stack ID exists.          */
   if(BluetoothStackID)
   {
      /* The Port ID appears to be at least semi-valid, now check the   */
      /* required parameters for this command.                          */
      if((TempParam) && (TempParam->NumberofParameters > 0))
      {
         /* Check to see if this is a request to setup an audio         */
         /* connection or disconnect an audio connection.               */
         if(TempParam->Params[0].intParam)
         {
            /* This is a request to setup an audio connection, call the */
            /* Setup Audio Connection function.                         */
            ret_val = SetupAudioConnection();
         }
         else
         {
            /* This is a request to disconnect an audio connection, call*/
            /* the Release Audio Connection function.                   */
            ret_val = ReleaseAudioConnection();
         }
      }
      else
      {
         /* The required parameter is invalid.                          */
         Display(("Usage: Audio [Release = 0, Setup = 1].\r\n"));

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


   /* The following function is responsible for Setting up an Audio     */
   /* Connection.  This function returns zero on successful execution   */
   /* and a negative value on all errors.                               */
static int SetupAudioConnection(void)
{
   int Result;
   int ret_val;

   /* First, check that valid Bluetooth Stack ID exists.                */
   if(BluetoothStackID)
   {
      /* Now check to make sure that the Port ID appears to be          */
      /* semi-valid.                                                    */
      if(((ServerConnected)?HDSServerID:HDSClientID))
      {
         /* The Port ID appears to be a semi-valid value.  Now submit   */
         /* the command.                                                */
             Result  = HDSET_Setup_Audio_Connection(BluetoothStackID, ((ServerConnected)?HDSServerID:HDSClientID), FALSE);

         /* Set the return value of this function equal to the Result of*/
         /* the function call.                                          */
         ret_val = Result;

         /* Now check to see if the command was submitted successfully. */
         if(!Result)
         {
            /* The function was submitted successfully.                 */
            Display(("HDSET_Setup_Audio_Connection: Function Successful.\r\n"));
         }
         else
         {
            /* There was an error submitting the function.              */
            Display(("HDSET_Setup_Audio_Connection() Failure: %d.\r\n", Result));
         }
      }
      else
      {
         /* One or more of the parameters are invalid.                  */
         Display(("Setup Audio Connection: Invalid Port ID.\r\n"));

         ret_val = FUNCTION_ERROR;
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

   /* The following function is responsible for Releasing an existing   */
   /* Audio Connection.  This function returns zero on successful       */
   /* execution and a negative value on all errors.                     */
static int ReleaseAudioConnection(void)
{
   int Result;
   int ret_val;

   /* First, check that valid Bluetooth Stack ID exists.                */
   if(BluetoothStackID)
   {
      /* Now check to make sure that the Port ID appears to be          */
      /* semi-valid.                                                    */
      if(((ServerConnected)?HDSServerID:HDSClientID))
      {
         /* The Port ID appears to be a semi-valid value.  Now submit   */
         /* the command.                                                */
         Result  = HDSET_Release_Audio_Connection(BluetoothStackID, ((ServerConnected)?HDSServerID:HDSClientID));

         /* Set the return value of this function equal to the Result of*/
         /* the function call.                                          */
         ret_val = Result;

         /* Now check to see if the command was submitted successfully. */
         if(!Result)
         {
            /* The function was submitted successfully.                 */
            Display(("HDSET_Release_Audio_Connection: Function Successful.\r\n"));
         }
         else
         {
            /* There was an error submitting the function.              */
            Display(("HDSET_Release_Audio_Connection() Failure: %d.\r\n", Result));
         }
      }
      else
      {
         /* One or more of the parameters are invalid.                  */
         Display(("Release Audio Connection: Invalid Port ID.\r\n"));

         ret_val = FUNCTION_ERROR;
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


    /* Interface Mode to Server.                                        */
static int HeadsetMode(ParameterList_t *TempParam)
{
    UI_Mode = UI_MODE_IS_SERVER;

    UserInterface_Server();

    return(EXIT_MODE);
}

    /* The following function is responsible for changing the User      */
    /* Interface Mode to Client.                                        */
static int AudioGatewayMode(ParameterList_t *TempParam)
{
    UI_Mode = UI_MODE_IS_CLIENT;
    UserInterface_Client();

    return(EXIT_MODE);
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
                  {
                     Display(("%02X", ((Byte_t *)(&(GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Authentication_Event_Data.Link_Key_Info.Link_Key)))[Index]));
                  }

                  Display(("\r\n"));

                  /* Now store the link Key in either a free location OR*/
                  /* over the old key location.                         */
                  ASSIGN_BD_ADDR(NULL_BD_ADDR, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);

                  for(Index=0,Result=-1;Index<(sizeof(LinkKeyInfo)/sizeof(LinkKeyInfo_t));Index++)
                  {
                     if(COMPARE_BD_ADDR(LinkKeyInfo[Index].BD_ADDR, GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Remote_Device))
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
                  {
                     Index = Result;
                  }

                  /* Check to see if we found a location to store the   */
                  /* Link Key information into.                         */
                  if(Index != (-1))
                  {
                     LinkKeyInfo[Index].BD_ADDR = GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Remote_Device;
                     LinkKeyInfo[Index].LinkKey = GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Authentication_Event_Data.Link_Key_Info.Link_Key;

                     Display(("Link Key Stored locally.\r\n"));
                  }
                  else
                  {
                     Display(("Link Key NOT Stored locally: Link Key array is full.\r\n"));
                  }
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
                  {
                     Display(("GAP_Authentication_Response() Success.\r\n"));
                  }
                  else
                  {
                     Display(("GAP_Authentication_Response() Failure: %d.\r\n", Result));
                  }
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
                     Display(("\r\nAuto Accepting: %lu\r\n", GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Authentication_Event_Data.Numeric_Value));

                     Result = GAP_Authentication_Response(BluetoothStackID, GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Remote_Device, &GAP_Authentication_Information);

                     if(!Result)
                     {
                        Display(("GAP_Authentication_Response() Success.\r\n"));
                     }
                     else
                     {
                        Display(("GAP_Authentication_Response() Failure: %d.\r\n", Result));
                     }

                     /* Flag that there is no longer a current          */
                     /* Authentication procedure in progress.           */
                     ASSIGN_BD_ADDR(CurrentRemoteBD_ADDR, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
                  }
                  else
                  {
                     Display(("User Confirmation: %lu\r\n", GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Authentication_Event_Data.Numeric_Value));

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
                  {
                     Display(("GAP_Authentication_Response() Success.\r\n"));
                  }
                  else
                  {
                     Display(("GAP_Authentication_Response() Failure: %d.\r\n", Result));
                  }
                  break;
               case atPasskeyNotification:
                  BD_ADDRToStr(GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Remote_Device, BoardStr);
                  Display(("atPasskeyNotification: %s\r\n", BoardStr));

                  Display(("Passkey Value: %lu\r\n", GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Authentication_Event_Data.Numeric_Value));
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
               {
                  Display(("GAP Remote Name Result: %s.\r\n", GAP_Remote_Name_Event_Data->Remote_Name));
               }
               else
               {
                  Display(("GAP Remote Name Result: NULL.\r\n"));
               }
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

    /* The following function represents the Headset Profile Event      */
    /* Receive Data Callback (Headset and Audio Gateway). This function */
    /* will be called whenever a Headset or Audio Gateway Event occurs  */
    /* that is associated with the specified Bluetooth Stack ID. This   */
    /* function takes as its parameters the Bluetooth Stack ID, the     */
    /* Headset Profile Event Data that occurred and the Headset Profile */
    /* Event Callback Parameter that was specified when this Callback   */
    /* was installed.  The caller is free to use the contents of the    */
    /* Headset Event Data ONLY in the context of this callback.  If the */
    /* caller requires the Data for a longer period of time, then the   */
    /* callback function MUST copy the data into another Data Buffer.   */
    /* This function is guaranteed NOT to be invoked more than once     */
    /* simultaneously for the specified installed callback (i.e. this   */
    /* function DOES NOT have be reentrant). It Needs to be noted       */
    /* however, that if the same Callback is installed more than once,  */
    /* then the callbacks will be called serially.  Because of this, the*/
    /* processing in this function should be as efficient as possible.  */
    /* It should also be noted that this function is called in the      */
    /* Thread Context of a Thread that the User does NOT own.           */
    /* Therefore, processing in this function should be as efficient as */
    /* possible (this argument holds anyway because another Headset     */
    /* Profile Event will not be processed while this function call is  */
    /* outstanding).                                                    */
    /* * NOTE * This function MUST NOT Block and wait for events that   */
    /*          can only be satisfied by Receiving other Headset Events.*/
    /*          A Deadlock WILL occur because NO GAP Event Callbacks    */
    /*          will be issued while this function is currently         */
    /*          outstanding.                                            */
static void BTPSAPI HDSET_Event_Callback(unsigned int BluetoothStackID, HDSET_Event_Data_t *HDSET_Event_Data, unsigned long CallbackParameter)
{
    char           BoardStr[13];

    if((BluetoothStackID) && (HDSET_Event_Data))
    {
        Display(("\r\n"));
        switch(HDSET_Event_Data->Event_Data_Type)
        {
            case etHDSET_Open_Port_Indication:
               /* A Client has connected to the Server, display the     */
               /* BD_ADDR of the connecting device.                     */
               Display(("HDSET Open Port Indication, HDSETID: 0x%04X\r\n", HDSET_Event_Data->Event_Data.HDSET_Open_Port_Indication_Data->HDSETPortID));
               BD_ADDRToStr(HDSET_Event_Data->Event_Data.HDSET_Open_Port_Indication_Data->BD_ADDR, BoardStr);
               Display(("BD_ADDR: %s\r\n", BoardStr));

               /* Flag the connection state.                            */
               CurrentConnectionState = csConnected;

               ServerConnected = TRUE;

               break;
            case etHDSET_Open_Port_Confirmation:
               /* The Client received a Connect Confirmation, display   */
               /* relevant information.                                 */
               Display(("HDSET Open Port Confirmation, HDSETID: 0x%04X, Status 0x%04X.\r\n", HDSET_Event_Data->Event_Data.HDSET_Open_Port_Confirmation_Data->HDSETPortID,
                                                                                           HDSET_Event_Data->Event_Data.HDSET_Open_Port_Confirmation_Data->PortOpenStatus));

               /* Check to see if the Client Port was opened            */
               /* successfully.                                         */
               if(HDSET_Event_Data->Event_Data.HDSET_Open_Port_Confirmation_Data->PortOpenStatus)
               {
                  /* There was an error while trying to open a Client   */
                  /* Port.  Invalidate the current Port ID.             */
                  HDSClientID = 0;
               }
               else
               {
                  /* Flag the current state as connected.               */
                  CurrentConnectionState = csConnected;
               }
               break;
            case etHDSET_Close_Port_Indication:
               /* A Close Port Indication was received.  Display the    */
               /* appropriate message.                                  */
               Display(("HDSET Close Port Indication, HDSETID: 0x%04X, Status 0x%04X.\r\n", HDSET_Event_Data->Event_Data.HDSET_Close_Port_Indication_Data->HDSETPortID,
                                                                                          HDSET_Event_Data->Event_Data.HDSET_Close_Port_Indication_Data->PortCloseStatus));

               /* Flag the current connection state as not connected.   */
               CurrentConnectionState = csDisconnected;

               /* The Client Port was closed so invalidate the ID.      */
               HDSClientID = 0;
               break;
            case etHDSET_Ring_Indication:
               /* A Ring Indication was received, display the relevant  */
               /* information.                                          */
               Display(("HDSET Ring Indication, ID: 0x%04X.\r\n", HDSET_Event_Data->Event_Data.HDSET_Ring_Indication_Data->HDSETPortID));
               Display((" Answer with respond command: PressButton \r\n"));
               break;
            case etHDSET_Button_Pressed_Indication:
               /* A Button Pressed Indication was received, display the */
               /* relevant information.                                 */
               Display(("HDSET Button Pressed Indication, ID: 0x%04X, Connection Present: 0x%04X.\r\n", HDSET_Event_Data->Event_Data.HDSET_Button_Pressed_Indication_Data->HDSETPortID,
                                                                                                      HDSET_Event_Data->Event_Data.HDSET_Button_Pressed_Indication_Data->ConnectionPresent));
               break;
            case etHDSET_Speaker_Gain_Indication:
               /* A Speaker Gain Indication was received, display the   */
               /* relevant information.                                 */
               Display(("HDSET Speaker Gain Indication, ID: 0x%04X, Gain: 0x%04X.\r\n", HDSET_Event_Data->Event_Data.HDSET_Speaker_Gain_Indication_Data->HDSETPortID,
                                                                                      HDSET_Event_Data->Event_Data.HDSET_Speaker_Gain_Indication_Data->SpeakerGain));
               break;
            case etHDSET_Microphone_Gain_Indication:
               /* A Microphone Gain Indication was received, display the*/
               /* relevant information.                                 */
               Display(("HDSET Microphone Gain Indication, ID: 0x%04X, Gain: 0x%04X.\r\n", HDSET_Event_Data->Event_Data.HDSET_Microphone_Gain_Indication_Data->HDSETPortID,
                                                                                         HDSET_Event_Data->Event_Data.HDSET_Microphone_Gain_Indication_Data->MicrophoneGain));
               break;
            case etHDSET_Audio_Connection_Indication:
               /* An Audio Connection Indication was received, display  */
               /* the relevant information.                             */
               Display(("HDSET Audio Connection Indication, ID: 0x%04X.\r\n", HDSET_Event_Data->Event_Data.HDSET_Audio_Connection_Indication_Data->HDSETPortID));

               CurrentConnectionState = csAudioConnected;

               /* Enable the audio codec.                               */
               HAL_EnableAudioCodec(BluetoothStackID, aucHFP_HSP, CVSD_FREQUENCY, 1);

               break;
            case etHDSET_Audio_Disconnection_Indication:
               /* An Audio Disconnect Indication was received, display  */
               /* the relevant information.                             */
               Display(("HDSET Audio Disconnection Indication, ID: 0x%04X.\r\n", HDSET_Event_Data->Event_Data.HDSET_Audio_Disconnection_Indication_Data->HDSETPortID));

               if(CurrentConnectionState != csDisconnected)
                  CurrentConnectionState = csConnected;

               /* Disable the audio codec.                              */
               HAL_DisableAudioCodec();

               break;
            case etHDSET_Audio_Data_Indication:
                Display(("HDSET Audio Data Indication"));
               break;
            default:
               /* An unknown/unexpected Headset event was received.     */
               Display(("\r\nUnknown HDSET Event Received.\r\n"));
               break;
        }

        /* Make sure the output is displayed to the user.               */
        DisplayPrompt();
    }
    else
    {
      /* There was an error with one or more of the input parameters.   */
      Display(("\r\n"));

      Display(("\r\nHFRE callback data: Event_Data = NULL.\r\n"));

      DisplayPrompt();
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
   int ret_val = APPLICATION_ERROR_UNABLE_TO_OPEN_STACK;
   UI_Mode                = UI_MODE_SELECT;


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
                  UserInterface_Selection();

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
  if(UI_Mode == UI_MODE_IS_CLIENT)
     Display(("\r\nAG>"));
  else
  {
     if(UI_Mode == UI_MODE_IS_SERVER)
        Display(("\r\nHS>"));
     else
        Display(("\r\nChoose Mode>"));
  }
}

   /* The following function is used to process a command line string.  */
   /* This function takes as it's only parameter the command line string*/
   /* to be parsed and returns TRUE if a command was parsed and executed*/
   /* or FALSE otherwise.                                               */
Boolean_t ProcessCommandLine(char *String)
{
   return(CommandLineInterpreter(String));
}

