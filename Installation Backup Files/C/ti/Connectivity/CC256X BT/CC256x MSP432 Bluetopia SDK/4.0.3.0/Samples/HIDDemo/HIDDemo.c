/*****< hiddemo.c >************************************************************/
/*      Copyright 2003 - 2014 Stonestreet One.                                */
/*      All Rights Reserved.                                                  */
/*                                                                            */
/*      Copyright 2015 Texas Instruments Incorporated.                        */
/*      All Rights Reserved.                                                  */
/*                                                                            */
/*  HIDDemo - Simple application using HID Profile.                           */
/*                                                                            */
/*  Author:  Rory Sledge                                                      */
/*                                                                            */
/*** MODIFICATION HISTORY *****************************************************/
/*                                                                            */
/*   mm/dd/yy  F. Lastname    Description of Modification                     */
/*   --------  -----------    ------------------------------------------------*/
/*   01/03/03  R. Sledge      Initial creation.                               */
/*   03/03/15  D. Horowitz    Adding Demo Application version.                */
/******************************************************************************/
#include "Main.h"          /* Application Interface Abstraction.              */
#include "HIDDemo.h"       /* Main Application Prototypes and Constants.      */
#include "SS1BTPS.h"       /* Includes for the SS1 Bluetooth Protocol Stack.  */
#include "SS1BTHID.h"      /* Includes for the SS1 HID Profile.               */

#define MAX_SUPPORTED_COMMANDS                     (33)  /* Denotes the       */
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

#define MAX_INQUIRY_RESULTS                        (20)  /* Denotes the max   */
                                                         /* number of inquiry */
                                                         /* results.          */

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

#define INDENT_LENGTH                               (3)  /* Denotes the number*/
                                                         /* of character      */
                                                         /* spaces to be used */
                                                         /* for indenting when*/
                                                         /* displaying SDP    */
                                                         /* Data Elements.    */

   /* Determine the Name we will use for this compilation.              */
#define APP_DEMO_NAME                              "HIDDemo"


   /* The following represent the possible values of UI_Mode variable.  */
#define UI_MODE_IS_CLIENT      (2)
#define UI_MODE_IS_SERVER      (1)
#define UI_MODE_SELECT         (0)
#define UI_MODE_IS_INVALID     (-1)
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
   /* Internal Variables to this Module (Remember that all variables    */
   /* declared static are initialized to 0 automatically by the         */
   /* compiler as part of standard C/C++).                              */

static unsigned int        BluetoothStackID;        /* Variable which holds the Handle */
                                                    /* of the opened Bluetooth Protocol*/
                                                    /* Stack.                          */

static unsigned int        HIDID;                   /* Variable which holds the Handle */
                                                    /* of the active HID Connection.   */

static DWord_t             HIDDeviceServerSDPHandle;/* Variable which holds the Handle */
                                                    /* of the HID Device Server SDP    */
                                                    /* Service Record.                 */

static int                 UI_Mode;                 /* Variable used to indicate if the*/
                                                    /* program is to be run in Host    */
                                                    /* Mode or Device Mode.            */

static BD_ADDR_t           InquiryResultList[MAX_INQUIRY_RESULTS];  /* Variable which  */
                                                    /* contains the inquiry result     */
                                                    /* received from the most recently */
                                                    /* preformed inquiry.              */

static unsigned int        NumberofValidResponses;  /* Variable which holds the number */
                                                    /* of valid inquiry results within */
                                                    /* the inquiry results array.      */

static LinkKeyInfo_t       LinkKeyInfo[16];         /* Variable which holds the list of*/
                                                    /* BD_ADDR <-> Link Keys for       */
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

   /* The following table represent the Mouse Report Descriptor for this*/
   /* HID Mouse Device.                                                 */
static Byte_t MouseReportDescriptor[] =
{
   0x05, 0x01,   /* USAGE_PAGE (Generic Desktop)                        */
   0x09, 0x02,   /* USAGE (Mouse)                                       */
   0xa1, 0x01,   /* COLLECTION (Application)                            */
   0x09, 0x01,   /*   USAGE (Pointer)                                   */
   0xa1, 0x00,   /*   COLLECTION (Physical)                             */
   0x85, 0x02,   /*     REPORT_ID (2)                                   */
   0x05, 0x09,   /*     USAGE_PAGE (Button)                             */
   0x19, 0x01,   /*     USAGE_MINIMUM (Button 1)                        */
   0x29, 0x03,   /*     USAGE_MAXIMUM (Button 3)                        */
   0x15, 0x00,   /*     LOGICAL_MINIMUM (0)                             */
   0x25, 0x01,   /*     LOGICAL_MAXIMUM (1)                             */
   0x95, 0x03,   /*     REPORT_COUNT (3)                                */
   0x75, 0x01,   /*     REPORT_SIZE (1)                                 */
   0x81, 0x02,   /*     INPUT (Data,Var,Abs)                            */
   0x95, 0x01,   /*     REPORT_COUNT (1)                                */
   0x75, 0x05,   /*     REPORT_SIZE (5)                                 */
   0x81, 0x03,   /*     INPUT (Cnst,Var,Abs)                            */
   0x05, 0x01,   /*     USAGE_PAGE (Generic Desktop)                    */
   0x09, 0x30,   /*     USAGE (X)                                       */
   0x09, 0x31,   /*     USAGE (Y)                                       */
   0x15, 0x81,   /*     LOGICAL_MINIMUM (-127)                          */
   0x25, 0x7f,   /*     LOGICAL_MAXIMUM (127)                           */
   0x75, 0x08,   /*     REPORT_SIZE (8)                                 */
   0x95, 0x02,   /*     REPORT_COUNT (2)                                */
   0x81, 0x06,   /*     INPUT (Data,Var,Rel)                            */
   0xc0,         /*   END_COLLECTION                                    */
   0xc0          /* END_COLLECTION                                      */
} ;

   /* The following represents a Generic Mouse Report.                  */
static Byte_t GenericMouseReport[] =
{
   0x02, 0x80, 0x50, 0x00
} ;

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


   /* Internal function prototypes.                                     */
static void DisplaySelectionMenu(void);
static void DisplayHIDHostMenu(void);
static void DisplayHIDDeviceMenu(void);
static void DisplayFunctionError(char *Function,int Status);
static void PopulateHIDHostCommandTable(void);
static void PopulateHIDDeviceCommandTable(void);

static void UserInterface_Main(void);
static void UserInterface_Selection(void);

static Boolean_t CommandLineInterpreter(char *Command);
static unsigned long StringToUnsignedInteger(char *StringInteger);
static char *StringParser(char *String);
static int CommandParser(UserCommand_t *TempCommand, char *UserInput);
static int CommandInterpreter(UserCommand_t *TempCommand);
static int AddCommand(char *CommandName, CommandFunction_t CommandFunction);
static CommandFunction_t FindCommand(char *Command);
static void ClearCommands(void);

static void BD_ADDRToStr(BD_ADDR_t Board_Address, char *BoardStr);
static void DisplayFWVersion(void);

static int DisplayHelp(ParameterList_t *TempParam);
static int QueryMemory(ParameterList_t *TempParam);

static int InitializeHIDHost(void);
static int InitializeHIDDevice(void);

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

static int HIDRegisterHostServer(void);
static int HIDRegisterDeviceServer(void);

static int HIDConnectRemoteDevice(ParameterList_t *TempParam);
static int HIDConnectRemoteHost(ParameterList_t *TempParam);
static int HIDCloseConnection(ParameterList_t *TempParam);

static int HIDControlRequest(ParameterList_t *TempParam);
static int HIDGetReportRequest(ParameterList_t *TempParam);
static int HIDGetReportResponse(ParameterList_t *TempParam);
static int HIDSetReportRequest(ParameterList_t *TempParam);
static int HIDSetReportResponse(ParameterList_t *TempParam);
static int HIDGetProtocolRequest(ParameterList_t *TempParam);
static int HIDGetProtocolResponse(ParameterList_t *TempParam);
static int HIDSetProtocolRequest(ParameterList_t *TempParam);
static int HIDSetProtocolResponse(ParameterList_t *TempParam);
static int HIDGetIdleRequest(ParameterList_t *TempParam);
static int HIDGetIdleResponse(ParameterList_t *TempParam);
static int HIDSetIdleRequest(ParameterList_t *TempParam);
static int HIDSetIdleResponse(ParameterList_t *TempParam);
static int HIDDataWrite(ParameterList_t *TempParam);

static int HIDHostMode(ParameterList_t *TempParam);
static int HIDDeviceMode(ParameterList_t *TempParam);

   /* Callback Function Prototypes.                                     */
static void BTPSAPI GAP_Event_Callback(unsigned int BluetoothStackID, GAP_Event_Data_t *GAPEventData, unsigned long CallbackParameter);
static void BTPSAPI HID_Event_Callback(unsigned int BluetoothStackID, HID_Event_Data_t *HIDEventData, unsigned long CallbackParameter);

   /* The following function displays the HID Role Command Menu.        */
static void DisplaySelectionMenu(void)
{
   Display(("\r\n************************** Command Options **************************\r\n"));
   Display(("* Command Options: Host, Device                                     *\r\n"));
   Display(("*********************************************************************\r\n"));
}

   /* The following function displays the HID Host Command Menu.        */
static void DisplayHIDHostMenu(void)
{
   /* First display the upper command bar.                              */
   Display(("************************** Command Options **************************\r\n"));

   /* Next, display all of the commands.                                */
   Display(("*  Inquiry                                                          *\r\n"));
   Display(("*  DisplayInquiryList                                               *\r\n"));
   Display(("*  Pair [Inquiry Index] [Bonding Type]                              *\r\n"));
   Display(("*  EndPairing [Inquiry Index]                                       *\r\n"));
   Display(("*  PINCodeResponse [PIN Code]                                       *\r\n"));
   Display(("*  PassKeyResponse [Numeric Passkey]                                *\r\n"));
   Display(("*  UserConfirmationResponse [Confirmation Flag]                     *\r\n"));
   Display(("*  SetDiscoverabilityMode [Discoverability Mode]                    *\r\n"));
   Display(("*  SetConnectabilityMode [Connectability Mode]                      *\r\n"));
   Display(("*  SetPairabilityMode [Pairability Mode]                            *\r\n"));
   Display(("*  ChangeSimplePairingParameters [I/O Capabilities] [MITM Flag]     *\r\n"));
   Display(("*  GetLocalAddress                                                  *\r\n"));
   Display(("*  GetLocalName                                                     *\r\n"));
   Display(("*  SetLocalName [Local Device Name (no spaces allowed)]             *\r\n"));
   Display(("*  GetClassOfDevice                                                 *\r\n"));
   Display(("*  SetClassOfDevice [Class of Device]                               *\r\n"));
   Display(("*  GetRemoteName [Inquiry Index]                                    *\r\n"));
   Display(("*  ConnectRemoteHIDDevice [Inquiry Index]                           *\r\n"));
   Display(("*  CloseConnection                                                  *\r\n"));
   Display(("*  ControlRequest [Control Operation]                               *\r\n"));
   Display(("*  GetReportRequest [Size] [ReportType] [ReportID] [BufferSize]     *\r\n"));
   Display(("*  SetReportRequest [ReportType]                                    *\r\n"));
   Display(("*  GetProtocolRequest                                               *\r\n"));
   Display(("*  SetProtocolRequest [Protocol]                                    *\r\n"));
   Display(("*  GetIdleRequest                                                   *\r\n"));
   Display(("*  SetIdleRequest [IdleRate]                                        *\r\n"));
   Display(("*  DataWrite [ReportType]                                           *\r\n"));
   Display(("*  Help                                                             *\r\n"));
   Display(("*  Quit                                                             *\r\n"));

   Display(("*********************************************************************\r\n"));
}

   /* The following function displays the HID Device Command Menu.      */
static void DisplayHIDDeviceMenu(void)
{
   /* First display the upper command bar.                              */
   Display(("************************** Command Options **************************\r\n"));

   /* Next, display all of the commands.                                */
   Display(("*  Inquiry                                                          *\r\n"));
   Display(("*  DisplayInquiryList                                               *\r\n"));
   Display(("*  Pair [Inquiry Index] [Bonding Type]                              *\r\n"));
   Display(("*  EndPairing [Inquiry Index]                                       *\r\n"));
   Display(("*  PINCodeResponse [PIN Code]                                       *\r\n"));
   Display(("*  PassKeyResponse [Numeric Passkey]                                *\r\n"));
   Display(("*  UserConfirmationResponse [Confirmation Flag]                     *\r\n"));
   Display(("*  SetDiscoverabilityMode [Discoverability Mode]                    *\r\n"));
   Display(("*  SetConnectabilityMode [Connectability Mode]                      *\r\n"));
   Display(("*  SetPairabilityMode [Pairability Mode]                            *\r\n"));
   Display(("*  ChangeSimplePairingParameters [I/O Capabilities] [MITM Flag]     *\r\n"));
   Display(("*  GetLocalAddress                                                  *\r\n"));
   Display(("*  GetLocalName                                                     *\r\n"));
   Display(("*  SetLocalName [Local Device Name (no spaces allowed)]             *\r\n"));
   Display(("*  GetClassOfDevice                                                 *\r\n"));
   Display(("*  SetClassOfDevice [Class of Device]                               *\r\n"));
   Display(("*  GetRemoteName [Inquiry Index]                                    *\r\n"));
   Display(("*  ConnectRemoteHIDHost [Inquiry Index]                             *\r\n"));
   Display(("*  CloseConnection                                                  *\r\n"));
   Display(("*  ControlRequest                                                   *\r\n"));
   Display(("*  GetReportResponse [ResultType] [ReportType]                      *\r\n"));
   Display(("*  SetReportResponse [ResultType]                                   *\r\n"));
   Display(("*  GetProtocolResponse [ResultType] [Protocol]                      *\r\n"));
   Display(("*  SetProtocolResponse [ResultType]                                 *\r\n"));
   Display(("*  GetIdleResponse [ResultType] [IdleRate]                          *\r\n"));
   Display(("*  SetIdleResponse [ResultType]                                     *\r\n"));
   Display(("*  DataWrite [ReportType]                                           *\r\n"));
   Display(("*  Help                                                             *\r\n"));
   Display(("*  Quit                                                             *\r\n"));

   Display(("*********************************************************************\r\n"));
}

   /* The following function Clears all commands currently in the       */
   /* command table and populates it with the Commands that are         */
   /* available for the HID Host.                                       */
static void PopulateHIDHostCommandTable(void)
{
   /* First Clear all of the commands in the Command Table.             */
   ClearCommands();

   /* Now add all of the commands that are associated with the HID Host */
   /* to the Command Table.                                             */
   AddCommand("CONNECTREMOTEHIDDEVICE", HIDConnectRemoteDevice);
   AddCommand("CLOSECONNECTION", HIDCloseConnection);
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
   AddCommand("CONTROLREQUEST", HIDControlRequest);
   AddCommand("GETREPORTREQUEST", HIDGetReportRequest);
   AddCommand("SETREPORTREQUEST", HIDSetReportRequest);
   AddCommand("GETPROTOCOLREQUEST", HIDGetProtocolRequest);
   AddCommand("SETPROTOCOLREQUEST", HIDSetProtocolRequest);
   AddCommand("GETIDLEREQUEST", HIDGetIdleRequest);
   AddCommand("SETIDLEREQUEST", HIDSetIdleRequest);
   AddCommand("DATAWRITE", HIDDataWrite);
   AddCommand("HELP", DisplayHelp);
   AddCommand("QUERYMEMORY", QueryMemory);
}

   /* The following function Clears all commands currently in the       */
   /* command table and populates it with the Commands that are         */
   /* available for the HID Device unit.                                */
static void PopulateHIDDeviceCommandTable(void)
{
   /* First Clear all of the commands in the Command Table.             */
   ClearCommands();

   /* Now add all of the commands that are associated with the HID      */
   /* Device unit to the Command Table.                                 */
   AddCommand("CONNECTREMOTEHIDHOST", HIDConnectRemoteHost);
   AddCommand("CLOSECONNECTION", HIDCloseConnection);
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
   AddCommand("CONTROLREQUEST", HIDControlRequest);
   AddCommand("GETREPORTRESPONSE", HIDGetReportResponse);
   AddCommand("SETREPORTRESPONSE", HIDSetReportResponse);
   AddCommand("GETPROTOCOLRESPONSE", HIDGetProtocolResponse);
   AddCommand("SETPROTOCOLRESPONSE", HIDSetProtocolResponse);
   AddCommand("GETIDLERESPONSE", HIDGetIdleResponse);
   AddCommand("SETIDLERESPONSE", HIDSetIdleResponse);
   AddCommand("DATAWRITE", HIDDataWrite);
   AddCommand("HELP", DisplayHelp);
   AddCommand("QUERYMEMORY", QueryMemory);
}

   /* This function is responsible for taking the users input and do the*/
   /* appropriate thing with it.  First, this function get a string of  */
   /* user input, parse the user input in to command and parameters, and*/
   /* finally executing the command or display an error message if the  */
   /* input is corrupt.                                                 */
static void UserInterface_Main(void)
{
   /* Next display the available commands.                              */
   DisplayHelp(NULL);

   ClearCommands();

   /* Determine if we are currently running in Host or Device mode.     */
   if(UI_Mode == UI_MODE_IS_CLIENT)
   {
      /* We are currently running in Host mode, add the appropriate     */
      /* commands to the command table and display the menu options.    */
      PopulateHIDHostCommandTable();
   }
   else
   {
      /* We are not currently running in Host mode therefore we must be */
      /* running in Device mode.  Add the appropriate commands to the   */
      /* command table and display the menu options.                    */
      PopulateHIDDeviceCommandTable();
   }
}

   /* The following function is responsible for choosing the user       */
   /* interface to present to the user.                                 */
static void UserInterface_Selection(void)
{
   /* Set the Host mode to an invalid selection.                        */
   UI_Mode = UI_MODE_SELECT;
   /* Next display the available commands.                              */
   DisplaySelectionMenu();

   ClearCommands();

   AddCommand("HOST", HIDHostMode);
   AddCommand("DEVICE", HIDDeviceMode);
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
            /* Close any open HID server.                               */
            HID_Un_Register_Server(BluetoothStackID);
            /* Clear the variable which holds the Handle of the active  */
            /* HID Connection                                           */
            HIDID = 0;

            if(HIDDeviceServerSDPHandle)
               SDP_Delete_Service_Record(BluetoothStackID, HIDDeviceServerSDPHandle);

            /* Run the main user interface.                             */
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
      for(Index=0, ret_val=String;Index<BTPS_StringLength(String);Index++)
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
            if(((*CommandFunction)(&TempCommand->Parameters)) == 0)
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
static void BD_ADDRToStr(BD_ADDR_t Board_Address, char *BoardStr)
{
   BTPS_SprintF(BoardStr, "0x%02X%02X%02X%02X%02X%02X", Board_Address.BD_ADDR5,
                                                        Board_Address.BD_ADDR4,
                                                        Board_Address.BD_ADDR3,
                                                        Board_Address.BD_ADDR2,
                                                        Board_Address.BD_ADDR1,
                                                        Board_Address.BD_ADDR0);
}

   /* The following function is responsible for redisplaying the Menu   */
   /* options to the user.  This function returns zero on successful    */
   /* execution or a negative value on all errors.                      */
static int DisplayHelp(ParameterList_t *TempParam)
{
   /* First check to if Currently in Host or Device Mode.               */
   if(UI_Mode == UI_MODE_IS_CLIENT)
   {
      /* Currently in Host Mode, display the HID Host Menu.             */
      DisplayHIDHostMenu();
   }
   else
   {
      if(UI_Mode == UI_MODE_IS_SERVER)
      {
         /* Currently in Device Mode, display the HID Device Menu.      */
         DisplayHIDDeviceMenu();
      }
      else
      {
         /* Currently in Selection Mode, display the Selection Menu.    */
         DisplaySelectionMenu();
      }
   }

   return(0);
}

   /* The following function is responsible for setting the initial     */
   /* state of the Main Application to be a HID Host.  This function    */
   /* returns zero on successful execution and a negative value on all  */
   /* errors.                                                           */
static int InitializeHIDHost(void)
{
   int ret_val;

   /* The device is now connectable and discoverable so open up a HID   */
   /* Host Server.                                                      */
   if(!HIDRegisterHostServer())
      ret_val = 0;
   else
      ret_val = FUNCTION_ERROR;

   return(ret_val);
}

   /* Displays a function error message.                                */
static void DisplayFunctionError(char *Function,int Status)
{
   Display(("Error - %s returned %d.\r\n", Function, Status));
}

   /* The following function is for displaying The FW Version by reading*/
   /* The Local version information form the FW.                        */
static void DisplayFWVersion(void)
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


   /* The following function is responsible for setting the initial     */
   /* state of the Main Application to be a HID Device.  This function  */
   /* returns zero on successful execution and a negative value on all  */
   /* errors.                                                           */
static int InitializeHIDDevice(void)
{
   int ret_val;

   /* The device is now connectable and discoverable so open up a HID   */
   /* Device Server.                                                    */
   if(!HIDRegisterDeviceServer())
      ret_val = 0;
   else
      ret_val = FUNCTION_ERROR;

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

            /* Set the Name for the initial use.                        */
            GAP_Set_Local_Device_Name (BluetoothStackID,APP_DEMO_NAME);

            /* Go ahead and allow Master/Slave Role Switch.             */
            L2CA_Link_Connect_Params.L2CA_Link_Connect_Request_Config  = cqAllowRoleSwitch;
            L2CA_Link_Connect_Params.L2CA_Link_Connect_Response_Config = csMaintainCurrentRole;

            L2CA_Set_Link_Connection_Configuration(BluetoothStackID, &L2CA_Link_Connect_Params);

            if(HCI_Command_Supported(BluetoothStackID, HCI_SUPPORTED_COMMAND_WRITE_DEFAULT_LINK_POLICY_BIT_NUMBER) > 0)
               HCI_Write_Default_Link_Policy_Settings(BluetoothStackID, HCI_LINK_POLICY_SETTINGS_ENABLE_MASTER_SLAVE_SWITCH, &Status);

            /* Delete all Stored Link Keys.                             */
            ASSIGN_BD_ADDR(BD_ADDR, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);

            DeleteLinkKey(BD_ADDR);
         }
         else
         {
            /* The Stack was NOT initialized successfully, inform the   */
            /* user and set the return value of the initialization      */
            /* function to an error.                                    */
            Display(("Stack Initialization Failed: %d.\r\n", Result));

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
      Display(("Stack Already Initialized.\n"));

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
      /* Close any open HID server.                                     */
      HID_Un_Register_Server(BluetoothStackID);

      if(HIDDeviceServerSDPHandle)
         SDP_Delete_Service_Record(BluetoothStackID, HIDDeviceServerSDPHandle);

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
         Display(("Return Value is %d GAP_Perform_Inquiry() SUCCESS.\n", ret_val));

         NumberofValidResponses = 0;
      }
      else
      {
         /* A error occurred while performing the Inquiry.              */
         Display(("Return Value is %d GAP_Perform_Inquiry() FAILURE.\n", ret_val));
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
      if(!HIDID)
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

   /* The following function is responsible for opening a HID Host      */
   /* Server.  This function returns zero on successful execution and a */
   /* negative value on all errors.                                     */
static int HIDRegisterHostServer(void)
{
   int                 Result;
   HID_Configuration_t HIDConfiguration;

   /* First check to see if a valid Bluetooth Stack ID exists.          */
   if(BluetoothStackID)
   {
      /* The Bluetooth Stack ID appears to be at least semi-valid, next */
      /* populate a HID Configuration structure.                        */
      HIDConfiguration.InMTU                           = L2CAP_MAXIMUM_SUPPORTED_STACK_MTU;

      HIDConfiguration.InFlow.MaxFlow.ServiceType      = 0x01;
      HIDConfiguration.InFlow.MaxFlow.TokenRate        = 0x00;
      HIDConfiguration.InFlow.MaxFlow.TokenBucketSize  = 0x00;
      HIDConfiguration.InFlow.MaxFlow.PeakBandwidth    = 0x00;
      HIDConfiguration.InFlow.MaxFlow.Latency          = 0xFFFFFFFF;
      HIDConfiguration.InFlow.MaxFlow.DelayVariation   = 0xFFFFFFFF;

      HIDConfiguration.OutFlow.MaxFlow.ServiceType     = 0x01;
      HIDConfiguration.OutFlow.MaxFlow.TokenRate       = 0x00;
      HIDConfiguration.OutFlow.MaxFlow.TokenBucketSize = 0x00;
      HIDConfiguration.OutFlow.MaxFlow.PeakBandwidth   = 0x00;
      HIDConfiguration.OutFlow.MaxFlow.Latency         = 0xFFFFFFFF;
      HIDConfiguration.OutFlow.MaxFlow.DelayVariation  = 0xFFFFFFFF;

      HIDConfiguration.OutFlow.MinFlow.ServiceType     = 0x01;
      HIDConfiguration.OutFlow.MinFlow.TokenRate       = 0x00;
      HIDConfiguration.OutFlow.MinFlow.TokenBucketSize = 0x00;
      HIDConfiguration.OutFlow.MinFlow.PeakBandwidth   = 0x00;
      HIDConfiguration.OutFlow.MinFlow.Latency         = 0xFFFFFFFF;
      HIDConfiguration.OutFlow.MinFlow.DelayVariation  = 0xFFFFFFFF;

      /* Now attempt to register a Bluetooth HID Host server.           */
      Result = HID_Register_Host_Server(BluetoothStackID, &HIDConfiguration, HID_Event_Callback, 0);

      if(!Result)
      {
         /* The server was successfully registered, set the Server      */
         /* Registered flag to true.                                    */
         Display(("HID_Register_Host_Server: Function Successful.\r\n"));
      }
      else
      {
         /* An error occurred while attempting to register the server.  */
         /* Display an error message indicating the error.              */
         Display(("HID_Register_Host_Server: Function Failure: %d.\r\n", Result));
      }
   }
   else
   {
      /* No valid Bluetooth Stack ID exists.                            */
      Result = INVALID_STACK_ID_ERROR;
   }

   return(Result);
}

   /* The following function is responsible for opening a HID Device    */
   /* Server and Registering the associated SDP Record.  This function  */
   /* returns zero on successful execution and a negative value on all  */
   /* errors.                                                           */
static int HIDRegisterDeviceServer(void)
{
   int                 Result;
   HID_Descriptor_t    Descriptor;
   HID_Configuration_t HIDConfiguration;

   /* First check to see if a valid Bluetooth Stack ID exists.          */
   if(BluetoothStackID)
   {
      /* No server is currently registered, populate a HID Configuration*/
      /* structure.                                                     */
      HIDConfiguration.InMTU                           = L2CAP_MAXIMUM_SUPPORTED_STACK_MTU;

      HIDConfiguration.InFlow.MaxFlow.ServiceType      = 0x01;
      HIDConfiguration.InFlow.MaxFlow.TokenRate        = 0x00;
      HIDConfiguration.InFlow.MaxFlow.TokenBucketSize  = 0x00;
      HIDConfiguration.InFlow.MaxFlow.PeakBandwidth    = 0x00;
      HIDConfiguration.InFlow.MaxFlow.Latency          = 0xFFFFFFFF;
      HIDConfiguration.InFlow.MaxFlow.DelayVariation   = 0xFFFFFFFF;

      HIDConfiguration.OutFlow.MaxFlow.ServiceType     = 0x01;
      HIDConfiguration.OutFlow.MaxFlow.TokenRate       = 0x00;
      HIDConfiguration.OutFlow.MaxFlow.TokenBucketSize = 0x00;
      HIDConfiguration.OutFlow.MaxFlow.PeakBandwidth   = 0x00;
      HIDConfiguration.OutFlow.MaxFlow.Latency         = 0xFFFFFFFF;
      HIDConfiguration.OutFlow.MaxFlow.DelayVariation  = 0xFFFFFFFF;

      HIDConfiguration.OutFlow.MinFlow.ServiceType     = 0x01;
      HIDConfiguration.OutFlow.MinFlow.TokenRate       = 0x00;
      HIDConfiguration.OutFlow.MinFlow.TokenBucketSize = 0x00;
      HIDConfiguration.OutFlow.MinFlow.PeakBandwidth   = 0x00;
      HIDConfiguration.OutFlow.MinFlow.Latency         = 0xFFFFFFFF;
      HIDConfiguration.OutFlow.MinFlow.DelayVariation  = 0xFFFFFFFF;

      /* Now attempt to register a Bluetooth HID Device server.         */
      Result = HID_Register_Device_Server(BluetoothStackID, &HIDConfiguration, HID_Event_Callback, 0);

      if(!Result)
      {
         /* The server was successfully registered, set the Server      */
         /* Registered flag to true.                                    */
         Display(("HID_Register_Device_Server: Function Successful.\r\n"));

         /* The device Server was successfully registered, now lets add */
         /* an SDP Record for this Server.  First populate an Device    */
         /* Descriptor for this HID Device.                             */
         Descriptor.DescriptorType   = 0x22;
         Descriptor.DescriptorLength = sizeof(MouseReportDescriptor);
         Descriptor.Descriptor       = MouseReportDescriptor;

         /* Now attempt to add the record.                              */
         Result = HID_Register_Device_SDP_Record(BluetoothStackID, (HID_NORMALLY_CONNECTABLE_BIT | HID_BATTERY_POWER_BIT), 0x0100, 0x0111, 0x80, 1, &Descriptor, "Stonestreet One Bluetooth Mouse", &HIDDeviceServerSDPHandle);

         /* Check the result of the above function call for success.    */
         if(!Result)
         {
            /* Display a message indicating that the SDP record for the */
            /* HID Device Server was registered successfully.           */
            Display(("HID_Register_Device_SDP_Record: Function Successful.\r\n"));
         }
         else
         {

            /* Display an Error Message and make sure the Current Server*/
            /* SDP Handle is invalid, and close the HID Host Server we  */
            /* just opened because we weren't able to register a SDP    */
            /* Record.                                                  */
            Display(("HID_Register_Device_SDP_Record: Function Failure.\r\n"));

            HIDDeviceServerSDPHandle = 0;

            /* Now try and unregister the registered server.            */
            Result = HID_Un_Register_Server(BluetoothStackID);

            /* Next check the return value of the issued command see if */
            /* it was successful.                                       */
            if(!Result)
            {
               /* Display a message indicating that the server was      */
               /* successfully unregistered.                            */
               Display(("HID_Un_Register_Server: Function Successful.\r\n"));
            }
            else
            {
               /* An error occurred while attempting to unregister the  */
               /* server.                                               */
               Display(("HID_Un_Register_Server: Function Failure: %d.\r\n", Result));
            }
         }
      }
      else
      {
         /* An error occurred while attempting to register the server.  */
         /* Display an error message indicating the error.              */
         Display(("HID_Register_Device_Server: Function Failure: %d.\r\n", Result));
      }
   }
   else
   {
      /* No valid Bluetooth Stack ID exists.                            */
      Result = INVALID_STACK_ID_ERROR;
   }

   return(Result);
}

   /* The following function is responsible for Connecting to a Remote  */
   /* HID Device.  This function returns zero on successful execution   */
   /* and a negative value on all errors.                               */
static int HIDConnectRemoteDevice(ParameterList_t *TempParam)
{
   int                 ret_val;
   char                BoardStr[16];
   BD_ADDR_t           NullADDR;
   HID_Configuration_t HIDConfiguration;

   ASSIGN_BD_ADDR(NullADDR, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);

   /* First, check that valid Bluetooth Stack ID exists.                */
   if(BluetoothStackID)
   {
      /* Now check to make sure that an on going connection doesn't     */
      /* already exist.                                                 */
      if(!HIDID)
      {
         /* There are currently no ongoing connections, now check to    */
         /* make sure that the parameters required for this function    */
         /* appear to be valid.                                         */
         if((TempParam) && (TempParam->NumberofParameters > 0) && (TempParam->Params->intParam) && (NumberofValidResponses) && (TempParam->Params->intParam<=NumberofValidResponses) && (!COMPARE_BD_ADDR(InquiryResultList[(TempParam->Params->intParam-1)], NullADDR)))
         {
            /* The above parameters are valid, inform the that the      */
            /* program is about to Attempt to connect to a remote HID   */
            /* Device.                                                  */
            BD_ADDRToStr(InquiryResultList[(TempParam->Params->intParam-1)], BoardStr);
            Display(("Open Remote HID Device(BD_ADDR = %s)\r\n", BoardStr));

            /* Populate a HID Configuration structure.                  */
            HIDConfiguration.InMTU                           = L2CAP_MAXIMUM_SUPPORTED_STACK_MTU;

            HIDConfiguration.InFlow.MaxFlow.ServiceType      = 0x01;
            HIDConfiguration.InFlow.MaxFlow.TokenRate        = 0x00;
            HIDConfiguration.InFlow.MaxFlow.TokenBucketSize  = 0x00;
            HIDConfiguration.InFlow.MaxFlow.PeakBandwidth    = 0x00;
            HIDConfiguration.InFlow.MaxFlow.Latency          = 0xFFFFFFFF;
            HIDConfiguration.InFlow.MaxFlow.DelayVariation   = 0xFFFFFFFF;

            HIDConfiguration.OutFlow.MaxFlow.ServiceType     = 0x01;
            HIDConfiguration.OutFlow.MaxFlow.TokenRate       = 0x00;
            HIDConfiguration.OutFlow.MaxFlow.TokenBucketSize = 0x00;
            HIDConfiguration.OutFlow.MaxFlow.PeakBandwidth   = 0x00;
            HIDConfiguration.OutFlow.MaxFlow.Latency         = 0xFFFFFFFF;
            HIDConfiguration.OutFlow.MaxFlow.DelayVariation  = 0xFFFFFFFF;

            HIDConfiguration.OutFlow.MinFlow.ServiceType     = 0x01;
            HIDConfiguration.OutFlow.MinFlow.TokenRate       = 0x00;
            HIDConfiguration.OutFlow.MinFlow.TokenBucketSize = 0x00;
            HIDConfiguration.OutFlow.MinFlow.PeakBandwidth   = 0x00;
            HIDConfiguration.OutFlow.MinFlow.Latency         = 0xFFFFFFFF;
            HIDConfiguration.OutFlow.MinFlow.DelayVariation  = 0xFFFFFFFF;

            ret_val = HID_Connect_Remote_Device(BluetoothStackID, InquiryResultList[(TempParam->Params->intParam-1)], &HIDConfiguration, HID_Event_Callback, 0);

            /* Now check to see if the function call was successfully   */
            /* made.                                                    */
            if(ret_val > 0)
            {
               /* The Connect Request was successfully submitted.       */
               Display(("HID_Connect_Remote_Device: Function Successful (ID = %04X).\r\n", ret_val));

               HIDID   = ret_val;
               ret_val = 0;
            }
            else
            {
               /* There was an error submitting the connection request. */
               Display(("HID_Connect_Remote_Device: Function Failure: %d.\r\n", ret_val));
            }
         }
         else
         {
            /* One or more of the necessary parameters is/are invalid.  */
            Display(("Usage: ConnectRemoteHIDDevice [Inquiry Index].\r\n"));

            ret_val = INVALID_PARAMETERS_ERROR;
         }
      }
      else
      {
         /* An Ongoing Connection already exists, this program only     */
         /* supports one connection at a time.                          */
         Display(("Ongoing connection already exists.\r\n"));

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

   /* The following function is responsible for Connecting to a Remote  */
   /* HID Host.  This function returns zero on successful execution and */
   /* a negative value on all errors.                                   */
static int HIDConnectRemoteHost(ParameterList_t *TempParam)
{
   int                 ret_val;
   char                BoardStr[16];
   BD_ADDR_t           NullADDR;
   HID_Configuration_t HIDConfiguration;

   ASSIGN_BD_ADDR(NullADDR, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);

   /* First, check that valid Bluetooth Stack ID exists.                */
   if(BluetoothStackID)
   {
      /* Now check to make sure that an on going connection doesn't     */
      /* already exist.                                                 */
      if(!HIDID)
      {
         /* There are currently no ongoing connections, now check to    */
         /* make sure that the parameters required for this function    */
         /* appear to be valid.                                         */
         if((TempParam) && (TempParam->NumberofParameters > 0) && (TempParam->Params->intParam) && (NumberofValidResponses) && (TempParam->Params->intParam<=NumberofValidResponses) && (!COMPARE_BD_ADDR(InquiryResultList[(TempParam->Params->intParam-1)], NullADDR)))
         {
            /* The above parameters are valid, inform the that the      */
            /* program is about to Attempt to connect to a remote HID   */
            /* Host.                                                    */
            BD_ADDRToStr(InquiryResultList[(TempParam->Params->intParam-1)], BoardStr);
            Display(("Open Remote HID Host(BD_ADDR = %s)\r\n", BoardStr));

            /* Populate a HID Configuration structure.                  */
            HIDConfiguration.InMTU                           = L2CAP_MAXIMUM_SUPPORTED_STACK_MTU;

            HIDConfiguration.InFlow.MaxFlow.ServiceType      = 0x01;
            HIDConfiguration.InFlow.MaxFlow.TokenRate        = 0x00;
            HIDConfiguration.InFlow.MaxFlow.TokenBucketSize  = 0x00;
            HIDConfiguration.InFlow.MaxFlow.PeakBandwidth    = 0x00;
            HIDConfiguration.InFlow.MaxFlow.Latency          = 0xFFFFFFFF;
            HIDConfiguration.InFlow.MaxFlow.DelayVariation   = 0xFFFFFFFF;

            HIDConfiguration.OutFlow.MaxFlow.ServiceType     = 0x01;
            HIDConfiguration.OutFlow.MaxFlow.TokenRate       = 0x00;
            HIDConfiguration.OutFlow.MaxFlow.TokenBucketSize = 0x00;
            HIDConfiguration.OutFlow.MaxFlow.PeakBandwidth   = 0x00;
            HIDConfiguration.OutFlow.MaxFlow.Latency         = 0xFFFFFFFF;
            HIDConfiguration.OutFlow.MaxFlow.DelayVariation  = 0xFFFFFFFF;

            HIDConfiguration.OutFlow.MinFlow.ServiceType     = 0x01;
            HIDConfiguration.OutFlow.MinFlow.TokenRate       = 0x00;
            HIDConfiguration.OutFlow.MinFlow.TokenBucketSize = 0x00;
            HIDConfiguration.OutFlow.MinFlow.PeakBandwidth   = 0x00;
            HIDConfiguration.OutFlow.MinFlow.Latency         = 0xFFFFFFFF;
            HIDConfiguration.OutFlow.MinFlow.DelayVariation  = 0xFFFFFFFF;

            ret_val = HID_Connect_Remote_Host(BluetoothStackID, InquiryResultList[(TempParam->Params->intParam-1)], &HIDConfiguration, HID_Event_Callback, 0);

            /* Now check to see if the function call was successfully   */
            /* made.                                                    */
            if(ret_val > 0)
            {
               /* The Connect Request was successfully submitted.       */
               Display(("HID_Connect_Remote_Host: Function Successful (ID = %04X).\r\n", ret_val));

               HIDID   = ret_val;
               ret_val = 0;
            }
            else
            {
               /* There was an error submitting the connection request. */
               Display(("HID_Connect_Remote_Host: Function Failure: %d.\r\n", ret_val));
            }
         }
         else
         {
            /* One or more of the necessary parameters is/are invalid.  */
            Display(("Usage: ConnectRemoteHIDHost [Inquiry Index].\r\n"));

            ret_val = INVALID_PARAMETERS_ERROR;
         }
      }
      else
      {
         /* An Ongoing Connection already exists, this program only     */
         /* supports one connection at a time.                          */
         Display(("Ongoing connection already exists.\r\n"));

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

   /* The following function is responsible for closing any ongoing     */
   /* connection.  This function returns zero on successful execution   */
   /* and a negative value on all errors.                               */
static int HIDCloseConnection(ParameterList_t *TempParam)
{
   int  ret_val;

   /* First, check that valid Bluetooth Stack ID exists.                */
   if(BluetoothStackID)
   {
      /* Check to see if the Current HIDID appears to be semi-valid.    */
      /* This parameter will only be valid if a Client Connection       */
      /* Exists.                                                        */
      if(HIDID)
      {
         /* The HIDID appears to be semi-valid.  Now try to close the   */
         /* Connection.                                                 */
         ret_val = HID_Close_Connection(BluetoothStackID, HIDID);

         if(!ret_val)
         {
            /* The function was called successfully.  Display a message */
            /* indicating that the Client was successfully closed.      */
            Display(("HID_Close_Connection: Function Successful.\r\n"));

            HIDID = 0;
         }
         else
         {
            /* An error occurred while attempting to close the          */
            /* Connection.                                              */
            Display(("HID_Close_Connection: Function Failure: %d.\r\n", ret_val));
         }
      }
      else
      {
         /* The HID ID is invalid.                                      */
         Display(("Close HID Connection: Invalid HIDID.\r\n"));

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

   /* The following function is responsible for sending a HID_CONTROL   */
   /* Transaction to the remote entity.  This function returns zero on  */
   /* successful execution and a negative value on all errors.          */
static int HIDControlRequest(ParameterList_t *TempParam)
{
   int ret_val;

   /* First, check that valid Bluetooth Stack ID exists.                */
   if(BluetoothStackID)
   {
      /* Check to see if the Current HIDID appears to be semi-valid.    */
      /* This parameter will only be valid if a Client Connection       */
      /* Exists.                                                        */
      if(HIDID)
      {
         /* The HIDID appears to be at least semi-valid, now check to   */
         /* make sure that the parameters required for this function    */
         /* appear to be valid.                                         */
         if((UI_Mode == UI_MODE_IS_SERVER) || ((UI_Mode == UI_MODE_IS_CLIENT) && (TempParam) && (TempParam->NumberofParameters > 0) && (TempParam->Params->intParam >= hcNop) && (TempParam->Params->intParam <= hcVirtualCableUnplug)))
         {
            /* The parameter appears to be at least semi-valid now      */
            /* attempt to submit the HID Control Request.               */
            ret_val = HID_Control_Request(BluetoothStackID, HIDID, (HID_Control_Operation_Type_t)((UI_Mode == UI_MODE_IS_SERVER)?(hcVirtualCableUnplug):(TempParam->Params->intParam)));

            /* Check to see if the command was successfully submitted.  */
            if(!ret_val)
            {
               /* The command was successfully submitted.  Display a    */
               /* message indicating that HID Control Request was       */
               /* successfully submitted.                               */
               Display(("HID_Control_Request: Function Successful.\r\n"));
            }
            else
            {
               /* An error occurred while attempting to submit the HID  */
               /* Control Request command.                              */
               Display(("HID_Control_Request: Function Failure: %d.\r\n", ret_val));
            }
         }
         else
         {
            /* The required parameter is invalid.                       */
            if(UI_Mode == UI_MODE_IS_CLIENT)
               Display(("Usage: ControlRequest [Control Operation].\r\n"));
            else
               Display(("Usage: ControlRequest.\r\n"));

            ret_val = INVALID_PARAMETERS_ERROR;
         }
      }
      else
      {
         /* The HID ID is invalid.                                      */
         Display(("HID Control Request: Invalid HIDID.\r\n"));
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

   /* The following function is responsible for sending a GET_REPORT    */
   /* Transaction to the remote HID Device.  This function returns zero */
   /* on successful execution and a negative value on all errors.       */
static int HIDGetReportRequest(ParameterList_t *TempParam)
{
   int ret_val;

   /* First, check that valid Bluetooth Stack ID exists.                */
   if(BluetoothStackID)
   {
      /* Check to see if the Current HIDID appears to be semi-valid.    */
      /* This parameter will only be valid if a Client Connection       */
      /* Exists.                                                        */
      if(HIDID)
      {
         /* The HIDID appears to be at least semi-valid, now check to   */
         /* make sure that the parameters required for this function    */
         /* appear to be valid.                                         */
         if((TempParam) && (TempParam->NumberofParameters > 2) && (TempParam->Params[0].intParam >= grSizeOfReport) && (TempParam->Params[0].intParam <= grUseBufferSize) && (TempParam->Params[1].intParam >= rtInput) && (TempParam->Params[1].intParam <= rtFeature))
         {
            /* The parameters appear to be at least semi-valid, now     */
            /* attempt to submit the HID Get Report request.            */
            ret_val = HID_Get_Report_Request(BluetoothStackID, HIDID, (HID_Get_Report_Size_Type_t)TempParam->Params[0].intParam, (HID_Report_Type_Type_t)TempParam->Params[1].intParam, (Byte_t)(TempParam->Params[2].intParam), (Word_t)(TempParam->Params[3].intParam));

            /* Check to see if the command was successfully submitted.  */
            if(!ret_val)
            {
               /* The command was successfully submitted.  Display a    */
               /* message indicating that HID Get Report Request was    */
               /* successfully submitted.                               */
               Display(("HID_Get_Report_Request: Function Successful.\r\n"));
            }
            else
            {
               /* An error occurred while attempting to submit the HID  */
               /* Get Report Request command.                           */
               Display(("HID_Get_Report_Request: Function Failure: %d.\r\n", ret_val));
            }
         }
         else
         {
            /* The required parameter is invalid.                       */
            Display(("Usage: GetReportRequest [Size (0 = grSizeOfReport, 1 = grUseBufferSize)] [ReportType (0 = rtOther, 1 = rtInput, 2 = rtOutput, 3 = rtFeature)] [ReportID] [BufferSize].\r\n"));
            ret_val = INVALID_PARAMETERS_ERROR;
         }
      }
      else
      {
         /* The HID ID is invalid.                                      */
         Display(("HID Get Report Request: Invalid HIDID.\r\n"));
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

   /* The following function is responsible for sending a response for  */
   /* an outstanding GET_REPORT Transaction to the remote HID Host.     */
   /* This function returns zero on successful execution and a negative */
   /* value on all errors.                                              */
static int HIDGetReportResponse(ParameterList_t *TempParam)
{
   int ret_val;

   /* First, check that valid Bluetooth Stack ID exists.                */
   if(BluetoothStackID)
   {
      /* Check to see if the Current HIDID appears to be semi-valid.    */
      /* This parameter will only be valid if a Client Connection       */
      /* Exists.                                                        */
      if(HIDID)
      {
         /* The HIDID appears to be at least semi-valid, now check to   */
         /* make sure that the parameters required for this function    */
         /* appear to be valid.                                         */
         if((TempParam) && (TempParam->NumberofParameters > 1) && (TempParam->Params[0].intParam >= rtSuccessful) && (TempParam->Params[0].intParam <= rtData) && (TempParam->Params[1].intParam >= rtInput) && (TempParam->Params[1].intParam <= rtFeature))
         {
            /* The parameters appear to be at least semi-valid, now     */
            /* attempt to submit the HID Get Report response.           */
            ret_val = HID_Get_Report_Response(BluetoothStackID, HIDID, (HID_Result_Type_t)TempParam->Params[0].intParam, (HID_Report_Type_Type_t)TempParam->Params[1].intParam, sizeof(GenericMouseReport), GenericMouseReport);

            /* Check to see if the command was successfully submitted.  */
            if(!ret_val)
            {
               /* The command was successfully submitted.  Display a    */
               /* message indicating that HID Get Report Response was   */
               /* successfully submitted.                               */
               Display(("HID_Get_Report_Response: Function Successful.\r\n"));
            }
            else
            {
               /* An error occurred while attempting to submit the HID  */
               /* Get Report Response command.                          */
               Display(("HID_Get_Report_Response: Function Failure: %d.\r\n", ret_val));
            }
         }
         else
         {
            /* The required parameter is invalid.                       */
            Display(("Usage: GetReportResponse [ResultType] [ReportType].\r\n"));
            ret_val = INVALID_PARAMETERS_ERROR;
         }
      }
      else
      {
         /* The HID ID is invalid.                                      */
         Display(("HID Get Report Response: Invalid HIDID.\r\n"));
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

   /* The following function is responsible for sending a SET_REPORT    */
   /* Transaction to the remote HID Device.  This function returns zero */
   /* on successful execution and a negative value on all errors.       */
static int HIDSetReportRequest(ParameterList_t *TempParam)
{
   int ret_val;

   /* First, check that valid Bluetooth Stack ID exists.                */
   if(BluetoothStackID)
   {
      /* Check to see if the Current HIDID appears to be semi-valid.    */
      /* This parameter will only be valid if a Client Connection       */
      /* Exists.                                                        */
      if(HIDID)
      {
         /* The HIDID appears to be at least semi-valid, now check to   */
         /* make sure that the parameters required for this function    */
         /* appear to be valid.                                         */
         if((TempParam) && (TempParam->NumberofParameters > 0) && (TempParam->Params[0].intParam >= rtInput) && (TempParam->Params[0].intParam <= rtFeature))
         {
            /* The parameters appear to be at least semi-valid, now     */
            /* attempt to submit the HID Set Report request.            */
            ret_val = HID_Set_Report_Request(BluetoothStackID, HIDID, (HID_Report_Type_Type_t)TempParam->Params[0].intParam, sizeof(GenericMouseReport), GenericMouseReport);

            /* Check to see if the command was successfully submitted.  */
            if(!ret_val)
            {
               /* The command was successfully submitted.  Display a    */
               /* message indicating that HID Set Report Request was    */
               /* successfully submitted.                               */
               Display(("HID_Set_Report_Request: Function Successful.\r\n"));
            }
            else
            {
               /* An error occurred while attempting to submit the HID  */
               /* Set Report Request command.                           */
               Display(("HID_Set_Report_Request: Function Failure: %d.\r\n", ret_val));
            }
         }
         else
         {
            /* The required parameter is invalid.                       */
            Display(("Usage: SetReportRequest [ReportType].\r\n"));

            ret_val = INVALID_PARAMETERS_ERROR;
         }
      }
      else
      {
         /* The HID ID is invalid.                                      */
         Display(("HID Set Report Request: Invalid HIDID.\r\n"));

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

   /* The following function is responsible for sending a response for  */
   /* an outstanding SET_REPORT Transaction to the remote HID Host.     */
   /* This function returns zero on successful execution and a negative */
   /* value on all errors.                                              */
static int HIDSetReportResponse(ParameterList_t *TempParam)
{
   int ret_val;

   /* First, check that valid Bluetooth Stack ID exists.                */
   if(BluetoothStackID)
   {
      /* Check to see if the Current HIDID appears to be semi-valid.    */
      /* This parameter will only be valid if a Client Connection       */
      /* Exists.                                                        */
      if(HIDID)
      {
         /* The HIDID appears to be at least semi-valid, now check to   */
         /* make sure that the parameters required for this function    */
         /* appear to be valid.                                         */
         if((TempParam) && (TempParam->NumberofParameters > 0) && (TempParam->Params[0].intParam >= rtSuccessful) && (TempParam->Params[0].intParam <= rtData))
         {
            /* The parameters appear to be at least semi-valid, now     */
            /* attempt to submit the HID Set Report response.           */
            ret_val = HID_Set_Report_Response(BluetoothStackID, HIDID, (HID_Result_Type_t)TempParam->Params[0].intParam);

            /* Check to see if the command was successfully submitted.  */
            if(!ret_val)
            {
               /* The command was successfully submitted.  Display a    */
               /* message indicating that HID Set Report Response was   */
               /* successfully submitted.                               */
               Display(("HID_Set_Report_Response: Function Successful.\r\n"));
            }
            else
            {
               /* An error occurred while attempting to submit the HID  */
               /* Set Report Response command.                          */
               Display(("HID_Set_Report_Response: Function Failure: %d.\r\n", ret_val));
            }
         }
         else
         {
            /* The required parameter is invalid.                       */
            Display(("Usage: SetReportResponse [ResultType].\r\n"));

            ret_val = INVALID_PARAMETERS_ERROR;
         }
      }
      else
      {
         /* The HID ID is invalid.                                      */
         Display(("HID Set Report Response: Invalid HIDID.\r\n"));

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

   /* The following function is responsible for sending a GET_PROTOCOL  */
   /* Transaction to the remote HID Device.  This function returns zero */
   /* on successful execution and a negative value on all errors.       */
static int HIDGetProtocolRequest(ParameterList_t *TempParam)
{
   int ret_val;

   /* First, check that valid Bluetooth Stack ID exists.                */
   if(BluetoothStackID)
   {
      /* Check to see if the Current HIDID appears to be semi-valid.    */
      /* This parameter will only be valid if a Client Connection       */
      /* Exists.                                                        */
      if(HIDID)
      {
         /* The HIDID appears to be at least semi-valid, now attempt to */
         /* submit the HID Get Protocol request.                        */
         ret_val = HID_Get_Protocol_Request(BluetoothStackID, HIDID);

         /* Check to see if the command was successfully submitted.     */
         if(!ret_val)
         {
            /* The command was successfully submitted.  Display a       */
            /* message indicating that HID Get Protocol Request was     */
            /* successfully submitted.                                  */
            Display(("HID_Get_Protocol_Request: Function Successful.\r\n"));
         }
         else
         {
            /* An error occurred while attempting to submit the HID Get */
            /* Protocol Request command.                                */
            Display(("HID_Get_Protocol_Request: Function Failure: %d.\r\n", ret_val));
         }
      }
      else
      {
         /* The HID ID is invalid.                                      */
         Display(("HID Get Protocol Request: Invalid HIDID.\r\n"));

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

   /* The following function is responsible for sending a response for  */
   /* an outstanding GET_PROTOCOL Transaction to the remote HID Host.   */
   /* This function returns zero on successful execution and a negative */
   /* value on all errors.                                              */
static int HIDGetProtocolResponse(ParameterList_t *TempParam)
{
   int ret_val;

   /* First, check that valid Bluetooth Stack ID exists.                */
   if(BluetoothStackID)
   {
      /* Check to see if the Current HIDID appears to be semi-valid.    */
      /* This parameter will only be valid if a Client Connection       */
      /* Exists.                                                        */
      if(HIDID)
      {
         /* The HIDID appears to be at least semi-valid, now check to   */
         /* make sure that the parameters required for this function    */
         /* appear to be valid.                                         */
         if((TempParam) && (TempParam->NumberofParameters > 1) && (TempParam->Params[0].intParam >= rtSuccessful) && (TempParam->Params[0].intParam <= rtData) && (TempParam->Params[1].intParam >= ptBoot) && (TempParam->Params[1].intParam <= ptReport))
         {
            /* The parameters appears to be at least semi-valid, now    */
            /* attempt to submit the HID Get Protocol Response.         */
            ret_val = HID_Get_Protocol_Response(BluetoothStackID, HIDID, (HID_Result_Type_t)TempParam->Params[0].intParam, (HID_Protocol_Type_t)TempParam->Params[1].intParam);

            /* Check to see if the command was successfully submitted.  */
            if(!ret_val)
            {
               /* The command was successfully submitted.  Display a    */
               /* message indicating that HID Get Protocol Response was */
               /* successfully submitted.                               */
               Display(("HID_Get_Protocol_Response: Function Successful.\r\n"));
            }
            else
            {
               /* An error occurred while attempting to submit the HID  */
               /* Get Protocol Response command.                        */
               Display(("HID_Get_Protocol_Response: Function Failure: %d.\r\n", ret_val));
            }
         }
         else
         {
            /* The required parameter is invalid.                       */
            Display(("Usage: GetProtocolResponse [ResultType] [Protocol].\r\n"));

            ret_val = INVALID_PARAMETERS_ERROR;
         }
      }
      else
      {
         /* The HID ID is invalid.                                      */
         Display(("HID Get Protocol Response: Invalid HIDID.\r\n"));
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

   /* The following function is responsible for sending a SET_PROTOCOL  */
   /* Transaction to the remote HID Device.  This function returns zero */
   /* on successful execution and a negative value on all errors.       */
static int HIDSetProtocolRequest(ParameterList_t *TempParam)
{
   int ret_val;

   /* First, check that valid Bluetooth Stack ID exists.                */
   if(BluetoothStackID)
   {
      /* Check to see if the Current HIDID appears to be semi-valid.    */
      /* This parameter will only be valid if a Client Connection       */
      /* Exists.                                                        */
      if(HIDID)
      {
         /* The HIDID appears to be at least semi-valid, now check to   */
         /* make sure that the parameters required for this function    */
         /* appear to be valid.                                         */
         if((TempParam) && (TempParam->NumberofParameters > 0) && (TempParam->Params[0].intParam >= ptBoot) && (TempParam->Params[0].intParam <= ptReport))
         {
            /* The parameter appears to be at least semi-valid, now     */
            /* attempt to submit the HID Set Protocol request.          */
            ret_val = HID_Set_Protocol_Request(BluetoothStackID, HIDID, (HID_Protocol_Type_t)TempParam->Params[0].intParam);

            /* Check to see if the command was successfully submitted.  */
            if(!ret_val)
            {
               /* The command was successfully submitted.  Display a    */
               /* message indicating that HID Set Protocol Request was  */
               /* successfully submitted.                               */
               Display(("HID_Set_Protocol_Request: Function Successful.\r\n"));
            }
            else
            {
               /* An error occurred while attempting to submit the HID  */
               /* Set Protocol Request command.                         */
               Display(("HID_Set_Protocol_Request: Function Failure: %d.\r\n", ret_val));
            }
         }
         else
         {
            /* The required parameter is invalid.                       */
            Display(("Usage: SetProtocolRequest [Protocol].\r\n"));

            ret_val = INVALID_PARAMETERS_ERROR;
         }
      }
      else
      {
         /* The HID ID is invalid.                                      */
         Display(("HID Set Protocol Request: Invalid HIDID.\r\n"));
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

   /* The following function is responsible for sending a response for  */
   /* an outstanding SET_PROTOCOL Transaction to the remote HID Host.   */
   /* This function returns zero on successful execution and a negative */
   /* value on all errors.                                              */
static int HIDSetProtocolResponse(ParameterList_t *TempParam)
{
   int ret_val;

   /* First, check that valid Bluetooth Stack ID exists.                */
   if(BluetoothStackID)
   {
      /* Check to see if the Current HIDID appears to be semi-valid.    */
      /* This parameter will only be valid if a Client Connection       */
      /* Exists.                                                        */
      if(HIDID)
      {
         /* The HIDID appears to be at least semi-valid, now check to   */
         /* make sure that the parameters required for this function    */
         /* appear to be valid.                                         */
         if((TempParam) && (TempParam->NumberofParameters > 0) && (TempParam->Params[0].intParam >= rtSuccessful) && (TempParam->Params[0].intParam <= rtData))
         {
            /* The parameter appears to be at least semi-valid, now     */
            /* attempt to submit the HID Set Protocol response.         */
            ret_val = HID_Set_Protocol_Response(BluetoothStackID, HIDID, (HID_Result_Type_t)TempParam->Params[0].intParam);

            /* Check to see if the command was successfully submitted.  */
            if(!ret_val)
            {
               /* The command was successfully submitted.  Display a    */
               /* message indicating that HID Set Protocol Response was */
               /* successfully submitted.                               */
               Display(("HID_Set_Protocol_Response: Function Successful.\r\n"));
            }
            else
            {
               /* An error occurred while attempting to submit the HID  */
               /* Set Protocol Response command.                        */
               Display(("HID_Set_Protocol_Response: Function Failure: %d.\r\n", ret_val));
            }
         }
         else
         {
            /* The required parameter is invalid.                       */
            Display(("Usage: SetProtocolResponse [ResultType].\r\n"));
            ret_val = INVALID_PARAMETERS_ERROR;
         }
      }
      else
      {
         /* The HID ID is invalid.                                      */
         Display(("HID Set Protocol Response: Invalid HIDID.\r\n"));
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

   /* The following function is responsible for sending a GET_IDLE      */
   /* Transaction to the remote HID Device.  This function returns zero */
   /* on successful execution and a negative value on all errors.       */
static int HIDGetIdleRequest(ParameterList_t *TempParam)
{
   int ret_val;

   /* First, check that valid Bluetooth Stack ID exists.                */
   if(BluetoothStackID)
   {
      /* Check to see if the Current HIDID appears to be semi-valid.    */
      /* This parameter will only be valid if a Client Connection       */
      /* Exists.                                                        */
      if(HIDID)
      {
         /* The HIDID appears to be at least semi-valid, now attempt to */
         /* submit the HID Get Idle request.                            */
         ret_val = HID_Get_Idle_Request(BluetoothStackID, HIDID);

         /* Check to see if the command was successfully submitted.     */
         if(!ret_val)
         {
            /* The command was successfully submitted.  Display a       */
            /* message indicating that HID Get Idle Request was         */
            /* successfully submitted.                                  */
            Display(("HID_Get_Idle_Request: Function Successful.\r\n"));
         }
         else
         {
            /* An error occurred while attempting to submit the HID Get */
            /* Idle Request command.                                    */
            Display(("HID_Get_Idle_Request: Function Failure: %d.\r\n", ret_val));
         }
      }
      else
      {
         /* The HID ID is invalid.                                      */
         Display(("HID Get Idle Request: Invalid HIDID.\r\n"));
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

   /* The following function is responsible for sending a response for  */
   /* an outstanding GET_IDLE Transaction to the remote HID Host.  This */
   /* function returns zero on successful execution and a negative value*/
   /* on all errors.                                                    */
static int HIDGetIdleResponse(ParameterList_t *TempParam)
{
   int ret_val;

   /* First, check that valid Bluetooth Stack ID exists.                */
   if(BluetoothStackID)
   {
      /* Check to see if the Current HIDID appears to be semi-valid.    */
      /* This parameter will only be valid if a Client Connection       */
      /* Exists.                                                        */
      if(HIDID)
      {
         /* The HIDID appears to be at least semi-valid, now check to   */
         /* make sure that the parameters required for this function    */
         /* appear to be valid.                                         */
         if((TempParam) && (TempParam->NumberofParameters > 1) && (TempParam->Params[0].intParam >= rtSuccessful) && (TempParam->Params[0].intParam <= rtData))
         {
            /* The parameters appear to be at least semi-valid, now     */
            /* attempt to submit the HID Get Idle Response.             */
            ret_val = HID_Get_Idle_Response(BluetoothStackID, HIDID, (HID_Result_Type_t)TempParam->Params[0].intParam, (Byte_t)TempParam->Params[1].intParam);

            /* Check to see if the command was successfully submitted.  */
            if(!ret_val)
            {
               /* The command was successfully submitted.  Display a    */
               /* message indicating that HID Get Idle Response was     */
               /* successfully submitted.                               */
               Display(("HID_Get_Idle_Response: Function Successful.\r\n"));
            }
            else
            {
               /* An error occurred while attempting to submit the HID  */
               /* Get Idle Response command.                            */
               Display(("HID_Get_Idle_Response: Function Failure: %d.\r\n", ret_val));
            }
         }
         else
         {
            /* The required parameter is invalid.                       */
            Display(("Usage: GetIdleResponse [ResultType] [IdleRate].\r\n"));

            ret_val = INVALID_PARAMETERS_ERROR;
         }
      }
      else
      {
         /* The HID ID is invalid.                                      */
         Display(("HID Get Idle Response: Invalid HIDID.\r\n"));
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

   /* The following function is responsible for sending a SET_IDLE      */
   /* Transaction to the remote HID Device.  This function returns zero */
   /* on successful execution and a negative value on all errors.       */
static int HIDSetIdleRequest(ParameterList_t *TempParam)
{
   int ret_val;

   /* First, check that valid Bluetooth Stack ID exists.                */
   if(BluetoothStackID)
   {
      /* Check to see if the Current HIDID appears to be semi-valid.    */
      /* This parameter will only be valid if a Client Connection       */
      /* Exists.                                                        */
      if(HIDID)
      {
         /* The HIDID appears to be at least semi-valid, now check to   */
         /* make sure that the parameters required for this function    */
         /* appear to be valid.                                         */
         if((TempParam) && (TempParam->NumberofParameters > 0))
         {
            /* The parameter appears to be at least semi-valid, now     */
            /* attempt to submit the HID Set Idle request.              */
            ret_val = HID_Set_Idle_Request(BluetoothStackID, HIDID, (Byte_t)TempParam->Params[0].intParam);

            /* Check to see if the command was successfully submitted.  */
            if(!ret_val)
            {
               /* The command was successfully submitted.  Display a    */
               /* message indicating that HID Set Idle Request was      */
               /* successfully submitted.                               */
               Display(("HID_Set_Idle_Request: Function Successful.\r\n"));
            }
            else
            {
               /* An error occurred while attempting to submit the HID  */
               /* Set Idle Request command.                             */
               Display(("HID_Set_Idle_Request: Function Failure: %d.\r\n", ret_val));
            }
         }
         else
         {
            /* The required parameter is invalid.                       */
            Display(("Usage: SetIdleRequest [IdleRate].\r\n"));
            ret_val = INVALID_PARAMETERS_ERROR;
         }
      }
      else
      {
         /* The HID ID is invalid.                                      */
         Display(("HID Set Idle Request: Invalid HIDID.\r\n"));
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

   /* The following function is responsible for sending a response for  */
   /* an outstanding SET_IDLE Transaction to the remote HID Host.  This */
   /* function returns zero on successful execution and a negative value*/
   /* on all errors.                                                    */
static int HIDSetIdleResponse(ParameterList_t *TempParam)
{
   int ret_val;

   /* First, check that valid Bluetooth Stack ID exists.                */
   if(BluetoothStackID)
   {
      /* Check to see if the Current HIDID appears to be semi-valid.    */
      /* This parameter will only be valid if a Client Connection       */
      /* Exists.                                                        */
      if(HIDID)
      {
         /* The HIDID appears to be at least semi-valid, now check to   */
         /* make sure that the parameters required for this function    */
         /* appear to be valid.                                         */
         if((TempParam) && (TempParam->NumberofParameters > 0) && (TempParam->Params[0].intParam >= rtSuccessful) && (TempParam->Params[0].intParam <= rtData))
         {
            /* The parameter appears to be at least semi-valid, now     */
            /* attempt to submit the HID Set Idle Response.             */
            ret_val = HID_Set_Idle_Response(BluetoothStackID, HIDID, (HID_Result_Type_t)TempParam->Params[0].intParam);

            /* Check to see if the command was successfully submitted.  */
            if(!ret_val)
            {
               /* The command was successfully submitted.  Display a    */
               /* message indicating that HID Set Idle Response was     */
               /* successfully submitted.                               */
               Display(("HID_Set_Idle_Response: Function Successful.\r\n"));
            }
            else
            {
               /* An error occurred while attempting to submit the HID  */
               /* Set Idle Response command.                            */
               Display(("HID_Set_Idle_Response: Function Failure: %d.\r\n", ret_val));
            }
         }
         else
         {
            /* The required parameter is invalid.                       */
            Display(("Usage: SetIdleResponse [ResultType].\r\n"));

            ret_val = INVALID_PARAMETERS_ERROR;
         }
      }
      else
      {
         /* The HID ID is invalid.                                      */
         Display(("HID Set Idle Response: Invalid HIDID.\r\n"));

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

   /* The following function is responsible for sending a DATA          */
   /* Transaction on the Interrupt Channel to the remote entity.  This  */
   /* function returns zero on successful execution and a negative value*/
   /* on all errors.                                                    */
static int HIDDataWrite(ParameterList_t *TempParam)
{
   int ret_val;

   /* First, check that valid Bluetooth Stack ID exists.                */
   if(BluetoothStackID)
   {
      /* Check to see if the Current HIDID appears to be semi-valid.    */
      /* This parameter will only be valid if a Client Connection       */
      /* Exists.                                                        */
      if(HIDID)
      {
         /* The HIDID appears to be at least semi-valid, now check to   */
         /* make sure that the parameters required for this function    */
         /* appear to be valid.                                         */
         if((TempParam) && (TempParam->NumberofParameters > 0) && (TempParam->Params[0].intParam >= rtInput) && (TempParam->Params[0].intParam <= rtFeature))
         {
            /* The parameter appears to be at least semi-valid, now     */
            /* attempt to submit the HID Data Write.                    */
            ret_val = HID_Data_Write(BluetoothStackID, HIDID, (HID_Report_Type_Type_t)TempParam->Params[0].intParam, sizeof(GenericMouseReport), GenericMouseReport);

            /* Check to see if the command was successfully submitted.  */
            if(!ret_val)
            {
               /* The command was successfully submitted.  Display a    */
               /* message indicating that HID Data Write was            */
               /* successfully submitted.                               */
               Display(("HID_Data_Write: Function Successful.\r\n"));
            }
            else
            {
               /* An error occurred while attempting to submit the HID  */
               /* Data Write command.                                   */
               Display(("HID_Data_Write: Function Failure: %d.\r\n", ret_val));
            }
         }
         else
         {
            /* The required parameter is invalid.                       */
            Display(("Usage: DataWrite [ReportType].\r\n"));

            ret_val = INVALID_PARAMETERS_ERROR;
         }
      }
      else
      {
         /* The HID ID is invalid.                                      */
         Display(("HID Set Idle Response: Invalid HIDID.\r\n"));

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

   /* The following function is responsible for changing the User       */
   /* Interface Mode to HID Host.                                       */
static int HIDHostMode(ParameterList_t *TempParam)
{
   UI_Mode = UI_MODE_IS_CLIENT;

   if(!InitializeHIDHost())
      UserInterface_Main();
   else
      CloseStack();

   return(0);
}

   /* The following function is responsible for changing the User       */
   /* Interface Mode to HID Device.                                     */
static int HIDDeviceMode(ParameterList_t *TempParam)
{
   UI_Mode = UI_MODE_IS_SERVER;

   if(!InitializeHIDDevice())
      UserInterface_Main();
   else
      CloseStack();

   return(0);
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

                  RemoteIOCapability = (GAP_IO_Capability_t)GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Authentication_Event_Data.IO_Capabilities.IO_Capability;
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

   /* The following function is for an HID Event Callback.  This        */
   /* function will be called whenever a HID Event occurs that is       */
   /* associated with the Bluetooth Stack.  This function passes to the */
   /* caller the HID Event Data that occurred and the HID Event Callback*/
   /* Parameter that was specified when this Callback was installed.    */
   /* The caller is free to use the contents of the HID Event Data ONLY */
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
   /* another HID Event will not be processed while this function call  */
   /* is outstanding).                                                  */
   /* * NOTE * This function MUST NOT Block and wait for events that    */
   /*          can only be satisfied by Receiving HID Event Packets.  A */
   /*          Deadlock WILL occur because NO HID Event Callbacks will  */
   /*          be issued while this function is currently outstanding.  */
static void BTPSAPI HID_Event_Callback(unsigned int BluetoothStackID, HID_Event_Data_t *HIDEventData, unsigned long CallbackParameter)
{
   char         BoardStr[16];
   unsigned int Index;

   /* First, check to see if the required parameters appear to be       */
   /* semi-valid.                                                       */
   if((BluetoothStackID) && (HIDEventData != NULL))
   {
      Display(("\r\n"));

      /* The parameters appear to be semi-valid, now check to see what  */
      /* type the incoming event is.                                    */
      switch(HIDEventData->Event_Data_Type)
      {
         case etHID_Open_Indication:
            /* Check to see if a connection currently exists.           */
            if(!HIDID)
            {
               /* There are currently no ongoing connections, display   */
               /* the BD_ADDR and relevant information of the connecting*/
               /* device.                                               */
               BD_ADDRToStr(HIDEventData->Event_Data.HID_Open_Indication_Data->BD_ADDR, BoardStr);
               Display(("HID Open Indication, ID: 0x%04X, Board: %s\r\n", HIDEventData->Event_Data.HID_Open_Indication_Data->HIDID, BoardStr));

               HIDID = HIDEventData->Event_Data.HID_Open_Indication_Data->HIDID;
            }
            else
            {
               /* There is currently an ongoing connection, since this  */
               /* application can only support one connection at a time */
               /* close the new connection.                             */
               HID_Close_Connection(BluetoothStackID, HIDEventData->Event_Data.HID_Open_Indication_Data->HIDID);
            }
            break;
         case etHID_Open_Confirmation:
            /* There are currently no ongoing connections, the Client   */
            /* received a Connect Confirmation, display relevant        */
            /* information.                                             */
            Display(("HID Open Confirmation, ID: 0x%04X, Status: 0x%04X\r\n", HIDEventData->Event_Data.HID_Open_Confirmation_Data->HIDID,
                                                                            HIDEventData->Event_Data.HID_Open_Confirmation_Data->OpenStatus));

            if(HIDEventData->Event_Data.HID_Open_Confirmation_Data->OpenStatus != HID_OPEN_PORT_STATUS_SUCCESS)
               HIDID = 0;
            break;
         case etHID_Close_Indication:
            /* A Close Indication was received, display all relevant    */
            /* information.                                             */
            Display(("HID Close Indication, ID: 0x%04X\r\n", HIDEventData->Event_Data.HID_Close_Indication_Data->HIDID));
            HIDID = 0;
            break;
         case etHID_Control_Indication:
            /* A HID Control Indication was received, display all       */
            /* relevant information.                                    */
            Display(("HID Control Indication, ID: 0x%04X, Control Operation: ", HIDEventData->Event_Data.HID_Control_Indication_Data->HIDID));

            switch(HIDEventData->Event_Data.HID_Control_Indication_Data->ControlOperation)
            {
               case hcNop:
                  Display(("hcNop\r\n"));
                  break;
               case hcHardReset:
                  Display(("hcHardReset\r\n"));
                  break;
               case hcSoftReset:
                  Display(("hcSoftReset\r\n"));
                  break;
               case hcSuspend:
                  Display(("hcSuspend\r\n"));
                  break;
               case hcExitSuspend:
                  Display(("hcExitSuspend\r\n"));
                  break;
               case hcVirtualCableUnplug:
                  Display(("hcVirtualCableUnplug\r\n"));
                  break;
               default:
                  Display(("Unknown Control Operation Type\r\n"));
                  break;
            }
            break;
         case etHID_Get_Report_Indication:
            /* A HID Get Report Indication was received, display all    */
            /* relevant information.                                    */
            Display(("HID Get Report Indication, ID: 0x%04X, ", HIDEventData->Event_Data.HID_Get_Report_Indication_Data->HIDID));

            switch(HIDEventData->Event_Data.HID_Get_Report_Indication_Data->ReportType)
            {
               case rtOther:
                  Display(("ReportType: rtOther, "));
                  break;
               case rtInput:
                  Display(("ReportType: rtInput, "));
                  break;
               case rtOutput:
                  Display(("ReportType: rtOutput, "));
                  break;
               case rtFeature:
                  Display(("ReportType: rtFeature, "));
                  break;
               default:
                  Display(("ReportType: Unknown Report Type, "));
                  break;
            }

            Display(("ReportID: %u, ", HIDEventData->Event_Data.HID_Get_Report_Indication_Data->ReportID));

            switch(HIDEventData->Event_Data.HID_Get_Report_Indication_Data->Size)
            {
               case grSizeOfReport:
                  Display(("Size: grSizeOfReport, "));
                  break;
               case grUseBufferSize:
                  Display(("Size: grUseBufferSize, "));
                  break;
               default:
                  Display(("Size: Unknown Size Type, "));
                  break;
            }

            Display(("BufferSize: %u\r\n", HIDEventData->Event_Data.HID_Get_Report_Indication_Data->BufferSize));
            break;
         case etHID_Get_Report_Confirmation:
            /* A HID Get Report Confirmation was received, display all  */
            /* relevant information.                                    */
            Display(("HID Get Report Confirmation, ID: 0x%04X, ", HIDEventData->Event_Data.HID_Get_Report_Confirmation_Data->HIDID));

            switch(HIDEventData->Event_Data.HID_Get_Report_Confirmation_Data->Status)
            {
               case rtSuccessful:
                  Display(("Status: rtSuccessful, "));
                  break;
               case rtNotReady:
                  Display(("Status: rtNotReady, "));
                  break;
               case rtErrInvalidReportID:
                  Display(("Status: rtErrInvalidReportID, "));
                  break;
               case rtErrUnsupportedRequest:
                  Display(("Status: rtErrUnsupportedRequest, "));
                  break;
               case rtErrInvalidParameter:
                  Display(("Status: rtErrInvalidParameter, "));
                  break;
               case rtErrUnknown:
                  Display(("Status: rtErrUnknown, "));
                  break;
               case rtErrFatal:
                  Display(("Status: rtErrFatal, "));
                  break;
               case rtData:
                  Display(("Status: rtData, "));
                  break;
               default:
                  Display(("Status: Unknown Status Type, "));
                  break;
            }

            switch(HIDEventData->Event_Data.HID_Get_Report_Confirmation_Data->ReportType)
            {
               case rtOther:
                  Display(("ReportType: rtOther, "));
                  break;
               case rtInput:
                  Display(("ReportType: rtInput, "));
                  break;
               case rtOutput:
                  Display(("ReportType: rtOutput, "));
                  break;
               case rtFeature:
                  Display(("ReportType: rtFeature, "));
                  break;
               default:
                  Display(("ReportType: Unknown Report Type, "));
                  break;
            }

            Display(("ReportLength: %u, \r\n", HIDEventData->Event_Data.HID_Get_Report_Confirmation_Data->ReportLength));

            Display(("Report: "));

            for(Index=0;Index<(unsigned int)HIDEventData->Event_Data.HID_Get_Report_Confirmation_Data->ReportLength;Index++)
            {
               Display(("0x%02X ", HIDEventData->Event_Data.HID_Get_Report_Confirmation_Data->ReportDataPayload[Index]));
            }

            Display(("\r\n"));
            break;
         case etHID_Set_Report_Indication:
            /* A HID Set Report Indication was received, display all    */
            /* relevant information.                                    */
            Display(("HID Set Report Indication, ID: 0x%04X, ", HIDEventData->Event_Data.HID_Set_Report_Indication_Data->HIDID));

            switch(HIDEventData->Event_Data.HID_Set_Report_Indication_Data->ReportType)
            {
               case rtOther:
                  Display(("ReportType: rtOther, "));
                  break;
               case rtInput:
                  Display(("ReportType: rtInput, "));
                  break;
               case rtOutput:
                  Display(("ReportType: rtOutput, "));
                  break;
               case rtFeature:
                  Display(("ReportType: rtFeature, "));
                  break;
               default:
                  Display(("ReportType: Unknown Report Type, "));
                  break;
            }

            Display(("ReportLength: %u, \r\n", HIDEventData->Event_Data.HID_Set_Report_Indication_Data->ReportLength));

            Display(("Report: "));

            for(Index=0;Index<(unsigned int)HIDEventData->Event_Data.HID_Set_Report_Indication_Data->ReportLength;Index++)
            {
               Display(("0x%02X ", HIDEventData->Event_Data.HID_Set_Report_Indication_Data->ReportDataPayload[Index]));
            }

            Display(("\r\n"));
            break;
         case etHID_Set_Report_Confirmation:
            /* A HID Set Report Confirmation was received, display all  */
            /* relevant information.                                    */
            Display(("HID Set Report Confirmation, ID: 0x%04X, ", HIDEventData->Event_Data.HID_Set_Report_Confirmation_Data->HIDID));

            switch(HIDEventData->Event_Data.HID_Set_Report_Confirmation_Data->Status)
            {
               case rtSuccessful:
                  Display(("Status: rtSuccessful\r\n"));
                  break;
               case rtNotReady:
                  Display(("Status: rtNotReady\r\n"));
                  break;
               case rtErrInvalidReportID:
                  Display(("Status: rtErrInvalidReportID\r\n"));
                  break;
               case rtErrUnsupportedRequest:
                  Display(("Status: rtErrUnsupportedRequest\r\n"));
                  break;
               case rtErrInvalidParameter:
                  Display(("Status: rtErrInvalidParameter\r\n"));
                  break;
               case rtErrUnknown:
                  Display(("Status: rtErrUnknown\r\n"));
                  break;
               case rtErrFatal:
                  Display(("Status: rtErrFatal\r\n"));
                  break;
               case rtData:
                  Display(("Status: rtData\r\n"));
                  break;
               default:
                  Display(("Status: Unknown Status Type\r\n"));
                  break;
            }
            break;
         case etHID_Get_Protocol_Indication:
            /* A HID Get Protocol Indication was received, display all  */
            /* relevant information.                                    */
            Display(("HID Get Protocol Indication, ID: 0x%04X\r\n", HIDEventData->Event_Data.HID_Get_Protocol_Indication_Data->HIDID));
            break;
         case etHID_Get_Protocol_Confirmation:
            /* A HID Get Protocol Confirmation was received, display all*/
            /* relevant information.                                    */
            Display(("HID Get Protocol Confirmation, ID: 0x%04X, ", HIDEventData->Event_Data.HID_Get_Protocol_Confirmation_Data->HIDID));

            switch(HIDEventData->Event_Data.HID_Get_Protocol_Confirmation_Data->Status)
            {
               case rtSuccessful:
                  Display(("Status: rtSuccessful, "));
                  break;
               case rtNotReady:
                  Display(("Status: rtNotReady, "));
                  break;
               case rtErrInvalidReportID:
                  Display(("Status: rtErrInvalidReportID, "));
                  break;
               case rtErrUnsupportedRequest:
                  Display(("Status: rtErrUnsupportedRequest, "));
                  break;
               case rtErrInvalidParameter:
                  Display(("Status: rtErrInvalidParameter, "));
                  break;
               case rtErrUnknown:
                  Display(("Status: rtErrUnknown, "));
                  break;
               case rtErrFatal:
                  Display(("Status: rtErrFatal, "));
                  break;
               case rtData:
                  Display(("Status: rtData, "));
                  break;
               default:
                  Display(("Status: Unknown Status Type, "));
                  break;
            }

            if(HIDEventData->Event_Data.HID_Get_Protocol_Confirmation_Data->Protocol == ptBoot)
               Display(("Protocol: ptBoot\r\n"));
            else
               Display(("Protocol: ptReport\r\n"));
            break;
         case etHID_Set_Protocol_Indication:
            /* A HID Set Protocol Indication was received, display all  */
            /* relevant information.                                    */
            Display(("HID Set Protocol Indication, ID: 0x%04X, Protocol: %s\r\n", HIDEventData->Event_Data.HID_Set_Protocol_Indication_Data->HIDID,
                                                                                (HIDEventData->Event_Data.HID_Set_Protocol_Indication_Data->Protocol==ptBoot)?"ptBoot":"ptReport"));
            break;
         case etHID_Set_Protocol_Confirmation:
            /* A HID Set Protocol Confirmation was received, display all*/
            /* relevant information.                                    */
            Display(("HID Set Protocol Confirmation, ID: 0x%04X, ", HIDEventData->Event_Data.HID_Set_Protocol_Confirmation_Data->HIDID));

            switch(HIDEventData->Event_Data.HID_Set_Protocol_Confirmation_Data->Status)
            {
               case rtSuccessful:
                  Display(("Status: rtSuccessful\r\n"));
                  break;
               case rtNotReady:
                  Display(("Status: rtNotReady\r\n"));
                  break;
               case rtErrInvalidReportID:
                  Display(("Status: rtErrInvalidReportID\r\n"));
                  break;
               case rtErrUnsupportedRequest:
                  Display(("Status: rtErrUnsupportedRequest\r\n"));
                  break;
               case rtErrInvalidParameter:
                  Display(("Status: rtErrInvalidParameter\r\n"));
                  break;
               case rtErrUnknown:
                  Display(("Status: rtErrUnknown\r\n"));
                  break;
               case rtErrFatal:
                  Display(("Status: rtErrFatal\r\n"));
                  break;
               case rtData:
                  Display(("Status: rtData\r\n"));
                  break;
               default:
                  Display(("Status: Unknown Status Type\r\n"));
                  break;
            }
            break;
         case etHID_Get_Idle_Indication:
            /* A HID Get Idle Indication was received, display all      */
            /* relevant information.                                    */
            Display(("HID Get Idle Indication, ID: 0x%04X\r\n", HIDEventData->Event_Data.HID_Get_Idle_Indication_Data->HIDID));
            break;
         case etHID_Get_Idle_Confirmation:
            /* A HID Get Idle Confirmation was received, display all    */
            /* relevant information.                                    */
            Display(("HID Get Idle Confirmation, ID: 0x%04X, ", HIDEventData->Event_Data.HID_Get_Idle_Confirmation_Data->HIDID));

            switch(HIDEventData->Event_Data.HID_Get_Idle_Confirmation_Data->Status)
            {
               case rtSuccessful:
                  Display(("Status: rtSuccessful, "));
                  break;
               case rtNotReady:
                  Display(("Status: rtNotReady, "));
                  break;
               case rtErrInvalidReportID:
                  Display(("Status: rtErrInvalidReportID, "));
                  break;
               case rtErrUnsupportedRequest:
                  Display(("Status: rtErrUnsupportedRequest, "));
                  break;
               case rtErrInvalidParameter:
                  Display(("Status: rtErrInvalidParameter, "));
                  break;
               case rtErrUnknown:
                  Display(("Status: rtErrUnknown, "));
                  break;
               case rtErrFatal:
                  Display(("Status: rtErrFatal, "));
                  break;
               case rtData:
                  Display(("Status: rtData, "));
                  break;
               default:
                  Display(("Status: Unknown Status Type, "));
                  break;
            }

            Display(("IdleRate: %u\r\n", HIDEventData->Event_Data.HID_Get_Idle_Confirmation_Data->IdleRate));
            break;
         case etHID_Set_Idle_Indication:
            /* A HID Set Idle Indication was received, display all      */
            /* relevant information.                                    */
            Display(("HID Set Idle Indication, ID: 0x%04X, IdleRate: %u\r\n", HIDEventData->Event_Data.HID_Set_Idle_Indication_Data->HIDID,
                                                                            HIDEventData->Event_Data.HID_Set_Idle_Indication_Data->IdleRate));
            break;
         case etHID_Set_Idle_Confirmation:
            /* A HID Set Idle Confirmation was received, display all    */
            /* relevant information.                                    */
            Display(("HID Set Idle Confirmation, ID: 0x%04X, ", HIDEventData->Event_Data.HID_Set_Idle_Confirmation_Data->HIDID));

            switch(HIDEventData->Event_Data.HID_Set_Idle_Confirmation_Data->Status)
            {
               case rtSuccessful:
                  Display(("Status: rtSuccessful\r\n"));
                  break;
               case rtNotReady:
                  Display(("Status: rtNotReady\r\n"));
                  break;
               case rtErrInvalidReportID:
                  Display(("Status: rtErrInvalidReportID\r\n"));
                  break;
               case rtErrUnsupportedRequest:
                  Display(("Status: rtErrUnsupportedRequest\r\n"));
                  break;
               case rtErrInvalidParameter:
                  Display(("Status: rtErrInvalidParameter\r\n"));
                  break;
               case rtErrUnknown:
                  Display(("Status: rtErrUnknown\r\n"));
                  break;
               case rtErrFatal:
                  Display(("Status: rtErrFatal\r\n"));
                  break;
               case rtData:
                  Display(("Status: rtData\r\n"));
                  break;
               default:
                  Display(("Status: Unknown Status Type\r\n"));
                  break;
            }
            break;
         case etHID_Data_Indication:
            /* A HID Data Indication was received, display all relevant */
            /* information.                                             */
            Display(("HID Data Indication, ID: 0x%04X, ", HIDEventData->Event_Data.HID_Data_Indication_Data->HIDID));

            switch(HIDEventData->Event_Data.HID_Data_Indication_Data->ReportType)
            {
               case rtOther:
                  Display(("ReportType: rtOther, "));
                  break;
               case rtInput:
                  Display(("ReportType: rtInput, "));
                  break;
               case rtOutput:
                  Display(("ReportType: rtOutput, "));
                  break;
               case rtFeature:
                  Display(("ReportType: rtFeature, "));
                  break;
               default:
                  Display(("ReportType: Unknown Report Type, "));
                  break;
            }

            Display(("ReportLength: %u, \r\n", HIDEventData->Event_Data.HID_Data_Indication_Data->ReportLength));

            Display(("Report: "));

            for(Index=0;Index<(unsigned int)HIDEventData->Event_Data.HID_Data_Indication_Data->ReportLength;Index++)
            {
               Display(("0x%02X ", HIDEventData->Event_Data.HID_Data_Indication_Data->ReportDataPayload[Index]));
            }

            Display(("\r\n"));
            break;
         default:
            /* An unknown/unexpected HID event was received.            */
            Display(("\r\nUnknown HID Event Received.\r\n"));
            break;
      }
   }
   else
   {
      /* There was an error with one or more of the input parameters.   */
      Display(("\r\nHID callback data: Event_Data = NULL.\r\n"));
   }

   /* Output an Input Shell-type prompt.                                */
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
                  /* Run the selection user interface.                  */
                  UserInterface_Selection();

                  /* Display a prompt.                                  */
                  DisplayPrompt();

                  /* Return success to the caller.                      */
                  ret_val = (int)BluetoothStackID;
               }
               else
                  Display(("Error - SetPairable() returned %d.\r\n", ret_val));
            }
            else
               Display(("Error - SetDisc() returned %d.\r\n", ret_val));
         }
         else
            Display(("Error - SetConnect() returned %d.\r\n", ret_val));

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

   /* Displays the correct prompt.                                      */
void DisplayPrompt(void)
{
   if(UI_Mode == UI_MODE_IS_CLIENT)
      Display(("\r\nHID Host>"));
   else
   {
      if(UI_Mode == UI_MODE_IS_SERVER)
         Display(("\r\nHID Device>"));
      else
         Display(("\r\nSelect Mode>"));
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
