/*****< mapdemo.c >************************************************************/
/*      Copyright 2011 - 2014 Stonestreet One.                                */
/*      All Rights Reserved.                                                  */
/*                                                                            */
/*      Copyright 2015 Texas Instruments Incorporated.                        */
/*      All Rights Reserved.                                                  */
/*                                                                            */
/*  MAPDEMO - Simple embedded application using MAP Profile.                  */
/*                                                                            */
/*  Author:  Tim Cook                                                         */
/*                                                                            */
/*** MODIFICATION HISTORY *****************************************************/
/*                                                                            */
/*   mm/dd/yy  F. Lastname    Description of Modification                     */
/*   --------  -----------    ------------------------------------------------*/
/*   01/24/11  T. Cook        Initial creation.                               */
/******************************************************************************/

#include "Main.h"                /* Application Interface Abstraction.        */
#include "MAPDemo.h"             /* Application Header.                       */
#include "HAL.h"                 /* Hardware Abstraction Layer Header.        */
#include "SS1BTPS.h"             /* Main SS1 Bluetooth Stack Header.          */
#include "BTPSKRNL.h"            /* BTPS Kernel Header.                       */
#include "SS1BTVS.h"             /* Vendor Specific Prototypes/Constants.     */
#include "SS1BTMAP.h"            /* SS1 Message Access Profile APIs.          */
#include "MsgStore.h"            /* Sample Data Message Store Include.        */

#define MAX_SUPPORTED_COMMANDS                     (48)  /* Denotes the       */
                                                         /* maximum number of */
                                                         /* User Commands that*/
                                                         /* are supported by  */
                                                         /* this application. */

#define MAX_COMMAND_LENGTH                         (64)  /* Denotes the max   */
                                                         /* buffer size used  */
                                                         /* for user commands */
                                                         /* input via the     */
                                                         /* User Interface.   */

#define MAX_NUM_OF_PARAMETERS                       (5)  /* Denotes the max   */
                                                         /* number of         */
                                                         /* parameters a      */
                                                         /* command can have. */

#define MAX_INQUIRY_RESULTS                        (20)  /* Denotes the max   */
                                                         /* number of inquiry */
                                                         /* results.          */

#define MAX_SUPPORTED_LINK_KEYS                    (1)   /* Max supported Link*/
                                                         /* keys.             */

#define DEFAULT_IO_CAPABILITY          (icDisplayYesNo)  /* Denotes the       */
                                                         /* default I/O       */
                                                         /* Capability that is*/
                                                         /* used with Secure  */
                                                         /* Simple Pairing.   */

#define DEFAULT_MITM_PROTECTION                  (TRUE)  /* Denotes the       */
                                                         /* default value used*/
                                                         /* for Man in the    */
                                                         /* Middle (MITM)     */
                                                         /* protection used   */
                                                         /* with Secure Simple*/
                                                         /* Pairing.          */

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
                                                         /* error occurred due*/
                                                         /* to attempted      */
                                                         /* execution of a    */
                                                         /* Command when a    */
                                                         /* Bluetooth Protocol*/
                                                         /* Stack has not been*/
                                                         /* opened.           */

#define UNABLE_TO_REGISTER_SERVER                  (-9)  /* Denotes that an   */
                                                         /* error occurred    */
                                                         /* when trying to    */
                                                         /* create a MAP      */
                                                         /* Server.           */

#define READ_STATUS_STRING_OFFSET         32             /* Offset of the Read*/
                                                         /* Status in a       */
                                                         /* Message string.   */

#define INDENT_LENGTH                               (3)  /* Denotes the number*/
                                                         /* of character      */
                                                         /* spaces to be used */
                                                         /* for indenting when*/
                                                         /* displaying SDP    */
                                                         /* Data Elements.    */

#define EXIT_MODE                                 (-10)  /* Flags exit from   */
                                                         /* any Mode.         */

   /* Determine the Name we will use for this compilation.              */
#define APP_DEMO_NAME                              "MAPDemo"

   /* The following represent the possible values of UI_Mode variable.  */
#define UI_MODE_IS_CLIENT      (2)
#define UI_MODE_IS_SERVER      (1)
#define UI_MODE_SELECT         (0)
#define UI_MODE_IS_INVALID     (-1)

   /* Following converts a Sniff Parameter in Milliseconds to frames.   */
#define MILLISECONDS_TO_BASEBAND_SLOTS(_x)   ((_x) / (0.625))

   /* The following type definition represents the container type which */
   /* holds the mapping between Bluetooth devices (based on the BD_ADDR)*/
   /* and the Link Key (BD_ADDR <-> Link Key Mapping).                  */
typedef struct _tagLinkKeyInfo_t
{
   BD_ADDR_t  BD_ADDR;
   Link_Key_t LinkKey;
} LinkKeyInfo_t;

   /* The following type definition represents the container type which */
   /* holds the mapping between Profile UUIDs and Profile Names (UUID   */
   /* <-> Name).                                                        */
typedef struct _tagUUIDInfo_t
{
   char       *Name;
   UUID_128_t  UUID;
} UUIDInfo_t;

   /* The following enumerated type represents all of the operations    */
   /* that can be on-going at any given instant (either client or       */
   /* server).                                                          */
typedef enum
{
   coNone,
   coAbort,
   coSetFolder,
   coGetFolderListing,
   coGetMessageListing,
   coGetMessage,
   coSetMessageStatus,
   coPushMessage,
   coUpdateInbox,
   coDisconnect
} CurrentOperation_t;

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
   /* User to represent a structure to hold a BD_ADDR return from       */
   /* BD_ADDRToStr.                                                     */
typedef char BoardStr_t[16];
   /* Internal Variables to this Module (Remember that all variables    */
   /* declared static are initialized to 0 automatically by the         */
   /* compiler as part of standard C/C++).                              */
static int                    UI_Mode;              /* Holds the UI Mode.              */

static unsigned int           BluetoothStackID;     /* Variable which holds the Handle */
                                                    /* of the opened Bluetooth Protocol*/
                                                    /* Stack.                          */

static unsigned int           MAPID;                /* Variable used to hold the       */
                                                    /* MAP Client/Server ID of the     */
                                                    /* currently active MAP Profile    */
                                                    /* Role.                           */

static Word_t                 Connection_Handle;    /* Holds the Connection Handle of  */
                                                    /* the most recent SPP Connection. */

static DWord_t                ServerSDPRecordHandle;/* Variable used to hold the       */
                                                    /* MAP Server SDP Service Record   */
                                                    /* Handle of the MAP Server SDP    */
                                                    /* Service Record.                 */

static Boolean_t              Connected;            /* Variable which reflects whether */
                                                    /* or not there is an active       */
                                                    /* (on-going) connection present.  */

static int                    HCIEventCallbackHandle;  /* Holds the handle of the      */
                                                    /* registered HCI Event Callback.  */

static DWord_t                NotificationSDPRecordHandle; /* Variable used to hold the*/
                                                    /* MAP Notification Server SDP     */
                                                    /* Service Record Handle of the    */
                                                    /* MAP Notification Server SDP     */
                                                    /* Service Record.                 */

static unsigned int           MAPNID;               /* Variable used to hold the       */
                                                    /* MAP Notification Client/Server  */
                                                    /* ID of the currently active      */
                                                    /* MAP Notification Profile Role.  */

static Boolean_t              NotificationConnected;/* Variable which reflects whether */
                                                    /* or not there is an active       */
                                                    /* (on-going) Notification         */
                                                    /* connection present.             */

static char                   ReceiveMessagePath[256];/* Variable which holds the      */
                                                    /* current path where received     */
                                                    /* messages (server side) are      */
                                                    /* stored.                         */

static CurrentOperation_t     CurrentOperation;     /* Variable which holds the current*/
                                                    /* (on-going operation).           */

static char                   CurrentFileName[256]; /* Variables are used when sending */
static unsigned int           CurrentBufferSize;    /* and receiving data.  These      */
static unsigned int           CurrentBufferSent;    /* variables are used by both the  */
static char                  *CurrentBuffer;        /* client and server when an       */
                                                    /* operation requires data to be   */
                                                    /* transferred to the remote       */
                                                    /* device.                         */

static unsigned int           BluetoothStackID;     /* Variable which holds the Handle */
                                                    /* of the opened Bluetooth Protocol*/
                                                    /* Stack.                          */

static BD_ADDR_t              InquiryResultList[MAX_INQUIRY_RESULTS];/* Variable which */
                                                    /* contains the inquiry result     */
                                                    /* received from the most recently */
                                                    /* preformed inquiry.              */

static unsigned int           NumberofValidResponses;/* Variable which holds the number*/
                                                    /* of valid inquiry results within */
                                                    /* the inquiry results array.      */

static LinkKeyInfo_t          LinkKeyInfo[MAX_SUPPORTED_LINK_KEYS];/* Variable holds   */
                                                    /* BD_ADDR <-> Link Keys for       */
                                                    /* pairing.                        */

static BD_ADDR_t              CurrentRemoteBD_ADDR; /* Variable which holds the        */
                                                    /* current BD_ADDR of the device   */
                                                    /* which is currently pairing or   */
                                                    /* authenticating.                 */

static GAP_IO_Capability_t    IOCapability;         /* Variable which holds the        */
                                                    /* current I/O Capabilities that   */
                                                    /* are to be used for Secure Simple*/
                                                    /* Pairing.                        */

static Boolean_t              OOBSupport;           /* Variable which flags whether    */
                                                    /* or not Out of Band Secure Simple*/
                                                    /* Pairing exchange is supported.  */

static Boolean_t              MITMProtection;       /* Variable which flags whether or */
                                                    /* not Man in the Middle (MITM)    */
                                                    /* protection is to be requested   */
                                                    /* during a Secure Simple Pairing  */
                                                    /* procedure.                      */

static GAP_IO_Capabilities_t  RemoteIOCapabilities; /* Variable which holds the        */
                                                    /* remote I/O Capabilities of the  */
                                                    /* remote device (used during      */
                                                    /* authentication/pairing).        */

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

static char                Input[MAX_COMMAND_LENGTH];/* Buffer which holds the user    */
                                                    /* input to be parsed.             */

static DWord_t             MaxBaudRate;             /* Variable stores the maximum     */
                                                    /* HCI UART baud rate supported by */
                                                    /* this platform.                  */

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

#define NUM_SUPPORTED_HCI_VERSIONS              (sizeof(HCIVersionStrings)/sizeof(char *) - 1)

   /* The following string table is used to map the API I/O Capabilities*/
   /* values to an easily displayable string.                           */
static BTPSCONST char *IOCapabilitiesStrings[] =
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
   { "Handsfree",             { 0x00, 0x00, 0x11, 0x1E, 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB } },
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


static Word_t              TempBuffer[256];

#define SIZEOF_TEMP_BUFFER (sizeof(TempBuffer) / sizeof(Word_t))

static char                TempCallbackBuffer[256];

#define SIZEOF_TEMP_CALLBACK_BUFFER (sizeof(TempCallbackBuffer) / sizeof(char))

   /* The following defines a data sequence that will be used to        */
   /* generate message data.                                            */
static BTPSCONST char DataStr[]  = "BEGIN:VCARD\nVERSION:2.1\nNAME:MAPDemo\nNOTE:Test Msg\nEND:VCARD\n";

   /* Internal function prototypes.                                     */
static void UserInterface_Client(void);
static void UserInterface_Server(void);
static void UserInterface_Selection(void);
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
static void DisplayFunctionError(char *Function, int Status);
static void DisplayFunctionSuccess(char *Function);
static void DisplayFWVersion (void);

static int OpenStack(HCI_DriverInformation_t *HCI_DriverInformation, BTPS_Initialization_t *BTPS_Initialization);
static int CloseStack(void);

static int SetDisc(void);
static int SetConnect(void);
static int SetPairable(void);
static int DeleteLinkKey(BD_ADDR_t BD_ADDR);

static int DisplayHelp(ParameterList_t *TempParam);
static int QueryMemory(ParameterList_t *TempParam);
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
static int ServiceDiscovery(ParameterList_t *TempParam);

static int OpenServer(ParameterList_t *TempParam);
static int CloseServer(ParameterList_t *TempParam);

static int OpenNotificationServer(ParameterList_t *TempParam);
static int CloseNotificationServer(ParameterList_t *TempParam);

static int OpenRemoteServer(ParameterList_t *TempParam);
static int CloseConnection(ParameterList_t *TempParam);

static int OpenRemoteNotificationServer(ParameterList_t *TempParam);
static int CloseNotificationConnection(ParameterList_t *TempParam);

static int Abort(ParameterList_t *TempParam);
static int SetFolder(ParameterList_t *TempParam);
static int GetFolderListing(ParameterList_t *TempParam);
static int GetMessageListing(ParameterList_t *TempParam);
static int GetMessage(ParameterList_t *TempParam);
static int SetMessageStatus(ParameterList_t *TempParam);
static int PushMessage(ParameterList_t *TempParam);
static int UpdateInbox(ParameterList_t *TempParam);
static int SendEventReport(ParameterList_t *TempParam);
static int RequestNotification(ParameterList_t *TempParam);
static int SetBaudRate(ParameterList_t *TempParam);
static int QueryMemory(ParameterList_t *TempParam);

static int ServerMode(ParameterList_t *TempParam);
static int ClientMode(ParameterList_t *TempParam);

static void SendEventRequest(EventType_t EventType, MessageType_t MessageType, char *FolderName, char *Handle);

static void ProcessSetFolder(MAP_Set_Folder_Indication_Data_t *MAP_Set_Folder_Indication_Data);
static void ProcessGetFolderListing(MAP_Get_Folder_Listing_Indication_Data_t *MAP_Get_Folder_Listing_Indication_Data);
static void ProcessGetMessageListing(MAP_Get_Message_Listing_Indication_Data_t *MAP_Get_Message_Listing_Indication_Data);
static void ProcessGetMessage(MAP_Get_Message_Indication_Data_t *MAP_Get_Message_Indication_Data);
static void ProcessPushMessage(MAP_Push_Message_Indication_Data_t *MAP_Push_Message_Indication_Data);
static void ProcessUpdateInbox(MAP_Update_Inbox_Indication_Data_t *MAP_Update_Inbox_Indication_Data);
static void ProcessNotificationRegistration(MAP_Notification_Registration_Indication_Data_t *MAP_Notification_Registration_Indication_Data);

static void DisplaySDPAttributeResponse(SDP_Service_Attribute_Response_Data_t *SDPServiceAttributeResponse, unsigned int InitLevel);
static void DisplaySDPSearchAttributeResponse(SDP_Service_Search_Attribute_Response_Data_t *SDPServiceSearchAttributeResponse);
static void DisplayDataElement(SDP_Data_Element_t *SDPDataElement, unsigned int Level);

   /* BTPS Callback function prototypes.                                */
static void BTPSAPI HCI_Event_Callback(unsigned int BluetoothStackID, HCI_Event_Data_t *HCI_Event_Data, unsigned long CallbackParameter);
static void BTPSAPI GAP_Event_Callback(unsigned int BluetoothStackID, GAP_Event_Data_t *GAP_Event_Data, unsigned long CallbackParameter);
static void BTPSAPI SDP_Event_Callback(unsigned int BluetoothStackID, unsigned int SDPRequestID, SDP_Response_Data_t *SDP_Response_Data, unsigned long CallbackParameter);
static void BTPSAPI MAP_Event_Callback_Server(unsigned int BluetoothStackID, MAP_Event_Data_t *MAPEventData, unsigned long CallbackParameter);
static void BTPSAPI MAP_Event_Callback_Client(unsigned int BluetoothStackID, MAP_Event_Data_t *MAPEventData, unsigned long CallbackParameter);

   /* This function is responsible for taking the input from the user   */
   /* and dispatching the appropriate Command Function.  First, this    */
   /* function retrieves a String of user input, parses the user input  */
   /* into Command and Parameters, and finally executes the Command or  */
   /* Displays an Error Message if the input is not a valid Command.    */
static void UserInterface_Client(void)
{
   /* Next display the available commands.                              */
   DisplayHelp(NULL);
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
   AddCommand("SERVICEDISCOVERY", ServiceDiscovery);
   AddCommand("OPEN", OpenRemoteServer);
   AddCommand("CLOSE", CloseConnection);
   AddCommand("OPENNOTIFICATION", OpenNotificationServer);
   AddCommand("CLOSENOTIFICATION", CloseNotificationServer);
   AddCommand("REQUESTNOTIFICATION", RequestNotification);
   AddCommand("SETFOLDER", SetFolder);
   AddCommand("GETFOLDERLISTING", GetFolderListing);
   AddCommand("GETMESSAGELISTING", GetMessageListing);
   AddCommand("GETMESSAGE", GetMessage);
   AddCommand("SETMESSAGESTATUS", SetMessageStatus);
   AddCommand("PUSHMESSAGE", PushMessage);
   AddCommand("UPDATEINBOX", UpdateInbox);
   AddCommand("ABORT", Abort);
   AddCommand("SETBAUDRATE", SetBaudRate);

   AddCommand("HELP", DisplayHelp);
   AddCommand("QUERYMEMORY", QueryMemory);
   /* Go ahead and make sure that the input command parser buffer is    */
   /* fully empty.                                                      */
   BTPS_MemInitialize(Input, 0, sizeof(Input));
}

   /* This function is responsible for taking the input from the user   */
   /* and dispatching the appropriate Command Function.  First, this    */
   /* function retrieves a String of user input, parses the user input  */
   /* into Command and Parameters, and finally executes the Command or  */
   /* Displays an Error Message if the input is not a valid Command.    */
static void UserInterface_Server(void)
{
   /* Next display the available commands.                              */
   DisplayHelp(NULL);

   /* Clear the installed command.                                      */
   ClearCommands();

   /* Install the commands revelant for this UI.                        */
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
   AddCommand("OPENNOTIFICATION", OpenRemoteNotificationServer);
   AddCommand("CLOSENOTIFICATION", CloseNotificationConnection);
   AddCommand("SENDEVENTREPORT", SendEventReport);
   AddCommand("SETBAUDRATE", SetBaudRate);
   AddCommand("HELP", DisplayHelp);
   AddCommand("QUERYMEMORY", QueryMemory);

   /* Go ahead and make sure that the input command parser buffer is    */
   /* fully empty.                                                      */
   BTPS_MemInitialize(Input, 0, sizeof(Input));
}

   /* The following function is responsible for choosing the user       */
   /* interface to present to the user.                                 */
static void UserInterface_Selection(void)
{
   /* Next display the available commands.                              */
   DisplayHelp(NULL);

   ClearCommands();

   AddCommand("SERVER", ServerMode);
   AddCommand("CLIENT", ClientMode);
   AddCommand("HELP", DisplayHelp);

   /* Go ahead and make sure that the input command parser buffer is    */
   /* fully empty.                                                      */
   BTPS_MemInitialize(Input, 0, sizeof(Input));
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
            if(MAPID)
               CloseServer(NULL);

            if(MAPID)
               CloseConnection(NULL);

            /* Return to the Selection Menu.                            */
            UI_Mode = UI_MODE_SELECT;
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
         Input        += BTPS_StringLength(TempCommand->Command) + 1;
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
               Input        += BTPS_StringLength(LastParameter) + 1;
               StringLength -= BTPS_StringLength(LastParameter) + 1;

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
   /* A negative return value implies that command was not found and is */
   /* invalid.                                                          */
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
   unsigned int      CommandLength;
   unsigned int      Index;
   CommandFunction_t ret_val;

   /* First, make sure that the command specified is semi-valid.        */
   if(Command)
   {
      /* Now loop through each element in the table to see if there is  */
      /* a match.                                                       */
      for(Index=0, ret_val = NULL; ((Index<NumberCommands) && (!ret_val)); Index++)
      {
         CommandLength = BTPS_StringLength(CommandTable[Index].CommandName);

         if((BTPS_StringLength(Command) == CommandLength) && (BTPS_MemCompare(Command, CommandTable[Index].CommandName, CommandLength) == 0))
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
   Display(("Usage: %s.\r\n",UsageString));
}

   /* Displays a function error message.                                */
static void DisplayFunctionError(char *Function, int Status)
{
   Display(("%s Failed: %d.\r\n", Function, Status));
}

   /* Displays a function success message.                              */
static void DisplayFunctionSuccess(char *Function)
{
   Display(("%s success.\r\n",Function));
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
   int                        ret_val = 0;
   char                       BluetoothAddress[16];
   Byte_t                     Status;
   BD_ADDR_t                  BD_ADDR;
   HCI_Version_t              HCIVersion;
   L2CA_Link_Connect_Params_t L2CA_Link_Connect_Params;

   /* First check to see if the Stack has already been opened.          */
   if(!BluetoothStackID)
   {
      /* Next, makes sure that the Driver Information passed appears to */
      /* be semi-valid.                                                 */
      if((HCI_DriverInformation) && (BTPS_Initialization))
      {

         /* Initialize BTPSKNRL.                                        */
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

            /* Set the Name for the initial use.                        */
            GAP_Set_Local_Device_Name (BluetoothStackID,APP_DEMO_NAME);

            /* Go ahead and allow Master/Slave Role Switch.             */
            L2CA_Link_Connect_Params.L2CA_Link_Connect_Request_Config  = cqAllowRoleSwitch;
            L2CA_Link_Connect_Params.L2CA_Link_Connect_Response_Config = csMaintainCurrentRole;

            L2CA_Set_Link_Connection_Configuration(BluetoothStackID, &L2CA_Link_Connect_Params);

            if(HCI_Command_Supported(BluetoothStackID, HCI_SUPPORTED_COMMAND_WRITE_DEFAULT_LINK_POLICY_BIT_NUMBER) > 0)
               HCI_Write_Default_Link_Policy_Settings(BluetoothStackID, (HCI_LINK_POLICY_SETTINGS_ENABLE_MASTER_SLAVE_SWITCH|HCI_LINK_POLICY_SETTINGS_ENABLE_SNIFF_MODE), &Status);

            /* Delete all Stored Link Keys.                             */
            ASSIGN_BD_ADDR(BD_ADDR, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);

            DeleteLinkKey(BD_ADDR);
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
      /* Check to see if there is currently a Connection or Server Open.*/
      if(MAPID)
      {
         /* There is currently a Server or Connection open, check to see*/
         /* if the Current Service Type is Client.                      */
         if(UI_Mode == UI_MODE_IS_CLIENT)
         {
            /* There is a Client Connection open, Close the Connection. */
            CloseConnection(NULL);
         }
         else
         {
            /* There is a Server open, Close the Server.                */
            CloseServer(NULL);
         }
      }

      HCI_Un_Register_Callback(BluetoothStackID, HCIEventCallbackHandle);

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
      Result = GAP_Set_Pairability_Mode(BluetoothStackID, pmPairableMode);

      /* Next, check the return value of the GAP Set Pairability mode   */
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
            ASSIGN_BD_ADDR(LinkKeyInfo[Result].BD_ADDR, 0, 0, 0, 0, 0, 0);

            break;
         }
      }
   }

   return(Result);
}

   /* The following function is responsible for displaying the current  */
   /* Command Options for either MAP Client or Server.  The input       */
   /* parameter to this function is completely ignored, and only needs  */
   /* to be passed in because all Commands that can be entered at the   */
   /* Prompt pass in the parsed information.  This function displays the*/
   /* current Command Options that are available and always returns     */
   /* zero.                                                             */
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
      Display(("*                  SetPairabilityMode, ServiceDiscovery,         *\r\n"));
      Display(("*                  ChangeSimplePairingParameters,                *\r\n"));
      Display(("*                  GetLocalAddress, GetLocalName, SetLocalName,  *\r\n"));
      Display(("*                  GetClassOfDevice, SetClassOfDevice,           *\r\n"));
      Display(("*                  GetRemoteName, Open, Close,                   *\r\n"));
      Display(("*                  OpenNotification, CloseNotification,          *\r\n"));
      Display(("*                  RequestNotification, SetFolder,               *\r\n"));
      Display(("*                  GetFolderListing, GetMessage,                 *\r\n"));
      Display(("*                  GetMessageListing, SetMessageStatus,          *\r\n"));
      Display(("*                  PushMessage, UpdateInbox, Abort,              *\r\n"));
      Display(("*                  SetBaudRate,                                  *\r\n"));
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
         Display(("*                  SetPairabilityMode, ServiceDiscovery,         *\r\n"));
         Display(("*                  ChangeSimplePairingParameters,                *\r\n"));
         Display(("*                  GetLocalAddress, GetLocalName, SetLocalName,  *\r\n"));
         Display(("*                  GetClassOfDevice, SetClassOfDevice,           *\r\n"));
         Display(("*                  GetRemoteName, OpenServer, CloseServer,       *\r\n"));
         Display(("*                  OpenNotification, CloseNotification,          *\r\n"));
         Display(("*                  SendEventReport, SetBaudRate,                 *\r\n"));
         Display(("*                  Help, Quit                                    *\r\n"));
         Display(("******************************************************************\r\n"));
      }
      else
      {
         Display(("\r\n"));
         Display(("******************************************************************\r\n"));
         Display(("* Command Options: Server, Client, Help                          *\r\n"));
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
   int Result;
   int ret_val = 0;

   /* First, check that valid Bluetooth Stack ID exists.                */
   if(BluetoothStackID)
   {
      /* Use the GAP_Perform_Inquiry() function to perform an Inquiry.  */
      /* The Inquiry will last 10 seconds or until MAX_INQUIRY_RESULTS  */
      /* Bluetooth Devices are found.  When the Inquiry Results become  */
      /* available the GAP_Event_Callback is called.                    */
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
   /* display the current Inquiry List (with Indexes).  This is useful  */
   /* in case the user has forgotten what Inquiry Index a particular    */
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
   int                     Result;
   int                     ret_val;
   char                   *Mode;
   GAP_Pairability_Mode_t  PairabilityMode;

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

         /* Finally map the Man in the Middle (MITM) Protection valid.  */
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
      /* Next, make sure that we are not already connected.             */
      if(!Connected)
      {
         /* There are currently no active connections, make sure that   */
         /* all of the parameters required for this function appear to  */
         /* be at least semi-valid.                                     */
         if((TempParam) && (TempParam->NumberofParameters > 0) && (TempParam->Params[0].intParam) && (NumberofValidResponses) && (TempParam->Params[0].intParam <= NumberofValidResponses) && (!COMPARE_NULL_BD_ADDR(InquiryResultList[(TempParam->Params[0].intParam - 1)])))
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
               Display(("GAP_Initiate_Bonding(%s): Success.\r\n", (BondingType == btDedicated)?"Dedicated":"General"));

               /* Flag success to the caller.                           */
               ret_val = 0;
            }
            else
            {
               /* Display a message indicating that an error occurred   */
               /* while initiating bonding.                             */
               DisplayFunctionError("GAP_Initiate_Bonding", Result);

               ret_val = FUNCTION_ERROR;
            }
         }
         else
         {
            /* One or more of the necessary parameters is/are invalid.  */
            DisplayUsage("Pair [Inquiry Index] [0 = Dedicated, 1 = General (optional)]");

            ret_val = INVALID_PARAMETERS_ERROR;
         }
      }
      else
      {
         /* Display an error to the user describing that Pairing can    */
         /* only occur when we are not connected.                       */
         Display(("Only valid when not connected.\r\n"));

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
   int Result;
   int ret_val;

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
   int   Result;
   int   ret_val;
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

   /* The following function is responsible for querying the Bluetooth  */
   /* Device Name of the specified remote Bluetooth Device.  This       */
   /* function returns zero on successful execution and a negative value*/
   /* on all errors.                                                    */
static int GetRemoteName(ParameterList_t *TempParam)
{
   int Result;
   int ret_val;

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

   /* The following function is responsible for issuing a Service Search*/
   /* Attribute Request to a Remote SDP Server.  This function returns  */
   /* zero if successful and a negative value if an error occurred.     */
static int ServiceDiscovery(ParameterList_t *TempParam)
{
   int                           Result;
   int                           ret_val;
   int                           Index;
   SDP_UUID_Entry_t              SDPUUIDEntry;
   SDP_Attribute_ID_List_Entry_t AttributeID;

   /* First, check that valid Bluetooth Stack ID exists.                */
   if(BluetoothStackID)
   {
      /* Now let's make sure that all of the parameters required for    */
      /* this function appear to be at least semi-valid.                */
      if((TempParam) && (TempParam->NumberofParameters > 1) && (((TempParam->Params[1].intParam) && (TempParam->Params[1].intParam <= NUM_UUIDS)) || ((!TempParam->Params[1].intParam) && (TempParam->NumberofParameters > 2))) && (TempParam->Params[0].intParam) && (NumberofValidResponses) && (TempParam->Params[0].intParam <= NumberofValidResponses) && (!COMPARE_NULL_BD_ADDR(InquiryResultList[(TempParam->Params[0].intParam - 1)])))
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
         DisplayUsage("ServiceDiscovery [Inquiry Index] [Profile Index] [16/32 bit UUID (Manual only)]");
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

   /* The following function is responsible for creating a local MAP    */
   /* Server.  This function returns zero if successful and a negative  */
   /* value if an error occurred.                                       */
static int OpenServer(ParameterList_t *TempParam)
{
   int ret_val;
   int Result;

   /* First check to see if a valid Bluetooth Stack ID exists.          */
   if(BluetoothStackID)
   {
      /* Now check to make sure that the Server Port ID passed to this  */
      /* function is valid.                                             */
      if((TempParam) && (TempParam->NumberofParameters >= 2) && (TempParam->Params[0].intParam) && (TempParam->Params[1].strParam) && (BTPS_StringLength(TempParam->Params[1].strParam)))
      {
         /* Now check to make sure that a Server doesn't already exist. */
         if(!MAPID)
         {
            BTPS_StringCopy(ReceiveMessagePath, TempParam->Params[1].strParam);

            /* Make sure the Received Messages Path ends in '/'         */
            /* character.  (this way we can append the received file    */
            /* names directly to the path).                             */
            if(ReceiveMessagePath[BTPS_StringLength(ReceiveMessagePath) - 1] != '/')
               ReceiveMessagePath[BTPS_StringLength(ReceiveMessagePath)] = '/';

            /* The above parameters are valid, attempt to open a Local  */
            /* MAP Port.                                                */
            Result = MAP_Open_Message_Access_Server(BluetoothStackID, TempParam->Params[0].intParam, MAP_Event_Callback_Server, 0);

            /* Now check to see if the function call was successfully   */
            /* made.                                                    */
            if(Result > 0)
            {
               /* Now that a Service Name has been created try and      */
               /* Register the SDP Record.                              */
               if(!MAP_Register_Message_Access_Server_SDP_Record(BluetoothStackID, Result, "MAP Server", 1, MAP_SERVER_SUPPORTED_MESSAGE_TYPE_EMAIL, &ServerSDPRecordHandle))
               {
                  /* The Server was opened successfully and the SDP     */
                  /* Record was successfully added.  The return value of*/
                  /* the call is the MAP ID and is required for future  */
                  /* MAP Profile calls.                                 */
                  MAPID   = Result;

                  ret_val = 0;

                  Display(("MAP_Open_Message_Access_Server() Successful (ID = %04X).\r\n", MAPID));
                  Display(("  Received Messages Path: %s\r\n", ReceiveMessagePath));
               }
               else
               {
                  /* Error registering SDP Record, go ahead and close   */
                  /* down the server.                                   */
                  MAP_Close_Server(BluetoothStackID, Result);

                  Display(("MAP_Register_Message_Access_Server_SDP_Record() Failure.\r\n"));

                  ret_val = FUNCTION_ERROR;
               }
            }
            else
            {
               /* There was an error opening the Server.                */
               Display(("MAP_Open_Message_Access_Server() Failure: %d.\r\n", Result));

               ret_val = Result;
            }
         }
         else
         {
            /* A Connection is already open, this program only supports */
            /* one Server or Client at a time.                          */
            Display(("MAP Server already open.\r\n"));

            ret_val = FUNCTION_ERROR;
         }
      }
      else
      {
         DisplayUsage("OpenServer [Port Number] [Received Messages Path]");

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

   /* The following function is responsible for deleting a local MAP    */
   /* Server that was created via a successful call to the OpenServer() */
   /* function.  This function returns zero if successful and a negative*/
   /* value if an error occurred.                                       */
static int CloseServer(ParameterList_t *TempParam)
{
   int Result;
   int ret_val;

   /* First check to see if a valid Bluetooth Stack ID exists.          */
   if(BluetoothStackID)
   {
      if(MAPID)
      {
         /* Free any allocated data buffers for ongoing transactions.   */
         if((CurrentOperation != coNone) && (CurrentBuffer))
         {
            BTPS_FreeMemory(CurrentBuffer);

            CurrentBuffer = NULL;
         }

         /* MAP Server opened, close any open SDP Record.               */
         if(ServerSDPRecordHandle)
         {
            MAP_Un_Register_SDP_Record(BluetoothStackID, MAPID, ServerSDPRecordHandle);

            ServerSDPRecordHandle = 0;
         }

         /* Next attempt to close this Server.                          */
         Result = MAP_Close_Server(BluetoothStackID, MAPID);

         if(!Result)
         {
            /* Display a message indicating that the server was         */
            /* successfully closed.                                     */
            Display(("MAP_Close_Server() Success.\r\n"));

            /* Flag the port has been closed.                           */
            MAPID            = 0;

            ret_val          = 0;

            CurrentOperation = coNone;

            /* If a Notification Connection is present, go ahead and    */
            /* close it as well.                                        */
            if((NotificationConnected) && (MAPNID))
               CloseNotificationConnection(NULL);
         }
         else
         {
            /* An error occurred while attempting to close the server.  */
            Display(("MAP_Close_Server() Failure: %d.\r\n", Result));

            ret_val = Result;
         }
      }
      else
      {
         /* No valid MAP Server exists.                                 */
         Display(("No Server is currently open.\r\n"));

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

   /* The following function is responsible for creating a local MAP    */
   /* Notification Server.  This function returns zero if successful and*/
   /* a negative value if an error occurred.                            */
static int OpenNotificationServer(ParameterList_t *TempParam)
{
   int Result;
   int ret_val;

   /* First check to see if a valid Bluetooth Stack ID exists.          */
   if(BluetoothStackID)
   {
      /* Now check to make sure that the Server Port ID passed to this  */
      /* function is valid.                                             */
      if((TempParam) && (TempParam->NumberofParameters >= 1) && (TempParam->Params[0].intParam))
      {
         /* Now check to make sure that there is already a connection.  */
         if((Connected) && (MAPID))
         {
            /* Now check to make sure that a Server doesn't already     */
            /* exist.                                                   */
            if(!MAPNID)
            {
               /* The above parameters are valid, attempt to open a     */
               /* Local MAP Port.                                       */
               Result = MAP_Open_Message_Notification_Server(BluetoothStackID, TempParam->Params[0].intParam, MAPID, MAP_Event_Callback_Client, 0);

               /* Now check to see if the function call was successfully*/
               /* made.                                                 */
               if(Result > 0)
               {
                  /* Register the SDP Record.                           */
                  if(!MAP_Register_Message_Notification_Server_SDP_Record(BluetoothStackID, Result, "MAP Notification Server", &NotificationSDPRecordHandle))
                  {
                     /* The Server was opened successfully and the SDP  */
                     /* Record was successfully added.  The return value*/
                     /* of the call is the MAP ID and is required for   */
                     /* future MAP Profile calls.                       */
                     MAPNID  = Result;

                     Display(("MAP_Open_Message_Notification_Server() Successful (ID = %04X).\r\n", MAPNID));

                     ret_val = 0;
                  }
                  else
                  {
                     /* Error registering SDP Record, go ahead and close*/
                     /* down the server.                                */
                     MAP_Close_Server(BluetoothStackID, Result);

                     Display(("MAP_Register_Message_Notification_Server_SDP_Record() Failure.\r\n"));

                     ret_val = FUNCTION_ERROR;
                  }
               }
               else
               {
                  /* There was an error opening the Server.             */
                  Display(("MAP_Open_Message_Notification_Server() Failure: %d.\r\n", Result));

                  ret_val = FUNCTION_ERROR;
               }
            }
            else
            {
               /* A Server is already registered.                       */
               Display(("Server already registered.\r\n"));

               ret_val = INVALID_PARAMETERS_ERROR;
            }
         }
         else
         {
            /* Client is NOT connected.                                 */
            Display(("MAP Client is not connected.\r\n"));

            ret_val = INVALID_PARAMETERS_ERROR;
         }
      }
      else
      {
         DisplayUsage("OpenNotification [Port Number]");

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

   /* The following function is responsible for deleting a local MAP    */
   /* Notification Server that was created via a successful call to the */
   /* OpenNotificationServer() function.  This function returns zero if */
   /* successful and a negative value if an error occurred.             */
static int CloseNotificationServer(ParameterList_t *TempParam)
{
   int Result;
   int ret_val;

   /* First check to see if a valid Bluetooth Stack ID exists.          */
   if(BluetoothStackID)
   {
      if(MAPNID)
      {
         /* MAP Server opened, close any open SDP Record.               */
         if(NotificationSDPRecordHandle)
         {
            MAP_Un_Register_SDP_Record(BluetoothStackID, MAPNID, NotificationSDPRecordHandle);

            ServerSDPRecordHandle = 0;
         }

         /* Next attempt to close this Server.                          */
         Result = MAP_Close_Server(BluetoothStackID, MAPNID);

         if(!Result)
         {
            /* Display a message indicating that the server was         */
            /* successfully closed.                                     */
            Display(("MAP_Close_Server() Success.\r\n"));

            /* Flag the port has been closed.                           */
            MAPNID                = 0;
            NotificationConnected = FALSE;

            ret_val               = 0;
         }
         else
         {
            /* An error occurred while attempting to close the server.  */
            Display(("MAP_Close_Server() Failure: %d.\r\n", Result));

            ret_val = FUNCTION_ERROR;
         }
      }
      else
      {
         /* No valid MAP Server exists.                                 */
         Display(("No Server is currently open.\r\n"));

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

   /* The following function is responsible for initiating a connection */
   /* with a Remote MAP Server.  This function returns zero if          */
   /* successful and a negative value if an error occurred.             */
static int OpenRemoteServer(ParameterList_t *TempParam)
{
   int       Result;
   int       ret_val;
   char      BoardStr[16];

   /* First check to see if a valid Bluetooth Stack ID exists.          */
   if(BluetoothStackID)
   {
      /* Now check to make sure that a Client doesn't already exist.    */
      if(!MAPID)
      {
         /* A Client port is not already open, now check to make sure   */
         /* that the parameters required for this function appear to be */
         /* valid.                                                      */
         if((TempParam) && (TempParam->NumberofParameters > 1) && (TempParam->Params[0].intParam) && (NumberofValidResponses) && (TempParam->Params[0].intParam<=NumberofValidResponses) && (!COMPARE_NULL_BD_ADDR(InquiryResultList[(TempParam->Params[0].intParam-1)])))
         {
            /* Now check to make sure that the Server Port ID passed to */
            /* this function is valid.                                  */
            if(TempParam->Params[1].intParam)
            {
               BD_ADDRToStr(InquiryResultList[(TempParam->Params[0].intParam-1)], BoardStr);

               /* The above parameters are valid, inform the user that  */
               /* the program is about to open a Remote MAP Port.       */
               Display(("Opening Remote MAP Server (BD_ADDR = %s, Port = %04X)\r\n", BoardStr, TempParam->Params[1].intParam));

               /* Attempt to open a connection to the selected remote   */
               /* MAP Access Profile Server.                            */
               Result = MAP_Open_Remote_Message_Access_Server_Port(BluetoothStackID, InquiryResultList[(TempParam->Params[0].intParam-1)], TempParam->Params[1].intParam, MAP_Event_Callback_Client, 0);

               /* Now check to see if the function call was successfully*/
               /* made.                                                 */
               if(Result > 0)
               {
                  /* The Client was opened successfully.  The return    */
                  /* value of the call is the MAP ID and is required for*/
                  /* future MAP Profile calls.                          */
                  MAPID     = Result;

                  Connected = TRUE;

                  ret_val   = 0;

                  Display(("MAP_Open_Remote_Message_Access_Server_Port: Function Successful (ID = %04X).\r\n", MAPID));
               }
               else
               {
                  /* There was an error opening the Client.             */
                  Display(("MAP_Open_Remote_Message_Access_Server_Port() Failure: %d.\r\n", Result));

                  ret_val = Result;
               }
            }
            else
            {
               DisplayUsage("Connect [Inquiry Index] [Server Port]");

               ret_val = INVALID_PARAMETERS_ERROR;
            }
         }
         else
         {
            DisplayUsage("Connect [Inquiry Index] [Server Port]");

            ret_val = INVALID_PARAMETERS_ERROR;
         }
      }
      else
      {
         /* A Connection is already open, this program only supports one*/
         /* Server or Client at a time.                                 */
         Display(("Port already open.\r\n"));

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

   /* The following function is responsible for terminating a connection*/
   /* with a remote MAP Client or Server.  This function returns zero   */
   /* if successful and a negative value if an error occurred.          */
static int CloseConnection(ParameterList_t *TempParam)
{
   int ret_val;
   int Result;

   /* First check to see if a valid Bluetooth Stack exists.             */
   if(BluetoothStackID)
   {
      /* Check to see if the MAPID appears to be semi-valid.  This      */
      /* parameter will only be valid if a Connection is open or a      */
      /* Server is Open.                                                */
      if(MAPID)
      {
         /* Free any allocated data buffers for ongoing transactions.   */
         if((CurrentOperation != coNone) && (CurrentBuffer))
         {
            BTPS_FreeMemory(CurrentBuffer);

            CurrentBuffer = NULL;
         }

         /* The Current MAPID appears to be semi-valid.  Now try to     */
         /* close the connection.                                       */
         Result = MAP_Close_Connection(BluetoothStackID, MAPID);

         if(!Result)
         {
            /* The function was called successfully.  Display a message */
            /* indicating that the Connection Close was successfully    */
            /* submitted.                                               */
            Display(("MAP_Close_Connection: Function Successful.\r\n"));

            Connected        = FALSE;
            CurrentOperation = coNone;

            ret_val          = 0;

            if(UI_Mode == UI_MODE_IS_CLIENT)
               MAPID = 0;
         }
         else
         {
            /* An error occurred while attempting to close the          */
            /* Connection.                                              */
            Display(("MAP_Close_Connection() Failure: %d.\r\n", Result));

            ret_val = Result;
         }
      }
      else
      {
         /* The Current MAPID is invalid so no Connection or Server is  */
         /* open.                                                       */
         Display(("Invalid MAP ID: MAP Close Connection.\r\n"));

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

   /* The following function is responsible for initiating a connection */
   /* with a Remote MAP Notification Server.  This function returns zero*/
   /* if successful and a negative value if an error occurred.          */
static int OpenRemoteNotificationServer(ParameterList_t *TempParam)
{
   int Result;
   int ret_val;

   /* First check to see if a valid Bluetooth Stack ID exists.          */
   if(BluetoothStackID)
   {
      /* First, make sure that a connection already exists (MAP).       */
      if((Connected) && (MAPID))
      {
         /* Now check to make sure that a Client doesn't already exist  */
         /* (Notification).                                             */
         if(!MAPNID)
         {
            /* A Client port is not already open, now check to make sure*/
            /* that the parameters required for this function appear to */
            /* be valid.                                                */
            if((TempParam) && (TempParam->NumberofParameters > 0) && (TempParam->Params[0].intParam))
            {
               /* The above parameters are valid, inform the user that  */
               /* the program is about to open a Remote MAP Notification*/
               /* Port.                                                 */
               Display(("Opening Remote MAP Notification Server (Port = %04X)\r\n", TempParam->Params[0].intParam));

               /* Attempt to open a connection to the selected remote   */
               /* MAP Access Profile Server.                            */
               Result = MAP_Open_Remote_Message_Notification_Server_Port(BluetoothStackID, MAPID, TempParam->Params[0].intParam, MAP_Event_Callback_Server, 0);

               /* Now check to see if the function call was successfully*/
               /* made.                                                 */
               if(Result > 0)
               {
                  /* The Client was opened successfully.  The return    */
                  /* value of the call is the MAP Notification ID and is*/
                  /* required for future MAP Profile calls.             */
                  MAPNID   = Result;

                  ret_val  = 0;

                  Display(("MAP_Open_Remote_Message_Notification_Server_Port: Function Successful (ID = %04X).\r\n", MAPNID));
               }
               else
               {
                  /* There was an error opening the Client.             */
                  Display(("MAP_Open_Remote_Message_Notification_Server_Port() Failure: %d.\r\n", Result));

                  ret_val = Result;
               }
            }
            else
            {
               DisplayUsage("OpenNotification [Server Port]");

               ret_val = INVALID_PARAMETERS_ERROR;
            }
         }
         else
         {
            /* Map Notification Client already open.                    */
            Display(("Map Notification Client already open.\r\n"));

            ret_val = INVALID_PARAMETERS_ERROR;
         }
      }
      else
      {
         /* MAP Server connection was not connected.                    */
         Display(("MAP Connection does not exist.\r\n"));

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

   /* The following function is responsible for terminating a connection*/
   /* with a remote MAP Notification Client or Server.  This function   */
   /* returns zero if successful and a negative value if an error       */
   /* occurred.                                                         */
static int CloseNotificationConnection(ParameterList_t *TempParam)
{
   int ret_val;
   int Result;

   /* First check to see if a valid Bluetooth Stack exists.             */
   if(BluetoothStackID)
   {
      /* Check to see if the MAP Notification ID appears to be          */
      /* semi-valid.  This parameter will only be valid if a Connection */
      /* is open.                                                       */
      if(MAPNID)
      {
         /* The Current MAP Notification ID appears to be semi-valid.   */
         /* Now try to close the connection.                            */
         Result = MAP_Close_Connection(BluetoothStackID, MAPNID);

         if(!Result)
         {
            /* The function was called successfully.  Display a message */
            /* indicating that the Connection Close was successfully    */
            /* submitted.                                               */
            Display(("MAP_Close_Connection: Function Successful.\r\n"));

            NotificationConnected  = FALSE;
            MAPNID                 = 0;

            ret_val                = 0;
         }
         else
         {
            /* An error occurred while attempting to close the          */
            /* Connection.                                              */
            Display(("MAP_Close_Connection() Failure: %d.\r\n", Result));

            ret_val = Result;
         }
      }
      else
      {
         /* The Current MAP Notification ID is invalid so no Connection */
         /* open.                                                       */
         Display(("Invalid MAP Notification ID: MAP Close Connection.\r\n"));

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

   /* The following function is responsible for issuing an Abort with a */
   /* remote MAP Server by issuing an OBEX Abort Command.  This         */
   /* function returns zero if successful and a negative value if an    */
   /* error occurred.                                                   */
static int Abort(ParameterList_t *TempParam)
{
   int Result;
   int ret_val;

   /* First check to see if a valid Bluetooth Stack ID exists.          */
   if(BluetoothStackID)
   {
      /* Now check to make sure that the MAP ID appears to be           */
      /* semi-valid.                                                    */
      if(MAPID)
      {
         /* The MAP ID appears to be is a semi-valid value.  Now submit */
         /* the command to begin this operation.                        */
         Result = MAP_Abort_Request(BluetoothStackID, MAPID);

         if(!Result)
         {
            /* The function was submitted successfully.                 */

            /* Update the Current Operation.                            */
            CurrentOperation = coAbort;

            ret_val          = 0;

            Display(("MAP_Abort_Request: Function Successful.\r\n"));
         }
         else
         {
            /* There was an error submitting the function.              */
            Display(("MAP_Abort_Request() Failure: %d.\r\n", Result));

            ret_val = Result;
         }
      }
      else
      {
         /* One or more of the parameters are invalid.                  */
         Display(("MAP Abort Request: Invalid MAPID.\r\n"));

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

   /* The following function is responsible for issuing a MAP Set Folder*/
   /* Request to a remote MAP Server.  This function returns zero if    */
   /* successful and a negative value if an error occurred.             */
static int SetFolder(ParameterList_t *TempParam)
{
   int Result;
   int ret_val;
   int Index;

   /* First check to see if a valid Bluetooth Stack ID exists.          */
   if(BluetoothStackID)
   {
      /* Now check to make sure that the MAP ID appears to be           */
      /* semi-valid.                                                    */
      if(MAPID)
      {
         /* The MAP ID appears to be is a semi-valid value.  Next, check*/
         /* to see if required parameters were specified.               */
         if((TempParam) && (TempParam->NumberofParameters >= 1) && (TempParam->Params[0].intParam >= (int)sfRoot) && (TempParam->Params[0].intParam <= (int)sfUp))
         {
            /* Check to see if a Folder was specified if the down option*/
            /* was specified.                                           */
            if((TempParam->Params[0].intParam != (int)sfDown) || (((TempParam->Params[0].intParam == (int)sfDown) && (TempParam->NumberofParameters >= 2) && (TempParam->Params[1].strParam) && (BTPS_StringLength(TempParam->Params[1].strParam)))))
            {
               /* Folder Name needs to be specified in UTF-16.  Go ahead*/
               /* and simply convert the ASCII into UTF-16 Encoded      */
               /* ASCII.                                                */
               if(TempParam->Params[0].intParam == (int)sfDown)
               {
                  /* Convert from start until the end into UTF-16.      */
                  Index = 0;
                  while(TempParam->Params[1].strParam[Index] && (Index < (SIZEOF_TEMP_BUFFER - 1)))
                  {
                     TempBuffer[Index] = TempParam->Params[1].strParam[Index];

                     Index++;
                  }

                  /* NULL terminate the String.                         */
                  TempBuffer[Index] = 0;
               }
               else
                  TempBuffer [0] = '\0';

               Result = MAP_Set_Folder_Request(BluetoothStackID, MAPID, (MAP_Set_Folder_Option_t)TempParam->Params[0].intParam, TempBuffer);

               if(!Result)
               {
                  /* The function was submitted successfully.           */
                  Display(("MAP_Set_Folder_Request() Successful.\r\n"));

                  /* Update the Current Operation.                      */
                  CurrentOperation = coSetFolder;

                  ret_val          = 0;
               }
               else
               {
                  /* There was an error submitting the function.        */
                  Display(("MAP_Set_Folder_Request() Failure: %d.\r\n", Result));

                  ret_val = Result;
               }
            }
            else
            {
               DisplayUsage("SetFolder [Path Option (0 = Root, 1 = Down, 2 = Up)] [Folder (only required for Down option)]");

               ret_val = INVALID_PARAMETERS_ERROR;
            }
         }
         else
         {
            DisplayUsage("SetFolder [Path Option (0 = Root, 1 = Down, 2 = Up)] [Folder (only required for Down option)]");

            ret_val = INVALID_PARAMETERS_ERROR;
         }
      }
      else
      {
         /* One or more of the parameters are invalid.                  */
         Display(("Set Folder Request: Invalid MAPID.\r\n"));

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

   /* The following function is responsible for issuing a MAP Get Folder*/
   /* Listing Request to a remote MAP Server.  This function returns    */
   /* zero if successful and a negative value if an error occurred.     */
static int GetFolderListing(ParameterList_t *TempParam)
{
   int Result;
   int ret_val;

   /* First check to see if a valid Bluetooth Stack ID exists.          */
   if(BluetoothStackID)
   {
      /* Now check to make sure that the MAP ID appears to be           */
      /* semi-valid.                                                    */
      if(MAPID)
      {
         /* The MAP ID appears to be is a semi-valid value.  Next, check*/
         /* to see if required parameters were specified.               */
         if((TempParam) && (TempParam->NumberofParameters >= 2))
         {
            /* Check to see if a filename was specified to save the     */
            /* result to.                                               */
            if((TempParam->NumberofParameters >= 3) && (TempParam->Params[2].strParam) && (BTPS_StringLength(TempParam->Params[2].strParam)))
               BTPS_StringCopy(CurrentFileName, TempParam->Params[2].strParam);
            else
               CurrentFileName[0] = '\0';

            Result = MAP_Get_Folder_Listing_Request(BluetoothStackID, MAPID, (Word_t)TempParam->Params[0].intParam, (Word_t)TempParam->Params[1].intParam);

            if(!Result)
            {
               /* The function was submitted successfully.              */
               Display(("MAP_Get_Folder_Listing_Request() Successful.\r\n"));

               if((TempParam->Params[0].intParam) && (CurrentFileName[0]))
                  Display(("Filename used to store results: %s\r\n", CurrentFileName));

               /* Update the Current Operation.                         */
               CurrentOperation = coGetFolderListing;

               ret_val          = 0;
            }
            else
            {
               /* There was an error submitting the function.           */
               Display(("MAP_Get_Folder_Listing_Request() Failure: %d.\r\n", Result));

               ret_val = Result;
            }
         }
         else
         {
            DisplayUsage("GetFolderListing [Max List Count] [List Start Offset] [Save Filename (optional)]");

            ret_val = INVALID_PARAMETERS_ERROR;
         }
      }
      else
      {
         /* One or more of the parameters are invalid.                  */
         Display(("Get Folder Listing Request: Invalid MAPID.\r\n"));

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

   /* The following function is responsible for issuing a MAP Get       */
   /* Message Listing Request to a remote MAP Server.  This function    */
   /* returns zero if successful and a negative value if an error       */
   /* occurred.                                                         */
static int GetMessageListing(ParameterList_t *TempParam)
{
   int    Result;
   int    ret_val;
   char  *Folder;
   int    Index;

   /* First check to see if a valid Bluetooth Stack ID exists.          */
   if(BluetoothStackID)
   {
      /* Now check to make sure that the MAP ID appears to be           */
      /* semi-valid.                                                    */
      if(MAPID)
      {
         /* The MAP ID appears to be is a semi-valid value.  Next, check*/
         /* to see if required parameters were specified.               */
         if((TempParam) && (TempParam->NumberofParameters >= 2))
         {
            /* Determine the correct Folder (if specified).             */
            if((TempParam->NumberofParameters >= 3) && (TempParam->Params[2].strParam))
            {
               /* Folder specified, check to see if it was only         */
               /* specified so that the save file could be specified.   */
               if((BTPS_StringLength(TempParam->Params[2].strParam) == 1) && (TempParam->Params[2].strParam[0] == '.'))
                  Folder = NULL;
               else
                  Folder = TempParam->Params[2].strParam;
            }
            else
               Folder = NULL;

            /* Check to see if a filename was specified to save the     */
            /* result to.                                               */
            if((TempParam->NumberofParameters >= 4) && (TempParam->Params[3].strParam) && (BTPS_StringLength(TempParam->Params[3].strParam)))
               BTPS_StringCopy(CurrentFileName, TempParam->Params[3].strParam);
            else
               CurrentFileName[0] = '\0';

            /* Folder Name needs to be specified in UTF-16.  Go ahead   */
            /* and simply convert the ASCII into UTF-16 Encoded ASCII.  */
            if(Folder)
            {
               /* Convert from start until the end into UTF-16.         */
               Index = 0;
               while(Folder[Index] && (Index < (SIZEOF_TEMP_BUFFER - 1)))
               {
                  TempBuffer[Index] = Folder[Index];

                  Index++;
               }

               /* NULL terminate the String.                            */
               TempBuffer[Index] = 0;
            }

            Result = MAP_Get_Message_Listing_Request(BluetoothStackID, MAPID, Folder?TempBuffer:NULL, (Word_t)TempParam->Params[0].intParam, (Word_t)TempParam->Params[1].intParam, NULL);

            if(!Result)
            {
               /* The function was submitted successfully.              */
               Display(("MAP_Get_Message_Listing_Request() Successful.\r\n"));

               if((TempParam->Params[0].intParam) && (CurrentFileName[0]))
                  Display(("Filename used to store results: %s\r\n", CurrentFileName));

               /* Update the Current Operation.                         */
               CurrentOperation = coGetMessageListing;

               ret_val          = 0;
            }
            else
            {
               /* There was an error submitting the function.           */
               Display(("MAP_Get_Message_Listing_Request() Failure: %d.\r\n", Result));

               ret_val = Result;
            }
         }
         else
         {
            DisplayUsage("GetMessageListing [Max List Count] [List Start Offset] [Folder Name (optional - specify '.' to specify Save File Name)] [Save Filename (optional)]");

            ret_val = INVALID_PARAMETERS_ERROR;
         }
      }
      else
      {
         /* One or more of the parameters are invalid.                  */
         Display(("Get Message Listing Request: Invalid MAPID.\r\n"));

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

   /* The following function is responsible for issuing a MAP Get       */
   /* Message Request to a remote MAP Server.  This function returns    */
   /* zero if successful and a negative value if an error occurred.     */
static int GetMessage(ParameterList_t *TempParam)
{
   int           Result;
   int           ret_val;
   MAP_CharSet_t CharSet;

   /* First check to see if a valid Bluetooth Stack ID exists.          */
   if(BluetoothStackID)
   {
      /* Now check to make sure that the MAP ID appears to be           */
      /* semi-valid.                                                    */
      if(MAPID)
      {
         /* The MAP ID appears to be is a semi-valid value.  Next, check*/
         /* to see if required parameters were specified.               */
         if((TempParam) && (TempParam->NumberofParameters >= 3) && (TempParam->Params[0].strParam) && (BTPS_StringLength(TempParam->Params[0].strParam)) && ((TempParam->Params[2].intParam == (int)ftUnfragmented) || (TempParam->Params[2].intParam == (int)ftFirst) || (TempParam->Params[2].intParam == (int)ftLast)))
         {
            /* Note the Character Set.                                  */
            if(TempParam->Params[1].intParam)
               CharSet = csUTF8;
            else
               CharSet = csNative;

            /* Check to see if a filename was specified to save the     */
            /* result to.                                               */
            if((TempParam->NumberofParameters >= 4) && (TempParam->Params[3].strParam) && (BTPS_StringLength(TempParam->Params[3].strParam)))
               BTPS_StringCopy(CurrentFileName, TempParam->Params[3].strParam);
            else
               CurrentFileName[0] = '\0';

            Result = MAP_Get_Message_Request(BluetoothStackID, MAPID, TempParam->Params[0].strParam, FALSE, CharSet, (MAP_Fractional_Type_t)TempParam->Params[1].intParam);

            if(!Result)
            {
               /* The function was submitted successfully.              */
               Display(("MAP_Get_Message_Request() Successful.\r\n"));

               if(CurrentFileName[0])
                  Display(("Filename used to store results: %s\r\n", CurrentFileName));

               /* Update the Current Operation.                         */
               CurrentOperation = coGetMessage;

               ret_val          = 0;
            }
            else
            {
               /* There was an error submitting the function.           */
               Display(("MAP_Get_Message_Request() Failure: %d.\r\n", Result));

               ret_val = Result;
            }
         }
         else
         {
            DisplayUsage("GetMessage [Message Handle] [Char Set (0 = Native, 1 = UTF8)] [Fractional Type (0 = Unfragmented, 1 = First, 4 = Last)] [Save Filename (optional)]");

            ret_val = INVALID_PARAMETERS_ERROR;
         }
      }
      else
      {
         /* One or more of the parameters are invalid.                  */
         Display(("Get Message Request: Invalid MAPID.\r\n"));

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

   /* The following function is responsible for issuing a MAP Set       */
   /* Message Status Request to the remote MAP Server.  This function   */
   /* returns zero if successful and a negative value if an error       */
   /* occurred.                                                         */
static int SetMessageStatus(ParameterList_t *TempParam)
{
   int Result;
   int ret_val;

   /* First check to see if a valid Bluetooth Stack ID exists.          */
   if(BluetoothStackID)
   {
      /* Now check to make sure that the MAP ID appears to be           */
      /* semi-valid.                                                    */
      if(MAPID)
      {
         /* The MAP ID appears to be is a semi-valid value.  Next, check*/
         /* to see if required parameters were specified.               */
         if((TempParam) && (TempParam->NumberofParameters >= 3) && (TempParam->Params[0].strParam) && (BTPS_StringLength(TempParam->Params[0].strParam)) && (TempParam->Params[1].intParam >= (int)siReadStatus) && (TempParam->Params[1].intParam <= (int)siDeletedStatus))
         {
            Result = MAP_Set_Message_Status_Request(BluetoothStackID, MAPID, TempParam->Params[0].strParam, (MAP_Status_Indicator_t)TempParam->Params[1].intParam, TempParam->Params[2].intParam);

            if(!Result)
            {
               /* The function was submitted successfully.              */
               Display(("MAP_Set_Message_Status_Request() Successful.\r\n"));

               /* Update the Current Operation.                         */
               CurrentOperation = coSetMessageStatus;

               ret_val          = 0;
            }
            else
            {
               /* There was an error submitting the function.           */
               Display(("MAP_Set_Message_Status_Request() Failure: %d.\r\n", Result));

               ret_val = Result;
            }
         }
         else
         {
            DisplayUsage("SetMessageStatus [Message Handle] [Status Indicator (0 = Read Status, 1 = Delete Status)] [Status Value]");

            ret_val = INVALID_PARAMETERS_ERROR;
         }
      }
      else
      {
         /* One or more of the parameters are invalid.                  */
         Display(("Set Message Status: Invalid MAPID.\r\n"));

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

   /* The following function is responsible for issuing a MAP Push      */
   /* Message Request to a remote MAP Server.  This function returns    */
   /* zero if successful and a negative value if an error occurred.     */
static int PushMessage(ParameterList_t *TempParam)
{
   int           Result;
   int           ret_val;

   /* First check to see if a valid Bluetooth Stack ID exists.          */
   if(BluetoothStackID)
   {
      /* Now check to make sure that the MAP ID appears to be           */
      /* semi-valid.                                                    */
      if(MAPID)
      {
         /* The MAP ID appears to be is a semi-valid value.  Next, check*/
         /* to see if required parameters were specified.               */
         if((TempParam) && (TempParam->NumberofParameters >= 2))
         {
            CurrentBufferSize = BTPS_StringLength(DataStr);
            CurrentBuffer     = (char *)BTPS_AllocateMemory(CurrentBufferSize);

            BTPS_MemCopy(CurrentBuffer, DataStr, CurrentBufferSize);

            Result = MAP_Push_Message_Request(BluetoothStackID, MAPID, NULL, TempParam->Params[0].intParam, TempParam->Params[1].intParam, csUTF8, CurrentBufferSize, (Byte_t *)CurrentBuffer, &CurrentBufferSent, TRUE);

            if(!Result)
            {
               /* The function was submitted successfully.              */
               Display(("MAP_Push_Message_Request() Successful.\r\n"));

               /* Update the Current Operation.                         */
               CurrentOperation = coPushMessage;

               ret_val          = 0;
            }
            else
            {
               /* There was an error submitting the function.           */
               Display(("MAP_Push_Message_Request() Failure: %d.\r\n", Result));

               /* Free any data that was allocated.                     */
               BTPS_FreeMemory(CurrentBuffer);

               CurrentBuffer     = NULL;
               CurrentBufferSize = 0;
               CurrentBufferSent = 0;

               ret_val = Result;
            }
         }
         else
         {
            Display(("Usage: PushMessage [Transparent (0 = False, 1 = True)] [Retry (0 = False, 1 = True)] [Message File Name].\r\n"));

            ret_val = INVALID_PARAMETERS_ERROR;
         }
      }
      else
      {
         /* One or more of the parameters are invalid.                  */
         Display(("Get Message Request: Invalid MAPID.\r\n"));

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

   /* The following function is responsible for issuing a MAP           */
   /* UpdateInbox Request to the remote MAP Server.  This function      */
   /* returns zero if successful and a negative value if an error       */
   /* occurred.                                                         */
static int UpdateInbox(ParameterList_t *TempParam)
{
   int Result;
   int ret_val;

   /* First check to see if a valid Bluetooth Stack ID exists.          */
   if(BluetoothStackID)
   {
      /* Now check to make sure that the MAP ID appears to be           */
      /* semi-valid.                                                    */
      if(MAPID)
      {
         /* The MAP ID appears to be is a semi-valid value.  Now submit */
         /* the command to begin this operation.                        */
         Result = MAP_Update_Inbox_Request(BluetoothStackID, MAPID);

         if(!Result)
         {
            /* The function was submitted successfully.                 */

            /* Update the Current Operation.                            */
            CurrentOperation = coUpdateInbox;

            ret_val          = 0;

            Display(("MAP_Update_Inbox_Request: Function Successful.\r\n"));
         }
         else
         {
            /* There was an error submitting the function.              */
            Display(("MAP_Update_Inbox_Request() Failure: %d.\r\n", Result));

            ret_val = Result;
         }
      }
      else
      {
         /* One or more of the parameters are invalid.                  */
         Display(("MAP Update Inbox Request: Invalid MAPID.\r\n"));

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

   /* The following function is responsible for issuing a MAP Send      */
   /* Status Event Request to the remote MAP Server.  This function     */
   /* returns zero if successful and a negative value if an error       */
   /* occurred.                                                         */
static int SendEventReport(ParameterList_t *TempParam)
{
   int ret_val;

   /* First check to see if a valid Bluetooth Stack ID exists.          */
   if(BluetoothStackID)
   {
      /* Now check to make sure that the MAP ID appears to be           */
      /* semi-valid.                                                    */
      if((MAPID) && (MAPNID))
      {
         /* The MAP ID and MAP Notification ID appears to be is a       */
         /* semi-valid value.  Next, check to see if required parameters*/
         /* were specified.                                             */
         if((TempParam->NumberofParameters >= 3) && (TempParam->Params[0].strParam) && (TempParam->Params[1].strParam))
         {
            /* Check to make sure the Event Type and Message Types are  */
            /* valid.                                                   */
            if((unsigned int)TempParam->Params[2].intParam <= etMessageDeleted)
            {
               if((unsigned int)TempParam->Params[3].intParam <= mtMMS)
               {
                  /* Parameters appear to be semi-valid, submit the     */
                  /* request.                                           */
                  SendEventRequest((EventType_t)(TempParam->Params[2].intParam), (MessageType_t)(TempParam->Params[3].intParam), TempParam->Params[1].strParam, TempParam->Params[0].strParam);

                  ret_val = 0;
               }
               else
               {
                  DisplayUsage("SendEventReport [Handle] [Folder] [EventType (0 - 7)] [Message Type (0-3)]");

                  ret_val = INVALID_PARAMETERS_ERROR;
               }
            }
            else
            {
               DisplayUsage("SendEventReport [Handle] [Folder] [EventType (0 - 7)] [Message Type (0-3)]");

               ret_val = INVALID_PARAMETERS_ERROR;
            }
         }
         else
         {
            DisplayUsage("SendEventReport [Handle] [Folder] [EventType (0 - 7)] [Message Type (0-3)]");

            ret_val = INVALID_PARAMETERS_ERROR;
         }
      }
      else
      {
         /* One or more of the parameters are invalid.                  */
         Display(("Get Message Request: Invalid MAPID.\r\n"));

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

   /* The following function is responsible for issuing a MAP Request   */
   /* Notification Request to the remote MAP Server.  This function     */
   /* returns zero if successful and a negative value if an error       */
   /* occurred.                                                         */
static int RequestNotification(ParameterList_t *TempParam)
{
   int Result;
   int ret_val;

   /* First check to see if a valid Bluetooth Stack ID exists.          */
   if(BluetoothStackID)
   {
      /* Now check to make sure that the MAP Notification ID appears to */
      /* be semi-valid.                                                 */
      if(MAPNID)
      {
         /* The MAP ID appears to be is a semi-valid value.  Next,      */
         /* determine if this is a request for a notification or a      */
         /* request to disconnect the notification.                     */
         if((TempParam) && (TempParam->NumberofParameters > 0))
         {
            /* Now submit the command to begin this operation.          */
            Result = MAP_Set_Notification_Registration_Request(BluetoothStackID, MAPID, (Boolean_t)((TempParam->Params[0].intParam)?TRUE:FALSE));

            if(!Result)
            {
               /* The function was submitted successfully.              */
               Display(("MAP_Set_Notification_Registration_Request: Function Successful.\r\n"));

               ret_val = 0;
            }
            else
            {
               /* There was an error submitting the function.           */
               Display(("MAP_Set_Notification_Registration_Request() Failure: %d.\r\n", Result));

               ret_val = Result;
            }
         }
         else
         {
            DisplayUsage("RequestNotification [Connection (0 = Disconnect, 1 = Connect)]");

            ret_val = INVALID_PARAMETERS_ERROR;
         }
      }
      else
      {
         Display(("No Notification Server Open.\r\n"));

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

   /* The following function is a utility function that exists to       */
   /* actually send a MAP Send Event Request to the remote MAP          */
   /* Notification Server.                                              */
static void SendEventRequest(EventType_t EventType, MessageType_t MessageType, char *FolderName, char *Handle)
{
   int          Result;
   int          Index;
   unsigned int AmountSent;

   if((BluetoothStackID) && (MAPNID) && (NotificationConnected))
   {
      /* Place the Event Report Header in the fixed buffer.  There are  */
      /* no Event Reports that can not fit in the Minimum OBEX packet.  */
      Index = BTPS_SprintF(((char *)TempBuffer), "%s%s%s", TELECOM_MESSAGE_EVENT_REPORT_PREFIX, TELECOM_MESSAGE_EVENT_REPORT_ENTRY_PREFIX, GetEventTypeString(EventType));

      if(EventType == etNewMessage)
      {
         Index += BTPS_SprintF((char *)&((char *)TempBuffer)[Index], "%s%s", TELECOM_MESSAGE_EVENT_REPORT_ENTRY_MIDDLE_1, Handle);
         Index += BTPS_SprintF((char *)&((char *)TempBuffer)[Index], "%stelecom/msg/%s", TELECOM_MESSAGE_EVENT_REPORT_ENTRY_MIDDLE_2, FolderName);
         Index += BTPS_SprintF((char *)&((char *)TempBuffer)[Index], "%s%s", TELECOM_MESSAGE_EVENT_REPORT_ENTRY_MIDDLE_3, GetMessageTypeString(MessageType));
      }
      else
      {
         if((EventType != etMemoryFull) && (EventType != etMemoryAvailable))
         {
            Index += BTPS_SprintF((char *)&((char *)TempBuffer)[Index], "%s%s", TELECOM_MESSAGE_EVENT_REPORT_ENTRY_MIDDLE_1, Handle);
         }
      }

      Index += BTPS_SprintF((char *)&((char *)TempBuffer)[Index], "%s%s", TELECOM_MESSAGE_EVENT_REPORT_ENTRY_SUFFIX, TELECOM_MESSAGE_EVENT_REPORT_SUFFIX);

      Result = MAP_Send_Event_Request(BluetoothStackID, MAPNID, Index, (Byte_t *)TempBuffer, &AmountSent, TRUE);

      if(Result >= 0)
         Display(("MAP_Send_Event_Request() Successful.\r\n"));
      else
         Display(("MAP_Send_Event_Request() Failure %d.\r\n", Result));
   }
   else
      Display(("Invalid parameter or No longer connected.\r\n"));
}

   /* The following function is used to Process the MAP Set Folder      */
   /* Indication Event.                                                 */
static void ProcessSetFolder(MAP_Set_Folder_Indication_Data_t *MAP_Set_Folder_Indication_Data)
{
   int           Result;
   char         *NameString;
   Byte_t        ResponseCode;
   unsigned int  NameLength;

   if((BluetoothStackID) && (MAPID) && (Connected) && (MAP_Set_Folder_Indication_Data))
   {
      /* All that is left to do is simply attempt to change the Folder  */
      /* path.                                                          */

      /* First we need to map the UTF-16 Folder name to ASCII (this     */
      /* application will only support ASCII folder names).             */
      if(MAP_Set_Folder_Indication_Data->FolderName)
      {
         NameLength = 0;
         while((MAP_Set_Folder_Indication_Data->FolderName[NameLength]) && (NameLength < (SIZEOF_TEMP_CALLBACK_BUFFER - 1)))
         {
            TempCallbackBuffer[NameLength] = (char)MAP_Set_Folder_Indication_Data->FolderName[NameLength];

            NameLength++;
         }

         /* Make sure the name is NULL terminated.                      */
         TempCallbackBuffer[NameLength] = '\0';
      }

      if(!ChangeMessageFolder(MAP_Set_Folder_Indication_Data->PathOption, TempCallbackBuffer))
      {
         Display(("Folder successfully set to: %s\r\n", ((NameString = GetCurrentMessageFolderString()) != NULL)?NameString:""));

         ResponseCode = MAP_OBEX_RESPONSE_OK;
      }
      else
      {
         Display(("Unable to set Folder: %d, \"%s\"\r\n", MAP_Set_Folder_Indication_Data->PathOption, MAP_Set_Folder_Indication_Data->FolderName?TempCallbackBuffer:""));

         ResponseCode = MAP_OBEX_RESPONSE_NOT_FOUND;
      }

      Result = MAP_Set_Folder_Response(BluetoothStackID, MAPID, ResponseCode);

      if(Result >= 0)
         Display(("MAP_Set_Folder_Response() Successful.\r\n"));
      else
         Display(("MAP_Set_Folder_Response() Failure %d.\r\n", Result));
   }
   else
      Display(("Invalid parameter or No longer connected.\r\n"));
}

   /* The following function is used to Process the MAP Get Folder      */
   /* Listing Indication Event.                                         */
static void ProcessGetFolderListing(MAP_Get_Folder_Listing_Indication_Data_t *MAP_Get_Folder_Listing_Indication_Data)
{
   int           Result;
   int           NumberFolders;
   Word_t        NumberEntries;
   unsigned int  Temp;
   unsigned int  Index;
   unsigned int  TotalNumberFolders;
   FolderEntry_t FolderEntry;

   if((BluetoothStackID) && (MAPID) && (Connected) && (MAP_Get_Folder_Listing_Indication_Data))
   {
      /* Determine if this is the start of new operation or a           */
      /* continuation.                                                  */
      if(CurrentOperation == coNone)
      {
         /* Start of a new operation.                                   */

         /* Determine the number of Folders in the current folder.      */
         if((NumberFolders = QueryNumberFolderEntries(GetCurrentMessageFolder())) >= 0)
         {
            /* Determine the total size of the folder listing.          */
            /* * NOTE * We will honor the List Start Offset and the Max */
            /*          List Count Parameters here.                     */
            for(Index=0,CurrentBufferSize=(BTPS_StringLength(TELECOM_FOLDER_LISTING_PREFIX) + BTPS_StringLength(TELECOM_FOLDER_LISTING_SUFFIX)),TotalNumberFolders=0;Index<(unsigned int)NumberFolders;Index++)
            {
               if((Index >= (unsigned int)MAP_Get_Folder_Listing_Indication_Data->ListStartOffset) && (TotalNumberFolders < (unsigned int)MAP_Get_Folder_Listing_Indication_Data->MaxListCount))
               {
                  if(QueryFolderEntry(GetCurrentMessageFolder(), Index, &FolderEntry))
                  {
                     /* Now add up the necessary length for this entry. */
                     if(FolderEntry.FolderName)
                     {
                        CurrentBufferSize += (BTPS_StringLength(TELECOM_FOLDER_LISTING_ENTRY_PREFIX) + BTPS_StringLength(TELECOM_FOLDER_LISTING_ENTRY_MIDDLE) + BTPS_StringLength(TELECOM_FOLDER_LISTING_ENTRY_SUFFIX));

                        CurrentBufferSize += BTPS_StringLength(FolderEntry.FolderName);

                        if(FolderEntry.CreateDateTime)
                           CurrentBufferSize += BTPS_StringLength(FolderEntry.CreateDateTime);

                        TotalNumberFolders++;
                     }
                  }
               }
            }

            /* Now that we know the entire length, go ahead and allocate*/
            /* space to hold the Folder Listing.                        */

            /* Check to see if this is a request to determine the       */
            /* maximum number of entries.                               */
            if(!MAP_Get_Folder_Listing_Indication_Data->MaxListCount)
            {
               Display(("Request for Number of Folders: %d\r\n", NumberFolders));

               NumberEntries = (Word_t)NumberFolders;

               Result = MAP_Get_Folder_Listing_Response(BluetoothStackID, MAPID, MAP_OBEX_RESPONSE_OK, &NumberEntries, 0, NULL, NULL);
            }
            else
            {
               /* Allocate a buffer to hold the requested Folder Listing*/
               /* into.                                                 */
               /* * NOTE * We will allocate an extra byte to take care  */
               /*          of the NULL terminator.                      */
               if((CurrentBuffer = BTPS_AllocateMemory(CurrentBufferSize + 1)) != NULL)
               {
                  /* Buffer allocated, go ahead and build the Folder    */
                  /* Listing buffer.  Place the Listing Header on the   */
                  /* data (required).                                   */
                  BTPS_SprintF((char *)CurrentBuffer, TELECOM_FOLDER_LISTING_PREFIX);

                  for(Index=0,TotalNumberFolders=0;Index<(unsigned int)NumberFolders;Index++)
                  {
                     if((Index >= (unsigned int)MAP_Get_Folder_Listing_Indication_Data->ListStartOffset) && (TotalNumberFolders < (unsigned int)MAP_Get_Folder_Listing_Indication_Data->MaxListCount))
                     {
                        if(QueryFolderEntry(GetCurrentMessageFolder(), Index, &FolderEntry))
                        {
                           if(FolderEntry.FolderName)
                           {
                              BTPS_StringCopy(&(CurrentBuffer[BTPS_StringLength(CurrentBuffer)]), TELECOM_FOLDER_LISTING_ENTRY_PREFIX);

                              BTPS_StringCopy(&(CurrentBuffer[BTPS_StringLength(CurrentBuffer)]), FolderEntry.FolderName);

                              BTPS_StringCopy(&(CurrentBuffer[BTPS_StringLength(CurrentBuffer)]), TELECOM_FOLDER_LISTING_ENTRY_MIDDLE);

                              if(FolderEntry.CreateDateTime)
                                 BTPS_StringCopy(&(CurrentBuffer[BTPS_StringLength(CurrentBuffer)]), FolderEntry.CreateDateTime);

                              BTPS_StringCopy(&(CurrentBuffer[BTPS_StringLength(CurrentBuffer)]), TELECOM_FOLDER_LISTING_ENTRY_SUFFIX);

                              TotalNumberFolders++;
                           }
                        }
                     }
                  }

                  /* Place the Listing Footer on the data (required).   */
                  BTPS_StringCopy(&(CurrentBuffer[BTPS_StringLength(CurrentBuffer)]), TELECOM_FOLDER_LISTING_SUFFIX);

                  CurrentBufferSize = BTPS_StringLength(CurrentBuffer);

                  /* All finished, go ahead and send the Folder Listing.*/
                  Result = MAP_Get_Folder_Listing_Response(BluetoothStackID, MAPID, MAP_OBEX_RESPONSE_OK, NULL, CurrentBufferSize, (Byte_t *)CurrentBuffer, &CurrentBufferSent);

                  /* Flag whether or not the operation completed        */
                  /* successfully.                                      */
                  if((Result >= 0) && (CurrentBufferSent != CurrentBufferSize))
                     CurrentOperation = coGetFolderListing;
                  else
                  {
                     BTPS_FreeMemory(CurrentBuffer);

                     CurrentBuffer = NULL;
                  }
               }
               else
               {
                  Display(("Unable to allocate memory.\r\n"));

                  Result = MAP_Get_Folder_Listing_Response(BluetoothStackID, MAPID, MAP_OBEX_RESPONSE_SERVICE_UNAVAILABLE, NULL, 0, NULL, NULL);
               }
            }
         }
         else
         {
            Display(("Unable to Get Folder Listing: Invalid current directory.\r\n"));

            Result = MAP_Get_Folder_Listing_Response(BluetoothStackID, MAPID, MAP_OBEX_RESPONSE_NOT_FOUND, NULL, 0, NULL, NULL);
         }
      }
      else
      {
         Display(("Continuation for MAP Get Folder Listing\r\n"));

         Result = MAP_Get_Folder_Listing_Response(BluetoothStackID, MAPID, MAP_OBEX_RESPONSE_OK, NULL, (CurrentBufferSize - CurrentBufferSent), (Byte_t *)&(CurrentBuffer[CurrentBufferSent]), &Temp);

         /* Flag whether or not the operation completed successfully.   */
         if((Result >= 0) && (Temp != (CurrentBufferSize - CurrentBufferSent)))
            CurrentBufferSent += Temp;
         else
         {
            BTPS_FreeMemory(CurrentBuffer);

            CurrentBuffer    = NULL;

            CurrentOperation = coNone;
         }
      }

      if(Result >= 0)
         Display(("MAP_Get_Folder_Listing_Response() Successful.\r\n"));
      else
         Display(("MAP_Get_Folder_Listing_Response() Failure %d.\r\n", Result));
   }
   else
      Display(("Invalid parameter or No longer connected.\r\n"));
}

   /* The following function is used to Process the MAP Get Message     */
   /* Listing Indication Event.                                         */
static void ProcessGetMessageListing(MAP_Get_Message_Listing_Indication_Data_t *MAP_Get_Message_Listing_Indication_Data)
{
   int              Result;
   int              NumberMessages;
   Word_t           NumberEntries;
   Boolean_t        ChangeFolder;
   unsigned int     NameLength;
   unsigned int     Temp;
   unsigned int     Index;
   unsigned int     TotalNumberMessages;
   MessageEntry_t  *MessageEntry;
   MAP_TimeDate_t   CurrentTime;
   CurrentFolder_t  Folder;

   if((BluetoothStackID) && (MAPID) && (Connected) && (MAP_Get_Message_Listing_Indication_Data))
   {
      /* Determine if this is the start of new operation or a           */
      /* continuation.                                                  */
      if(CurrentOperation == coNone)
      {
         /* Start of a new operation.                                   */

         /* Initialize that no error has occurred.                      */
         Temp   = 0;
         Result = 0;

         /* Check to see if the caller is specifying a sub-folder or the*/
         /* current folder.                                             */
         if((MAP_Get_Message_Listing_Indication_Data->FolderName) && (MAP_Get_Message_Listing_Indication_Data->FolderName[0]))
         {
            /* First we need to map the UTF-16 Folder name to ASCII     */
            /* (this application will only support ASCII folder names). */
            NameLength = 0;
            while((MAP_Get_Message_Listing_Indication_Data->FolderName[NameLength]) && (NameLength < (SIZEOF_TEMP_CALLBACK_BUFFER - 1)))
            {
               TempCallbackBuffer[NameLength] = (char)MAP_Get_Message_Listing_Indication_Data->FolderName[NameLength];

               NameLength++;
            }

            /* Make sure the name is NULL terminated.                   */
            TempCallbackBuffer[NameLength] = '\0';

            Display((TempCallbackBuffer));
            Display(("\r\n"));

            /* Sub-folder, so navigate into it.                         */
            if(!ChangeMessageFolder(sfDown, TempCallbackBuffer))
               ChangeFolder = TRUE;
            else
            {
               /* Invalid directory specified, inform remote side of an */
               /* error.                                                */
               Display(("Unable to Get Message Listing: Invalid folder specified.\r\n"));

               Result = MAP_Get_Message_Listing_Response(BluetoothStackID, MAPID, MAP_OBEX_RESPONSE_NOT_FOUND, NULL, FALSE, NULL, 0, NULL, NULL);

               /* Flag an error.                                        */
               Temp         = 1;
               ChangeFolder = FALSE;
            }
         }
         else
            ChangeFolder = FALSE;

         /* Only continue to process the request if there was not an    */
         /* error.                                                      */
         if(!Temp)
         {
            /* Determine the number of Messages in the current folder.  */
            Folder = GetCurrentMessageFolder();
            if((NumberMessages = QueryNumberMessageEntries(GetCurrentMessageFolder())) >= 0)
            {
               /* Determine the total size of the message listing.      */
               /* * NOTE * We will honor the List Start Offset and the  */
               /*          Max List Count Parameters here.              */
               /* * NOTE * We do NOT SUPPORT any type of filtering in   */
               /*          this implementation !!!!!!!!!!!!!!!!!!!!!!!! */
               for(Index=0,CurrentBufferSize=(BTPS_StringLength(TELECOM_MESSAGE_LISTING_PREFIX) + BTPS_StringLength(TELECOM_MESSAGE_LISTING_SUFFIX)),TotalNumberMessages=0;Index<(unsigned int)NumberMessages;Index++)
               {
                  if((Index >= (unsigned int)MAP_Get_Message_Listing_Indication_Data->ListStartOffset) && (TotalNumberMessages < (unsigned int)MAP_Get_Message_Listing_Indication_Data->MaxListCount))
                  {
                     if(QueryMessageEntryByIndex(Folder, Index, &MessageEntry))
                     {
                        /* Now add up the necessary length for this     */
                        /* entry.                                       */
                        if((MessageEntry) && (MessageEntry->MessageHandle))
                        {
                           CurrentBufferSize += (BTPS_StringLength(TELECOM_MESSAGE_LISTING_ENTRY_PREFIX) + BTPS_StringLength(TELECOM_MESSAGE_LISTING_ENTRY_SUFFIX));

                           CurrentBufferSize += BTPS_StringLength(MessageEntry->MessageHandle);

                           CurrentBufferSize += BTPS_StringLength(TELECOM_MESSAGE_LISTING_ENTRY_MIDDLE_1);

                           if(MessageEntry->Subject)
                              CurrentBufferSize += BTPS_StringLength(MessageEntry->Subject);

                           CurrentBufferSize += BTPS_StringLength(TELECOM_MESSAGE_LISTING_ENTRY_MIDDLE_2);

                           if(MessageEntry->DateTime)
                              CurrentBufferSize += BTPS_StringLength(MessageEntry->DateTime);

                           CurrentBufferSize += BTPS_StringLength(TELECOM_MESSAGE_LISTING_ENTRY_MIDDLE_3);

                           if(MessageEntry->Sender)
                              CurrentBufferSize += BTPS_StringLength(MessageEntry->Sender);

                           CurrentBufferSize += BTPS_StringLength(TELECOM_MESSAGE_LISTING_ENTRY_MIDDLE_4);

                           if(MessageEntry->SenderName)
                              CurrentBufferSize += BTPS_StringLength(MessageEntry->SenderName);

                           CurrentBufferSize += BTPS_StringLength(TELECOM_MESSAGE_LISTING_ENTRY_MIDDLE_5);

                           if(MessageEntry->SenderAddressing)
                              CurrentBufferSize += BTPS_StringLength(MessageEntry->SenderAddressing);

                           CurrentBufferSize += BTPS_StringLength(TELECOM_MESSAGE_LISTING_ENTRY_MIDDLE_6);

                           if(MessageEntry->ReplyToAddressing)
                              CurrentBufferSize += BTPS_StringLength(MessageEntry->ReplyToAddressing);

                           CurrentBufferSize += BTPS_StringLength(TELECOM_MESSAGE_LISTING_ENTRY_MIDDLE_7);

                           if(MessageEntry->RecipientName)
                              CurrentBufferSize += BTPS_StringLength(MessageEntry->RecipientName);

                           CurrentBufferSize += BTPS_StringLength(TELECOM_MESSAGE_LISTING_ENTRY_MIDDLE_8);

                           if(MessageEntry->RecipientAddressing)
                              CurrentBufferSize += BTPS_StringLength(MessageEntry->RecipientAddressing);

                           CurrentBufferSize += BTPS_StringLength(TELECOM_MESSAGE_LISTING_ENTRY_MIDDLE_9);

                           if(MessageEntry->Type)
                              CurrentBufferSize += BTPS_StringLength(MessageEntry->Type);

                           CurrentBufferSize += BTPS_StringLength(TELECOM_MESSAGE_LISTING_ENTRY_MIDDLE_10);

                           BTPS_SprintF(TempCallbackBuffer, "%lu", MessageEntry->MessageSize);
                           CurrentBufferSize += BTPS_StringLength(TempCallbackBuffer);

                           CurrentBufferSize += BTPS_StringLength(TELECOM_MESSAGE_LISTING_ENTRY_MIDDLE_11);

                           if(MessageEntry->Text)
                              CurrentBufferSize += BTPS_StringLength(MessageEntry->Text);

                           CurrentBufferSize += BTPS_StringLength(TELECOM_MESSAGE_LISTING_ENTRY_MIDDLE_12);

                           if(MessageEntry->ReceptionStatus)
                              CurrentBufferSize += BTPS_StringLength(MessageEntry->ReceptionStatus);

                           CurrentBufferSize += BTPS_StringLength(TELECOM_MESSAGE_LISTING_ENTRY_MIDDLE_13);

                           BTPS_SprintF(TempCallbackBuffer, "%lu", MessageEntry->AttachmentSize);
                           CurrentBufferSize += BTPS_StringLength(TempCallbackBuffer);

                           CurrentBufferSize += BTPS_StringLength(TELECOM_MESSAGE_LISTING_ENTRY_MIDDLE_14);

                           if(MessageEntry->Priority < svNotDefined)
                              CurrentBufferSize += BTPS_StringLength("yes");

                           CurrentBufferSize += BTPS_StringLength(TELECOM_MESSAGE_LISTING_ENTRY_MIDDLE_15);

                           if(MessageEntry->Read < svNotDefined)
                              CurrentBufferSize += BTPS_StringLength("yes");

                           CurrentBufferSize += BTPS_StringLength(TELECOM_MESSAGE_LISTING_ENTRY_MIDDLE_16);

                           if(MessageEntry->Sent < svNotDefined)
                              CurrentBufferSize += BTPS_StringLength("yes");

                           CurrentBufferSize += BTPS_StringLength(TELECOM_MESSAGE_LISTING_ENTRY_MIDDLE_17);

                           if(MessageEntry->Protected < svNotDefined)
                              CurrentBufferSize += BTPS_StringLength("yes");

                           TotalNumberMessages++;
                        }
                     }
                  }
               }

               /* Format the Current Local Time in the Time/Date        */
               /* structure.  TODO By customer on embedded platforms.   */
               BTPS_MemInitialize(&CurrentTime, 0, sizeof(CurrentTime));

               /* Check to see if this is a request to determine the    */
               /* maximum number of entries.                            */
               if(!MAP_Get_Message_Listing_Indication_Data->MaxListCount)
               {
                  Display(("Request for Number of Messages: %d\r\n", NumberMessages));

                  NumberEntries = (Word_t)NumberMessages;

                  Result        = MAP_Get_Message_Listing_Response(BluetoothStackID, MAPID, MAP_OBEX_RESPONSE_OK, &NumberEntries, FALSE, &CurrentTime, 0, NULL, NULL);
               }
               else
               {
                  /* Allocate a buffer to hold the requested Message    */
                  /* Listing into.                                      */
                  /* * NOTE * We will allocate an extra byte to take    */
                  /*          care of the NULL terminator.              */
                  if((CurrentBuffer = BTPS_AllocateMemory(CurrentBufferSize + 1)) != NULL)
                  {
                     /* Buffer allocated, go ahead and build the Message*/
                     /* Listing buffer.  Place the Listing Header on the*/
                     /* data (required).                                */
                     BTPS_SprintF((char *)CurrentBuffer, TELECOM_MESSAGE_LISTING_PREFIX);

                     for(Index=0,TotalNumberMessages=0;Index<(unsigned int)NumberMessages;Index++)
                     {
                        if((Index >= (unsigned int)MAP_Get_Message_Listing_Indication_Data->ListStartOffset) && (TotalNumberMessages < (unsigned int)MAP_Get_Message_Listing_Indication_Data->MaxListCount))
                        {
                           if(QueryMessageEntryByIndex(Folder, Index, &MessageEntry))
                           {
                              if((MessageEntry) && (MessageEntry->MessageHandle))
                              {
                                 BTPS_StringCopy(&(CurrentBuffer[BTPS_StringLength(CurrentBuffer)]), TELECOM_MESSAGE_LISTING_ENTRY_PREFIX);

                                 BTPS_StringCopy(&(CurrentBuffer[BTPS_StringLength(CurrentBuffer)]), MessageEntry->MessageHandle);

                                 BTPS_StringCopy(&(CurrentBuffer[BTPS_StringLength(CurrentBuffer)]), TELECOM_MESSAGE_LISTING_ENTRY_MIDDLE_1);

                                 if(MessageEntry->Subject)
                                    BTPS_StringCopy(&(CurrentBuffer[BTPS_StringLength(CurrentBuffer)]), MessageEntry->Subject);

                                 BTPS_StringCopy(&(CurrentBuffer[BTPS_StringLength(CurrentBuffer)]), TELECOM_MESSAGE_LISTING_ENTRY_MIDDLE_2);

                                 if(MessageEntry->DateTime)
                                    BTPS_StringCopy(&(CurrentBuffer[BTPS_StringLength(CurrentBuffer)]), MessageEntry->DateTime);

                                 BTPS_StringCopy(&(CurrentBuffer[BTPS_StringLength(CurrentBuffer)]), TELECOM_MESSAGE_LISTING_ENTRY_MIDDLE_3);

                                 if(MessageEntry->Sender)
                                    BTPS_StringCopy(&(CurrentBuffer[BTPS_StringLength(CurrentBuffer)]), MessageEntry->Sender);

                                 BTPS_StringCopy(&(CurrentBuffer[BTPS_StringLength(CurrentBuffer)]), TELECOM_MESSAGE_LISTING_ENTRY_MIDDLE_4);

                                 if(MessageEntry->SenderName)
                                    BTPS_StringCopy(&(CurrentBuffer[BTPS_StringLength(CurrentBuffer)]), MessageEntry->SenderName);

                                 BTPS_StringCopy(&(CurrentBuffer[BTPS_StringLength(CurrentBuffer)]), TELECOM_MESSAGE_LISTING_ENTRY_MIDDLE_5);

                                 if(MessageEntry->SenderAddressing)
                                    BTPS_StringCopy(&(CurrentBuffer[BTPS_StringLength(CurrentBuffer)]), MessageEntry->SenderAddressing);

                                 BTPS_StringCopy(&(CurrentBuffer[BTPS_StringLength(CurrentBuffer)]), TELECOM_MESSAGE_LISTING_ENTRY_MIDDLE_6);

                                 if(MessageEntry->ReplyToAddressing)
                                    BTPS_StringCopy(&(CurrentBuffer[BTPS_StringLength(CurrentBuffer)]), MessageEntry->ReplyToAddressing);

                                 BTPS_StringCopy(&(CurrentBuffer[BTPS_StringLength(CurrentBuffer)]), TELECOM_MESSAGE_LISTING_ENTRY_MIDDLE_7);

                                 if(MessageEntry->RecipientName)
                                    BTPS_StringCopy(&(CurrentBuffer[BTPS_StringLength(CurrentBuffer)]), MessageEntry->RecipientName);

                                 BTPS_StringCopy(&(CurrentBuffer[BTPS_StringLength(CurrentBuffer)]), TELECOM_MESSAGE_LISTING_ENTRY_MIDDLE_8);

                                 if(MessageEntry->RecipientAddressing)
                                    BTPS_StringCopy(&(CurrentBuffer[BTPS_StringLength(CurrentBuffer)]), MessageEntry->RecipientAddressing);

                                 BTPS_StringCopy(&(CurrentBuffer[BTPS_StringLength(CurrentBuffer)]), TELECOM_MESSAGE_LISTING_ENTRY_MIDDLE_9);

                                 if(MessageEntry->Type)
                                    BTPS_StringCopy(&(CurrentBuffer[BTPS_StringLength(CurrentBuffer)]), MessageEntry->Type);

                                 BTPS_StringCopy(&(CurrentBuffer[BTPS_StringLength(CurrentBuffer)]), TELECOM_MESSAGE_LISTING_ENTRY_MIDDLE_10);

                                 BTPS_SprintF(TempCallbackBuffer, "%lu", MessageEntry->MessageSize);
                                 BTPS_StringCopy(&(CurrentBuffer[BTPS_StringLength(CurrentBuffer)]), TempCallbackBuffer);

                                 BTPS_StringCopy(&(CurrentBuffer[BTPS_StringLength(CurrentBuffer)]), TELECOM_MESSAGE_LISTING_ENTRY_MIDDLE_11);

                                 if(MessageEntry->Text)
                                    BTPS_StringCopy(&(CurrentBuffer[BTPS_StringLength(CurrentBuffer)]), MessageEntry->Text);

                                 BTPS_StringCopy(&(CurrentBuffer[BTPS_StringLength(CurrentBuffer)]), TELECOM_MESSAGE_LISTING_ENTRY_MIDDLE_12);

                                 if(MessageEntry->ReceptionStatus)
                                    BTPS_StringCopy(&(CurrentBuffer[BTPS_StringLength(CurrentBuffer)]), MessageEntry->ReceptionStatus);

                                 BTPS_StringCopy(&(CurrentBuffer[BTPS_StringLength(CurrentBuffer)]), TELECOM_MESSAGE_LISTING_ENTRY_MIDDLE_13);

                                 BTPS_SprintF(TempCallbackBuffer, "%lu", MessageEntry->AttachmentSize);
                                 BTPS_StringCopy(&(CurrentBuffer[BTPS_StringLength(CurrentBuffer)]), TempCallbackBuffer);

                                 BTPS_StringCopy(&(CurrentBuffer[BTPS_StringLength(CurrentBuffer)]), TELECOM_MESSAGE_LISTING_ENTRY_MIDDLE_14);

                                 if(MessageEntry->Priority < svNotDefined)
                                    BTPS_StringCopy(&(CurrentBuffer[BTPS_StringLength(CurrentBuffer)]), (MessageEntry->Priority == svNo)?"no":"yes");

                                 BTPS_StringCopy(&(CurrentBuffer[BTPS_StringLength(CurrentBuffer)]), TELECOM_MESSAGE_LISTING_ENTRY_MIDDLE_15);

                                 if(MessageEntry->Read < svNotDefined)
                                    BTPS_StringCopy(&(CurrentBuffer[BTPS_StringLength(CurrentBuffer)]), (MessageEntry->Read == svNo)?"no":"yes");

                                 BTPS_StringCopy(&(CurrentBuffer[BTPS_StringLength(CurrentBuffer)]), TELECOM_MESSAGE_LISTING_ENTRY_MIDDLE_16);

                                 if(MessageEntry->Sent < svNotDefined)
                                    BTPS_StringCopy(&(CurrentBuffer[BTPS_StringLength(CurrentBuffer)]), (MessageEntry->Sent == svNo)?"no":"yes");

                                 BTPS_StringCopy(&(CurrentBuffer[BTPS_StringLength(CurrentBuffer)]), TELECOM_MESSAGE_LISTING_ENTRY_MIDDLE_17);

                                 if(MessageEntry->Protected < svNotDefined)
                                    BTPS_StringCopy(&(CurrentBuffer[BTPS_StringLength(CurrentBuffer)]), (MessageEntry->Protected == svNo)?"no":"yes");

                                 BTPS_StringCopy(&(CurrentBuffer[BTPS_StringLength(CurrentBuffer)]), TELECOM_MESSAGE_LISTING_ENTRY_SUFFIX);

                                 TotalNumberMessages++;
                              }
                           }
                        }
                     }

                     /* Place the Listing Footer on the data (required).*/
                     BTPS_StringCopy(&(CurrentBuffer[BTPS_StringLength(CurrentBuffer)]), TELECOM_MESSAGE_LISTING_SUFFIX);

                     /* All finished, go ahead and send the Message     */
                     /* Listing.                                        */
                     CurrentBufferSize = BTPS_StringLength(CurrentBuffer);

                     /* Note the Number of Entries to return in the     */
                     /* response.                                       */
                     NumberEntries     = (Word_t)TotalNumberMessages;

                     /* All finished, go ahead and send the Message     */
                     /* Listing.                                        */
                     Result            = MAP_Get_Message_Listing_Response(BluetoothStackID, MAPID, MAP_OBEX_RESPONSE_OK, &NumberEntries, FALSE, &CurrentTime, CurrentBufferSize, (Byte_t *)CurrentBuffer, &CurrentBufferSent);

                     /* Flag whether or not the operation completed     */
                     /* successfully.                                   */
                     if((Result >= 0) && (CurrentBufferSent != CurrentBufferSize))
                        CurrentOperation = coGetMessageListing;
                     else
                     {
                        BTPS_FreeMemory(CurrentBuffer);

                        CurrentBuffer = NULL;
                     }
                  }
                  else
                  {
                     Display(("Unable to allocate memory.\r\n"));

                     Result = MAP_Get_Message_Listing_Response(BluetoothStackID, MAPID, MAP_OBEX_RESPONSE_SERVICE_UNAVAILABLE, NULL, FALSE, NULL, 0, NULL, NULL);
                  }
               }
            }
            else
            {
               Display(("Unable to Get Message Listing: Invalid current directory.\r\n"));

               Result = MAP_Get_Message_Listing_Response(BluetoothStackID, MAPID, MAP_OBEX_RESPONSE_NOT_FOUND, NULL, FALSE, NULL, 0, NULL, NULL);
            }

            /* If we changed folders, we need to go back to the parent. */
            if(ChangeFolder)
               ChangeMessageFolder(sfUp, NULL);
         }
      }
      else
      {
         Display(("Continuation for MAP Get Message Listing\r\n"));

         Result = MAP_Get_Message_Listing_Response(BluetoothStackID, MAPID, MAP_OBEX_RESPONSE_OK, NULL, FALSE, NULL, (CurrentBufferSize - CurrentBufferSent), (Byte_t *)&(CurrentBuffer[CurrentBufferSent]), &Temp);

         /* Flag whether or not the operation completed successfully.   */
         if((Result >= 0) && (Temp != (CurrentBufferSize - CurrentBufferSent)))
            CurrentBufferSent += Temp;
         else
         {
            BTPS_FreeMemory(CurrentBuffer);

            CurrentBuffer    = NULL;

            CurrentOperation = coNone;
         }
      }

      if(Result >= 0)
         Display(("MAP_Get_Message_Listing_Response() Successful.\r\n"));
      else
         Display(("MAP_Get_Message_Listing_Response() Failure %d.\r\n", Result));
   }
   else
      Display(("Invalid parameter or No longer connected.\r\n"));
}

   /* The following function is used to Process the MAP Get Message     */
   /* Indication Event.                                                 */
static void ProcessGetMessage(MAP_Get_Message_Indication_Data_t *MAP_Get_Message_Indication_Data)
{
   int             Result;
   unsigned int    Temp;
   MessageEntry_t *MessageEntry;

   if((BluetoothStackID) && (MAPID) && (Connected) && (MAP_Get_Message_Indication_Data))
   {
      /* Determine if this is the start of new operation or a           */
      /* continuation.                                                  */
      if(CurrentOperation == coNone)
      {
         /* Start of a new operation.                                   */

         if((MAP_Get_Message_Indication_Data->MessageHandle) && (BTPS_StringLength(MAP_Get_Message_Indication_Data->MessageHandle)))
         {
            Display((MAP_Get_Message_Indication_Data->MessageHandle));
            Display(("\r\n"));
         }

         /* Query the specified Message.                                */
         if((QueryMessageEntryByHandle(cfInvalid, MAP_Get_Message_Indication_Data->MessageHandle, &MessageEntry)) && (MessageEntry))
         {
            /* Message found, format up the correct BMSG.               */
            /* * NOTE * This implementation does not format up a BMSG   */
            /*          dynamically.  We will simply return the         */
            /*          retrieved, pre-formatted, BMSG.                 */

            /* Allocate a buffer to hold the requested Message into.    */
            CurrentBufferSize = BTPS_StringLength(MessageEntry->MessageData);

            if((CurrentBuffer = BTPS_AllocateMemory(CurrentBufferSize)) != NULL)
            {
               BTPS_MemCopy(CurrentBuffer, MessageEntry->MessageData, CurrentBufferSize);

               /* Everything is taken care of, send the response.       */
               Result = MAP_Get_Message_Response(BluetoothStackID, MAPID, MAP_OBEX_RESPONSE_OK, ftUnfragmented, CurrentBufferSize, (Byte_t *)CurrentBuffer, &CurrentBufferSent);

               /* Flag whether or not the operation completed           */
               /* successfully.                                         */
               if((Result >= 0) && (CurrentBufferSent != CurrentBufferSize))
                  CurrentOperation = coGetMessage;
               else
               {
                  BTPS_FreeMemory(CurrentBuffer);

                  CurrentBuffer = NULL;
               }
            }
            else
            {
               Display(("Unable to allocate memory.\r\n"));

               Result = MAP_Get_Message_Response(BluetoothStackID, MAPID, MAP_OBEX_RESPONSE_SERVICE_UNAVAILABLE, ftUnfragmented, 0, NULL, NULL);
            }
         }
         else
         {
            Display(("Unable to Get Message: Invalid Message Handle.\r\n"));

            Result = MAP_Get_Message_Response(BluetoothStackID, MAPID, MAP_OBEX_RESPONSE_NOT_FOUND, ftUnfragmented, 0, NULL, NULL);
         }
      }
      else
      {
         Display(("Continuation for MAP Get Message\r\n"));

         Result = MAP_Get_Message_Response(BluetoothStackID, MAPID, MAP_OBEX_RESPONSE_OK, ftUnfragmented, (CurrentBufferSize - CurrentBufferSent), (Byte_t *)&(CurrentBuffer[CurrentBufferSent]), &Temp);

         /* Flag whether or not the operation completed successfully.   */
         if((Result >= 0) && (Temp != (CurrentBufferSize - CurrentBufferSent)))
            CurrentBufferSent += Temp;
         else
         {
            BTPS_FreeMemory(CurrentBuffer);

            CurrentBuffer    = NULL;

            CurrentOperation = coNone;
         }
      }

      if(Result >= 0)
         Display(("MAP_Get_Message_Response() Successful.\r\n"));
      else
         Display(("MAP_Get_Message_Response() Failure %d.\r\n", Result));
   }
   else
      Display(("Invalid parameter or No longer connected.\r\n"));
}

   /* The following function is used to Process the MAP Set Message     */
   /* Status Indication Event.                                          */
static void ProcessSetMessageStatus(MAP_Set_Message_Status_Indication_Data_t *MAP_Set_Message_Status_Indication_Data)
{
   int             Result;
   MessageEntry_t *MessageEntry;

   if((BluetoothStackID) && (MAPID) && (Connected) && (MAP_Set_Message_Status_Indication_Data))
   {
      if((MAP_Set_Message_Status_Indication_Data->MessageHandle) && (BTPS_StringLength(MAP_Set_Message_Status_Indication_Data->MessageHandle)))
      {
         Display((MAP_Set_Message_Status_Indication_Data->MessageHandle));
         Display(("\r\n"));
      }

      /* Query the specified Message.                                   */
      if((QueryMessageEntryByHandle(cfInvalid, MAP_Set_Message_Status_Indication_Data->MessageHandle, &MessageEntry)) && (MessageEntry))
      {
         /* Message found.  Note that in this implementation we will    */
         /* ignore actually updating the actual Message.                */

         Display(("Message Handle: %s\r\n", MAP_Set_Message_Status_Indication_Data->MessageHandle));

         Display(("Status Indicator: %d\r\n", MAP_Set_Message_Status_Indication_Data->StatusIndicator));

         Display(("Status Value: %d\r\n", MAP_Set_Message_Status_Indication_Data->StatusValue));

         Display(("Message %s found: no updates applied (not implemented).\r\n", MAP_Set_Message_Status_Indication_Data->MessageHandle));

         /* Handle the Read Status.                                     */
         if(MAP_Set_Message_Status_Indication_Data->StatusIndicator == siReadStatus)
         {
            /* Check to see if we are to mark as Read or UnRead.        */
            if(MAP_Set_Message_Status_Indication_Data->StatusValue)
            {
               MessageEntry->Read = svYes;

               BTPS_MemCopy(&MessageEntry->MessageData[READ_STATUS_STRING_OFFSET], "READ  ", 6);
            }
            else
            {
               MessageEntry->Read = svNo;

               BTPS_MemCopy(&MessageEntry->MessageData[READ_STATUS_STRING_OFFSET], "UNREAD", 6);
            }
         }

         /* Handle the Delete Status.                                   */
         if(MAP_Set_Message_Status_Indication_Data->StatusIndicator == siDeletedStatus)
         {
            /* Check to see if we are to mark as Delete or UnDelete.    */
            if(MAP_Set_Message_Status_Indication_Data->StatusValue)
            {
               if(!MoveMessageEntryToFolderByHandle(cfDeleted, MAP_Set_Message_Status_Indication_Data->MessageHandle))
                  Display(("Unable to Set Message Status: Invalid Message Handle.\r\n"));
            }
            else
               MoveMessageEntryToFolderByHandle(cfInbox, MAP_Set_Message_Status_Indication_Data->MessageHandle);
         }

         Result = MAP_Set_Message_Status_Response(BluetoothStackID, MAPID, MAP_OBEX_RESPONSE_OK);
      }
      else
      {
         Display(("Unable to Set Message Status: Invalid Message Handle.\r\n"));

         Result = MAP_Set_Message_Status_Response(BluetoothStackID, MAPID, MAP_OBEX_RESPONSE_NOT_FOUND);
      }

      if(Result >= 0)
         Display(("MAP_Set_Message_Status_Response() Successful.\r\n"));
      else
         Display(("MAP_Set_Message_Status_Response() Failure %d.\r\n", Result));
   }
   else
      Display(("Invalid parameter or No longer connected.\r\n"));
}

   /* The following function is used to Process the MAP Push Message    */
   /* Indication Event.                                                 */
static void ProcessPushMessage(MAP_Push_Message_Indication_Data_t *MAP_Push_Message_Indication_Data)
{
   int             Result;
   Boolean_t       Final;
   static char     MessageHandle[MAP_MESSAGE_HANDLE_LENGTH+1];
   unsigned int    NameLength;
   unsigned int    Temp;
   CurrentFolder_t Folder;

   if((BluetoothStackID) && (MAPID) && (Connected) && (MAP_Push_Message_Indication_Data))
   {
      /* Determine if this is the start of new operation or a           */
      /* continuation.                                                  */
      if(CurrentOperation == coNone)
      {
         /* Start of a new operation.                                   */

         Display(("Start of MAP Push Message\r\n"));

         /* Initialize that no error has occurred.                      */
         Temp                  = 0;
         TempCallbackBuffer[0] = 0;
         Folder                = GetCurrentMessageFolder();

         /* Check to see if the caller is specifying a sub-folder or the*/
         /* current folder.                                             */
         if((MAP_Push_Message_Indication_Data->FolderName) && (MAP_Push_Message_Indication_Data->FolderName[0]))
         {
            /* First we need to map the UTF-16 Folder name to ASCII     */
            /* (this application will only support ASCII folder names). */
            NameLength = 0;
            while((MAP_Push_Message_Indication_Data->FolderName[NameLength]) && (NameLength < (SIZEOF_TEMP_CALLBACK_BUFFER - 1)))
            {
               TempCallbackBuffer[NameLength] = (char)MAP_Push_Message_Indication_Data->FolderName[NameLength];

               NameLength++;
            }

            /* Make sure the name is NULL terminated.                   */
            TempCallbackBuffer[NameLength] = '\0';

            Display(("%s\r\n", TempCallbackBuffer));

            /* Sub-folder, so navigate into it.                         */
            if(!ChangeMessageFolder(sfDown, TempCallbackBuffer))
            {
               /* get the Destination Folder.                           */
               Folder = GetCurrentMessageFolder();

               /* Go ahead and Change the Folder back since we are not  */
               /* actually going to store the Message into the Message  */
               /* Store (we will just save it as a file).               */
               ChangeMessageFolder(sfUp, NULL);
            }
            else
            {
               /* Invalid directory specified, inform remote side of an */
               /* error.                                                */
               Display(("Unable to Push Message: Invalid folder specified.\r\n"));

               Result = MAP_Push_Message_Response(BluetoothStackID, MAPID, MAP_OBEX_RESPONSE_NOT_FOUND, NULL);

               /* Flag an error.                                        */
               Temp = 1;
            }
         }

         /* Only continue to process the request if there was not an    */
         /* error.                                                      */
         if(!Temp)
         {
            /* Next, attempt to get a Message Handle for the new        */
            /* message.                                                 */
            /* * NOTE * We will use this Message Handle as the actual   */
            /*          File Name to store the File into.               */
            if(GenerateUniqueMessageHandle(sizeof(MessageHandle), MessageHandle))
            {
               Display(("Message Handle Generated: %s\r\n", MessageHandle));

               /* Format up the correct File Name (using the Message    */
               /* Handle and the Current Receive Message Directory.     */
               /* (note that we already made sure that we had a         */
               /* delimiter character at the end of the Current Receive */
               /* Message Path, so we just need to concatenate the      */
               /* Message Handle with the Receive Message Path.         */
               BTPS_StringCopy(CurrentFileName, ReceiveMessagePath);
               BTPS_StringCopy((CurrentFileName + BTPS_StringLength(CurrentFileName)), MessageHandle);

               /* All finished processing everything, simply inform the */
               /* user of all the received information (as well as the  */
               /* Message Handle).                                      */
               Display(("Final: %d\r\n", MAP_Push_Message_Indication_Data->Final));

               Display(("Folder Name: %s\r\n", TempCallbackBuffer[0]?TempCallbackBuffer:""));

               Display(("Transparent Value: %d\r\n", MAP_Push_Message_Indication_Data->Transparent));

               Display(("Retry Value: %d\r\n", MAP_Push_Message_Indication_Data->Retry));

               Display(("Character Set: %d\r\n", MAP_Push_Message_Indication_Data->CharSet));

               Display(("Data Length: %u\r\n", MAP_Push_Message_Indication_Data->DataLength));

               if(MAP_Push_Message_Indication_Data->DataLength)
               {
                  Display(("Data: \r\n"));

                  for(Temp = 0, TempCallbackBuffer[0] = '\0'; (Temp < MAP_Push_Message_Indication_Data->DataLength) && (Temp < (SIZEOF_TEMP_CALLBACK_BUFFER - 1)); Temp++)
                     BTPS_SprintF(&(TempCallbackBuffer[BTPS_StringLength(TempCallbackBuffer)]), "%c", MAP_Push_Message_Indication_Data->DataBuffer[Temp]);

                  Display((TempCallbackBuffer));
                  Display(("\r\n"));
               }

               Final = MAP_Push_Message_Indication_Data->Final;

               /* Set the current operation to a Put Message.           */
               CurrentOperation = (Final)?coNone:coPushMessage;

               if(Final)
               {
                  /* If we have received the entire message than process*/
                  /* the message that was received.                     */
                  InsertMessageEntryIntoFolder(Folder, MessageHandle);
               }

               /* All that is left to do is to respond to the request.  */
               Result = MAP_Push_Message_Response(BluetoothStackID, MAPID, (Byte_t)((Final)?MAP_OBEX_RESPONSE_OK:MAP_OBEX_RESPONSE_CONTINUE), ((Final)?MessageHandle:NULL));
            }
            else
            {
               Display(("Unable to Push Message: Unable to create Message Handle.\r\n"));

               Result = MAP_Push_Message_Response(BluetoothStackID, MAPID, MAP_OBEX_RESPONSE_SERVER_ERROR, NULL);
            }
         }
      }
      else
      {
         Display(("Continuation for MAP Push Message\r\n"));

         Display(("Data Length: %u\r\n", MAP_Push_Message_Indication_Data->DataLength));

         if(MAP_Push_Message_Indication_Data->DataLength)
         {
            Display(("Data:\r\n"));

            for(Temp = 0, TempCallbackBuffer[0] = '\0'; (Temp < MAP_Push_Message_Indication_Data->DataLength) && (Temp < (SIZEOF_TEMP_CALLBACK_BUFFER - 1));Temp++)
               BTPS_SprintF(&(TempCallbackBuffer[BTPS_StringLength(TempCallbackBuffer)]), "%c", MAP_Push_Message_Indication_Data->DataBuffer[Temp]);

            Display((TempCallbackBuffer));
            Display(("\r\n"));
         }

         /* Get the operation state.                                    */
         Final = MAP_Push_Message_Indication_Data->Final;

         /* Set the current operation to a Put Message.                 */
         CurrentOperation = (Final)?coNone:coPushMessage;

         Result = MAP_Push_Message_Response(BluetoothStackID, MAPID, (Byte_t)((Final)?MAP_OBEX_RESPONSE_OK:MAP_OBEX_RESPONSE_CONTINUE), ((Final)?MessageHandle:NULL));
      }

      if(Result >= 0)
         Display(("MAP_Push_Message_Response() Successful.\r\n"));
      else
         Display(("MAP_Push_Message_Response() Failure %d.\r\n", Result));
   }
   else
      Display(("Invalid parameter or No longer connected.\r\n"));
}

   /* The following function is used to Process the MAP Process Update  */
   /* Inbox Indication Event.                                           */
static void ProcessUpdateInbox(MAP_Update_Inbox_Indication_Data_t *MAP_Update_Inbox_Indication_Data)
{
   int Result;

   if((BluetoothStackID) && (MAPID) && (Connected) && (MAP_Update_Inbox_Indication_Data))
   {
      Display(("Inbox Update Received.\r\n"));

      /* There really isn't anything in this implementation to do at    */
      /* this time other than to response with success.                 */
      Result = MAP_Update_Inbox_Response(BluetoothStackID, MAPID, MAP_OBEX_RESPONSE_OK);

      if(Result >= 0)
         Display(("MAP_Update_Inbox_Response() Successful.\r\n"));
      else
         Display(("MAP_Update_Inbox_Response() Failure %d.\r\n", Result));
   }
   else
      Display(("Invalid parameter or No longer connected.\r\n"));
}

   /* The following function is used to Process the MAP Notification    */
   /* Registration Indication Event.                                    */
static void ProcessNotificationRegistration(MAP_Notification_Registration_Indication_Data_t *MAP_Notification_Registration_Indication_Data)
{
   int Result;

   if((BluetoothStackID) && (Connected) && (MAP_Notification_Registration_Indication_Data))
   {
      Display(("Notification Status %d.\r\n", MAP_Notification_Registration_Indication_Data->NotificationStatus));

      if(MAP_Notification_Registration_Indication_Data->NotificationStatus)
      {
         if(!MAPNID)
         {
            /* Send a Successful response to the remote device.         */
            Result = MAP_Set_Notification_Registration_Response(BluetoothStackID, MAPID, MAP_OBEX_RESPONSE_OK);
         }
         else
            Result = MAP_Set_Notification_Registration_Response(BluetoothStackID, MAPID, MAP_OBEX_RESPONSE_FORBIDDEN);

         if(Result >= 0)
            Display(("MAP_Set_Notification_Registration_Response() Successful.\r\n"));
         else
            Display(("MAP_Set_Notification_Registration_Response() Failure %d.\r\n", Result));
      }
      else
      {
         MAP_Set_Notification_Registration_Response(BluetoothStackID, MAPID, MAP_OBEX_RESPONSE_OK);
         if(MAPNID)
         {
            Result = MAP_Disconnect_Request(BluetoothStackID, MAPNID);
            MAPNID = 0;

            if(Result >= 0)
               Display(("MAP_Disconnect_Request() Successful.\r\n"));
            else
               Display(("MAP_Disconnect_Request() Failure %d.\r\n", Result));
         }
      }
   }
   else
      Display(("Invalid parameter or No longer connected.\r\n"));
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
         Display(("%*s Attribute ID 0x%04X\r\n", (InitLevel*INDENT_LENGTH), "", SDPServiceAttributeResponse->SDP_Service_Attribute_Value_Data[Index].Attribute_ID));

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
         Display(("%*s Type: Unsigned Int = 0x%02X\r\n", (Level*INDENT_LENGTH), "", SDPDataElement->SDP_Data_Element.UnsignedInteger1Byte));
         break;
      case deUnsignedInteger2Bytes:
         /* Display the Unsigned Integer (2 Bytes) Type.                */
         Display(("%*s Type: Unsigned Int = 0x%04X\r\n", (Level*INDENT_LENGTH), "", SDPDataElement->SDP_Data_Element.UnsignedInteger2Bytes));
         break;
      case deUnsignedInteger4Bytes:
         /* Display the Unsigned Integer (4 Bytes) Type.                */
         Display(("%*s Type: Unsigned Int = 0x%08X\r\n", (Level*INDENT_LENGTH), "", (unsigned int)SDPDataElement->SDP_Data_Element.UnsignedInteger4Bytes));
         break;
      case deUnsignedInteger8Bytes:
         /* Display the Unsigned Integer (8 Bytes) Type.                */
         Display(("%*s Type: Unsigned Int = 0x%02X%02X%02X%02X%02X%02X%02X%02X\r\n", (Level*INDENT_LENGTH), "", SDPDataElement->SDP_Data_Element.UnsignedInteger8Bytes[7],
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
         Display(("%*s Type: Unsigned Int = 0x%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X\r\n", (Level*INDENT_LENGTH), "", SDPDataElement->SDP_Data_Element.UnsignedInteger16Bytes[15],
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
         Display(("%*s Type: Signed Int = 0x%02X\r\n", (Level*INDENT_LENGTH), "", SDPDataElement->SDP_Data_Element.SignedInteger1Byte));
         break;
      case deSignedInteger2Bytes:
         /* Display the Signed Integer (2 Bytes) Type.                  */
         Display(("%*s Type: Signed Int = 0x%04X\r\n", (Level*INDENT_LENGTH), "", SDPDataElement->SDP_Data_Element.SignedInteger2Bytes));
         break;
      case deSignedInteger4Bytes:
         /* Display the Signed Integer (4 Bytes) Type.                  */
         Display(("%*s Type: Signed Int = 0x%08X\r\n", (Level*INDENT_LENGTH), "", (unsigned int)SDPDataElement->SDP_Data_Element.SignedInteger4Bytes));
         break;
      case deSignedInteger8Bytes:
         /* Display the Signed Integer (8 Bytes) Type.                  */
         Display(("%*s Type: Signed Int = 0x%02X%02X%02X%02X%02X%02X%02X%02X\r\n", (Level*INDENT_LENGTH), "", SDPDataElement->SDP_Data_Element.SignedInteger8Bytes[7],
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
         Display(("%*s Type: Signed Int = 0x%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X\r\n", (Level*INDENT_LENGTH), "", SDPDataElement->SDP_Data_Element.SignedInteger16Bytes[15],
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
         Index = (SDPDataElement->SDP_Data_Element_Length < sizeof(TempCallbackBuffer))?SDPDataElement->SDP_Data_Element_Length:(sizeof(TempCallbackBuffer)-1);

         /* Copy the Text String into the Buffer and then NULL terminate*/
         /* it.                                                         */
         BTPS_MemCopy(TempCallbackBuffer, SDPDataElement->SDP_Data_Element.TextString, Index);
         TempCallbackBuffer[Index] = '\0';

         Display(("%*s Type: Text String = %s\r\n", (Level*INDENT_LENGTH), "", TempCallbackBuffer));
         break;
      case deBoolean:
         Display(("%*s Type: Boolean = %s\r\n", (Level*INDENT_LENGTH), "", (SDPDataElement->SDP_Data_Element.Boolean)?"TRUE":"FALSE"));
         break;
      case deURL:
         /* First retrieve the Length of the URL String so that we can  */
         /* copy the Data into our Buffer.                              */
         Index = (SDPDataElement->SDP_Data_Element_Length < sizeof(TempCallbackBuffer))?SDPDataElement->SDP_Data_Element_Length:(sizeof(TempCallbackBuffer)-1);

         /* Copy the URL String into the Buffer and then NULL terminate */
         /* it.                                                         */
         BTPS_MemCopy(TempCallbackBuffer, SDPDataElement->SDP_Data_Element.URL, Index);
         TempCallbackBuffer[Index] = '\0';

         Display(("%*s Type: URL = %s\r\n", (Level*INDENT_LENGTH), "", TempCallbackBuffer));
         break;
      case deUUID_16:
         Display(("%*s Type: UUID_16 = 0x%02X%02X\r\n", (Level*INDENT_LENGTH), "", SDPDataElement->SDP_Data_Element.UUID_16.UUID_Byte0,
                                                                                 SDPDataElement->SDP_Data_Element.UUID_16.UUID_Byte1));
         break;
      case deUUID_32:
         Display(("%*s Type: UUID_32 = 0x%02X%02X%02X%02X\r\n", (Level*INDENT_LENGTH), "", SDPDataElement->SDP_Data_Element.UUID_32.UUID_Byte0,
                                                                                         SDPDataElement->SDP_Data_Element.UUID_32.UUID_Byte1,
                                                                                         SDPDataElement->SDP_Data_Element.UUID_32.UUID_Byte2,
                                                                                         SDPDataElement->SDP_Data_Element.UUID_32.UUID_Byte3));
         break;
      case deUUID_128:
         Display(("%*s Type: UUID_128 = 0x%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X\r\n", (Level*INDENT_LENGTH), "", SDPDataElement->SDP_Data_Element.UUID_128.UUID_Byte0,
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
         Display(("%*s Type: Data Element Sequence\r\n", (Level*INDENT_LENGTH), ""));

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

   /* The following function is responsible for changing the User       */
   /* Interface Mode to Server.                                         */
static int ServerMode(ParameterList_t *TempParam)
{
   UI_Mode = UI_MODE_IS_SERVER;

   UserInterface_Server();

   return(EXIT_MODE);
}

   /* The following function is responsible for changing the User       */
   /* Interface Mode to Client.                                         */
static int ClientMode(ParameterList_t *TempParam)
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
   Boolean_t                         OOB_Data;
   Boolean_t                         MITM;
   GAP_IO_Capability_t               RemoteIOCapability;
   GAP_Pairability_Mode_t            PairabilityMode;
   GAP_Inquiry_Event_Data_t         *GAP_Inquiry_Event_Data;
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

                     Display(("Result: %d, %s.\r\n", (Index+1), Callback_BoardStr));
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
                  BD_ADDRToStr(GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Remote_Device, Callback_BoardStr);
                  Display(("\r\n"));
                  Display(("atAuthenticationStatus: %d for %s\r\n", GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Authentication_Event_Data.Authentication_Status, Callback_BoardStr));

                  /* Flag that there is no longer a current             */
                  /* Authentication procedure in progress.              */
                  ASSIGN_BD_ADDR(CurrentRemoteBD_ADDR, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);

                  /* Check to see if there was an error.  If so, then   */
                  /* delete any stored link key associated with the     */
                  /* device.                                            */
                  if(GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Authentication_Event_Data.Authentication_Status)
                  {
                     for(Index=0;Index<(sizeof(LinkKeyInfo)/sizeof(LinkKeyInfo_t));Index++)
                     {
                        if(COMPARE_BD_ADDR(LinkKeyInfo[Index].BD_ADDR, GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Remote_Device))
                        {
                           LinkKeyInfo[Index].BD_ADDR = CurrentRemoteBD_ADDR;
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

                  for(Index=0, Result=-1; Index<(sizeof(LinkKeyInfo)/sizeof(LinkKeyInfo_t)); Index++)
                  {
                     if(COMPARE_BD_ADDR(LinkKeyInfo[Index].BD_ADDR, GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Remote_Device))
                        break;
                     else
                     {
                        if((Result == (-1)) && (COMPARE_NULL_BD_ADDR(LinkKeyInfo[Index].BD_ADDR)))
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
                     /* * NOTE * We do not store the Link Key if the    */
                     /*          other side specified no bonding (for   */
                     /*          SSP). If this was legacy pairing then  */
                     /*          we need to store the Link Key always.  */
                     if(((GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Authentication_Event_Data.Link_Key_Info.Key_Type == HCI_LINK_KEY_TYPE_COMBINATION_KEY) || ((GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Authentication_Event_Data.Link_Key_Info.Key_Type != HCI_LINK_KEY_TYPE_COMBINATION_KEY) && (RemoteIOCapabilities.Bonding_Type != ibNoBonding))))
                     {
                        LinkKeyInfo[Index].BD_ADDR = GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Remote_Device;
                        LinkKeyInfo[Index].LinkKey = GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Authentication_Event_Data.Link_Key_Info.Link_Key;

                        Display(("Link Key Stored.\r\n"));
                     }
                     else
                        Display(("Link Key Not Stored, No Bonding specified.\r\n"));
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

                  /* * NOTE * If the remote side has specified No       */
                  /*          Bonding (i.e. don't store the Link Key,   */
                  /*          and no MITM, then we will not specify MITM*/
                  /*          so that "Just Works" will be invoked.     */
                  /* * NOTE * We can look at the:                       */
                  /*             Remote_IO_Capabilities_Known           */
                  /*          member to determine if this is in response*/
                  /*          to a remote device requesting the         */
                  /*          authentication (TRUE) or if the local     */
                  /*          device initiated the authentication       */
                  /*          (FALSE).  If the local device initiated   */
                  /*          the authentication then we should specify */
                  /*          the selected value for MITM.              */
                  if(GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Authentication_Event_Data.Remote_IO_Capabilities_Known)
                  {
                     /* If we are non-pairable and the remote side has  */
                     /* requested anything other than Non Bonding then  */
                     /* we need to reject the request.                  */
                     if((!GAP_Query_Pairability_Mode(BluetoothStackID, &PairabilityMode)) && (PairabilityMode == pmNonPairableMode))
                     {
                        if(RemoteIOCapabilities.Bonding_Type != ibNoBonding)
                           GAP_Authentication_Information.Authentication_Data_Length = 0;
                     }

                     if((RemoteIOCapabilities.Bonding_Type == ibNoBonding) && (!RemoteIOCapabilities.MITM_Protection_Required))
                       GAP_Authentication_Information.Authentication_Data.IO_Capabilities.MITM_Protection_Required = FALSE;
                  }

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

                  /* Note the Remote Capabilities (for future operations*/
                  /* - like auto accept and storing the storing the link*/
                  /* key).                                              */
                  RemoteIOCapabilities = GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Authentication_Event_Data.IO_Capabilities;
                  break;
               case atUserConfirmationRequest:
                  BD_ADDRToStr(GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Remote_Device, Callback_BoardStr);
                  Display(("\r\n"));
                  Display(("atUserConfirmationRequest: %s\r\n", Callback_BoardStr));

                  CurrentRemoteBD_ADDR = GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Remote_Device;

                  /* Determine if we need to invoke "Just Works"        */
                  /* pairing.                                           */
                  /* * NOTE * We need to determine if we substituted    */
                  /*          No MITM earlier for the I/O Request.  If  */
                  /*          we did, then we will accept "Just Works"  */
                  /*          pairing as well.                          */
                  if((IOCapability != icDisplayYesNo) || ((RemoteIOCapabilities.Bonding_Type == ibNoBonding) && (!RemoteIOCapabilities.MITM_Protection_Required)))
                  {
                     /* Invoke JUST Works Process...                    */
                     GAP_Authentication_Information.GAP_Authentication_Type          = atUserConfirmation;
                     GAP_Authentication_Information.Authentication_Data_Length       = (Byte_t)sizeof(Byte_t);
                     GAP_Authentication_Information.Authentication_Data.Confirmation = TRUE;

                     /* Submit the Authentication Response.             */
                     Display(("\r\nAuto Accepting: %u\r\n", GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Authentication_Event_Data.Numeric_Value));

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
                     Display(("User Confirmation: %u\r\n", (unsigned long)GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Authentication_Event_Data.Numeric_Value));

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
                  BD_ADDRToStr(GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Remote_Device, Callback_BoardStr);
                  Display(("\r\n"));
                  Display(("atRemoteOutOfBandDataRequest: %s\r\n", Callback_BoardStr));

                  /* This application does not support OOB data so      */
                  /* respond with a data length of Zero to force a      */
                  /* negative reply.                                    */
                  GAP_Authentication_Information.GAP_Authentication_Type    = atOutOfBandData;
                  GAP_Authentication_Information.Authentication_Data_Length = 0;

                  Result = GAP_Authentication_Response(BluetoothStackID, GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Remote_Device, &GAP_Authentication_Information);

                  if(!Result)
                     DisplayFunctionSuccess("GAP_Authentication_Response");
                  else
                     DisplayFunctionError("GAP_Authentication_Response", Result);
                  break;
               case atPasskeyNotification:
                  BD_ADDRToStr(GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Remote_Device, Callback_BoardStr);
                  Display(("\r\n"));
                  Display(("atPasskeyNotification: %s\r\n", Callback_BoardStr));

                  Display(("Passkey Value: %d\r\n", GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Authentication_Event_Data.Numeric_Value));
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

   /* The following function represents the MAP Profile Server Event    */
   /* Callback.  This function will be called whenever a MAP Server     */
   /* Event occurs that is associated with the specified Bluetooth Stack*/
   /* ID.  This function takes as its parameters the Bluetooth Stack ID,*/
   /* the MAP Event Data that occurred and the MAP Profile Event        */
   /* Callback Parameter that was specified when this Callback was      */
   /* installed.  The caller is free to use the contents of the MAP     */
   /* Event Data ONLY in the context of this callback.  If the caller   */
   /* requires the Data for a longer period of time, then the callback  */
   /* function MUST copy the data into another Data Buffer.  This       */
   /* function is guaranteed NOT to be invoked more than once           */
   /* simultaneously for the specified installed callback (i.e. this    */
   /* function DOES NOT have be reentrant).  It Needs to be noted       */
   /* however, that if the same Callback is installed more than once,   */
   /* then the callbacks will be called serially.  Because of this, the */
   /* processing in this function should be as efficient as possible.   */
   /* It should also be noted that this function is called in the Thread*/
   /* Context of a Thread that the User does NOT own.  Therefore,       */
   /* processing in this function should be as efficient as possible    */
   /* (this argument holds anyway because another MAP Profile Event     */
   /* will not be processed while this function call is outstanding).   */
   /* * NOTE * This function MUST NOT Block and wait for events that can*/
   /*          only be satisfied by Receiving other MAP Events.  A      */
   /*          Deadlock WILL occur because NO MAP Event Callbacks will  */
   /*          be issued while this function is currently outstanding.  */
static void BTPSAPI MAP_Event_Callback_Server(unsigned int BluetoothStackID, MAP_Event_Data_t *MAPEventData, unsigned long CallbackParameter)
{
   char BoardStr[16];
   int  Result;

   /* Make sure that parameters passed in appear to be at least         */
   /* semi-valid.                                                       */
   if((BluetoothStackID) && (MAPEventData))
   {
      Display(("\r\n"));

      switch(MAPEventData->Event_Data_Type)
      {
         case etMAP_Open_Port_Indication:
            Display(("Open Port Indication\r\n"));
            BD_ADDRToStr(MAPEventData->Event_Data.MAP_Open_Port_Indication_Data->BD_ADDR, BoardStr);
            Display(("BD_ADDR: %s\r\n", BoardStr));

            /* Clear any outstanding operation information.             */
            CurrentOperation = coNone;

            /* Flag that we are connected.                              */
            Connected        = TRUE;

            /* Make sure we inform the Message Store module that we are */
            /* currently operating in the Root directory.               */
            ChangeMessageFolder(sfRoot, NULL);

            /* Query the connection handle.                             */
            Result = GAP_Query_Connection_Handle(BluetoothStackID, MAPEventData->Event_Data.MAP_Open_Port_Indication_Data->BD_ADDR, &Connection_Handle);
            if(Result)
            {
               /* Failed to Query the Connection Handle.                */
               DisplayFunctionError("GAP_Query_Connection_Handle()", Result);

               Connection_Handle = 0;
            }
            break;
         case etMAP_Open_Port_Confirmation:
            if(MAPEventData->Event_Data.MAP_Open_Port_Confirmation_Data->OpenStatus == MAP_OPEN_STATUS_SUCCESS)
            {
               NotificationConnected = TRUE;
            }
            else
            {
               MAPNID = 0;
            }

            Display(("etMAP_Open_Port_Confirmation Result %d\r\n", MAPEventData->Event_Data.MAP_Open_Port_Confirmation_Data->OpenStatus));
            break;
         case etMAP_Close_Port_Indication:
            Display(("Close Port Indication\r\n"));

            if(MAPEventData->Event_Data.MAP_Close_Port_Indication_Data->MAPID == (unsigned int)MAPID)
            {
               /* Free any allocated data buffers for ongoing           */
               /* transactions.                                         */
               if((CurrentOperation != coNone) && (CurrentBuffer))
               {
                  BTPS_FreeMemory(CurrentBuffer);

                  CurrentBuffer = NULL;
               }

               /* Reset appropriate internal state info.                */
               Connected         = FALSE;
               CurrentOperation  = coNone;
               Connection_Handle = 0;

               if(MAPNID)
               {
                  MAP_Close_Connection(BluetoothStackID, MAPNID);

                  MAPNID                = 0;
                  NotificationConnected = FALSE;
               }
            }
            else
            {
               MAPNID                = 0;
               NotificationConnected = FALSE;
            }
            break;
         case etMAP_Set_Folder_Indication:
            Display(("Set Folder Indication\r\n"));

            ProcessSetFolder(MAPEventData->Event_Data.MAP_Set_Folder_Indication_Data);
            break;
         case etMAP_Get_Folder_Listing_Indication:
            Display(("Get Folder Listing Indication\r\n"));

            ProcessGetFolderListing(MAPEventData->Event_Data.MAP_Get_Folder_Listing_Indication_Data);
            break;
         case etMAP_Get_Message_Listing_Indication:
            Display(("Get Message Listing Indication\r\n"));

            ProcessGetMessageListing(MAPEventData->Event_Data.MAP_Get_Message_Listing_Indication_Data);
            break;
         case etMAP_Get_Message_Indication:
            Display(("Get Message Indication\r\n"));

            ProcessGetMessage(MAPEventData->Event_Data.MAP_Get_Message_Indication_Data);
            break;
         case etMAP_Set_Message_Status_Indication:
            Display(("Set Message Status Indication\r\n"));

            ProcessSetMessageStatus(MAPEventData->Event_Data.MAP_Set_Message_Status_Indication_Data);
            break;
         case etMAP_Push_Message_Indication:
            Display(("Push Message Indication\r\n"));

            ProcessPushMessage(MAPEventData->Event_Data.MAP_Push_Message_Indication_Data);
            break;
         case etMAP_Update_Inbox_Indication:
            Display(("Update Inbox Indication\r\n"));

            ProcessUpdateInbox(MAPEventData->Event_Data.MAP_Update_Inbox_Indication_Data);
            break;
         case etMAP_Abort_Indication:
            Display(("Abort Indication\r\n"));

            /* Free any allocated data buffers for ongoing transactions.*/
            if((CurrentOperation != coNone) && (CurrentBuffer))
            {
               BTPS_FreeMemory(CurrentBuffer);

               CurrentBuffer = NULL;
            }

            /* Clear any outstanding operation information.             */
            CurrentOperation = coNone;
            break;
         case etMAP_Notification_Registration_Indication:
            Display(("Notification Registration Indication\r\n"));

            ProcessNotificationRegistration(MAPEventData->Event_Data.MAP_Notification_Registration_Indication_Data);
            break;
         case etMAP_Send_Event_Confirmation:
            Display(("Send Event Confirmation Result 0x%02X\r\n", MAPEventData->Event_Data.MAP_Send_Event_Confirmation_Data->ResponseCode));
            break;
         default:
            /* Unhandled Event.                                         */
            Display(("Unexpected (Unhandled) Event %d.\r\n", MAPEventData->Event_Data_Type));
            break;
      }

      Display(("Server>"));
   }
}

   /* The following function represents the MAP Profile Client Event    */
   /* Callback.  This function will be called whenever a MAP Client     */
   /* Event occurs that is associated with the specified Bluetooth Stack*/
   /* ID.  This function takes as its parameters the Bluetooth Stack ID,*/
   /* the MAP Event Data that occurred and the MAP Profile Event        */
   /* Callback Parameter that was specified when this Callback was      */
   /* installed.  The caller is free to use the contents of the MAP     */
   /* Event Data ONLY in the context of this callback.  If the caller   */
   /* requires the Data for a longer period of time, then the callback  */
   /* function MUST copy the data into another Data Buffer.  This       */
   /* function is guaranteed NOT to be invoked more than once           */
   /* simultaneously for the specified installed callback (i.e. this    */
   /* function DOES NOT have be reentrant).  It Needs to be noted       */
   /* however, that if the same Callback is installed more than once,   */
   /* then the callbacks will be called serially.  Because of this, the */
   /* processing in this function should be as efficient as possible.   */
   /* It should also be noted that this function is called in the Thread*/
   /* Context of a Thread that the User does NOT own.  Therefore,       */
   /* processing in this function should be as efficient as possible    */
   /* (this argument holds anyway because another MAP Profile Event     */
   /* will not be processed while this function call is outstanding).   */
   /* * NOTE * This function MUST NOT Block and wait for events that can*/
   /*          only be satisfied by Receiving other MAP Events.  A      */
   /*          Deadlock WILL occur because NO MAP Event Callbacks will  */
   /*          be issued while this function is currently outstanding.  */
static void BTPSAPI MAP_Event_Callback_Client(unsigned int BluetoothStackID, MAP_Event_Data_t *MAPEventData, unsigned long CallbackParameter)
{
   int          Result;
   unsigned int Index;
   unsigned int TempLength;

   /* Make sure that parameters passed in appear to be at least         */
   /* semi-valid.                                                       */
   if((BluetoothStackID) && (MAPEventData))
   {
      Display(("\r\n"));

      switch(MAPEventData->Event_Data_Type)
      {
         case etMAP_Open_Port_Indication:
            Display(("Open Port Indication:  Notification Client Connected\r\n"));

            NotificationConnected = TRUE;
            break;
         case etMAP_Open_Port_Confirmation:
            Display(("Open Port Confirmation\r\n"));

            Display(("Response Code: 0x%02X\r\n", MAPEventData->Event_Data.MAP_Open_Port_Confirmation_Data->OpenStatus));

            /* If this confirmation indicates failure we need to clear  */
            /* MAPID.                                                   */
            if(MAPEventData->Event_Data.MAP_Open_Port_Confirmation_Data->OpenStatus)
            {
               MAPID     = 0;
               Connected = FALSE;
            }

            /* Reset the current operation value.                       */
            CurrentOperation = coNone;
            break;
         case etMAP_Close_Port_Indication:
            Display(("Close Port Indication\r\n"));

            /* Get the ID of the port that was closed.                  */
            if(MAPEventData->Event_Data.MAP_Close_Port_Indication_Data->MAPID == (unsigned int)MAPID)
            {
               /* Free any allocated data buffers for ongoing           */
               /* transactions.                                         */
               if((CurrentOperation != coNone) && (CurrentBuffer))
               {
                  BTPS_FreeMemory(CurrentBuffer);

                  CurrentBuffer = NULL;
               }

               /* Reset appropriate internal state info.                */
               MAPID             = 0;
               Connected         = FALSE;
               CurrentOperation  = coNone;
               Connection_Handle = 0;

               if(MAPNID)
               {
                  MAP_Close_Server(BluetoothStackID, MAPNID);

                  MAP_Un_Register_SDP_Record(BluetoothStackID, MAPNID, NotificationSDPRecordHandle);

                  MAPNID                      = 0;
                  NotificationConnected       = FALSE;
                  NotificationSDPRecordHandle = 0;
               }
            }
            else
               NotificationConnected = FALSE;
            break;
         case etMAP_Set_Folder_Confirmation:
            Display(("Set Folder Confirmation\r\n"));
            Display(("Response Code: 0x%02X\r\n", MAPEventData->Event_Data.MAP_Set_Folder_Confirmation_Data->ResponseCode));

            CurrentOperation = coNone;
            break;
         case etMAP_Get_Folder_Listing_Confirmation:
            Display(("Get Folder Listing Confirmation\r\n"));
            Display(("Response Code: 0x%02X\r\n", MAPEventData->Event_Data.MAP_Get_Folder_Listing_Confirmation_Data->ResponseCode));
            Display(("Number Folders: %d\r\n", MAPEventData->Event_Data.MAP_Get_Folder_Listing_Confirmation_Data->NumberOfFolders));
            Display(("Data Length: %u\r\n", MAPEventData->Event_Data.MAP_Get_Folder_Listing_Confirmation_Data->DataLength));

            if(MAPEventData->Event_Data.MAP_Get_Folder_Listing_Confirmation_Data->DataLength)
            {
               Display(("Data:\r\n"));

               for(Index = 0, TempCallbackBuffer[0] = '\0'; (Index < MAPEventData->Event_Data.MAP_Get_Folder_Listing_Confirmation_Data->DataLength) && (Index < (SIZEOF_TEMP_CALLBACK_BUFFER - 1));Index++)
                  BTPS_SprintF(&(TempCallbackBuffer[BTPS_StringLength(TempCallbackBuffer)]), "%c", MAPEventData->Event_Data.MAP_Get_Folder_Listing_Confirmation_Data->DataBuffer[Index]);

               Display((TempCallbackBuffer));
               Display(("\r\n"));
            }

            /* Determine if we need to issue another request to continue*/
            /* the operation.                                           */
            if(MAPEventData->Event_Data.MAP_Get_Folder_Listing_Confirmation_Data->ResponseCode == MAP_OBEX_RESPONSE_CONTINUE)
            {
               /* printf Results.                                       */
               Display(("Get Folder Listing not complete, Requesting more data.\r\n"));

               /* Continue the Get Folder Listing request.  Note that   */
               /* most parameters passed are ignored on a continuation  */
               /* call.                                                 */
               Result = MAP_Get_Folder_Listing_Request(BluetoothStackID, MAPID, MAP_MAX_LIST_COUNT_NOT_RESTRICTED, 0);

               if(Result >= 0)
               {
                  Display(("MAP_Get_Folder_Listing_Request() Successful.\r\n"));

                  CurrentOperation = coGetFolderListing;
               }
               else
               {
                  Display(("MAP_Get_Folder_Listing_Request() Failure %d.\r\n", Result));

                  CurrentOperation = coNone;
               }
            }
            else
            {
               /* Set current operation.                                */
               CurrentOperation = coNone;

               Display(("Get Folder Listing Completed.\r\n"));
            }
            break;
         case etMAP_Get_Message_Listing_Confirmation:
            Display(("Get Message Listing Confirmation\r\n"));
            Display(("Response Code: 0x%02X\r\n", MAPEventData->Event_Data.MAP_Get_Message_Listing_Confirmation_Data->ResponseCode));
            Display(("New Message: %d\r\n", MAPEventData->Event_Data.MAP_Get_Message_Listing_Confirmation_Data->NewMessage));
            Display(("Number Messages: %d\r\n", MAPEventData->Event_Data.MAP_Get_Message_Listing_Confirmation_Data->NumberOfMessages));
            Display(("Data Length: %u\r\n", MAPEventData->Event_Data.MAP_Get_Message_Listing_Confirmation_Data->DataLength));

            if(MAPEventData->Event_Data.MAP_Get_Message_Listing_Confirmation_Data->DataLength)
            {
               Display(("Data:\r\n"));

               for(Index = 0, TempCallbackBuffer[0] = '\0'; (Index < MAPEventData->Event_Data.MAP_Get_Message_Listing_Confirmation_Data->DataLength) && (Index < (SIZEOF_TEMP_CALLBACK_BUFFER - 1));Index++)
                  BTPS_SprintF(&(TempCallbackBuffer[BTPS_StringLength(TempCallbackBuffer)]), "%c", MAPEventData->Event_Data.MAP_Get_Message_Listing_Confirmation_Data->DataBuffer[Index]);

               Display((TempCallbackBuffer));
               Display(("\r\n"));
            }

            /* Determine if we need to issue another request to continue*/
            /* the operation.                                           */
            if(MAPEventData->Event_Data.MAP_Get_Message_Listing_Confirmation_Data->ResponseCode == MAP_OBEX_RESPONSE_CONTINUE)
            {
               /* printf Results.                                       */
               Display(("Get Message Listing not complete, Requesting more data.\r\n"));

               /* Continue the Get Message Listing request.  Note that  */
               /* most parameters passed are ignored on a continuation  */
               /* call.                                                 */
               Result = MAP_Get_Message_Listing_Request(BluetoothStackID, MAPID, NULL, MAP_MAX_LIST_COUNT_NOT_RESTRICTED, 0, NULL);

               if(Result >= 0)
               {
                  Display(("MAP_Get_Message_Listing_Request() Successful.\r\n"));

                  CurrentOperation = coGetMessageListing;
               }
               else
               {
                  Display(("MAP_Get_Message_Listing_Request() Failure %d.\r\n", Result));

                  CurrentOperation = coNone;
               }
            }
            else
            {
               /* Set current operation.                                */
               CurrentOperation = coNone;

               Display(("Get Message Listing Completed.\r\n"));
            }
            break;
         case etMAP_Get_Message_Confirmation:
            Display(("Get Message Confirmation\r\n"));
            Display(("Response Code: 0x%02X\r\n", MAPEventData->Event_Data.MAP_Get_Message_Confirmation_Data->ResponseCode));
            Display(("Fractional Type: %d\r\n", MAPEventData->Event_Data.MAP_Get_Message_Confirmation_Data->FractionalType));
            Display(("Data Length: %u\r\n", MAPEventData->Event_Data.MAP_Get_Message_Confirmation_Data->DataLength));

            if(MAPEventData->Event_Data.MAP_Get_Message_Confirmation_Data->DataLength)
            {
               Display(("Data:\r\n"));

               for(Index = 0, TempCallbackBuffer[0] = '\0'; (Index < MAPEventData->Event_Data.MAP_Get_Message_Confirmation_Data->DataLength) && (Index < (SIZEOF_TEMP_CALLBACK_BUFFER - 1));Index++)
                  BTPS_SprintF(&(TempCallbackBuffer[BTPS_StringLength(TempCallbackBuffer)]), "%c", MAPEventData->Event_Data.MAP_Get_Message_Confirmation_Data->DataBuffer[Index]);

               Display((TempCallbackBuffer));
               Display(("\r\n"));
            }

            /* Determine if we need to issue another request to continue*/
            /* the operation.                                           */
            if(MAPEventData->Event_Data.MAP_Get_Message_Confirmation_Data->ResponseCode == MAP_OBEX_RESPONSE_CONTINUE)
            {
               /* printf Results.                                       */
               Display(("Get Message not complete, Requesting more data.\r\n"));

               /* Continue the Get Message request.  Note that most     */
               /* parameters passed are ignored on a continuation call. */
               Result = MAP_Get_Message_Request(BluetoothStackID, MAPID, NULL, FALSE, csUTF8, ftUnfragmented);

               if(Result >= 0)
               {
                  Display(("MAP_Get_Message_Request() Successful.\r\n"));

                  CurrentOperation = coGetMessage;
               }
               else
               {
                  Display(("MAP_Get_Message_Request() Failure %d.\r\n", Result));

                  CurrentOperation = coNone;
               }
            }
            else
            {
               /* Set current operation.                                */
               CurrentOperation = coNone;

               Display(("Get Message Completed.\r\n"));
            }
            break;
         case etMAP_Set_Message_Status_Confirmation:
            Display(("Set Message Status Confirmation\r\n"));
            Display(("Response Code: 0x%02X\r\n", MAPEventData->Event_Data.MAP_Set_Message_Status_Confirmation_Data->ResponseCode));

            CurrentOperation = coNone;
            break;
         case etMAP_Push_Message_Confirmation:
            Display(("Push MessageConfirmation\r\n"));
            Display(("Response Code: 0x%02X\r\n", MAPEventData->Event_Data.MAP_Push_Message_Confirmation_Data->ResponseCode));

            /* Determine if we need to issue another request to continue*/
            /* the operation.                                           */
            if((MAPEventData->Event_Data.MAP_Push_Message_Confirmation_Data->ResponseCode == MAP_OBEX_RESPONSE_CONTINUE) && (CurrentBufferSent != CurrentBufferSize))
            {
               /* printf Results.                                       */
               Display(("Message Push not complete, Sending more data.\r\n"));

               /* Continue the Push Message request.  Note that most    */
               /* parameters passed are ignored on a continuation call. */
               Result = MAP_Push_Message_Request(BluetoothStackID, MAPID, NULL, FALSE, FALSE, csUTF8, (CurrentBufferSize - CurrentBufferSent), (Byte_t *)&(CurrentBuffer[CurrentBufferSent]), &TempLength, TRUE);

               /* Flag whether or not the operation completed           */
               /* successfully.                                         */
               if(Result >= 0)
                  CurrentBufferSent += TempLength;
               else
               {
                  BTPS_FreeMemory(CurrentBuffer);

                  CurrentBuffer    = NULL;

                  CurrentOperation = coNone;
               }
            }
            else
            {
               /* Set current operation.                                */
               CurrentOperation = coNone;

               Display(("Message Push Completed.\r\n"));

               if(CurrentBuffer)
                  BTPS_FreeMemory(CurrentBuffer);

               CurrentBuffer = NULL;
            }
            break;
         case etMAP_Update_Inbox_Confirmation:
            Display(("Update Inbox Confirmation\r\n"));
            Display(("Response Code: 0x%02X\r\n", MAPEventData->Event_Data.MAP_Update_Inbox_Confirmation_Data->ResponseCode));

            CurrentOperation = coNone;
            break;
         case etMAP_Abort_Confirmation:
            Display(("Abort Confirmation\r\n"));

            /* Reset the current operation value.                       */
            CurrentOperation = coNone;
            break;
         case etMAP_Notification_Registration_Confirmation:
            Display(("Notification Registration Confirmation Result 0x%02X\r\n", MAPEventData->Event_Data.MAP_Notification_Registration_Confirmation_Data->ResponseCode));
            break;
         case etMAP_Send_Event_Indication:
            if(MAPEventData->Event_Data.MAP_Send_Event_Indication_Data->DataLength)
            {
               Display(("Data: "));

               for(Index=0;Index<MAPEventData->Event_Data.MAP_Send_Event_Indication_Data->DataLength;Index++)
                  Display(("%c", MAPEventData->Event_Data.MAP_Send_Event_Indication_Data->DataBuffer[Index]));

               Display(("\r\n"));
            }

            MAP_Send_Event_Response(BluetoothStackID, MAPNID, MAP_OBEX_RESPONSE_OK);
            break;
         default:
            Display(("Unexpected (Unhandled) Event %d.\r\n", MAPEventData->Event_Data_Type));
            break;
      }

      Display(("Client>"));
   }
}

   /* The following function is responsible for processing HCI Mode     */
   /* change events.                                                    */
static void BTPSAPI HCI_Event_Callback(unsigned int BluetoothStackID, HCI_Event_Data_t *HCI_Event_Data, unsigned long CallbackParameter)
{
   char *Mode;

   /* Make sure that the input parameters that were passed to us are    */
   /* semi-valid.                                                       */
   if((BluetoothStackID) && (HCI_Event_Data))
   {
      /* Process the Event Data.                                        */
      switch(HCI_Event_Data->Event_Data_Type)
      {
         case etMode_Change_Event:
            if(HCI_Event_Data->Event_Data.HCI_Mode_Change_Event_Data)
            {
               switch(HCI_Event_Data->Event_Data.HCI_Mode_Change_Event_Data->Current_Mode)
               {
                  case HCI_CURRENT_MODE_HOLD_MODE:
                     Mode = "Hold";
                     break;
                  case HCI_CURRENT_MODE_SNIFF_MODE:
                     Mode = "Sniff";
                     break;
                  case HCI_CURRENT_MODE_PARK_MODE:
                     Mode = "Park";
                     break;
                  case HCI_CURRENT_MODE_ACTIVE_MODE:
                  default:
                     Mode = "Active";
                     break;
               }

               Display(("\r\n"));
               Display(("HCI Mode Change Event, Status: 0x%02X, Connection Handle: %d, Mode: %s, Interval: %d\r\n", HCI_Event_Data->Event_Data.HCI_Mode_Change_Event_Data->Status,
                                                                                                                    HCI_Event_Data->Event_Data.HCI_Mode_Change_Event_Data->Connection_Handle,
                                                                                                                    Mode,
                                                                                                                    HCI_Event_Data->Event_Data.HCI_Mode_Change_Event_Data->Interval));
               DisplayPrompt();
            }
            break;
      }
   }
}

   /* The following function is used to initialize the application      */
   /* instance.  This function should open the stack and prepare to     */
   /* execute commands based on user input.                             */
int InitializeApplication(HCI_DriverInformation_t *HCI_DriverInformation, BTPS_Initialization_t *BTPS_Initialization)
{
   int ret_val = APPLICATION_ERROR_UNABLE_TO_OPEN_STACK;

   /* Initialize some defaults.                                         */
   NumberofValidResponses = 0;

   /* Next, make sure that the Driver Information passed appears to be  */
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
                  /* Attempt to register a HCI Event Callback.          */
                  ret_val = HCI_Register_Event_Callback(BluetoothStackID, HCI_Event_Callback, (unsigned long)NULL);
                  if(ret_val > 0)
                  {
                     /* Assign the Callback Handle.                     */
                     HCIEventCallbackHandle = ret_val;

                     /* Save the maximum supported baud rate.           */
                     MaxBaudRate = (DWord_t)(HCI_DriverInformation->DriverInformation.COMMDriverInformation.BaudRate);

                     /* Set up the Selection Interface.                 */
                     UserInterface_Selection();

                     /* Display the first command prompt.               */
                     DisplayPrompt();

                     /* Return success to the caller.                   */
                     ret_val = (int)BluetoothStackID;
                  }
                  else
                     DisplayFunctionError("HCI_Register_Event_Callback()", ret_val);
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
   if(UI_Mode == UI_MODE_IS_CLIENT)
      Display(("\r\nClient>"));
   else
   {
      if(UI_Mode == UI_MODE_IS_SERVER)
         Display(("\r\nServer>"));
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
