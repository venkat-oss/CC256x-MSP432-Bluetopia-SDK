/*****< hogpdemo.c >***********************************************************/
/*      Copyright 2014 Stonestreet One.                                       */
/*      All Rights Reserved.                                                  */
/*                                                                            */
/*      Copyright 2015 Texas Instruments Incorporated.                        */
/*      All Rights Reserved.                                                  */
/*                                                                            */
/*  HOGPDemo - Embedded Bluetooth HID over GATT Sample Application.           */
/*                                                                            */
/*  Author:  Ryan McCord                                                      */
/*                                                                            */
/*** MODIFICATION HISTORY *****************************************************/
/*                                                                            */
/*   mm/dd/yy  F. Lastname    Description of Modification                     */
/*   --------  -----------    ------------------------------------------------*/
/*   03/13/14  R. McCord      Initial creation.                               */
/*   03/03/15  D. Horowitz    Adding Demo Application version.                */
/******************************************************************************/
/*** COMMENTS *****************************************************************/
/* The HOGPDemo can only send alpha-numeric characters to the host device.    */
/******************************************************************************/
#include <stdio.h>               /* Included for sscanf.                      */
#include <ctype.h>               /* Included for isalnum.                     */
#include "Main.h"                /* Application Interface Abstraction.        */
#include "HAL.h"                 /* Function for Hardware Abstraction.        */
#include "HOGPDemo.h"            /* Application Header.                       */
#include "SS1BTPS.h"             /* Main SS1 BT Stack Header.                 */
#include "SS1BTGAT.h"            /* Main SS1 GATT Header.                     */
#include "SS1BTGAP.h"            /* Main SS1 GAP Service Header.              */
#include "SS1BTBAS.h"            /* Main SS1 BAS Service Header.              */
#include "SS1BTDIS.h"            /* Main SS1 DIS Service Header.              */
#include "SS1BTHIDS.h"           /* Main SS1 HIDS Service Header.             */
#include "BTPSKRNL.h"            /* BTPS Kernel Header.                       */

#define MAX_SUPPORTED_COMMANDS                     (65)  /* Denotes the       */
                                                         /* maximum number of */
                                                         /* User Commands that*/
                                                         /* are supported by  */
                                                         /* this application. */

#define MAX_NUM_OF_PARAMETERS                       (5)  /* Denotes the max   */
                                                         /* number of         */
                                                         /* parameters a      */
                                                         /* command can have. */

#define MAX_SUPPORTED_LINK_KEYS                    (1)   /* Max supported Link*/
                                                         /* keys.             */

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

#define EXIT_TEST_MODE                             (-10) /* Flags exit from   */
                                                         /* Test Mode.        */

   /* The following MACRO is used to convert an ASCII character into the*/
   /* equivalent decimal value.  The MACRO converts lower case          */
   /* characters to upper case before the conversion.                   */
#define ToInt(_x)                                  (((_x) > 0x39)?(((_x) & ~0x20)-0x37):((_x)-0x30))

   /* Determine the Name we will use for this compilation.              */
#define LE_APP_DEMO_NAME                              "HOGPDemo"

   /* The following define the PnP values that are assigned in the PnP  */
   /* ID characteristic of the Device Information Service.              */
#define PNP_ID_VENDOR_ID_STONESTREET_ONE                 0x005E
#define PNP_ID_PRODUCT_ID                                0xDEAD
#define PNP_ID_PRODUCT_VERSION                           0xBEEF

   /* The following define the valid bits of the Modifier byte that this*/
   /* application will use.                                             */
#define HID_MODIFIER_BYTE_NUM_LOCK                       0x01
#define HID_MODIFIER_BYTE_CAPS_LOCK                      0x02

   /* The following define the valid bits that may be set as part of the*/
   /* Keyboard Output Report.                                           */
#define HID_KEYBOARD_OUTPUT_REPORT_NUM_LOCK              0x01
#define HID_KEYBOARD_OUTPUT_REPORT_CAPS_LOCK             0x02
#define HID_KEYBOARD_OUTPUT_REPORT_SCOLL_LOCK            0x04
#define HID_KEYBOARD_OUTPUT_REPORT_COMPOSE               0x08
#define HID_KEYBOARD_OUTPUT_REPORT_KANA                  0x10

   /* The following defines the size of the application Keyboard Input  */
   /* Report.                                                           */
#define HID_KEYBOARD_INPUT_REPORT_SIZE                   8

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

   /* Structure used to hold all of the GAP LE Parameters.              */
typedef struct _tagGAPLE_Parameters_t
{
   GAP_LE_Connectability_Mode_t ConnectableMode;
   GAP_Discoverability_Mode_t   DiscoverabilityMode;
   GAP_LE_IO_Capability_t       IOCapability;
   Boolean_t                    MITMProtection;
   Boolean_t                    OOBDataPresent;
} GAPLE_Parameters_t;

#define GAPLE_PARAMETERS_DATA_SIZE                       (sizeof(GAPLE_Parameters_t))

   /* The following structure is a container for information on         */
   /* connected devices.                                                */
typedef struct _tagConnectionInfo_t
{
   unsigned char         Flags;
   unsigned int          ConnectionID;
   unsigned int          PasskeyDigits;
   unsigned long         Passkey;
   GAP_LE_Address_Type_t AddressType;
   BD_ADDR_t             BD_ADDR;
   unsigned int          SecurityTimerID;
} ConnectionInfo_t;

#define CONNECTION_INFO_FLAGS_CONNECTION_ENCRYPTED          0x01
#define CONNECTION_INFO_FLAGS_CONNECTION_AWAITING_PASSKEY   0x02
#define CONNECTION_INFO_FLAGS_CONNECTION_VALID              0x80

   /* The following structure is used to hold all of the application    */
   /* state information.                                                */
typedef struct _tagApplicationStateInfo_t
{
   unsigned int         BluetoothStackID;
   Byte_t               Flags;
   HIDS_Protocol_Mode_t HIDProtocolMode;
   Byte_t               CurrentInputReport[HID_KEYBOARD_INPUT_REPORT_SIZE];
   Byte_t               CurrentOutputReport;
   unsigned int         GAPSInstanceID;
   unsigned int         DISInstanceID;
   unsigned int         BASInstanceID;
   unsigned int         HIDSInstanceID;
   unsigned int         BatteryLevel;
   ConnectionInfo_t     LEConnectionInfo;
} ApplicationStateInfo_t;

#define APPLICATION_STATE_INFO_FLAGS_LE_CONNECTED           0x01
#define APPLICATION_STATE_INFO_FLAGS_CAPS_LOCKED            0x02

   /* The following structure represents the information we will store  */
   /* on a Discovered GAP Service.                                      */
typedef struct _tagGAPS_Client_Info_t
{
   Word_t DeviceNameHandle;
   Word_t DeviceAppearanceHandle;
} GAPS_Client_Info_t;

   /* The following structure holds information on known Device         */
   /* Appearance Values.                                                */
typedef struct _tagGAPS_Device_Appearance_Mapping_t
{
   Word_t  Appearance;
   char   *String;
} GAPS_Device_Appearance_Mapping_t;

   /* The following structure for is used to hold a list of information */
   /* on all paired devices.                                            */
typedef struct _tagDeviceInfo_t
{
   Byte_t                   Flags;
   Byte_t                   EncryptionKeySize;
   GAP_LE_Address_Type_t    AddressType;
   BD_ADDR_t                BD_ADDR;
   BAS_Server_Information_t BASServerInformation;
   Word_t                   BootKeyboardInputConfiguration;
   Word_t                   ReportKeyboardInputConfiguration;
   unsigned int             LostNotifiedBatteryLevel;
   Encryption_Key_t         IRK;
   GAPS_Client_Info_t       GAPSClientInfo;
   struct _tagDeviceInfo_t  *NextDeviceInfoPtr;
} DeviceInfo_t;



#define DEVICE_INFO_DATA_SIZE                            (sizeof(DeviceInfo_t))

   /* Defines the bit mask flags that may be set in the DeviceInfo_t    */
   /* structure.                                                        */
#define DEVICE_INFO_FLAGS_IRK_VALID                      0x01
#define DEVICE_INFO_FLAGS_LTK_VALID                      0x02
#define DEVICE_INFO_FLAGS_SPPLE_SERVER                   0x04
#define DEVICE_INFO_FLAGS_SERVICE_DISCOVERY_OUTSTANDING  0x08
#define DEVICE_INFO_FLAGS_LINK_ENCRYPTED                 0x10

   /* Defines the bitmask flags that may be set in the Flags member of  */
   /* the DeviceInfo_t structure.                                       */
#define DEVICE_INFO_FLAGS_IRK_VALID                      0x01

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

                        /* The Encryption Root Key should be generated  */
                        /* in such a way as to guarantee 128 bits of    */
                        /* entropy.                                     */
static BTPSCONST Encryption_Key_t ER = {0x28, 0xBA, 0xE1, 0x35, 0x13, 0xB2, 0x20, 0x45, 0x16, 0xB2, 0x19, 0xD9, 0x80, 0xEE, 0x4A, 0x51};

                        /* The Identity Root Key should be generated    */
                        /* in such a way as to guarantee 128 bits of    */
                        /* entropy.                                     */
static BTPSCONST Encryption_Key_t IR = {0x41, 0x09, 0xF4, 0x88, 0x09, 0x6B, 0x70, 0xC0, 0x95, 0x23, 0x3C, 0x8C, 0x48, 0xFC, 0xC9, 0xFE};

                        /* The following keys can be regenerated on the */
                        /* fly using the constant IR and ER keys and    */
                        /* are used globally, for all devices.          */
static Encryption_Key_t DHK;
static Encryption_Key_t IRK;

   /* Internal Variables to this Module (Remember that all variables    */
   /* declared static are initialized to 0 automatically by the         */
   /* compiler as part of standard C/C++).                              */
static ApplicationStateInfo_t ApplicationStateInfo; /* Container for all of the        */
                                                    /* Application State Information.  */

static GAPLE_Parameters_t  LE_Parameters;           /* Holds GAP Parameters like       */
                                                    /* Discoverability, Connectability */
                                                    /* Modes.                          */

static DeviceInfo_t       *DeviceInfoList;          /* Holds the list head for the     */
                                                    /* device info list.               */

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

   /* The following string table is used to map the API I/O Capabilities*/
   /* values to an easily displayable string.                           */
static BTPSCONST char *IOCapabilitiesStrings[] =
{
   "Display Only",
   "Display Yes/No",
   "Keyboard Only",
   "No Input/Output",
   "Keyboard/Display"
} ;

#define NUM_SUPPORTED_HCI_VERSIONS              (sizeof(HCIVersionStrings)/sizeof(char *) - 1)

   /* The following table represent the Keyboard Report Descriptor for  */
   /* this HID Keyboard Device.                                         */
static Byte_t KeyboardReportDescriptor[] =
{
    0x05, 0x01,  /* USAGE_PAGE (Generic Desktop)                        */
    0x09, 0x06,  /* USAGE (Keyboard)                                    */
    0xa1, 0x01,  /* COLLECTION (Application)                            */
    0x05, 0x07,  /* USAGE_PAGE (Keyboard)                               */
    0x19, 0xe0,  /* USAGE_MINIMUM (Keyboard LeftControl)                */
    0x29, 0xe7,  /* USAGE_MAXIMUM (Keyboard Right GUI)                  */
    0x15, 0x00,  /* LOGICAL_MINIMUM (0)                                 */
    0x25, 0x01,  /* LOGICAL_MAXIMUM (1)                                 */
    0x75, 0x01,  /* REPORT_SIZE (1)                                     */
    0x95, 0x08,  /* REPORT_COUNT (8)                                    */
    0x81, 0x02,  /* INPUT (Data,Var,Abs)                                */
    0x95, 0x01,  /* REPORT_COUNT (1)                                    */
    0x75, 0x08,  /* REPORT_SIZE (8)                                     */
    0x81, 0x03,  /* INPUT (Cnst,Var,Abs)                                */
    0x95, 0x05,  /* REPORT_COUNT (5)                                    */
    0x75, 0x01,  /* REPORT_SIZE (1)                                     */
    0x05, 0x08,  /* USAGE_PAGE (LEDs)                                   */
    0x19, 0x01,  /* USAGE_MINIMUM (Num Lock)                            */
    0x29, 0x05,  /* USAGE_MAXIMUM (Kana)                                */
    0x91, 0x02,  /* OUTPUT (Data,Var,Abs)                               */
    0x95, 0x01,  /* REPORT_COUNT (1)                                    */
    0x75, 0x03,  /* REPORT_SIZE (3)                                     */
    0x91, 0x03,  /* OUTPUT (Cnst,Var,Abs)                               */
    0x95, 0x06,  /* REPORT_COUNT (6)                                    */
    0x75, 0x08,  /* REPORT_SIZE (8)                                     */
    0x15, 0x00,  /* LOGICAL_MINIMUM (0)                                 */
    0x25, 0x65,  /* LOGICAL_MAXIMUM (101)                               */
    0x05, 0x07,  /* USAGE_PAGE (Keyboard)                               */
    0x19, 0x00,  /* USAGE_MINIMUM (Reserved (no event indicated))       */
    0x29, 0x65,  /* USAGE_MAXIMUM (Keyboard Application)                */
    0x81, 0x00,  /* INPUT (Data,Ary,Abs)                                */
    0xc0         /* END_COLLECTION                                      */
};

   /* The following is used to map from ATT Error Codes to a printable  */
   /* string.                                                           */
static char *ErrorCodeStr[] =
{
   "ATT_PROTOCOL_ERROR_CODE_NO_ERROR",
   "ATT_PROTOCOL_ERROR_CODE_INVALID_HANDLE",
   "ATT_PROTOCOL_ERROR_CODE_READ_NOT_PERMITTED",
   "ATT_PROTOCOL_ERROR_CODE_WRITE_NOT_PERMITTED",
   "ATT_PROTOCOL_ERROR_CODE_INVALID_PDU",
   "ATT_PROTOCOL_ERROR_CODE_INSUFFICIENT_AUTHENTICATION",
   "ATT_PROTOCOL_ERROR_CODE_REQUEST_NOT_SUPPORTED",
   "ATT_PROTOCOL_ERROR_CODE_INVALID_OFFSET",
   "ATT_PROTOCOL_ERROR_CODE_INSUFFICIENT_AUTHORIZATION",
   "ATT_PROTOCOL_ERROR_CODE_PREPARE_QUEUE_FULL",
   "ATT_PROTOCOL_ERROR_CODE_ATTRIBUTE_NOT_FOUND",
   "ATT_PROTOCOL_ERROR_CODE_ATTRIBUTE_NOT_LONG",
   "ATT_PROTOCOL_ERROR_CODE_INSUFFICIENT_ENCRYPTION_KEY_SIZE",
   "ATT_PROTOCOL_ERROR_CODE_INVALID_ATTRIBUTE_VALUE_LENGTH",
   "ATT_PROTOCOL_ERROR_CODE_UNLIKELY_ERROR",
   "ATT_PROTOCOL_ERROR_CODE_INSUFFICIENT_ENCRYPTION",
   "ATT_PROTOCOL_ERROR_CODE_UNSUPPORTED_GROUP_TYPE",
   "ATT_PROTOCOL_ERROR_CODE_INSUFFICIENT_RESOURCES"
};

#define NUMBER_GATT_ERROR_CODES  (sizeof(ErrorCodeStr)/sizeof(char *))

   /* The following array is used to map Device Appearance Values to    */
   /* strings.                                                          */
static GAPS_Device_Appearance_Mapping_t AppearanceMappings[] =
{
   {GAP_DEVICE_APPEARENCE_VALUE_UNKNOWN,                        "Unknown"},
   {GAP_DEVICE_APPEARENCE_VALUE_GENERIC_PHONE,                  "Generic Phone"},
   {GAP_DEVICE_APPEARENCE_VALUE_GENERIC_COMPUTER,               "Generic Computer"},
   {GAP_DEVICE_APPEARENCE_VALUE_GENERIC_WATCH,                  "Generic Watch"},
   {GAP_DEVICE_APPEARENCE_VALUE_SPORTS_WATCH,                   "Sports Watch"},
   {GAP_DEVICE_APPEARENCE_VALUE_GENERIC_CLOCK,                  "Generic Clock"},
   {GAP_DEVICE_APPEARENCE_VALUE_GENERIC_DISPLAY,                "Generic Display"},
   {GAP_DEVICE_APPEARENCE_VALUE_GENERIC_GENERIC_REMOTE_CONTROL, "Generic Remote Control"},
   {GAP_DEVICE_APPEARENCE_VALUE_GENERIC_EYE_GLASSES,            "Eye Glasses"},
   {GAP_DEVICE_APPEARENCE_VALUE_GENERIC_TAG,                    "Generic Tag"},
   {GAP_DEVICE_APPEARENCE_VALUE_GENERIC_KEYRING,                "Generic Keyring"},
   {GAP_DEVICE_APPEARENCE_VALUE_GENERIC_MEDIA_PLAYER,           "Generic Media Player"},
   {GAP_DEVICE_APPEARENCE_VALUE_GENERIC_BARCODE_SCANNER,        "Generic Barcode Scanner"},
   {GAP_DEVICE_APPEARENCE_VALUE_GENERIC_THERMOMETER,            "Generic Thermometer"},
   {GAP_DEVICE_APPEARENCE_VALUE_THERMOMETER_EAR,                "Ear Thermometer"},
   {GAP_DEVICE_APPEARENCE_VALUE_GENERIC_HEART_RATE_SENSOR,      "Generic Heart Rate Sensor"},
   {GAP_DEVICE_APPEARENCE_VALUE_BELT_HEART_RATE_SENSOR,         "Belt Heart Rate Sensor"},
   {GAP_DEVICE_APPEARENCE_VALUE_GENERIC_BLOOD_PRESSURE,         "Generic Blood Pressure"},
   {GAP_DEVICE_APPEARENCE_VALUE_BLOOD_PRESSURE_ARM,             "Blood Pressure: ARM"},
   {GAP_DEVICE_APPEARENCE_VALUE_BLOOD_PRESSURE_WRIST,           "Blood Pressure: Wrist"},
   {GAP_DEVICE_APPEARENCE_VALUE_HUMAN_INTERFACE_DEVICE,         "Human Interface Device"},
   {GAP_DEVICE_APPEARENCE_VALUE_HID_KEYBOARD,                   "HID Keyboard"},
   {GAP_DEVICE_APPEARENCE_VALUE_HID_MOUSE,                      "HID Mouse"},
   {GAP_DEVICE_APPEARENCE_VALUE_HID_JOYSTICK,                   "HID Joystick"},
   {GAP_DEVICE_APPEARENCE_VALUE_HID_GAMEPAD,                    "HID Gamepad"},
   {GAP_DEVICE_APPEARENCE_VALUE_HID_DIGITIZER_TABLET,           "HID Digitizer Tablet"},
   {GAP_DEVICE_APPEARENCE_VALUE_HID_CARD_READER,                "HID Card Reader"},
   {GAP_DEVICE_APPEARENCE_VALUE_HID_DIGITAL_PEN,                "HID Digitizer Pen"},
   {GAP_DEVICE_APPEARENCE_VALUE_HID_BARCODE_SCANNER,            "HID Barcode Scanner"},
   {GAP_DEVICE_APPEARENCE_VALUE_GENERIC_GLUCOSE_METER,          "Generic Glucose Meter"}
};

#define NUMBER_OF_APPEARANCE_MAPPINGS     (sizeof(AppearanceMappings)/sizeof(GAPS_Device_Appearance_Mapping_t))

   /* Internal function prototypes.                                     */
static Boolean_t CreateNewDeviceInfoEntry(DeviceInfo_t **ListHead, GAP_LE_Address_Type_t ConnectionAddressType, BD_ADDR_t ConnectionBD_ADDR);
static DeviceInfo_t *SearchDeviceInfoEntryByBD_ADDR(DeviceInfo_t **ListHead, GAP_LE_Address_Type_t AddressType, BD_ADDR_t BD_ADDR);
static DeviceInfo_t *DeleteDeviceInfoEntry(DeviceInfo_t **ListHead, GAP_LE_Address_Type_t AddressType, BD_ADDR_t BD_ADDR);
static void FreeDeviceInfoEntryMemory(DeviceInfo_t *EntryToFree);
static void FreeDeviceInfoList(DeviceInfo_t **ListHead);

static void UserInterface_Device(void);
static Boolean_t CommandLineInterpreter(char *Command);
static unsigned long StringToUnsignedInteger(char *StringInteger);
static char *StringParser(char *String);
static int CommandParser(UserCommand_t *TempCommand, char *Input);
static int CommandInterpreter(UserCommand_t *TempCommand);
static int AddCommand(char *CommandName, CommandFunction_t CommandFunction);
static CommandFunction_t FindCommand(char *Command);
static void ClearCommands(void);

static void BD_ADDRToStr(BD_ADDR_t Board_Address, BoardStr_t BoardStr);
static void StrToBD_ADDR(char *BoardStr, BD_ADDR_t *Board_Address);

static void DisplayIOCapabilities(void);
static void DisplayPairingInformation(GAP_LE_Pairing_Capabilities_t Pairing_Capabilities);
static void DisplayUUID(GATT_UUID_t *UUID);
static void DisplayUsage(char *UsageString);
static void DisplayFunctionError(char *Function,int Status);
static void DisplayFunctionSuccess(char *Function);
static void DisplayFWVersion(void);
static int ConfigureDIS(void);
static int ConfigureBAS(void);
static int ConfigureHIDS(void);
static int OpenStack(HCI_DriverInformation_t *HCI_DriverInformation, BTPS_Initialization_t *BTPS_Initialization);
static int CloseStack(void);

static int SetPairable(void);

static Boolean_t AppearanceToString(Word_t Appearance, char **String);

static void GAPSPopulateHandles(GAPS_Client_Info_t *ClientInfo, GATT_Service_Discovery_Indication_Data_t *ServiceInfo);

static void FormatAdvertisingData(unsigned int BluetoothStackID);
static int StartAdvertising(unsigned int BluetoothStackID);
static int StopAdvertising(unsigned int BluetoothStackID);

static int DisconnectLEDevice(unsigned int BluetoothStackID, BD_ADDR_t BD_ADDR);

static void ConfigureCapabilities(GAP_LE_Pairing_Capabilities_t *Capabilities);
static int SlavePairingRequestResponse(unsigned int BluetoothStackID, BD_ADDR_t BD_ADDR);
static int SlaveSecurityReEstablishment(unsigned int BluetoothStackID, BD_ADDR_t BD_ADDR);
static int EncryptionInformationRequestResponse(unsigned int BluetoothStackID, BD_ADDR_t BD_ADDR, Byte_t KeySize, GAP_LE_Authentication_Response_Information_t *GAP_LE_Authentication_Response_Information);

static int DisplayHelp(ParameterList_t *TempParam);
static int QueryMemory(ParameterList_t *TempParam);
static int SetLEDiscoverabilityMode(ParameterList_t *TempParam);
static int SetLEConnectabilityMode(ParameterList_t *TempParam);
static int SetLEPairabilityMode(ParameterList_t *TempParam);
static int GetLocalAddress(ParameterList_t *TempParam);
static int ChangeLEPairingParameters(ParameterList_t *TempParam);
static int LEPassKeyResponse(ParameterList_t *TempParam);
static int AdvertiseLE(ParameterList_t *TempParam);
static int DisconnectLE(ParameterList_t *TempParam);
static int DiscoverGAPS(ParameterList_t *TempParam);
static int ReadLocalName(ParameterList_t *TempParam);
static int ReadRemoteName(ParameterList_t *TempParam);
static int ReadLocalAppearance(ParameterList_t *TempParam);
static int ReadRemoteAppearance(ParameterList_t *TempParam);

static int SetBatteryLevel(ParameterList_t *TempParam);
static int NotifyBatteryLevel(ParameterList_t *TempParam);
static int NotifyKeyboardReport(ParameterList_t *TempParam);

   /* BTPS Callback function prototypes.                                */
static void BTPSAPI GAP_LE_Event_Callback(unsigned int BluetoothStackID,GAP_LE_Event_Data_t *GAP_LE_Event_Data, unsigned long CallbackParameter);
static void BTPSAPI GATT_ClientEventCallback_GAPS(unsigned int BluetoothStackID, GATT_Client_Event_Data_t *GATT_Client_Event_Data, unsigned long CallbackParameter);
static void BTPSAPI GATT_Connection_Event_Callback(unsigned int BluetoothStackID, GATT_Connection_Event_Data_t *GATT_Connection_Event_Data, unsigned long CallbackParameter);
static void BTPSAPI GATT_Service_Discovery_Event_Callback(unsigned int BluetoothStackID, GATT_Service_Discovery_Event_Data_t *GATT_Service_Discovery_Event_Data, unsigned long CallbackParameter);
static void BTPSAPI BAS_Event_Callback(unsigned int BluetoothStackID, BAS_Event_Data_t *BAS_Event_Data, unsigned long CallbackParameter);
static void BTPSAPI HIDS_Event_Callback(unsigned int BluetoothStackID, HIDS_Event_Data_t *HIDS_Event_Data, unsigned long CallbackParameter);
static void BTPSAPI BSC_TimerCallback(unsigned int BluetoothStackID, unsigned int TimerID, unsigned long CallbackParameter);

   /* The following function adds the specified Entry to the specified  */
   /* List.  This function allocates and adds an entry to the list that */
   /* has the same attributes as parameters to this function.  This     */
   /* function will return FALSE if NO Entry was added.  This can occur */
   /* if the element passed in was deemed invalid or the actual List    */
   /* Head was invalid.                                                 */
   /* ** NOTE ** This function does not insert duplicate entries into   */
   /*            the list.  An element is considered a duplicate if the */
   /*            Connection BD_ADDR.  When this occurs, this function   */
   /*            returns NULL.                                          */
static Boolean_t CreateNewDeviceInfoEntry(DeviceInfo_t **ListHead, GAP_LE_Address_Type_t ConnectionAddressType, BD_ADDR_t ConnectionBD_ADDR)
{
   Boolean_t     ret_val = FALSE;
   DeviceInfo_t *DeviceInfoPtr;

   /* Verify that the passed in parameters seem semi-valid.             */
   if((ListHead) && (!COMPARE_NULL_BD_ADDR(ConnectionBD_ADDR)))
   {
      /* Allocate the memory for the entry.                             */
      if((DeviceInfoPtr = BTPS_AllocateMemory(sizeof(DeviceInfo_t))) != NULL)
      {
         /* Initialize the entry.                                       */
         BTPS_MemInitialize(DeviceInfoPtr, 0, sizeof(DeviceInfo_t));
         DeviceInfoPtr->AddressType = ConnectionAddressType;
         DeviceInfoPtr->BD_ADDR     = ConnectionBD_ADDR;

         ret_val = BSC_AddGenericListEntry_Actual(ekBD_ADDR_t, BTPS_STRUCTURE_OFFSET(DeviceInfo_t, BD_ADDR), BTPS_STRUCTURE_OFFSET(DeviceInfo_t, NextDeviceInfoPtr), (void **)(ListHead), (void *)(DeviceInfoPtr));
         if(!ret_val)
         {
            /* Failed to add to list so we should free the memory that  */
            /* we allocated for the entry.                              */
            BTPS_FreeMemory(DeviceInfoPtr);
         }
      }
   }

   return(ret_val);
}

   /* The following function searches the specified List for the        */
   /* specified Connection BD_ADDR.  This function returns NULL if      */
   /* either the List Head is invalid, the BD_ADDR is invalid, or the   */
   /* Connection BD_ADDR was NOT found.                                 */
static DeviceInfo_t *SearchDeviceInfoEntryByBD_ADDR(DeviceInfo_t **ListHead, GAP_LE_Address_Type_t AddressType, BD_ADDR_t BD_ADDR)
{
   DeviceInfo_t *DeviceInfo;

   /* Verify that the input parameters are semi-valid.                  */
   if((ListHead) && (!COMPARE_NULL_BD_ADDR(BD_ADDR)))
   {
      /* Check to see if this is a resolvable address type.  If so we   */
      /* will search the list based on the IRK.                         */
      if((AddressType == latRandom) && (GAP_LE_TEST_RESOLVABLE_ADDRESS_BITS(BD_ADDR)))
      {
         /* Walk the list and attempt to resolve this entry to an       */
         /* existing entry with IRK.                                    */
         DeviceInfo = *ListHead;
         while(DeviceInfo)
         {
            /* Check to see if the IRK is valid.                        */
            if(DeviceInfo->Flags & DEVICE_INFO_FLAGS_IRK_VALID)
            {
               /* Attempt to resolve this address with the stored IRK.  */
               if(GAP_LE_Resolve_Address(ApplicationStateInfo.BluetoothStackID, &(DeviceInfo->IRK), BD_ADDR))
               {
                  /* Address resolved so just exit from the loop.       */
                  break;
               }
            }

            DeviceInfo = DeviceInfo->NextDeviceInfoPtr;
         }
      }
      else
         DeviceInfo = NULL;

      /* If all else fail we will attempt to search the list by just the*/
      /* BD_ADDR.                                                       */
      if(DeviceInfo == NULL)
         DeviceInfo = BSC_SearchGenericListEntry(ekBD_ADDR_t, (void *)(&BD_ADDR), BTPS_STRUCTURE_OFFSET(DeviceInfo_t, BD_ADDR), BTPS_STRUCTURE_OFFSET(DeviceInfo_t, NextDeviceInfoPtr), (void **)(ListHead));
   }
   else
      DeviceInfo = NULL;

   return(DeviceInfo);
}

   /* The following function searches the specified Key Info List for   */
   /* the specified BD_ADDR and removes it from the List.  This function*/
   /* returns NULL if either the List Head is invalid, the BD_ADDR is   */
   /* invalid, or the specified Entry was NOT present in the list.  The */
   /* entry returned will have the Next Entry field set to NULL, and    */
   /* the caller is responsible for deleting the memory associated with */
   /* this entry by calling the FreeKeyEntryMemory() function.          */
static DeviceInfo_t *DeleteDeviceInfoEntry(DeviceInfo_t **ListHead, GAP_LE_Address_Type_t AddressType, BD_ADDR_t BD_ADDR)
{
   DeviceInfo_t *LastEntry;
   DeviceInfo_t *DeviceInfo;

   if((ListHead) && (!COMPARE_NULL_BD_ADDR(BD_ADDR)))
   {
      /* Check to see if this is a resolvable address type.  If so we   */
      /* will search the list based on the IRK.                         */
      if((AddressType == latRandom) && (GAP_LE_TEST_RESOLVABLE_ADDRESS_BITS(BD_ADDR)))
      {
         /* Now, let's search the list until we find the correct entry. */
         DeviceInfo = *ListHead;
         LastEntry  = NULL;
         while((DeviceInfo) && ((!(DeviceInfo->Flags & DEVICE_INFO_FLAGS_IRK_VALID)) || (!GAP_LE_Resolve_Address(ApplicationStateInfo.BluetoothStackID, &(DeviceInfo->IRK), BD_ADDR))))
         {
            LastEntry  = DeviceInfo;

            DeviceInfo = DeviceInfo->NextDeviceInfoPtr;
         }

         /* Check to see if we found the specified entry.               */
         if(DeviceInfo)
         {
            /* OK, now let's remove the entry from the list.  We have to*/
            /* check to see if the entry was the first entry in the     */
            /* list.                                                    */
            if(LastEntry)
            {
               /* Entry was NOT the first entry in the list.            */
               LastEntry->NextDeviceInfoPtr = DeviceInfo->NextDeviceInfoPtr;
            }
            else
               *ListHead = DeviceInfo->NextDeviceInfoPtr;

            DeviceInfo->NextDeviceInfoPtr = NULL;
         }
      }
      else
         DeviceInfo = NULL;

      /* If all else fail we will attempt to search the list by just the*/
      /* BD_ADDR.                                                       */
      if(DeviceInfo == NULL)
         DeviceInfo = BSC_DeleteGenericListEntry(ekBD_ADDR_t, (void *)(&BD_ADDR), BTPS_STRUCTURE_OFFSET(DeviceInfo_t, BD_ADDR), BTPS_STRUCTURE_OFFSET(DeviceInfo_t, NextDeviceInfoPtr), (void **)(ListHead));
   }
   else
      DeviceInfo = NULL;

   return(DeviceInfo);
}

   /* This function frees the specified Key Info Information member     */
   /* memory.                                                           */
static void FreeDeviceInfoEntryMemory(DeviceInfo_t *EntryToFree)
{
   BSC_FreeGenericListEntryMemory((void *)(EntryToFree));
}

   /* The following function deletes (and frees all memory) every       */
   /* element of the specified Key Info List.  Upon return of this      */
   /* function, the Head Pointer is set to NULL.                        */
static void FreeDeviceInfoList(DeviceInfo_t **ListHead)
{
   BSC_FreeGenericListEntryList((void **)(ListHead), BTPS_STRUCTURE_OFFSET(DeviceInfo_t, NextDeviceInfoPtr));
}

   /* This function is responsible for taking the input from the user   */
   /* and dispatching the appropriate Command Function.  First, this    */
   /* function retrieves a String of user input, parses the user input  */
   /* into Command and Parameters, and finally executes the Command or  */
   /* Displays an Error Message if the input is not a valid Command.    */
static void UserInterface_Device(void)
{
   /* Next display the available commands.                              */
   DisplayHelp(NULL);

   /* Clear the installed command.                                      */
   ClearCommands();

   /* Install the commands relevant for this UI.                        */
   AddCommand("GETLOCALADDRESS", GetLocalAddress);
   AddCommand("SETDISCOVERABILITYMODE", SetLEDiscoverabilityMode);
   AddCommand("SETCONNECTABILITYMODE", SetLEConnectabilityMode);
   AddCommand("SETPAIRABILITYMODE", SetLEPairabilityMode);
   AddCommand("CHANGEPAIRINGPARAMETERS", ChangeLEPairingParameters);
   AddCommand("ADVERTISE", AdvertiseLE);
   AddCommand("PASSKEYRESPONSE", LEPassKeyResponse);
   AddCommand("DISCONNECT", DisconnectLE);
   AddCommand("DISCOVERGAPS", DiscoverGAPS);
   AddCommand("GETLOCALNAME", ReadLocalName);
   AddCommand("GETREMOTENAME", ReadRemoteName);
   AddCommand("GETLOCALAPPEARANCE", ReadLocalAppearance);
   AddCommand("GETREMOTEAPPEARANCE", ReadRemoteAppearance);
   AddCommand("SETBATTERYLEVEL", SetBatteryLevel);
   AddCommand("NOTIFYKEYBOARDREPORT", NotifyKeyboardReport);
   AddCommand("NOTIFYBATTERYLEVEL", NotifyBatteryLevel);
   AddCommand("HELP", DisplayHelp);
   AddCommand("QUERYMEMORY", QueryMemory);

   DisplayPrompt();
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
            Display(("Invalid Command: %s.\r\n", TempCommand.Command));
            break;
         case FUNCTION_ERROR:
            Display(("Function Error.\r\n"));
            break;
      }

      /* Display a prompt.                                              */
      DisplayPrompt();

      /* Flag that a command was executed.                              */
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
            if((ret_val = ((*CommandFunction)(&TempCommand->Parameters))) != 0)
            {
               if ((ret_val != EXIT_CODE) && (ret_val != EXIT_TEST_MODE))
                  ret_val = FUNCTION_ERROR;
            }
         }
      }
      else
      {
         /* The command entered is exit, set return value to EXIT_CODE  */
         /* and return.  Note - Changed to invalid command error since  */
         /* quit isn't supported for HOG Device.                        */
         ret_val = INVALID_COMMAND_ERROR;
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

   /* The following function is responsible for the specified string    */
   /* into data of type BD_ADDR.  The first parameter of this function  */
   /* is the BD_ADDR string to be converted to a BD_ADDR.  The second   */
   /* parameter of this function is a pointer to the BD_ADDR in which   */
   /* the converted BD_ADDR String is to be stored.                     */
static void StrToBD_ADDR(char *BoardStr, BD_ADDR_t *Board_Address)
{
   char          *TempPtr;
   unsigned int   StringLength;
   unsigned int   Index;
   unsigned char  Value;

   if((BoardStr) && ((StringLength = BTPS_StringLength(BoardStr)) >= (sizeof(BD_ADDR_t) * 2)) && (Board_Address))
   {
      TempPtr = BoardStr;
      if((StringLength >= (sizeof(BD_ADDR_t) * 2) + 2) && (TempPtr[0] == '0') && ((TempPtr[1] == 'x') || (TempPtr[1] == 'X')))
         TempPtr += 2;

      for(Index=0;Index<6;Index++)
      {
         Value  = (char)(ToInt(*TempPtr) * 0x10);
         TempPtr++;
         Value += (char)ToInt(*TempPtr);
         TempPtr++;
         ((char *)Board_Address)[5-Index] = (Byte_t)Value;
      }
   }
   else
   {
      if(Board_Address)
         BTPS_MemInitialize(Board_Address, 0, sizeof(BD_ADDR_t));
   }
}

   /* Displays the current I/O Capabilities.                            */
static void DisplayIOCapabilities(void)
{
   Display(("LE:     I/O Capabilities: %s, MITM: %s.\r\n", IOCapabilitiesStrings[(unsigned int)(LE_Parameters.IOCapability - licDisplayOnly)], LE_Parameters.MITMProtection?"TRUE":"FALSE"));
}

   /* The following function displays the pairing capabilities that is  */
   /* passed into this function.                                        */
static void DisplayPairingInformation(GAP_LE_Pairing_Capabilities_t Pairing_Capabilities)
{
   /* Display the IO Capability.                                        */
   switch(Pairing_Capabilities.IO_Capability)
   {
      case licDisplayOnly:
         Display(("   IO Capability:       lcDisplayOnly.\r\n"));
         break;
      case licDisplayYesNo:
         Display(("   IO Capability:       lcDisplayYesNo.\r\n"));
         break;
      case licKeyboardOnly:
         Display(("   IO Capability:       lcKeyboardOnly.\r\n"));
         break;
      case licNoInputNoOutput:
         Display(("   IO Capability:       lcNoInputNoOutput.\r\n"));
         break;
      case licKeyboardDisplay:
         Display(("   IO Capability:       lcKeyboardDisplay.\r\n"));
         break;
   }

   Display(("   MITM:                %s.\r\n", (Pairing_Capabilities.MITM == TRUE)?"TRUE":"FALSE"));
   Display(("   Bonding Type:        %s.\r\n", (Pairing_Capabilities.Bonding_Type == lbtBonding)?"Bonding":"No Bonding"));
   Display(("   OOB:                 %s.\r\n", (Pairing_Capabilities.OOB_Present == TRUE)?"OOB":"OOB Not Present"));
   Display(("   Encryption Key Size: %d.\r\n", Pairing_Capabilities.Maximum_Encryption_Key_Size));
   Display(("   Sending Keys: \r\n"));
   Display(("      LTK:              %s.\r\n", ((Pairing_Capabilities.Sending_Keys.Encryption_Key == TRUE)?"YES":"NO")));
   Display(("      IRK:              %s.\r\n", ((Pairing_Capabilities.Sending_Keys.Identification_Key == TRUE)?"YES":"NO")));
   Display(("      CSRK:             %s.\r\n", ((Pairing_Capabilities.Sending_Keys.Signing_Key == TRUE)?"YES":"NO")));
   Display(("   Receiving Keys: \r\n"));
   Display(("      LTK:              %s.\r\n", ((Pairing_Capabilities.Receiving_Keys.Encryption_Key == TRUE)?"YES":"NO")));
   Display(("      IRK:              %s.\r\n", ((Pairing_Capabilities.Receiving_Keys.Identification_Key == TRUE)?"YES":"NO")));
   Display(("      CSRK:             %s.\r\n", ((Pairing_Capabilities.Receiving_Keys.Signing_Key == TRUE)?"YES":"NO")));
}

   /* The following function is provided to properly print a UUID.      */
static void DisplayUUID(GATT_UUID_t *UUID)
{
   if(UUID)
   {
      if(UUID->UUID_Type == guUUID_16)
         Display(("%02X%02X", UUID->UUID.UUID_16.UUID_Byte1, UUID->UUID.UUID_16.UUID_Byte0));
      else
      {
         if(UUID->UUID_Type == guUUID_128)
         {
            Display(("%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X", UUID->UUID.UUID_128.UUID_Byte15, UUID->UUID.UUID_128.UUID_Byte14, UUID->UUID.UUID_128.UUID_Byte13,
                                                                                         UUID->UUID.UUID_128.UUID_Byte12, UUID->UUID.UUID_128.UUID_Byte11, UUID->UUID.UUID_128.UUID_Byte10,
                                                                                         UUID->UUID.UUID_128.UUID_Byte9,  UUID->UUID.UUID_128.UUID_Byte8,  UUID->UUID.UUID_128.UUID_Byte7,
                                                                                         UUID->UUID.UUID_128.UUID_Byte6,  UUID->UUID.UUID_128.UUID_Byte5,  UUID->UUID.UUID_128.UUID_Byte4,
                                                                                         UUID->UUID.UUID_128.UUID_Byte3,  UUID->UUID.UUID_128.UUID_Byte2,  UUID->UUID.UUID_128.UUID_Byte1,
                                                                                         UUID->UUID.UUID_128.UUID_Byte0));
         }
      }
   }

   Display((".\r\n"));
}

   /* Displays a usage string..                                         */
static void DisplayUsage(char *UsageString)
{
   Display(("Usage: %s.\r\n",UsageString));
}

   /* Displays a function error message.                                */
static void DisplayFunctionError(char *Function,int Status)
{
   Display(("%s Failed: %d.\r\n", Function, Status));
}

   /* Displays a function success message.                              */
static void DisplayFunctionSuccess(char *Function)
{
   Display(("%s Success.\r\n",Function));
}

   /* The following function is for displaying The FW Version by reading*/
   /* The Local version information form the FW.                        */
static void DisplayFWVersion(void)
{
    FW_Version FW_Version_Details;

    /* This function retrieves the Local Version Information of the FW. */
    HCI_Read_Local_Version_Information(ApplicationStateInfo.BluetoothStackID, &FW_Version_Details.StatusResult, &FW_Version_Details.HCI_VersionResult, &FW_Version_Details.HCI_RevisionResult, &FW_Version_Details.LMP_VersionResult, &FW_Version_Details.Manufacturer_NameResult, &FW_Version_Details.LMP_SubversionResult);
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


   /* The following function is a utility function that is used to      */
   /* configure the Device Information Service that is registered by    */
   /* this application.  This function returns ZERO if successful or a  */
   /* negative error code.                                              */
static int ConfigureDIS(void)
{
   int               ret_val;
   unsigned int      ServiceID;
   DIS_PNP_ID_Data_t PNP_ID;

   /* Initialize the DIS Service.                                       */
   ret_val = DIS_Initialize_Service(ApplicationStateInfo.BluetoothStackID, &ServiceID);
   if(ret_val > 0)
   {
      Display(("Device Information Service registered, Service ID = %u.\r\n", ServiceID));

      /* Save the Instance ID of the DIS Service.                       */
      ApplicationStateInfo.DISInstanceID = (unsigned int)ret_val;

      /* Set the DIS Manufacturer Name.                                 */
      if((ret_val = DIS_Set_Manufacturer_Name(ApplicationStateInfo.BluetoothStackID, ApplicationStateInfo.DISInstanceID, BTPS_VERSION_PRODUCT_NAME_STRING)) == 0)
      {
         /* Set the DIS Software Revision.                              */
         if((ret_val = DIS_Set_Software_Revision(ApplicationStateInfo.BluetoothStackID, ApplicationStateInfo.DISInstanceID, BTPS_VERSION_VERSION_STRING)) == 0)
         {
            /* Configure the PNP ID value to use.                       */
            PNP_ID.VendorID_Source = DIS_PNP_ID_VENDOR_SOURCE_BLUETOOTH_SIG;
            PNP_ID.VendorID        = PNP_ID_VENDOR_ID_STONESTREET_ONE;
            PNP_ID.ProductID       = PNP_ID_PRODUCT_ID;
            PNP_ID.ProductVersion  = PNP_ID_PRODUCT_VERSION;

            /* Finally set the PNP ID.                                  */
            ret_val = DIS_Set_PNP_ID(ApplicationStateInfo.BluetoothStackID, ApplicationStateInfo.DISInstanceID, &PNP_ID);
         }
      }
   }
   else
      Display(("Error - DIS_Initialize_Service() %d.\r\n", ret_val));

   return(ret_val);
}

   /* The following function is a utility function that is used to      */
   /* configure the Battery Service that is registered by this          */
   /* application.  This function returns ZERO if successful or a       */
   /* negative error code.                                              */
static int ConfigureBAS(void)
{
   int          ret_val;
   unsigned int ServiceID;

   /* Initialize the BAS Service.                                       */
   ret_val = BAS_Initialize_Service(ApplicationStateInfo.BluetoothStackID, BAS_Event_Callback, 0, &ServiceID);
   if(ret_val > 0)
   {
      Display(("Battery Service registered, Service ID = %u.\r\n", ServiceID));

      /* Save the Instance ID of the BAS Service.                       */
      ApplicationStateInfo.BASInstanceID = (unsigned int)ret_val;

      ret_val                            = 0;
   }
   else
      Display(("Error - BAS_Initialize_Service() %d.\r\n", ret_val));

   return(ret_val);
}

   /* The following function is a utility function that is used to      */
   /* configure the HID Service that is registered by this application. */
   /* This function returns ZERO if successful or a negative error code.*/
static int ConfigureHIDS(void)
{
   int                          ret_val;
   unsigned int                 ServiceID;
   HIDS_HID_Information_Data_t  HIDInformation;
   HIDS_Report_Reference_Data_t ReportReferenceData[2];

   /* Configure the HID Information value.                              */
   HIDInformation.CountryCode = HIDS_HID_LOCALIZATION_BYTE_NO_LOCALIZATION;
   HIDInformation.Flags       = HIDS_HID_INFORMATION_FLAGS_NORMALLY_CONNECTABLE;
   HIDInformation.Version     = HIDS_HID_VERSION_NUMBER;

   /* Configure the Report Reference structures.  Note that since we    */
   /* have only 1 report of a type (Input,Output,Feature) we do not need*/
   /* to have a unique Reference ID and therefore we use a Report ID of */
   /* ZERO.                                                             */
   ReportReferenceData[0].ReportID   = 0;
   ReportReferenceData[0].ReportType = HIDS_REPORT_REFERENCE_REPORT_TYPE_INPUT_REPORT;
   ReportReferenceData[1].ReportID   = 0;
   ReportReferenceData[1].ReportType = HIDS_REPORT_REFERENCE_REPORT_TYPE_OUTPUT_REPORT;

   /* Initialize the HID Service.                                       */
   ret_val = HIDS_Initialize_Service(ApplicationStateInfo.BluetoothStackID, HIDS_FLAGS_SUPPORT_KEYBOARD, &HIDInformation, 0, NULL, 0, NULL, (sizeof(ReportReferenceData)/sizeof(HIDS_Report_Reference_Data_t)), ReportReferenceData, HIDS_Event_Callback, 0, &ServiceID);
   if(ret_val > 0)
   {
      Display(("HID Service registered, Service ID = %u.\r\n", ServiceID));

      /* Save the Instance ID of the HID Service.                       */
      ApplicationStateInfo.HIDSInstanceID = (unsigned int)ret_val;

      ret_val                             = 0;
   }
   else
      Display(("Error - HIDS_Initialize_Service() %d.\r\n", ret_val));

   return(ret_val);
}

   /* The following function is responsible for opening the SS1         */
   /* Bluetooth Protocol Stack.  This function accepts a pre-populated  */
   /* HCI Driver Information structure that contains the HCI Driver     */
   /* Transport Information.  This function returns zero on successful  */
   /* execution and a negative value on all errors.                     */
static int OpenStack(HCI_DriverInformation_t *HCI_DriverInformation, BTPS_Initialization_t *BTPS_Initialization)
{
   int                           Result;
   int                           ret_val = 0;
   char                          BluetoothAddress[16];
   Byte_t                        Status;
   BD_ADDR_t                     BD_ADDR;
   unsigned int                  ServiceID;
   HCI_Version_t                 HCIVersion;
   L2CA_Link_Connect_Params_t    L2CA_Link_Connect_Params;

   /* Next, makes sure that the Driver Information passed appears to be */
   /* semi-valid.                                                       */
   if(HCI_DriverInformation)
   {

      /* Initialize BTPSKNRL.                                           */
      BTPS_Init((void *)BTPS_Initialization);

         Display(("\r\nOpenStack().\r\n"));

      /* Clear the application state information.                       */
      BTPS_MemInitialize(&ApplicationStateInfo, 0, sizeof(ApplicationStateInfo));

      /* Initialize the Stack                                           */
      Result = BSC_Initialize(HCI_DriverInformation, 0);

      /* Next, check the return value of the initialization to see if it*/
      /* was successful.                                                */
      if(Result > 0)
      {
         /* The Stack was initialized successfully, inform the user and */
         /* set the return value of the initialization function to the  */
         /* Bluetooth Stack ID.                                         */
         ApplicationStateInfo.BluetoothStackID = Result;
         Display(("Bluetooth Stack ID: %d\r\n", ApplicationStateInfo.BluetoothStackID));

         ret_val          = 0;

         /* Attempt to enable the WBS feature.                          */
         Result = BSC_EnableFeature(ApplicationStateInfo.BluetoothStackID, BSC_FEATURE_BLUETOOTH_LOW_ENERGY);
         if(!Result)
         {
            Display(("LOW ENERGY Support initialized.\r\n"));
         }
         else
         {
            Display(("LOW ENERGY Support not initialized %d.\r\n", Result));
         }
         /* Initialize the Default Pairing Parameters.                  */
         LE_Parameters.IOCapability   = licKeyboardDisplay;
         LE_Parameters.MITMProtection = FALSE;
         LE_Parameters.OOBDataPresent = FALSE;

         if(!HCI_Version_Supported(ApplicationStateInfo.BluetoothStackID, &HCIVersion))
            Display(("Device Chipset: %s\r\n", (HCIVersion <= NUM_SUPPORTED_HCI_VERSIONS)?HCIVersionStrings[HCIVersion]:HCIVersionStrings[NUM_SUPPORTED_HCI_VERSIONS]));

         /* Printing the BTPS version                                   */
         Display(("BTPS Version  : %s \r\n", BTPS_VERSION_VERSION_STRING));
         /* Printing the FW version                                     */
         DisplayFWVersion();
         /* Printing the Demo Application name and version              */
         Display(("App Name      : %s \r\n", LE_APP_DEMO_NAME));
         Display(("App Version   : %s \r\n", DEMO_APPLICATION_VERSION_STRING));
         /* Let's output the Bluetooth Device Address so that the user  */
         /* knows what the Device Address is.                           */
         if(!GAP_Query_Local_BD_ADDR(ApplicationStateInfo.BluetoothStackID, &BD_ADDR))
         {
            BD_ADDRToStr(BD_ADDR, BluetoothAddress);

            Display(("BD_ADDR: %s\r\n", BluetoothAddress));
         }

         /* Go ahead and allow Master/Slave Role Switch.                */
         L2CA_Link_Connect_Params.L2CA_Link_Connect_Request_Config  = cqAllowRoleSwitch;
         L2CA_Link_Connect_Params.L2CA_Link_Connect_Response_Config = csMaintainCurrentRole;

         L2CA_Set_Link_Connection_Configuration(ApplicationStateInfo.BluetoothStackID, &L2CA_Link_Connect_Params);

         if(HCI_Command_Supported(ApplicationStateInfo.BluetoothStackID, HCI_SUPPORTED_COMMAND_WRITE_DEFAULT_LINK_POLICY_BIT_NUMBER) > 0)
            HCI_Write_Default_Link_Policy_Settings(ApplicationStateInfo.BluetoothStackID, (HCI_LINK_POLICY_SETTINGS_ENABLE_MASTER_SLAVE_SWITCH|HCI_LINK_POLICY_SETTINGS_ENABLE_SNIFF_MODE), &Status);

         /* Regenerate IRK and DHK from the constant Identity Root Key. */
         GAP_LE_Diversify_Function(ApplicationStateInfo.BluetoothStackID, (Encryption_Key_t *)(&IR), 1,0, &IRK);
         GAP_LE_Diversify_Function(ApplicationStateInfo.BluetoothStackID, (Encryption_Key_t *)(&IR), 3, 0, &DHK);

         /* Initialize the GATT Service.                                */
         if((Result = GATT_Initialize(ApplicationStateInfo.BluetoothStackID, GATT_INITIALIZATION_FLAGS_SUPPORT_LE, GATT_Connection_Event_Callback, 0)) == 0)
         {
            /* Initialize the GAPS Service.                             */
            Result = GAPS_Initialize_Service(ApplicationStateInfo.BluetoothStackID, &ServiceID);
            if(Result > 0)
            {
               /* Save the Instance ID of the GAP Service.              */
               ApplicationStateInfo.GAPSInstanceID = (unsigned int)Result;

               /* Set the GAP Device Name and Device Appearance.        */
               GAPS_Set_Device_Name(ApplicationStateInfo.BluetoothStackID, ApplicationStateInfo.GAPSInstanceID, LE_APP_DEMO_NAME);
               GAPS_Set_Device_Appearance(ApplicationStateInfo.BluetoothStackID, ApplicationStateInfo.GAPSInstanceID, GAP_DEVICE_APPEARENCE_VALUE_HID_KEYBOARD);

               /* Attempt to configure the DIS Service Instance.        */
               Result = ConfigureDIS();
               if(!Result)
               {
                  /* Attempt to configure the BAS Service Instance.     */
                  Result = ConfigureBAS();
                  if(!Result)
                  {
                     /* Attempt to configure the HID Service Instance.  */
                     Result = ConfigureHIDS();
                     if(!Result)
                     {
                        /* Format the Advertising Data.                 */
                        FormatAdvertisingData(ApplicationStateInfo.BluetoothStackID);

                        /* Reset the HID Protocol Mode to the default   */
                        /* mode (Report Mode).                          */
                        ApplicationStateInfo.HIDProtocolMode = pmReport;

                        /* Configure the Battery Level to be at 100%.   */
                        ApplicationStateInfo.BatteryLevel    = 100;

                        /* Return success to the caller.                */
                        ret_val                              = 0;
                     }
                     else
                     {
                        /* The Stack was NOT initialized successfully,  */
                        /* inform the user and set the return value of  */
                        /* the initialization function to an error.     */
                        DisplayFunctionError("ConfigureHIDS", Result);

                        ret_val = UNABLE_TO_INITIALIZE_STACK;
                     }
                  }
                  else
                  {
                     /* The Stack was NOT initialized successfully,     */
                     /* inform the user and set the return value of the */
                     /* initialization function to an error.            */
                     DisplayFunctionError("ConfigureBAS", Result);

                     ret_val = UNABLE_TO_INITIALIZE_STACK;
                  }
               }
               else
               {
                  /* The Stack was NOT initialized successfully, inform */
                  /* the user and set the return value of the           */
                  /* initialization function to an error.               */
                  DisplayFunctionError("ConfigureDIS", Result);

                  ret_val = UNABLE_TO_INITIALIZE_STACK;
               }
            }
            else
            {
               /* The Stack was NOT initialized successfully, inform the*/
               /* user and set the return value of the initialization   */
               /* function to an error.                                 */
               DisplayFunctionError("GAPS_Initialize_Service", Result);

               ret_val = UNABLE_TO_INITIALIZE_STACK;
            }

            /* Shutdown the stack if an error occurred.                 */
            if(ret_val < 0)
               CloseStack();
         }
         else
         {
            /* The Stack was NOT initialized successfully, inform the   */
            /* user and set the return value of the initialization      */
            /* function to an error.                                    */
            DisplayFunctionError("GATT_Initialize", Result);

            /* Shutdown the stack.                                      */
            CloseStack();

            ret_val = UNABLE_TO_INITIALIZE_STACK;
         }
      }
      else
      {
         /* The Stack was NOT initialized successfully, inform the user */
         /* and set the return value of the initialization function to  */
         /* an error.                                                   */
         DisplayFunctionError("BSC_Initialize", Result);

         ApplicationStateInfo.BluetoothStackID = 0;

         ret_val                               = UNABLE_TO_INITIALIZE_STACK;
      }
   }
   else
   {
      /* One or more of the necessary parameters are invalid.           */
      ret_val = INVALID_PARAMETERS_ERROR;
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
   if(ApplicationStateInfo.BluetoothStackID)
   {
      /* Cleanup GAP Service Module.                                    */
      if(ApplicationStateInfo.GAPSInstanceID)
         GAPS_Cleanup_Service(ApplicationStateInfo.BluetoothStackID, ApplicationStateInfo.GAPSInstanceID);

      /* Cleanup DIS Service Module.                                    */
      if(ApplicationStateInfo.DISInstanceID)
         DIS_Cleanup_Service(ApplicationStateInfo.BluetoothStackID, ApplicationStateInfo.DISInstanceID);

      /* Cleanup BAS Service Module.                                    */
      if(ApplicationStateInfo.BASInstanceID)
         BAS_Cleanup_Service(ApplicationStateInfo.BluetoothStackID, ApplicationStateInfo.BASInstanceID);

      /* Free the Device Information List.                              */
      FreeDeviceInfoList(&DeviceInfoList);

      /* Cleanup GATT Module.                                           */
      GATT_Cleanup(ApplicationStateInfo.BluetoothStackID);

      /* Simply close the Stack                                         */
      BSC_Shutdown(ApplicationStateInfo.BluetoothStackID);

      /* Free BTPSKRNL allocated memory.                                */
      BTPS_DeInit();

      Display(("Stack Shutdown.\r\n"));

      /* Flag that the Stack is no longer initialized.                  */
      BTPS_MemInitialize(&ApplicationStateInfo, 0, sizeof(ApplicationStateInfo));

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
   if(ApplicationStateInfo.BluetoothStackID)
   {
      /* Set the LE Pairability Mode.                                   */

      /* Attempt to set the attached device to be pairable.             */
      Result = GAP_LE_Set_Pairability_Mode(ApplicationStateInfo.BluetoothStackID, lpmPairableMode);

      /* Next, check the return value of the GAP Set Pairability mode   */
      /* command for successful execution.                              */
      if(!Result)
      {
         /* The device has been set to pairable mode, now register an   */
         /* Authentication Callback to handle the Authentication events */
         /* if required.                                                */
         Result = GAP_LE_Register_Remote_Authentication(ApplicationStateInfo.BluetoothStackID, GAP_LE_Event_Callback, (unsigned long)0);

         /* Next, check the return value of the GAP Register Remote     */
         /* Authentication command for successful execution.            */
         if(Result)
         {
            /* An error occurred while trying to execute this function. */
            DisplayFunctionError("GAP_LE_Register_Remote_Authentication", Result);

            ret_val = Result;
         }
      }
      else
      {
         /* An error occurred while trying to make the device pairable. */
         DisplayFunctionError("GAP_LE_Set_Pairability_Mode", Result);

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

   /* The following function is used to map a Appearance Value to it's  */
   /* string representation.  This function returns TRUE on success or  */
   /* FALSE otherwise.                                                  */
static Boolean_t AppearanceToString(Word_t Appearance, char **String)
{
   Boolean_t    ret_val;
   unsigned int Index;

   /* Verify that the input parameters are semi-valid.                  */
   if(String)
   {
      for(Index=0,ret_val=FALSE;Index<NUMBER_OF_APPEARANCE_MAPPINGS;++Index)
      {
         if(AppearanceMappings[Index].Appearance == Appearance)
         {
            *String = AppearanceMappings[Index].String;
            ret_val = TRUE;
            break;
         }
      }
   }
   else
      ret_val = FALSE;

   return(ret_val);
}

   /* The following function is a utility function that provides a      */
   /* mechanism of populating discovered GAP Service Handles.           */
static void GAPSPopulateHandles(GAPS_Client_Info_t *ClientInfo, GATT_Service_Discovery_Indication_Data_t *ServiceInfo)
{
   unsigned int                       Index1;
   GATT_Characteristic_Information_t *CurrentCharacteristic;

   /* Verify that the input parameters are semi-valid.                  */
   if((ClientInfo) && (ServiceInfo) && (ServiceInfo->ServiceInformation.UUID.UUID_Type == guUUID_16) && (GAP_COMPARE_GAP_SERVICE_UUID_TO_UUID_16(ServiceInfo->ServiceInformation.UUID.UUID.UUID_16)))
   {
      /* Loop through all characteristics discovered in the service     */
      /* and populate the correct entry.                                */
      CurrentCharacteristic = ServiceInfo->CharacteristicInformationList;
      if(CurrentCharacteristic)
      {
         for(Index1=0;Index1<ServiceInfo->NumberOfCharacteristics;Index1++,CurrentCharacteristic++)
         {
            /* All GAP Service UUIDs are defined to be 16 bit UUIDs.    */
            if(CurrentCharacteristic->Characteristic_UUID.UUID_Type == guUUID_16)
            {
               /* Determine which characteristic this is.               */
               if(!GAP_COMPARE_GAP_DEVICE_NAME_UUID_TO_UUID_16(CurrentCharacteristic->Characteristic_UUID.UUID.UUID_16))
               {
                  if(!GAP_COMPARE_GAP_DEVICE_APPEARANCE_UUID_TO_UUID_16(CurrentCharacteristic->Characteristic_UUID.UUID.UUID_16))
                     continue;
                  else
                  {
                     ClientInfo->DeviceAppearanceHandle = CurrentCharacteristic->Characteristic_Handle;
                     continue;
                  }
               }
               else
               {
                  ClientInfo->DeviceNameHandle = CurrentCharacteristic->Characteristic_Handle;
                  continue;
               }
            }
         }
      }
   }
}

   /* The following function is a utility function that formats the     */
   /* advertising data.                                                 */
static void FormatAdvertisingData(unsigned int BluetoothStackID)
{
   int                     Result;
   unsigned int            Length;
   unsigned int            StringLength;
   union
   {
      Advertising_Data_t   AdvertisingData;
      Scan_Response_Data_t ScanResponseData;
   } Advertisement_Data_Buffer;

   /* First, check that valid Bluetooth Stack ID exists.                */
   if(BluetoothStackID)
   {
      BTPS_MemInitialize(&(Advertisement_Data_Buffer.AdvertisingData), 0, sizeof(Advertising_Data_t));

      Length = 0;

      /* Set the Flags A/D Field (1 byte type and 1 byte Flags.         */
      Advertisement_Data_Buffer.AdvertisingData.Advertising_Data[0]  = 2;
      Advertisement_Data_Buffer.AdvertisingData.Advertising_Data[1]  = HCI_LE_ADVERTISING_REPORT_DATA_TYPE_FLAGS;
      Advertisement_Data_Buffer.AdvertisingData.Advertising_Data[2]  = HCI_LE_ADVERTISING_FLAGS_GENERAL_DISCOVERABLE_MODE_FLAGS_BIT_MASK;
      Advertisement_Data_Buffer.AdvertisingData.Advertising_Data[2] |= HCI_LE_ADVERTISING_FLAGS_BR_EDR_NOT_SUPPORTED_FLAGS_BIT_MASK;

      Length += Advertisement_Data_Buffer.AdvertisingData.Advertising_Data[0] + 1;

      /* Configure the Device Appearance value.                         */
      Advertisement_Data_Buffer.AdvertisingData.Advertising_Data[Length]   = 3;
      Advertisement_Data_Buffer.AdvertisingData.Advertising_Data[Length+1] = HCI_LE_ADVERTISING_REPORT_DATA_TYPE_APPEARANCE;
      ASSIGN_HOST_WORD_TO_LITTLE_ENDIAN_UNALIGNED_WORD(&(Advertisement_Data_Buffer.AdvertisingData.Advertising_Data[Length+2]), GAP_DEVICE_APPEARENCE_VALUE_HID_KEYBOARD);

      Length += Advertisement_Data_Buffer.AdvertisingData.Advertising_Data[Length] + 1;

      /* Configure the services that we say we support.                 */
      Advertisement_Data_Buffer.AdvertisingData.Advertising_Data[Length]   = 1 + (UUID_16_SIZE*3);
      Advertisement_Data_Buffer.AdvertisingData.Advertising_Data[Length+1] = HCI_LE_ADVERTISING_REPORT_DATA_TYPE_16_BIT_SERVICE_UUID_COMPLETE;

      HIDS_ASSIGN_HIDS_SERVICE_UUID_16(&(Advertisement_Data_Buffer.AdvertisingData.Advertising_Data[Length+2]));
      BAS_ASSIGN_BAS_SERVICE_UUID_16(&(Advertisement_Data_Buffer.AdvertisingData.Advertising_Data[Length+4]));
      DIS_ASSIGN_DIS_SERVICE_UUID_16(&(Advertisement_Data_Buffer.AdvertisingData.Advertising_Data[Length+6]));

      Length += Advertisement_Data_Buffer.AdvertisingData.Advertising_Data[Length] + 1;

      /* Set the Device Name String.                                    */
      StringLength = BTPS_StringLength(LE_APP_DEMO_NAME);
      if(StringLength < (ADVERTISING_DATA_MAXIMUM_SIZE - Length - 2))
         Advertisement_Data_Buffer.AdvertisingData.Advertising_Data[Length+1] = HCI_LE_ADVERTISING_REPORT_DATA_TYPE_LOCAL_NAME_COMPLETE;
      else
      {
         Advertisement_Data_Buffer.AdvertisingData.Advertising_Data[Length+1] = HCI_LE_ADVERTISING_REPORT_DATA_TYPE_LOCAL_NAME_SHORTENED;
         StringLength = (ADVERTISING_DATA_MAXIMUM_SIZE - Length - 2);
      }

      Advertisement_Data_Buffer.AdvertisingData.Advertising_Data[Length] = StringLength+1;

      BTPS_MemCopy(&(Advertisement_Data_Buffer.AdvertisingData.Advertising_Data[Length+2]), LE_APP_DEMO_NAME, StringLength);

      Length += Advertisement_Data_Buffer.AdvertisingData.Advertising_Data[Length] + 1;

      /* Write thee advertising data to the chip.                       */
      Result = GAP_LE_Set_Advertising_Data(BluetoothStackID, Length, &(Advertisement_Data_Buffer.AdvertisingData));
      if(!Result)
         Display(("Advertising Data Configured Successfully.\r\n"));
      else
         Display(("GAP_LE_Set_Advertising_Data(dtAdvertising) returned %d.\r\n", Result));
   }
}

   /* The following function is a utility function that starts an       */
   /* advertising process.                                              */
static int StartAdvertising(unsigned int BluetoothStackID)
{
   int                                 ret_val;
   GAP_LE_Advertising_Parameters_t     AdvertisingParameters;
   GAP_LE_Connectability_Parameters_t  ConnectabilityParameters;

   /* First, check that valid Bluetooth Stack ID exists.                */
   if(BluetoothStackID)
   {
      /* Set up the advertising parameters.                             */
      AdvertisingParameters.Advertising_Channel_Map   = HCI_LE_ADVERTISING_CHANNEL_MAP_DEFAULT;
      AdvertisingParameters.Scan_Request_Filter       = fpNoFilter;
      AdvertisingParameters.Connect_Request_Filter    = fpNoFilter;
      AdvertisingParameters.Advertising_Interval_Min  = 50;
      AdvertisingParameters.Advertising_Interval_Max  = 100;

      /* Configure the Connectability Parameters.                       */
      /* * NOTE * Since we do not ever put ourselves to be direct       */
      /*          connectable then we will set the DirectAddress to all */
      /*          0s.                                                   */
      ConnectabilityParameters.Connectability_Mode   = lcmConnectable;
      ConnectabilityParameters.Own_Address_Type      = latPublic;
      ConnectabilityParameters.Direct_Address_Type   = latPublic;
      ASSIGN_BD_ADDR(ConnectabilityParameters.Direct_Address, 0, 0, 0, 0, 0, 0);

      /* Now enable advertising.                                        */
      ret_val = GAP_LE_Advertising_Enable(BluetoothStackID, TRUE, &AdvertisingParameters, &ConnectabilityParameters, GAP_LE_Event_Callback, 0);
      if(!ret_val)
         Display(("GAP_LE_Advertising_Enable success.\r\n"));
      else
      {
         if(ret_val == -66)
         {
            Display(("GAP_LE_Advertising: Already Enabled.\r\n"));
         }
         else
         {
            Display(("GAP_LE_Advertising_Enable returned %d.\r\n", ret_val));
         }

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

   /* The following function is a utility function that stops an        */
   /* advertising process.                                              */
static int StopAdvertising(unsigned int BluetoothStackID)
{
   int ret_val;

   /* Now disable advertising.                                          */
   ret_val = GAP_LE_Advertising_Disable(BluetoothStackID);
   if(!ret_val)
      Display(("GAP_LE_Advertising_Disabled success.\r\n"));
   else
   {
      if(ret_val == -1)
      {
         Display(("GAP_LE_Advertising: Already Disabled.\r\n"));
      }
      else
      {
         Display(("GAP_LE_Advertising_Disabled returned %d.\r\n", ret_val));
      }
      ret_val = FUNCTION_ERROR;
   }

   return ret_val;
}

   /* The following function is provided to allow a mechanism to        */
   /* disconnect a currently connected device.                          */
static int DisconnectLEDevice(unsigned int BluetoothStackID, BD_ADDR_t BD_ADDR)
{
   int Result;

   /* First, determine if the input parameters appear to be semi-valid. */
   if((BluetoothStackID) && (!COMPARE_NULL_BD_ADDR(BD_ADDR)))
   {
      /* Make sure that a device with address BD_ADDR is connected      */
      if(ApplicationStateInfo.Flags & APPLICATION_STATE_INFO_FLAGS_LE_CONNECTED)
      {
         Result = GAP_LE_Disconnect(BluetoothStackID, BD_ADDR);
         if(!Result)
         {
            Display(("Disconnect Request successful.\r\n"));
         }
         else
         {
            /* Unable to disconnect device.                             */
            Display(("Unable to disconnect device: %d.\r\n", Result));
         }
      }
      else
      {
         /* Device not connected.                                       */
         Display(("Device is not connected.\r\n"));

         Result = 0;
      }
   }
   else
      Result = -1;

   return(Result);
}

   /* The following function provides a mechanism to configure a        */
   /* Pairing Capabilities structure with the application's pairing     */
   /* parameters.                                                       */
static void ConfigureCapabilities(GAP_LE_Pairing_Capabilities_t *Capabilities)
{
   /* Make sure the Capabilities pointer is semi-valid.                 */
   if(Capabilities)
   {
      /* Configure the Pairing Capabilities structure.                  */
      Capabilities->Bonding_Type                      = lbtBonding;
      Capabilities->IO_Capability                     = LE_Parameters.IOCapability;
      Capabilities->MITM                              = LE_Parameters.MITMProtection;
      Capabilities->OOB_Present                       = LE_Parameters.OOBDataPresent;

      /* ** NOTE ** This application always requests that we use the    */
      /*            maximum encryption because this feature is not a    */
      /*            very good one, if we set less than the maximum we   */
      /*            will internally in GAP generate a key of the        */
      /*            maximum size (we have to do it this way) and then   */
      /*            we will zero out how ever many of the MSBs          */
      /*            necessary to get the maximum size.  Also as a slave */
      /*            we will have to use Non-Volatile Memory (per device */
      /*            we are paired to) to store the negotiated Key Size. */
      /*            By requesting the maximum (and by not storing the   */
      /*            negotiated key size if less than the maximum) we    */
      /*            allow the slave to power cycle and regenerate the   */
      /*            LTK for each device it is paired to WITHOUT storing */
      /*            any information on the individual devices we are    */
      /*            paired to.                                          */
      Capabilities->Maximum_Encryption_Key_Size       = GAP_LE_MAXIMUM_ENCRYPTION_KEY_SIZE;

      /* This application only demonstrates using Long Term Key's (LTK) */
      /* for encryption of a LE Link (and receiving and IRK for         */
      /* identifying the remote device if it uses a resolvable random   */
      /* address), however we could request and send all possible keys  */
      /* here if we wanted to.                                          */
      Capabilities->Receiving_Keys.Encryption_Key     = FALSE;
      Capabilities->Receiving_Keys.Identification_Key = TRUE;
      Capabilities->Receiving_Keys.Signing_Key        = FALSE;

      Capabilities->Sending_Keys.Encryption_Key       = TRUE;
      Capabilities->Sending_Keys.Identification_Key   = FALSE;
      Capabilities->Sending_Keys.Signing_Key          = FALSE;
   }
}
   /* The following function provides a mechanism of sending a Slave    */
   /* Pairing Response to a Master's Pairing Request.                   */
static int SlavePairingRequestResponse(unsigned int BluetoothStackID, BD_ADDR_t BD_ADDR)
{
   int                                          ret_val;
   BoardStr_t                                   BoardStr;
   GAP_LE_Authentication_Response_Information_t AuthenticationResponseData;

   /* Make sure a Bluetooth Stack is open.                              */
   if(BluetoothStackID)
   {
      BD_ADDRToStr(BD_ADDR, BoardStr);
      Display(("Sending Pairing Response to %s.\r\n", BoardStr));

      /* We must be the slave if we have received a Pairing Request     */
      /* thus we will respond with our capabilities.                    */
      AuthenticationResponseData.GAP_LE_Authentication_Type = larPairingCapabilities;
      AuthenticationResponseData.Authentication_Data_Length = GAP_LE_PAIRING_CAPABILITIES_SIZE;

      /* Configure the Application Pairing Parameters.                  */
      ConfigureCapabilities(&(AuthenticationResponseData.Authentication_Data.Pairing_Capabilities));

      /* Attempt to pair to the remote device.                          */
      ret_val = GAP_LE_Authentication_Response(BluetoothStackID, BD_ADDR, &AuthenticationResponseData);
      if(ret_val)
         Display(("Error - GAP_LE_Authentication_Response returned %d.\r\n", ret_val));
   }
   else
      ret_val = INVALID_STACK_ID_ERROR;

   return(ret_val);
}

   /* The following function provides a mechanism of attempting to      */
   /* re-established security with a previously paired master..         */
static int SlaveSecurityReEstablishment(unsigned int BluetoothStackID, BD_ADDR_t BD_ADDR)
{
   int                           ret_val;
   GAP_LE_Security_Information_t SecurityInformation;

   /* Make sure a Bluetooth Stack is open.                              */
   if(BluetoothStackID)
   {
      /* Configure the Security Information.                            */
      SecurityInformation.Local_Device_Is_Master                              = FALSE;
      SecurityInformation.Security_Information.Slave_Information.Bonding_Type = lbtBonding;
      SecurityInformation.Security_Information.Slave_Information.MITM         = LE_Parameters.MITMProtection;

      /* Attempt to pair to the remote device.                          */
      ret_val = GAP_LE_Reestablish_Security(BluetoothStackID, BD_ADDR, &SecurityInformation, GAP_LE_Event_Callback, 0);
      if(!ret_val)
         Display(("GAP_LE_Reestablish_Security success.\r\n"));
      else
         Display(("Error - GAP_LE_Reestablish_Security returned %d.\r\n", ret_val));
   }
   else
      ret_val = INVALID_STACK_ID_ERROR;

   return(ret_val);
}

   /* The following function is provided to allow a mechanism of        */
   /* responding to a request for Encryption Information to send to a   */
   /* remote device.                                                    */
static int EncryptionInformationRequestResponse(unsigned int BluetoothStackID, BD_ADDR_t BD_ADDR, Byte_t KeySize, GAP_LE_Authentication_Response_Information_t *GAP_LE_Authentication_Response_Information)
{
   int    ret_val;
   Word_t LocalDiv;

   /* Make sure a Bluetooth Stack is open.                              */
   if(BluetoothStackID)
   {
      /* Make sure the input parameters are semi-valid.                 */
      if((!COMPARE_NULL_BD_ADDR(BD_ADDR)) && (GAP_LE_Authentication_Response_Information))
      {
         Display(("   Calling GAP_LE_Generate_Long_Term_Key.\r\n"));

         /* Generate a new LTK, EDIV and Rand tuple.                    */
         ret_val = GAP_LE_Generate_Long_Term_Key(BluetoothStackID, (Encryption_Key_t *)(&DHK), (Encryption_Key_t *)(&ER), &(GAP_LE_Authentication_Response_Information->Authentication_Data.Encryption_Information.LTK), &LocalDiv, &(GAP_LE_Authentication_Response_Information->Authentication_Data.Encryption_Information.EDIV), &(GAP_LE_Authentication_Response_Information->Authentication_Data.Encryption_Information.Rand));
         if(!ret_val)
         {
            Display(("   Encryption Information Request Response.\r\n"));

            /* Response to the request with the LTK, EDIV and Rand      */
            /* values.                                                  */
            GAP_LE_Authentication_Response_Information->GAP_LE_Authentication_Type                                     = larEncryptionInformation;
            GAP_LE_Authentication_Response_Information->Authentication_Data_Length                                     = GAP_LE_ENCRYPTION_INFORMATION_DATA_SIZE;
            GAP_LE_Authentication_Response_Information->Authentication_Data.Encryption_Information.Encryption_Key_Size = KeySize;

            ret_val = GAP_LE_Authentication_Response(BluetoothStackID, BD_ADDR, GAP_LE_Authentication_Response_Information);
            if(!ret_val)
            {
               Display(("   GAP_LE_Authentication_Response (larEncryptionInformation) success.\r\n", ret_val));
            }
            else
            {
               Display(("   Error - SM_Generate_Long_Term_Key returned %d.\r\n", ret_val));
            }
         }
         else
         {
            Display(("   Error - SM_Generate_Long_Term_Key returned %d.\r\n", ret_val));
         }
      }
      else
      {
         Display(("Invalid Parameters.\r\n"));

         ret_val = INVALID_PARAMETERS_ERROR;
      }
   }
   else
   {
      Display(("Stack ID Invalid.\r\n"));

      ret_val = INVALID_STACK_ID_ERROR;
   }

   return(ret_val);
}

   /* The following function is responsible for displaying the current  */
   /* Command Options for the HOG Device.  This function displays the   */
   /* current Command Options that are available and always returns     */
   /* zero.                                                             */
static int DisplayHelp(ParameterList_t *TempParam)
{
   Display(("\r\n"));
   Display(("******************************************************************\r\n"));
   Display(("* HOGPDemo                                                       *\r\n"));
   Display(("******************************************************************\r\n"));
   Display(("* Command Options General: Help,                                 *\r\n"));
   Display(("*                          GetLocalName,                         *\r\n"));
   Display(("*                          GetLocalAddress,                      *\r\n"));
   Display(("*                          GetLocalAppearance,                   *\r\n"));
   Display(("*                          SetDiscoverabilityMode,               *\r\n"));
   Display(("*                          SetConnectabilityMode,                *\r\n"));
   Display(("*                          SetPairabilityMode,                   *\r\n"));
   Display(("*                          ChangePairingParameters,              *\r\n"));
   Display(("*                          Advertise,                            *\r\n"));
   Display(("*                          PassKeyResponse,                      *\r\n"));
   Display(("*                          Disconnect,                           *\r\n"));
   Display(("*                          DiscoverGAPS,                         *\r\n"));
   Display(("*                          GetRemoteName,                        *\r\n"));
   Display(("*                          GetRemoteAppearance,                  *\r\n"));
   Display(("* Command Options Device:  SetBatteryLevel,                      *\r\n"));
   Display(("*                          NotifyBatteryLevel,                   *\r\n"));
   Display(("*                          NotifyKeyboardReport                  *\r\n"));
   Display(("******************************************************************\r\n"));

   return(0);
}

   /* The following function is responsible for setting the             */
   /* Discoverability Mode of the local device.  This function returns  */
   /* zero on successful execution and a negative value on all errors.  */
static int SetLEDiscoverabilityMode(ParameterList_t *TempParam)
{
   int                        ret_val;
   GAP_Discoverability_Mode_t DiscoverabilityMode;

   /* First, check that valid Bluetooth Stack ID exists.                */
   if(ApplicationStateInfo.BluetoothStackID)
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

         /* Set the LE Discoverability Mode.                            */
         LE_Parameters.DiscoverabilityMode = DiscoverabilityMode;

         /* The Mode was changed successfully.                          */
         Display(("Discoverability: %s.\r\n", (DiscoverabilityMode == dmNonDiscoverableMode)?"Non":((DiscoverabilityMode == dmGeneralDiscoverableMode)?"General":"Limited")));

         /* Flag success to the caller.                                 */
         ret_val = 0;
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
static int SetLEConnectabilityMode(ParameterList_t *TempParam)
{
   int ret_val;

   /* First, check that valid Bluetooth Stack ID exists.                */
   if(ApplicationStateInfo.BluetoothStackID)
   {
      /* Make sure that all of the parameters required for this function*/
      /* appear to be at least semi-valid.                              */
      if((TempParam) && (TempParam->NumberofParameters >= 1) && (TempParam->Params[0].intParam >= 0) && (TempParam->Params[0].intParam <= 1))
      {
         /* Parameters appear to be valid, map the specified parameters */
         /* into the API specific parameters.                           */
         /* * NOTE * The Connectability Mode in LE is only applicable   */
         /*          when advertising, if a device is not advertising   */
         /*          it is not connectable.                             */
         if(TempParam->Params[0].intParam == 0)
            LE_Parameters.ConnectableMode = lcmNonConnectable;
         else
            LE_Parameters.ConnectableMode = lcmConnectable;

         /* The Mode was changed successfully.                          */
         Display(("Connectability Mode: %s.\r\n", (LE_Parameters.ConnectableMode == lcmNonConnectable)?"Non Connectable":"Connectable"));

         /* Flag success to the caller.                                 */
         ret_val = 0;
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
static int SetLEPairabilityMode(ParameterList_t *TempParam)
{
   int                        Result;
   int                        ret_val;
   char                      *Mode;
   GAP_LE_Pairability_Mode_t  PairabilityMode;

   /* First, check that valid Bluetooth Stack ID exists.                */
   if(ApplicationStateInfo.BluetoothStackID)
   {
      /* Make sure that all of the parameters required for this function*/
      /* appear to be at least semi-valid.                              */
      if((TempParam) && (TempParam->NumberofParameters > 0) && (TempParam->Params[0].intParam >= 0) && (TempParam->Params[0].intParam <= 1))
      {
         /* Parameters appear to be valid, map the specified parameters */
         /* into the API specific parameters.                           */
         if(TempParam->Params[0].intParam == 0)
         {
            PairabilityMode = lpmNonPairableMode;
            Mode            = "lpmNonPairableMode";
         }
         else
         {
            PairabilityMode = lpmPairableMode;
            Mode            = "lpmPairableMode";
         }

         /* Parameters mapped, now set the Pairability Mode.            */
         Result = GAP_LE_Set_Pairability_Mode(ApplicationStateInfo.BluetoothStackID, PairabilityMode);

         /* Next, check the return value to see if the command was      */
         /* issued successfully.                                        */
         if(Result >= 0)
         {
            /* The Mode was changed successfully.                       */
            Display(("Pairability Mode Changed to %s.\r\n", Mode));

            /* If Secure Simple Pairing has been enabled, inform the    */
            /* user of the current Secure Simple Pairing parameters.    */
            if(PairabilityMode == lpmPairableMode)
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
         DisplayUsage("SetPairabilityMode [Mode (0 = Non Pairable, 1 = Pairable]");

         ret_val = INVALID_PARAMETERS_ERROR;
      }
   }
   else
   {
      Display(("Invalid Stack ID.\r\n"));

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
   int        Result;
   int        ret_val;
   BD_ADDR_t  BD_ADDR;
   BoardStr_t Function_BoardStr;

   /* First, check that valid Bluetooth Stack ID exists.                */
   if(ApplicationStateInfo.BluetoothStackID)
   {
      /* Attempt to submit the command.                                 */
      Result = GAP_Query_Local_BD_ADDR(ApplicationStateInfo.BluetoothStackID, &BD_ADDR);

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


   /* The following function is responsible for changing the Secure     */
   /* Simple Pairing Parameters that are exchanged during the Pairing   */
   /* procedure when Secure Simple Pairing (Security Level 4) is used.  */
   /* This function returns zero on successful execution and a negative */
   /* value on all errors.                                              */
static int ChangeLEPairingParameters(ParameterList_t *TempParam)
{
   int ret_val;

   /* First, check that valid Bluetooth Stack ID exists.                */
   if(ApplicationStateInfo.BluetoothStackID)
   {
      /* Make sure that all of the parameters required for this function*/
      /* appear to be at least semi-valid.                              */
      if((TempParam) && (TempParam->Params[0].intParam >= 0) && (TempParam->Params[0].intParam <= 4))
      {
         /* Parameters appear to be valid, map the specified parameters */
         /* into the API specific parameters.                           */
         if(TempParam->Params[0].intParam == 0)
            LE_Parameters.IOCapability = licDisplayOnly;
         else
         {
            if(TempParam->Params[0].intParam == 1)
               LE_Parameters.IOCapability = licDisplayYesNo;
            else
            {
               if(TempParam->Params[0].intParam == 2)
                  LE_Parameters.IOCapability = licKeyboardOnly;
               else
               {
                  if(TempParam->Params[0].intParam == 3)
                     LE_Parameters.IOCapability = licNoInputNoOutput;
                  else
                     LE_Parameters.IOCapability = licKeyboardDisplay;
               }
            }
         }

         /* Finally map the Man in the Middle (MITM) Protection value.  */
         LE_Parameters.MITMProtection = (Boolean_t)(TempParam->Params[1].intParam?TRUE:FALSE);

         /* Inform the user of the New I/O Capabilities.                */
         DisplayIOCapabilities();

         /* Flag success to the caller.                                 */
         ret_val = 0;
      }
      else
      {
         /* One or more of the necessary parameters is/are invalid.     */
         Display(("Usage: ChangePairingParameters [I/O Capability (0 = Display Only, 1 = Display Yes/No, 2 = Keyboard Only, 3 = No Input/Output, 4 = Keyboard/Display)] [MITM Requirement (0 = No, 1 = Yes)].\r\n"));

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
   /* Authentication Response with a Pass Key value specified via the   */
   /* input parameter.  This function returns zero on successful        */
   /* execution and a negative value on all errors.                     */
static int LEPassKeyResponse(ParameterList_t *TempParam)
{
   int                                          Result;
   int                                          ret_val;
   GAP_LE_Authentication_Response_Information_t GAP_LE_Authentication_Response_Information;

   /* First, check that valid Bluetooth Stack ID exists.                */
   if(ApplicationStateInfo.BluetoothStackID)
   {
      /* First, check to see if there is an on-going Pairing operation  */
      /* active.                                                        */
      if(!COMPARE_NULL_BD_ADDR(ApplicationStateInfo.LEConnectionInfo.BD_ADDR))
      {
         /* Make sure that all of the parameters required for this      */
         /* function appear to be at least semi-valid.                  */
         if((TempParam) && (TempParam->NumberofParameters > 0) && (BTPS_StringLength(TempParam->Params[0].strParam) <= GAP_LE_PASSKEY_MAXIMUM_NUMBER_OF_DIGITS))
         {
           /* Verify that we are still waiting on a passkey.            */
           if(ApplicationStateInfo.LEConnectionInfo.Flags & CONNECTION_INFO_FLAGS_CONNECTION_AWAITING_PASSKEY)
           {
              /* Flag that we are no longer waiting on a passkey.       */
              ApplicationStateInfo.LEConnectionInfo.Flags &= ~CONNECTION_INFO_FLAGS_CONNECTION_AWAITING_PASSKEY;

               /* Parameters appear to be valid, go ahead and populate  */
               /* the response structure.                               */
               GAP_LE_Authentication_Response_Information.GAP_LE_Authentication_Type  = larPasskey;
               GAP_LE_Authentication_Response_Information.Authentication_Data_Length  = (Byte_t)(sizeof(DWord_t));
               GAP_LE_Authentication_Response_Information.Authentication_Data.Passkey = (DWord_t)(TempParam->Params[0].intParam);

               /* Submit the Authentication Response.                   */
               Result = GAP_LE_Authentication_Response(ApplicationStateInfo.BluetoothStackID, ApplicationStateInfo.LEConnectionInfo.BD_ADDR, &GAP_LE_Authentication_Response_Information);

               /* Check the return value for the submitted command for  */
               /* success.                                              */
               if(!Result)
               {
                  /* Operation was successful, inform the user.         */
                  DisplayFunctionSuccess("Passkey Response");

                  /* Flag success to the caller.                        */
                  ret_val = 0;
               }
               else
               {
                  /* Inform the user that the Authentication Response   */
                  /* was not successful.                                */
                  DisplayFunctionError("GAP_LE_Authentication_Response", Result);

                  ret_val = FUNCTION_ERROR;
               }
           }
           else
           {
              Display(("LEPassKeyResponse: Not waiting for a passkey.\r\n"));

              ret_val = INVALID_PARAMETERS_ERROR;
           }
         }
         else
         {
            /* One or more of the necessary parameters is/are invalid.  */
            Display(("PassKeyResponse [Numeric Passkey(0 - 999999)].\r\n"));

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

   /* The following function is responsible for enabling LE             */
   /* Advertisements.  This function returns zero on successful         */
   /* execution and a negative value on all errors.                     */
static int AdvertiseLE(ParameterList_t *TempParam)
{
   int ret_val = 0;

   /* First, check that valid Bluetooth Stack ID exists.                */
   if(ApplicationStateInfo.BluetoothStackID)
   {
      /* Make sure that all of the parameters required for this function*/
      /* appear to be at least semi-valid.                              */
      if((TempParam) && (TempParam->NumberofParameters > 0) && (TempParam->Params[0].intParam >= 0) && (TempParam->Params[0].intParam <= 1))
      {
         /* Determine whether to enable or disable Advertising.         */
         if(TempParam->Params[0].intParam == 0)
         {
            /* Stop the advertising.                                    */
            ret_val = StopAdvertising(ApplicationStateInfo.BluetoothStackID);
         }
         else
         {
            ret_val = StartAdvertising(ApplicationStateInfo.BluetoothStackID);
         }
      }
      else
      {
         DisplayUsage("Advertise [(0 = Disable, 1 = Enable)].");

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

   /* The following function is responsible for disconnecting to an LE  */
   /* device.  This function returns zero if successful and a negative  */
   /* value if an error occurred.                                       */
static int DisconnectLE(ParameterList_t *TempParam)
{
   int       ret_val;
   BD_ADDR_t BD_ADDR;

   /* First, check that valid Bluetooth Stack ID exists.                */
   if(ApplicationStateInfo.BluetoothStackID)
   {
      /*Next, make sure that a valid device address exists.             */
      if((TempParam) && (TempParam->NumberofParameters > 0) && (TempParam->Params[0].strParam) && (BTPS_StringLength(TempParam->Params[0].strParam) >= (sizeof(BD_ADDR_t)*2)))
      {
         /* Convert the parameter to a Bluetooth Device Address.        */
         StrToBD_ADDR(TempParam->Params[0].strParam, &BD_ADDR);

         /* Attempt to disconnect the device.                           */
         if(!DisconnectLEDevice(ApplicationStateInfo.BluetoothStackID, ApplicationStateInfo.LEConnectionInfo.BD_ADDR))
            ret_val = 0;
         else
            ret_val = FUNCTION_ERROR;
      }
      else
      {
         /* Invalid parameters specified so flag an error to the user.  */
         Display(("Usage: DisconnectLE [BD_ADDR].\r\n"));

         /* Flag that an error occurred while submitting the command.   */
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

   /* The following function is responsible for performing a GAP Service*/
   /* Service Discovery Operation.  This function will return zero on   */
   /* successful execution and a negative value on errors.              */
static int DiscoverGAPS(ParameterList_t *TempParam)
{
   int           ret_val;
   BD_ADDR_t     BD_ADDR;
   GATT_UUID_t   UUID[1];
   DeviceInfo_t *DeviceInfo;

   /* Next, make sure that a valid device address exists.               */
   if((TempParam) && (TempParam->NumberofParameters > 0) && (TempParam->Params[0].strParam) && (BTPS_StringLength(TempParam->Params[0].strParam) >= (sizeof(BD_ADDR_t)*2)))
   {
      /* Convert the parameter to a Bluetooth Device Address.           */
      StrToBD_ADDR(TempParam->Params[0].strParam, &BD_ADDR);

      /* Get the device info for the connection device.                 */
      if((DeviceInfo = SearchDeviceInfoEntryByBD_ADDR(&DeviceInfoList, ApplicationStateInfo.LEConnectionInfo.AddressType, ApplicationStateInfo.LEConnectionInfo.BD_ADDR)) != NULL)
      {
         /* Verify that no service discovery is outstanding for this    */
         /* device.                                                     */
         if(!(DeviceInfo->Flags & DEVICE_INFO_FLAGS_SERVICE_DISCOVERY_OUTSTANDING))
         {
            /* Configure the filter so that only the SPP LE Service is  */
            /* discovered.                                              */
            UUID[0].UUID_Type = guUUID_16;
            GAP_ASSIGN_GAP_SERVICE_UUID_16(UUID[0].UUID.UUID_16);

            /* Start the service discovery process.                     */
            ret_val = GATT_Start_Service_Discovery(ApplicationStateInfo.BluetoothStackID, ApplicationStateInfo.LEConnectionInfo.ConnectionID, (sizeof(UUID)/sizeof(GATT_UUID_t)), UUID, GATT_Service_Discovery_Event_Callback, 0);
            if(!ret_val)
            {
               /* Display success message.                              */
               Display(("GATT_Start_Service_Discovery success.\r\n"));

               /* Flag that a Service Discovery Operation is            */
               /* outstanding.                                          */
               DeviceInfo->Flags |= DEVICE_INFO_FLAGS_SERVICE_DISCOVERY_OUTSTANDING;
            }
            else
            {
               /* An error occur so just clean-up.                      */
               Display(("Error - GATT_Start_Service_Discovery returned %d.\r\n", ret_val));

               ret_val = FUNCTION_ERROR;
            }
         }
         else
         {
            Display(("Service Discovery Operation Outstanding for Device.\r\n"));

            ret_val = FUNCTION_ERROR;
         }
      }
      else
      {
         Display(("No Device Info.\r\n"));

         ret_val = FUNCTION_ERROR;
      }
   }
   else
   {
      /* Invalid parameters specified so flag an error to the user.     */
      Display(("Usage: DiscoverGAPS [BD_ADDR].\r\n"));

      /* Flag that an error occurred while submitting the command.      */
      ret_val = INVALID_PARAMETERS_ERROR;
   }

   return(ret_val);
}

   /* The following function is responsible for reading the current     */
   /* Local Device Name.  This function will return zero on successful  */
   /* execution and a negative value on errors.                         */
static int ReadLocalName(ParameterList_t *TempParam)
{
   int  ret_val;
   char NameBuffer[BTPS_CONFIGURATION_GAPS_MAXIMUM_SUPPORTED_DEVICE_NAME+1];

   /* Verify that the GAP Service is registered.                        */
   if(ApplicationStateInfo.GAPSInstanceID)
   {
      /* Initialize the Name Buffer to all zeros.                       */
      BTPS_MemInitialize(NameBuffer, 0, sizeof(NameBuffer));

      /* Query the Local Name.                                          */
      ret_val = GAPS_Query_Device_Name(ApplicationStateInfo.BluetoothStackID, ApplicationStateInfo.GAPSInstanceID, NameBuffer);
      if(!ret_val)
         Display(("Device Name: %s.\r\n", NameBuffer));
      else
      {
         Display(("Error - GAPS_Query_Device_Name returned %d.\r\n", ret_val));

         ret_val = FUNCTION_ERROR;
      }
   }
   else
   {
      Display(("GAP Service not registered.\r\n"));

      ret_val = FUNCTION_ERROR;
   }

   return(ret_val);
}

   /* The following function is responsible for reading the Device Name */
   /* for the currently connected remote device.  This function will    */
   /* return zero on successful execution and a negative value on       */
   /* errors.                                                           */
static int ReadRemoteName(ParameterList_t *TempParam)
{
   int           ret_val;
   BD_ADDR_t     BD_ADDR;
   DeviceInfo_t *DeviceInfo;

   /* Next, make sure that a valid device address exists.               */
   if((TempParam) && (TempParam->NumberofParameters > 0) && (TempParam->Params[0].strParam) && (BTPS_StringLength(TempParam->Params[0].strParam) >= (sizeof(BD_ADDR_t)*2)))
   {
      /* Convert the parameter to a Bluetooth Device Address.           */
      StrToBD_ADDR(TempParam->Params[0].strParam, &BD_ADDR);

      /* Get the device info for the connection device.                 */
      if((DeviceInfo = SearchDeviceInfoEntryByBD_ADDR(&DeviceInfoList, ApplicationStateInfo.LEConnectionInfo.AddressType,  ApplicationStateInfo.LEConnectionInfo.BD_ADDR)) != NULL)
      {
         /* Verify that we discovered the Device Name Handle.           */
         if(DeviceInfo->GAPSClientInfo.DeviceNameHandle)
         {
            /* Attempt to read the remote device name.                  */
            ret_val = GATT_Read_Value_Request(ApplicationStateInfo.BluetoothStackID, ApplicationStateInfo.LEConnectionInfo.ConnectionID, DeviceInfo->GAPSClientInfo.DeviceNameHandle, GATT_ClientEventCallback_GAPS, (unsigned long)DeviceInfo->GAPSClientInfo.DeviceNameHandle);
            if(ret_val > 0)
            {
               Display(("Attempting to read Remote Device Name.\r\n"));

               ret_val = 0;
            }
            else
            {
               Display(("Error - GATT_Read_Value_Request returned %d.\r\n", ret_val));

               ret_val = FUNCTION_ERROR;
            }
         }
         else
         {
            Display(("GAP Service Device Name Handle not discovered.\r\n"));

            ret_val = FUNCTION_ERROR;
         }
      }
      else
      {
         Display(("No Device Info.\r\n"));

         ret_val = FUNCTION_ERROR;
      }
   }
   else
   {
      /* Invalid parameters specified so flag an error to the user.     */
      Display(("Usage: GetLERemoteName [BD_ADDR].\r\n"));

      /* Flag that an error occurred while submitting the command.      */
      ret_val = INVALID_PARAMETERS_ERROR;
   }

   return(ret_val);
}

   /* The following function is responsible for reading the Local Device*/
   /* Appearance value.  This function will return zero on successful   */
   /* execution and a negative value on errors.                         */
static int ReadLocalAppearance(ParameterList_t *TempParam)
{
   int     ret_val;
   char   *AppearanceString;
   Word_t  Appearance;

   /* Verify that the GAP Service is registered.                        */
   if(ApplicationStateInfo.GAPSInstanceID)
   {
      /* Query the Local Name.                                          */
      ret_val = GAPS_Query_Device_Appearance(ApplicationStateInfo.BluetoothStackID, ApplicationStateInfo.GAPSInstanceID, &Appearance);
      if(!ret_val)
      {
         /* Map the Appearance to a String.                             */
         if(AppearanceToString(Appearance, &AppearanceString))
            Display(("Device Appearance: %s(%u).\r\n", AppearanceString, Appearance));
         else
            Display(("Device Appearance: Unknown(%u).\r\n", Appearance));
      }
      else
      {
         Display(("Error - GAPS_Query_Device_Appearance returned %d.\r\n", ret_val));

         ret_val = FUNCTION_ERROR;
      }
   }
   else
   {
      Display(("GAP Service not registered.\r\n"));

      ret_val = FUNCTION_ERROR;
   }

   return(ret_val);
}

   /* The following function is responsible for reading the Device Name */
   /* for the currently connected remote device.  This function will    */
   /* return zero on successful execution and a negative value on       */
   /* errors.                                                           */
static int ReadRemoteAppearance(ParameterList_t *TempParam)
{
   int           ret_val;
   BD_ADDR_t     BD_ADDR;
   DeviceInfo_t *DeviceInfo;

   /* Next, make sure that a valid device address exists.               */
   if((TempParam) && (TempParam->NumberofParameters > 0) && (TempParam->Params[0].strParam) && (BTPS_StringLength(TempParam->Params[0].strParam) >= (sizeof(BD_ADDR_t)*2)))
   {
      /* Convert the parameter to a Bluetooth Device Address.           */
      StrToBD_ADDR(TempParam->Params[0].strParam, &BD_ADDR);

      /* Get the device info for the connection device.                 */
      if((DeviceInfo = SearchDeviceInfoEntryByBD_ADDR(&DeviceInfoList, ApplicationStateInfo.LEConnectionInfo.AddressType, ApplicationStateInfo.LEConnectionInfo.BD_ADDR)) != NULL)
      {
         /* Verify that we discovered the Device Name Handle.           */
         if(DeviceInfo->GAPSClientInfo.DeviceAppearanceHandle)
         {
            /* Attempt to read the remote device name.                  */
            ret_val = GATT_Read_Value_Request(ApplicationStateInfo.BluetoothStackID, ApplicationStateInfo.LEConnectionInfo.ConnectionID, DeviceInfo->GAPSClientInfo.DeviceAppearanceHandle, GATT_ClientEventCallback_GAPS, (unsigned long)DeviceInfo->GAPSClientInfo.DeviceAppearanceHandle);
            if(ret_val > 0)
            {
               Display(("Attempting to read Remote Device Appearance.\r\n"));

               ret_val = 0;
            }
            else
            {
               Display(("Error - GATT_Read_Value_Request returned %d.\r\n", ret_val));

               ret_val = FUNCTION_ERROR;
            }
         }
         else
         {
            Display(("GAP Service Device Appearance Handle not discovered.\r\n"));

            ret_val = FUNCTION_ERROR;
         }
      }
      else
      {
         Display(("No Device Info.\r\n"));

         ret_val = FUNCTION_ERROR;
      }
   }
   else
   {
      /* Invalid parameters specified so flag an error to the user.     */
      Display(("Usage: GetRemoteAppearance [BD_ADDR].\r\n"));

      /* Flag that an error occurred while submitting the command.      */
      ret_val = INVALID_PARAMETERS_ERROR;
   }

   return(ret_val);
}

   /* The following function is responsible for setting the Battery     */
   /* Level of the LE device.  This function takes an unsigned integer  */
   /* as a parameter for the desired battery level and returns zero for */
   /* success or a negative number indicating an error.                 */
static int SetBatteryLevel(ParameterList_t *TempParam)
{
   int ret_val = 0;

   /* Check to make sure that the input parameters are semi-valid.      */
   if((TempParam) && (TempParam->NumberofParameters == 1) && (TempParam->Params[0].intParam >= 0) && (TempParam->Params[0].intParam <= 100))
   {
      ApplicationStateInfo.BatteryLevel = TempParam->Params[0].intParam;
      Display(("SetBatteryLevel: Battery level successfully set to: %u\r\n",ApplicationStateInfo.BatteryLevel));
   }
   else
   {
      /* Invalid parameters specified so flag an error to the user.     */
      Display(("Usage: SetBatteryLevel [0-100].\r\n"));

      /* Flag that an error occurred while submitting the command.      */
      ret_val = INVALID_PARAMETERS_ERROR;
   }

   return ret_val;
}

   /* The following function is responsible for notifying the battery   */
   /* level to the connected host device (if the device has registered  */
   /* for notifications).  This function takes no parameters and returns*/
   /* zero on success or a negative number indicating an error.         */
static int NotifyBatteryLevel(ParameterList_t *TempParam)
{
   int           Result;
   DeviceInfo_t *DeviceInfo;

   /* Check to make sure that there is a LE connection.                 */
   if(ApplicationStateInfo.Flags & APPLICATION_STATE_INFO_FLAGS_LE_CONNECTED)
   {
      /* Make sure the input parameters are semi-valid.  (Force)        */
      if((TempParam == NULL) || ((TempParam) && (TempParam->NumberofParameters == 1) && ((TempParam->Params[0].intParam == 1) || (TempParam->Params[0].intParam == 0))))
      {
         /* Search for the device info structure for this device.       */
         if((DeviceInfo = SearchDeviceInfoEntryByBD_ADDR(&DeviceInfoList, ApplicationStateInfo.LEConnectionInfo.AddressType, ApplicationStateInfo.LEConnectionInfo.BD_ADDR)) != NULL)
         {
            /* Verify that we can send a notification to this device.   */
            if((DeviceInfo->BASServerInformation.Battery_Level_Client_Configuration & GATT_CLIENT_CONFIGURATION_CHARACTERISTIC_NOTIFY_ENABLE) && ((TempParam->Params[0].intParam) || ((!(TempParam->Params[0].intParam)) && (DeviceInfo->LostNotifiedBatteryLevel != ApplicationStateInfo.BatteryLevel))))
            {
               /* Attempt to send the notification to the device.       */
               Result = BAS_Notify_Battery_Level(ApplicationStateInfo.BluetoothStackID, ApplicationStateInfo.BASInstanceID, ApplicationStateInfo.LEConnectionInfo.ConnectionID, (Byte_t)ApplicationStateInfo.BatteryLevel);
               if(!Result)
                  DeviceInfo->LostNotifiedBatteryLevel = ApplicationStateInfo.BatteryLevel;
               else
                  Display(("Error - BAS_Notify_Battery_Level() %d.\r\n", Result));
            }
            else
            {
               Display(("NotifyBatteryLevel: Cannot send notification to device.\r\n"));

               /* Flag that an error occurred while submitting the      */
               /* command.                                              */
               Result = FUNCTION_ERROR;
            }
         }
         else
         {
            Display(("NotifyBatteryLevel: No device to notify.\r\n"));

            /* Flag that an error occurred while submitting the command.*/
            Result = FUNCTION_ERROR;
         }
      }
      else
      {
         /* Invalid parameters specified so flag an error to the user.  */
         Display(("Usage: NotifyBatteryLevel [Force (1|0)].\r\n"));

         /* Flag that an error occurred while submitting the command.   */
         Result = INVALID_PARAMETERS_ERROR;
      }
   }
   else
   {
      /* Invalid parameters specified so flag an error to the user.     */
      Display(("NotifyBatteryLevel: No LE connection.\r\n"));

      /* Flag that an error occurred while submitting the command.      */
      Result = FUNCTION_ERROR;
   }

   return Result;
}

   /* The following function is responsible for notifying a report to   */
   /* the connected host device (if the device has registered for       */
   /* notifications).  The function takes a string parameter that       */
   /* contains an alpha-numeric character that should be included in the*/
   /* report to be notified.  This function returns zero on success or a*/
   /* negative number indicating an error.                              */
static int NotifyKeyboardReport(ParameterList_t *TempParam)
{
   int                           Result = 0;
   Byte_t                        UsageID = 0x04;
   DeviceInfo_t                 *DeviceInfo;
   HIDS_Report_Reference_Data_t  ReportReferenceData;

   /* Make sure that there is a connection                              */
   if(ApplicationStateInfo.Flags & APPLICATION_STATE_INFO_FLAGS_LE_CONNECTED)
   {
      /* Check that the parameter is semi-valid.                        */
      if((TempParam) && (TempParam->Params[0].strParam) && (BTPS_StringLength(TempParam->Params[0].strParam) == 1) && (isalnum(TempParam->Params[0].strParam[0])))
      {
         /* Check if the character is a-z.                              */
         if((TempParam->Params[0].strParam[0] >= 97 && TempParam->Params[0].strParam[0] <= 122))
         {
            /* Set the Usage ID.  Note the usage ID for a is 0x04.  So  */
            /* subtract the ASCII code down to an offset to add to the  */
            /* base UsageID.                                            */
            UsageID += (TempParam->Params[0].strParam[0]-97);

            /* Set the current report.                                  */
            ApplicationStateInfo.CurrentInputReport[2] = UsageID;
         }
         else
         {
            /* Check if the character is A-Z.  Note this needs to be    */
            /* done because ASCII decimal value is different from lower */
            /* case letters.                                            */
            if(TempParam->Params[0].strParam[0] >= 65 && TempParam->Params[0].strParam[0] <= 90)
            {
               /* Set the Usage ID.  Note the usage ID for A is 0x04.   */
               /* So subtract the ASCII code down to an offset to add to*/
               /* the base UsageID.                                     */
               UsageID += (TempParam->Params[0].strParam[0]-65);

               /* Toggle caps lock for the current input report so that */
               /* the host device receives a capital letter.            */
               ApplicationStateInfo.CurrentInputReport[0] = HID_MODIFIER_BYTE_CAPS_LOCK;
               /* Set the current report.                               */
               ApplicationStateInfo.CurrentInputReport[2] = UsageID;
            }
            else
            {
               /* Check if the character is 0-9.                        */
               if(TempParam->Params[0].strParam[0] >= 48 && TempParam->Params[0].strParam[0] <= 57)
               {
                  /* Zero is a special case since in ASCII it starts    */
                  /* before one, but the usage ID starts after nine.    */
                  if(TempParam->Params[0].strParam[0] == 48)
                  {
                     /* Set the Usage ID.                               */
                     UsageID = 0x27;
                  }
                  else
                  {
                     /* Set the Usage ID.  Note the usage ID for 1 is   */
                     /* 0x1E.  Add 26 to get the base Usage ID (0x04) up*/
                     /* to 0x1E and then subtract the character ASCII   */
                     /* code to an offset to add to the base Usage ID.  */
                     UsageID += (26 + (TempParam->Params[0].strParam[0]-49));
                  }
                  /* Set the current report.                            */
                  ApplicationStateInfo.CurrentInputReport[2] = UsageID;
               }
            }
         }


         /* Search for the device info structure for this device.       */
         if((DeviceInfo = SearchDeviceInfoEntryByBD_ADDR(&DeviceInfoList, ApplicationStateInfo.LEConnectionInfo.AddressType, ApplicationStateInfo.LEConnectionInfo.BD_ADDR)) != NULL)
         {
            /* Verify that the connection has registered for the current*/
            /* notifications based on the operating mode and that the   */
            /* link is currently encrypted.                             */
            if((ApplicationStateInfo.LEConnectionInfo.Flags & CONNECTION_INFO_FLAGS_CONNECTION_ENCRYPTED) && (((ApplicationStateInfo.HIDProtocolMode == pmBoot) && (DeviceInfo->BootKeyboardInputConfiguration & GATT_CLIENT_CONFIGURATION_CHARACTERISTIC_NOTIFY_ENABLE)) || ((ApplicationStateInfo.HIDProtocolMode == pmReport) && (DeviceInfo->ReportKeyboardInputConfiguration & GATT_CLIENT_CONFIGURATION_CHARACTERISTIC_NOTIFY_ENABLE))))
            {
               /* Check to see what characteristic should be notified   */
               /* based on the operating mode.                          */
               if(ApplicationStateInfo.HIDProtocolMode == pmBoot)
               {
                  /* Send the Boot Keyboard Input Report notification.  */
                  Result = HIDS_Notify_Input_Report(ApplicationStateInfo.BluetoothStackID, ApplicationStateInfo.HIDSInstanceID, ApplicationStateInfo.LEConnectionInfo.ConnectionID, rtBootKeyboardInputReport, NULL, HID_KEYBOARD_INPUT_REPORT_SIZE, ApplicationStateInfo.CurrentInputReport);
               }
               else
               {
                  /* Configure the Report Reference structures.  Note   */
                  /* that since we have only 1 report of a type         */
                  /* (Input,Output,Feature) we do not need to have a    */
                  /* unique Reference ID and therefore we use a Report  */
                  /* ID of ZERO.                                        */
                  ReportReferenceData.ReportID   = 0;
                  ReportReferenceData.ReportType = HIDS_REPORT_REFERENCE_REPORT_TYPE_INPUT_REPORT;

                  /* Send the Report Mode Input Report notification.    */
                  Result = HIDS_Notify_Input_Report(ApplicationStateInfo.BluetoothStackID, ApplicationStateInfo.HIDSInstanceID, ApplicationStateInfo.LEConnectionInfo.ConnectionID, rtReport, &ReportReferenceData, HID_KEYBOARD_INPUT_REPORT_SIZE, ApplicationStateInfo.CurrentInputReport);
               }

               /* Check to see if any error occurred.                   */
               if(Result != HID_KEYBOARD_INPUT_REPORT_SIZE)
                  Display(("Error - HIDS_Notify_Input_Report() returned %d for %s mode.\r\n", Result, (ApplicationStateInfo.HIDProtocolMode == pmBoot)?"Boot":"Keyboard"));

               /* Send an empty report so that only one character gets  */
               /* sent.                                                 */
               ApplicationStateInfo.CurrentInputReport[0] = 0x00;
               ApplicationStateInfo.CurrentInputReport[2] = 0x00;

               /* Check to see what characteristic should be notified   */
               /* based on the operating mode.                          */
               if(ApplicationStateInfo.HIDProtocolMode == pmBoot)
               {
                  /* Send the Boot Keyboard Input Report notification.  */
                  Result = HIDS_Notify_Input_Report(ApplicationStateInfo.BluetoothStackID, ApplicationStateInfo.HIDSInstanceID, ApplicationStateInfo.LEConnectionInfo.ConnectionID, rtBootKeyboardInputReport, NULL, HID_KEYBOARD_INPUT_REPORT_SIZE, ApplicationStateInfo.CurrentInputReport);
               }
               else
               {
                  /* Configure the Report Reference structures.  Note   */
                  /* that since we have only 1 report of a type         */
                  /* (Input,Output,Feature) we do not need to have a    */
                  /* unique Reference ID and therefore we use a Report  */
                  /* ID of ZERO.                                        */
                  ReportReferenceData.ReportID   = 0;
                  ReportReferenceData.ReportType = HIDS_REPORT_REFERENCE_REPORT_TYPE_INPUT_REPORT;

                  /* Send the Report Mode Input Report notification.    */
                  Result = HIDS_Notify_Input_Report(ApplicationStateInfo.BluetoothStackID, ApplicationStateInfo.HIDSInstanceID, ApplicationStateInfo.LEConnectionInfo.ConnectionID, rtReport, &ReportReferenceData, HID_KEYBOARD_INPUT_REPORT_SIZE, ApplicationStateInfo.CurrentInputReport);
               }

               /* Check to see if any error occurred.                   */
               if(Result != HID_KEYBOARD_INPUT_REPORT_SIZE)
                  Display(("Error - HIDS_Notify_Input_Report() returned %d for %s mode.\r\n", Result, (ApplicationStateInfo.HIDProtocolMode == pmBoot)?"Boot":"Keyboard"));

               /* Simply return success to the caller.                  */
               Result = 0;
            }
            else
            {
               /* Invalid parameters specified so flag an error to the  */
               /* user.                                                 */
               Display(("NotifyKeyboardReport: Device has not registered for report notifications.\r\n"));

               /* Flag that an error occurred while submitting the      */
               /* command.                                              */
               Result = FUNCTION_ERROR;
            }
         }
         else
         {
            /* Invalid parameters specified so flag an error to the     */
            /* user.                                                    */
            Display(("NotifyKeyboardReport: No device to notify.\r\n"));

            /* Flag that an error occurred while submitting the command.*/
            Result = FUNCTION_ERROR;
         }
      }
      else
      {
         Display(("Usage: NotifyKeyboardReport [a-zA-Z0-9].\r\n"));
         Result = INVALID_PARAMETERS_ERROR;
      }
   }
   else
   {
      /* Invalid parameters specified so flag an error to the user.     */
      Display(("NotifyKeyboardReport: No LE connection.\r\n"));

      /* Flag that an error occurred while submitting the command.      */
      Result = FUNCTION_ERROR;
   }

   return Result;
}

   /* ***************************************************************** */
   /*                         Event Callbacks                           */
   /* ***************************************************************** */

   /* The following function is for the GAP LE Event Receive Data       */
   /* Callback.  This function will be called whenever a Callback has   */
   /* been registered for the specified GAP LE Action that is associated*/
   /* with the Bluetooth Stack.  This function passes to the caller the */
   /* GAP LE Event Data of the specified Event and the GAP LE Event     */
   /* Callback Parameter that was specified when this Callback was      */
   /* installed.  The caller is free to use the contents of the GAP LE  */
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
   /* (this argument holds anyway because other GAP Events will not be  */
   /* processed while this function call is outstanding).               */
   /* * NOTE * This function MUST NOT Block and wait for Events that can*/
   /*          only be satisfied by Receiving a Bluetooth Event         */
   /*          Callback.  A Deadlock WILL occur because NO Bluetooth    */
   /*          Callbacks will be issued while this function is currently*/
   /*          outstanding.                                             */
static void BTPSAPI GAP_LE_Event_Callback(unsigned int BluetoothStackID, GAP_LE_Event_Data_t *GAP_LE_Event_Data, unsigned long CallbackParameter)
{
   int                                           Result;
   BoardStr_t                                    BoardStr;
   DeviceInfo_t                                 *DeviceInfo;
   Long_Term_Key_t                               GeneratedLTK;
   GAP_LE_Authentication_Event_Data_t           *Authentication_Event_Data;
   GAP_LE_Authentication_Response_Information_t  GAP_LE_Authentication_Response_Information;

   /* Verify that all parameters to this callback are Semi-Valid.       */
   if((BluetoothStackID) && (GAP_LE_Event_Data))
   {
      switch(GAP_LE_Event_Data->Event_Data_Type)
      {
         case etLE_Connection_Complete:
            Display(("etLE_Connection_Complete with size %d.\r\n",(int)GAP_LE_Event_Data->Event_Data_Size));

            if(GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Complete_Event_Data)
            {
               BD_ADDRToStr(GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Complete_Event_Data->Peer_Address, BoardStr);

               Display(("   Status:       0x%02X.\r\n", GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Complete_Event_Data->Status));
               Display(("   Role:         %s.\r\n", (GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Complete_Event_Data->Master)?"Master":"Slave"));
               Display(("   Address Type: %s.\r\n", (GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Complete_Event_Data->Peer_Address_Type == latPublic)?"Public":"Random"));
               Display(("   BD_ADDR:      %s.\r\n\r\n", BoardStr));

               /* Check to see if the connection was successful.        */
               if(GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Complete_Event_Data->Status == HCI_ERROR_CODE_NO_ERROR)
               {
                  /* Save the Connection Information.                   */
                  ApplicationStateInfo.Flags                        |= APPLICATION_STATE_INFO_FLAGS_LE_CONNECTED;
                  ApplicationStateInfo.LEConnectionInfo.Flags        = CONNECTION_INFO_FLAGS_CONNECTION_VALID;
                  ApplicationStateInfo.LEConnectionInfo.BD_ADDR      = GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Complete_Event_Data->Peer_Address;
                  ApplicationStateInfo.LEConnectionInfo.AddressType  = GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Complete_Event_Data->Peer_Address_Type;

                  /* Make sure that no entry already exists.            */
                  if((DeviceInfo = SearchDeviceInfoEntryByBD_ADDR(&DeviceInfoList, GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Complete_Event_Data->Peer_Address_Type, GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Complete_Event_Data->Peer_Address)) == NULL)
                  {
                     /* No entry exists so create one.                  */
                     if(!CreateNewDeviceInfoEntry(&DeviceInfoList, GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Complete_Event_Data->Peer_Address_Type, GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Complete_Event_Data->Peer_Address))
                        Display(("Failed to add device to Device Info List.\r\n"));
                  }
                  else
                  {
                     /* We have paired with this device previously.     */
                     /* Therefore we will start a timer and if the      */
                     /* Master does not re-establish encryption when the*/
                     /* timer expires we will request that he does so.  */
                     Result = BSC_StartTimer(BluetoothStackID, (GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Complete_Event_Data->Current_Connection_Parameters.Connection_Interval * (GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Complete_Event_Data->Current_Connection_Parameters.Slave_Latency + 8)), BSC_TimerCallback, 0);
                     if(Result > 0)
                     {
                        /* Save the Security Timer ID.                  */
                        ApplicationStateInfo.LEConnectionInfo.SecurityTimerID = (unsigned int)Result;
                     }
                     else
                        Display(("Error - BSC_StartTimer() returned %d.\r\n", Result));
                  }
               }
               else
               {
                  /* Reset the HID Protocol Mode to the default mode    */
                  /* (Report Mode).                                     */
                  ApplicationStateInfo.HIDProtocolMode = pmReport;

                  /* Clear the LE Connection Information.               */
                  BTPS_MemInitialize(&(ApplicationStateInfo.LEConnectionInfo), 0, sizeof(ApplicationStateInfo.LEConnectionInfo));

                  /* Clear the LE Connection Flag.                      */
                  ApplicationStateInfo.Flags &= ~APPLICATION_STATE_INFO_FLAGS_LE_CONNECTED;
               }
            }
            break;
         case etLE_Disconnection_Complete:
            Display(("etLE_Disconnection_Complete with size %d.\r\n", (int)GAP_LE_Event_Data->Event_Data_Size));

            if(GAP_LE_Event_Data->Event_Data.GAP_LE_Disconnection_Complete_Event_Data)
            {
               Display(("   Status: 0x%02X.\r\n", GAP_LE_Event_Data->Event_Data.GAP_LE_Disconnection_Complete_Event_Data->Status));
               Display(("   Reason: 0x%02X.\r\n", GAP_LE_Event_Data->Event_Data.GAP_LE_Disconnection_Complete_Event_Data->Reason));

               BD_ADDRToStr(GAP_LE_Event_Data->Event_Data.GAP_LE_Disconnection_Complete_Event_Data->Peer_Address, BoardStr);
               Display(("   BD_ADDR: %s.\r\n", BoardStr));

               /* Check to see if the device info is present in the     */
               /* list.                                                 */
               if((DeviceInfo = SearchDeviceInfoEntryByBD_ADDR(&DeviceInfoList, GAP_LE_Event_Data->Event_Data.GAP_LE_Disconnection_Complete_Event_Data->Peer_Address_Type, GAP_LE_Event_Data->Event_Data.GAP_LE_Disconnection_Complete_Event_Data->Peer_Address)) != NULL)
               {
                  /* Check to see if the link is encrypted.  If it isn't*/
                  /* we will delete the device structure.               */
                  if(!(ApplicationStateInfo.LEConnectionInfo.Flags & CONNECTION_INFO_FLAGS_CONNECTION_ENCRYPTED))
                  {
                     /* Connection is not encrypted so delete the device*/
                     /* structure.                                      */
                     DeviceInfo = DeleteDeviceInfoEntry(&DeviceInfoList, DeviceInfo->AddressType, DeviceInfo->BD_ADDR);
                     if(DeviceInfo)
                        FreeDeviceInfoEntryMemory(DeviceInfo);
                  }
               }

               /* Reset the HID Protocol Mode to the default mode       */
               /* (Report Mode).                                        */
               ApplicationStateInfo.HIDProtocolMode = pmReport;

               /* Clear the LE Connection Information.                  */
               BTPS_MemInitialize(&(ApplicationStateInfo.LEConnectionInfo), 0, sizeof(ApplicationStateInfo.LEConnectionInfo));

               /* Clear the LE Connection Flag.                         */
               ApplicationStateInfo.Flags &= ~APPLICATION_STATE_INFO_FLAGS_LE_CONNECTED;
            }
            break;
         case etLE_Encryption_Change:
            Display(("etLE_Encryption_Change with size %d.\r\n",(int)GAP_LE_Event_Data->Event_Data_Size));

            /* Verify that the link is currently encrypted.             */
            if((GAP_LE_Event_Data->Event_Data.GAP_LE_Encryption_Change_Event_Data->Encryption_Change_Status == HCI_ERROR_CODE_NO_ERROR) && (GAP_LE_Event_Data->Event_Data.GAP_LE_Encryption_Change_Event_Data->Encryption_Mode == emEnabled))
               ApplicationStateInfo.LEConnectionInfo.Flags |= CONNECTION_INFO_FLAGS_CONNECTION_ENCRYPTED;
            else
               ApplicationStateInfo.LEConnectionInfo.Flags &= ~CONNECTION_INFO_FLAGS_CONNECTION_ENCRYPTED;
            break;
         case etLE_Encryption_Refresh_Complete:
            Display(("etLE_Encryption_Refresh_Complete with size %d.\r\n", (int)GAP_LE_Event_Data->Event_Data_Size));

            /* Verify that the link is currently encrypted.             */
            if(GAP_LE_Event_Data->Event_Data.GAP_LE_Encryption_Refresh_Complete_Event_Data->Status == HCI_ERROR_CODE_NO_ERROR)
               ApplicationStateInfo.LEConnectionInfo.Flags |= CONNECTION_INFO_FLAGS_CONNECTION_ENCRYPTED;
            else
               ApplicationStateInfo.LEConnectionInfo.Flags &= ~CONNECTION_INFO_FLAGS_CONNECTION_ENCRYPTED;
            break;
         case etLE_Authentication:
            Display(("etLE_Authentication with size %d.\r\n", (int)GAP_LE_Event_Data->Event_Data_Size));

            /* Make sure the authentication event data is valid before  */
            /* continuing.                                              */
            if((Authentication_Event_Data = GAP_LE_Event_Data->Event_Data.GAP_LE_Authentication_Event_Data) != NULL)
            {
               BD_ADDRToStr(Authentication_Event_Data->BD_ADDR, BoardStr);

               switch(Authentication_Event_Data->GAP_LE_Authentication_Event_Type)
               {
                  case latLongTermKeyRequest:
                     Display(("    latKeyRequest: \r\n"));
                     Display(("      BD_ADDR: %s.\r\n", BoardStr));

                     /* The other side of a connection is requesting    */
                     /* that we start encryption. Thus we should        */
                     /* regenerate LTK for this connection and send it  */
                     /* to the chip.                                    */
                     Result = GAP_LE_Regenerate_Long_Term_Key(BluetoothStackID, (Encryption_Key_t *)(&DHK), (Encryption_Key_t *)(&ER), Authentication_Event_Data->Authentication_Event_Data.Long_Term_Key_Request.EDIV, &(Authentication_Event_Data->Authentication_Event_Data.Long_Term_Key_Request.Rand), &GeneratedLTK);
                     if(!Result)
                     {
                        Display(("      GAP_LE_Regenerate_Long_Term_Key Success.\r\n"));

                        /* Respond with the Re-Generated Long Term Key. */
                        GAP_LE_Authentication_Response_Information.GAP_LE_Authentication_Type                                        = larLongTermKey;
                        GAP_LE_Authentication_Response_Information.Authentication_Data_Length                                        = GAP_LE_LONG_TERM_KEY_INFORMATION_DATA_SIZE;
                        GAP_LE_Authentication_Response_Information.Authentication_Data.Long_Term_Key_Information.Encryption_Key_Size = GAP_LE_MAXIMUM_ENCRYPTION_KEY_SIZE;
                        GAP_LE_Authentication_Response_Information.Authentication_Data.Long_Term_Key_Information.Long_Term_Key       = GeneratedLTK;
                     }
                     else
                     {
                        Display(("      GAP_LE_Regenerate_Long_Term_Key returned %d.\r\n",Result));

                        /* Since we failed to generate the requested key*/
                        /* we should respond with a negative response.  */
                        GAP_LE_Authentication_Response_Information.GAP_LE_Authentication_Type = larLongTermKey;
                        GAP_LE_Authentication_Response_Information.Authentication_Data_Length = 0;
                     }

                     /* Send the Authentication Response.               */
                     Result = GAP_LE_Authentication_Response(BluetoothStackID, Authentication_Event_Data->BD_ADDR, &GAP_LE_Authentication_Response_Information);
                     if(!Result)
                     {
                        /* Master is trying to re-encrypt the Link so   */
                        /* therefore we should cancel the Security Timer*/
                        /* if it is active.                             */
                        if(ApplicationStateInfo.LEConnectionInfo.SecurityTimerID)
                        {
                           BSC_StopTimer(BluetoothStackID, ApplicationStateInfo.LEConnectionInfo.SecurityTimerID);

                           ApplicationStateInfo.LEConnectionInfo.SecurityTimerID = 0;
                        }
                     }
                     else
                        Display(("      GAP_LE_Authentication_Response returned %d.\r\n",Result));
                     break;
                  case latPairingRequest:
                     Display(("Pairing Request: %s.\r\n",BoardStr));
                     DisplayPairingInformation(Authentication_Event_Data->Authentication_Event_Data.Pairing_Request);

                     /* Master is trying to pair with us so therefore we*/
                     /* should cancel the Security Timer if it is       */
                     /* active.                                         */
                     if(ApplicationStateInfo.LEConnectionInfo.SecurityTimerID)
                     {
                        BSC_StopTimer(BluetoothStackID, ApplicationStateInfo.LEConnectionInfo.SecurityTimerID);

                        ApplicationStateInfo.LEConnectionInfo.SecurityTimerID = 0;
                     }

                     /* This is a pairing request. Respond with a       */
                     /* Pairing Response.                               */
                     /* * NOTE * This is only sent from Master to Slave.*/
                     /*          Thus we must be the Slave in this      */
                     /*          connection.                            */

                     /* Send the Pairing Response.                      */
                     SlavePairingRequestResponse(BluetoothStackID, Authentication_Event_Data->BD_ADDR);
                     break;
                  case latConfirmationRequest:
                     Display(("latConfirmationRequest.\r\n"));

                     if(Authentication_Event_Data->Authentication_Event_Data.Confirmation_Request.Request_Type == crtNone)
                     {
                        Display(("Invoking Just Works.\r\n"));

                        /* Just Accept Just Works Pairing.              */
                        GAP_LE_Authentication_Response_Information.GAP_LE_Authentication_Type = larConfirmation;

                        /* By setting the Authentication_Data_Length to */
                        /* any NON-ZERO value we are informing the GAP  */
                        /* LE Layer that we are accepting Just Works    */
                        /* Pairing.                                     */
                        GAP_LE_Authentication_Response_Information.Authentication_Data_Length = DWORD_SIZE;

                        Result = GAP_LE_Authentication_Response(BluetoothStackID, Authentication_Event_Data->BD_ADDR, &GAP_LE_Authentication_Response_Information);
                        if(Result)
                           Display(("GAP_LE_Authentication_Response returned %d.\r\n",Result));
                     }
                     else
                     {
                        if(Authentication_Event_Data->Authentication_Event_Data.Confirmation_Request.Request_Type == crtPasskey)
                        {
                           Display(("Respond with: PassKeyResponse [passkey].\r\n"));

                           /* Flag that we are awaiting a Passkey Input.*/
                           ApplicationStateInfo.LEConnectionInfo.Flags         |= CONNECTION_INFO_FLAGS_CONNECTION_AWAITING_PASSKEY;
                           ApplicationStateInfo.LEConnectionInfo.PasskeyDigits  = 0;
                           ApplicationStateInfo.LEConnectionInfo.Passkey        = 0;
                        }
                        else
                        {
                           if(Authentication_Event_Data->Authentication_Event_Data.Confirmation_Request.Request_Type == crtDisplay)
                              Display(("Passkey: %06ld.\r\n", Authentication_Event_Data->Authentication_Event_Data.Confirmation_Request.Display_Passkey));
                        }
                     }
                     break;
                  case latEncryptionInformationRequest:
                     Display(("Encryption Information Request %s.\r\n", BoardStr));

                     /* Generate new LTK,EDIV and Rand and respond with */
                     /* them.                                           */
                     EncryptionInformationRequestResponse(BluetoothStackID, Authentication_Event_Data->BD_ADDR, Authentication_Event_Data->Authentication_Event_Data.Encryption_Request_Information.Encryption_Key_Size, &GAP_LE_Authentication_Response_Information);
                     break;
                  case latIdentityInformation:
                     Display(("latIdentityInformation.\r\n"));

                     /* Search for the Device entry for our current LE  */
                     /* connection.                                     */
                     if((DeviceInfo = SearchDeviceInfoEntryByBD_ADDR(&DeviceInfoList, ApplicationStateInfo.LEConnectionInfo.AddressType, ApplicationStateInfo.LEConnectionInfo.BD_ADDR)) != NULL)
                     {
                        /* Store the received IRK and also updated the  */
                        /* BD_ADDR that is stored to the "Base" BD_ADDR.*/
                        DeviceInfo->AddressType  = Authentication_Event_Data->Authentication_Event_Data.Identity_Information.Address_Type;
                        DeviceInfo->BD_ADDR      = Authentication_Event_Data->Authentication_Event_Data.Identity_Information.Address;
                        DeviceInfo->IRK          = Authentication_Event_Data->Authentication_Event_Data.Identity_Information.IRK;
                        DeviceInfo->Flags       |= DEVICE_INFO_FLAGS_IRK_VALID;
                     }
                     break;
                  case latSecurityEstablishmentComplete:
                     Display(("Security Re-Establishment Complete: %s.\r\n", BoardStr));
                     Display(("                            Status: 0x%02X.\r\n", Authentication_Event_Data->Authentication_Event_Data.Security_Establishment_Complete.Status));

                     /* Check to see if the Security Re-establishment   */
                     /* was successful (or if it failed since the remote*/
                     /* device attempted to re-pair.                    */
                     if((Authentication_Event_Data->Authentication_Event_Data.Security_Establishment_Complete.Status != GAP_LE_SECURITY_ESTABLISHMENT_STATUS_CODE_NO_ERROR) && (Authentication_Event_Data->Authentication_Event_Data.Security_Establishment_Complete.Status != GAP_LE_SECURITY_ESTABLISHMENT_STATUS_CODE_DEVICE_TRIED_TO_REPAIR))
                     {
                        /* Security Re-establishment was not successful */
                        /* so delete the stored device information and  */
                        /* disconnect the link.                         */
                        DisconnectLEDevice(BluetoothStackID, Authentication_Event_Data->BD_ADDR);

                        /* Delete the stored device info structure.     */
                        if((DeviceInfo = DeleteDeviceInfoEntry(&DeviceInfoList, ApplicationStateInfo.LEConnectionInfo.AddressType, ApplicationStateInfo.LEConnectionInfo.BD_ADDR)) != NULL)
                           FreeDeviceInfoEntryMemory(DeviceInfo);
                     }
                     break;
                  case latPairingStatus:
                     Display(("Pairing Status: %s.\r\n", BoardStr));
                     Display(("        Status: 0x%02X.\r\n", Authentication_Event_Data->Authentication_Event_Data.Pairing_Status.Status));

                     /* Check to see if we have paired successfully with*/
                     /* the device.                                     */
                     if(Authentication_Event_Data->Authentication_Event_Data.Pairing_Status.Status == GAP_LE_PAIRING_STATUS_NO_ERROR)
                     {
                        Display(("        Key Size: %d.\r\n", Authentication_Event_Data->Authentication_Event_Data.Pairing_Status.Negotiated_Encryption_Key_Size));

                        /* Search for the Device entry for our current  */
                        /* LE connection.                               */
                        if((DeviceInfo = SearchDeviceInfoEntryByBD_ADDR(&DeviceInfoList, ApplicationStateInfo.LEConnectionInfo.AddressType, ApplicationStateInfo.LEConnectionInfo.BD_ADDR)) != NULL)
                        {
                           /* Save the encryption key size.             */
                           DeviceInfo->EncryptionKeySize = Authentication_Event_Data->Authentication_Event_Data.Pairing_Status.Negotiated_Encryption_Key_Size;
                        }
                     }
                     else
                     {
                        /* Disconnect the Link.                         */
                        DisconnectLEDevice(BluetoothStackID, Authentication_Event_Data->BD_ADDR);

                        /* Delete the stored device info structure.     */
                        if((DeviceInfo = DeleteDeviceInfoEntry(&DeviceInfoList, ApplicationStateInfo.LEConnectionInfo.AddressType, ApplicationStateInfo.LEConnectionInfo.BD_ADDR)) != NULL)
                           FreeDeviceInfoEntryMemory(DeviceInfo);
                     }
                     break;
               }
            }
            break;
         case etLE_Connection_Parameter_Updated:
            Display(("etLE_Connection_Parameter_Updated with size %d.\r\n", (int)GAP_LE_Event_Data->Event_Data_Size));
            break;
      }

      DisplayPrompt();
   }
}

   /* The following function is for an GATT Client Event Callback.  This*/
   /* function will be called whenever a GATT Response is received for a*/
   /* request that was made when this function was registered.  This    */
   /* function passes to the caller the GATT Client Event Data that     */
   /* occurred and the GATT Client Event Callback Parameter that was    */
   /* specified when this Callback was installed.  The caller is free to*/
   /* use the contents of the GATT Client Event Data ONLY in the context*/
   /* of this callback.  If the caller requires the Data for a longer   */
   /* period of time, then the callback function MUST copy the data into*/
   /* another Data Buffer.  This function is guaranteed NOT to be       */
   /* invoked more than once simultaneously for the specified installed */
   /* callback (i.e.  this function DOES NOT have be reentrant).  It    */
   /* Needs to be noted however, that if the same Callback is installed */
   /* more than once, then the callbacks will be called serially.       */
   /* Because of this, the processing in this function should be as     */
   /* efficient as possible.  It should also be noted that this function*/
   /* is called in the Thread Context of a Thread that the User does NOT*/
   /* own.  Therefore, processing in this function should be as         */
   /* efficient as possible (this argument holds anyway because another */
   /* GATT Event (Server/Client or Connection) will not be processed    */
   /* while this function call is outstanding).                         */
   /* * NOTE * This function MUST NOT Block and wait for Events that can*/
   /*          only be satisfied by Receiving a Bluetooth Event         */
   /*          Callback.  A Deadlock WILL occur because NO Bluetooth    */
   /*          Callbacks will be issued while this function is currently*/
   /*          outstanding.                                             */
static void BTPSAPI GATT_ClientEventCallback_GAPS(unsigned int BluetoothStackID, GATT_Client_Event_Data_t *GATT_Client_Event_Data, unsigned long CallbackParameter)
{
   char         *NameBuffer;
   Word_t        Appearance;
   BoardStr_t    BoardStr;
   DeviceInfo_t *DeviceInfo;

   /* Verify that all parameters to this callback are Semi-Valid.       */
   if((BluetoothStackID) && (GATT_Client_Event_Data))
   {
      /* Determine the event that occurred.                             */
      switch(GATT_Client_Event_Data->Event_Data_Type)
      {
         case etGATT_Client_Error_Response:
            if(GATT_Client_Event_Data->Event_Data.GATT_Request_Error_Data)
            {
               Display(("\r\nError Response.\r\n"));
               BD_ADDRToStr(GATT_Client_Event_Data->Event_Data.GATT_Request_Error_Data->RemoteDevice, BoardStr);
               Display(("Connection ID:   %u.\r\n", GATT_Client_Event_Data->Event_Data.GATT_Request_Error_Data->ConnectionID));
               Display(("Transaction ID:  %u.\r\n", GATT_Client_Event_Data->Event_Data.GATT_Request_Error_Data->TransactionID));
               Display(("Connection Type: %s.\r\n", (GATT_Client_Event_Data->Event_Data.GATT_Request_Error_Data->ConnectionType == gctLE)?"LE":"BR/EDR"));
               Display(("BD_ADDR:         %s.\r\n", BoardStr));
               Display(("Error Type:      %s.\r\n", (GATT_Client_Event_Data->Event_Data.GATT_Request_Error_Data->ErrorType == retErrorResponse)?"Response Error":"Response Timeout"));

               /* Only print out the rest if it is valid.               */
               if(GATT_Client_Event_Data->Event_Data.GATT_Request_Error_Data->ErrorType == retErrorResponse)
               {
                  Display(("Request Opcode:  0x%02X.\r\n", GATT_Client_Event_Data->Event_Data.GATT_Request_Error_Data->RequestOpCode));
                  Display(("Request Handle:  0x%04X.\r\n", GATT_Client_Event_Data->Event_Data.GATT_Request_Error_Data->RequestHandle));
                  Display(("Error Code:      0x%02X.\r\n", GATT_Client_Event_Data->Event_Data.GATT_Request_Error_Data->ErrorCode));

                  if(GATT_Client_Event_Data->Event_Data.GATT_Request_Error_Data->ErrorCode < NUMBER_GATT_ERROR_CODES)
                  {
                     Display(("Error Mesg:      %s.\r\n", ErrorCodeStr[GATT_Client_Event_Data->Event_Data.GATT_Request_Error_Data->ErrorCode]));
                  }
                  else
                  {
                     Display(("Error Mesg:      Unknown.\r\n", ErrorCodeStr[GATT_Client_Event_Data->Event_Data.GATT_Request_Error_Data->ErrorCode]));
                  }
               }
            }
            else
               Display(("Error - Null Error Response Data.\r\n"));
            break;
         case etGATT_Client_Read_Response:
            if(GATT_Client_Event_Data->Event_Data.GATT_Read_Response_Data)
            {
               if((DeviceInfo = SearchDeviceInfoEntryByBD_ADDR(&DeviceInfoList, ApplicationStateInfo.LEConnectionInfo.AddressType,  GATT_Client_Event_Data->Event_Data.GATT_Read_Response_Data->RemoteDevice)) != NULL)
               {
                  if((Word_t)CallbackParameter == DeviceInfo->GAPSClientInfo.DeviceNameHandle)
                  {
                     /* Display the remote device name.                 */
                     if((NameBuffer = (char *)BTPS_AllocateMemory(GATT_Client_Event_Data->Event_Data.GATT_Read_Response_Data->AttributeValueLength+1)) != NULL)
                     {
                        BTPS_MemInitialize(NameBuffer, 0, GATT_Client_Event_Data->Event_Data.GATT_Read_Response_Data->AttributeValueLength+1);
                        BTPS_MemCopy(NameBuffer, GATT_Client_Event_Data->Event_Data.GATT_Read_Response_Data->AttributeValue, GATT_Client_Event_Data->Event_Data.GATT_Read_Response_Data->AttributeValueLength);

                        Display(("\r\nRemote Device Name: %s.\r\n", NameBuffer));

                        BTPS_FreeMemory(NameBuffer);
                     }
                  }
                  else
                  {
                     if((Word_t)CallbackParameter == DeviceInfo->GAPSClientInfo.DeviceAppearanceHandle)
                     {
                        if(GATT_Client_Event_Data->Event_Data.GATT_Read_Response_Data->AttributeValueLength == GAP_DEVICE_APPEARENCE_VALUE_LENGTH)
                        {
                           Appearance = READ_UNALIGNED_WORD_LITTLE_ENDIAN(GATT_Client_Event_Data->Event_Data.GATT_Read_Response_Data->AttributeValue);
                           if(AppearanceToString(Appearance, &NameBuffer))
                              Display(("\r\nRemote Device Appearance: %s(%u).\r\n", NameBuffer, Appearance));
                           else
                              Display(("\r\nRemote Device Appearance: Unknown(%u).\r\n", Appearance));
                        }
                        else
                           Display(("Invalid Remote Appearance Value Length %u.\r\n", GATT_Client_Event_Data->Event_Data.GATT_Read_Response_Data->AttributeValueLength));
                     }
                  }
               }
            }
            else
               Display(("\r\nError - Null Read Response Data.\r\n"));
            break;
      }

      /* Print the command line prompt.                                 */
      DisplayPrompt();
   }
   else
   {
      /* There was an error with one or more of the input parameters.   */
      Display(("\r\n"));

      Display(("GATT Callback Data: Event_Data = NULL.\r\n"));

      DisplayPrompt();
   }
}

   /* The following function is for an GATT Connection Event Callback.  */
   /* This function is called for GATT Connection Events that occur on  */
   /* the specified Bluetooth Stack.  This function passes to the caller*/
   /* the GATT Connection Event Data that occurred and the GATT         */
   /* Connection Event Callback Parameter that was specified when this  */
   /* Callback was installed.  The caller is free to use the contents of*/
   /* the GATT Client Event Data ONLY in the context of this callback.  */
   /* If the caller requires the Data for a longer period of time, then */
   /* the callback function MUST copy the data into another Data Buffer.*/
   /* This function is guaranteed NOT to be invoked more than once      */
   /* simultaneously for the specified installed callback (i.e.  this   */
   /* function DOES NOT have be reentrant).  It Needs to be noted       */
   /* however, that if the same Callback is installed more than once,   */
   /* then the callbacks will be called serially.  Because of this, the */
   /* processing in this function should be as efficient as possible.   */
   /* It should also be noted that this function is called in the Thread*/
   /* Context of a Thread that the User does NOT own.  Therefore,       */
   /* processing in this function should be as efficient as possible    */
   /* (this argument holds anyway because another GATT Event            */
   /* (Server/Client or Connection) will not be processed while this    */
   /* function call is outstanding).                                    */
   /* * NOTE * This function MUST NOT Block and wait for Events that can*/
   /*          only be satisfied by Receiving a Bluetooth Event         */
   /*          Callback.  A Deadlock WILL occur because NO Bluetooth    */
   /*          Callbacks will be issued while this function is currently*/
   /*          outstanding.                                             */
static void BTPSAPI GATT_Connection_Event_Callback(unsigned int BluetoothStackID, GATT_Connection_Event_Data_t *GATT_Connection_Event_Data, unsigned long CallbackParameter)
{
   BoardStr_t      BoardStr;
   ParameterList_t TempParam;

   /* Verify that all parameters to this callback are Semi-Valid.       */
   if((BluetoothStackID) && (GATT_Connection_Event_Data))
   {
      /* Determine the Connection Event that occurred.                  */
      switch(GATT_Connection_Event_Data->Event_Data_Type)
      {
         case etGATT_Connection_Device_Connection:
            if(GATT_Connection_Event_Data->Event_Data.GATT_Device_Connection_Data)
            {
               Display(("\r\netGATT_Connection_Device_Connection with size %u: \r\n", GATT_Connection_Event_Data->Event_Data_Size));
               BD_ADDRToStr(GATT_Connection_Event_Data->Event_Data.GATT_Device_Connection_Data->RemoteDevice, BoardStr);
               Display(("   Connection ID:   %u.\r\n", GATT_Connection_Event_Data->Event_Data.GATT_Device_Connection_Data->ConnectionID));
               Display(("   Connection Type: %s.\r\n", ((GATT_Connection_Event_Data->Event_Data.GATT_Device_Connection_Data->ConnectionType == gctLE)?"LE":"BR/EDR")));
               Display(("   Remote Device:   %s.\r\n", BoardStr));
               Display(("   Connection MTU:  %u.\r\n", GATT_Connection_Event_Data->Event_Data.GATT_Device_Connection_Data->MTU));

               ApplicationStateInfo.Flags                         |= APPLICATION_STATE_INFO_FLAGS_LE_CONNECTED;
               ApplicationStateInfo.LEConnectionInfo.ConnectionID  = GATT_Connection_Event_Data->Event_Data.GATT_Device_Connection_Data->ConnectionID;

               /* Notify the battery level if necessary.                */
               Display(("\r\n"));
               TempParam.NumberofParameters = 1;
               TempParam.Params[0].intParam = 0;
               NotifyBatteryLevel(&TempParam);

               DisplayPrompt();
            }
            break;
      }
   }
   else
   {
      /* There was an error with one or more of the input parameters.   */
      Display(("\r\n"));

      Display(("GATT Connection Callback Data: Event_Data = NULL.\r\n"));
   }
}

   /* The following function is for an GATT Discovery Event Callback.   */
   /* This function will be called whenever a GATT Service is discovered*/
   /* or a previously started service discovery process is completed.   */
   /* This function passes to the caller the GATT Discovery Event Data  */
   /* that occurred and the GATT Client Event Callback Parameter that   */
   /* was specified when this Callback was installed.  The caller is    */
   /* free to use the contents of the GATT Discovery Event Data ONLY in */
   /* the context of this callback.  If the caller requires the Data for*/
   /* a longer period of time, then the callback function MUST copy the */
   /* data into another Data Buffer.  This function is guaranteed NOT to*/
   /* be invoked more than once simultaneously for the specified        */
   /* installed callback (i.e.  this function DOES NOT have be          */
   /* reentrant).  It Needs to be noted however, that if the same       */
   /* Callback is installed more than once, then the callbacks will be  */
   /* called serially.  Because of this, the processing in this function*/
   /* should be as efficient as possible.  It should also be noted that */
   /* this function is called in the Thread Context of a Thread that the*/
   /* User does NOT own.  Therefore, processing in this function should */
   /* be as efficient as possible (this argument holds anyway because   */
   /* another GATT Discovery Event will not be processed while this     */
   /* function call is outstanding).                                    */
   /* * NOTE * This function MUST NOT Block and wait for Events that can*/
   /*          only be satisfied by Receiving a Bluetooth Event         */
   /*          Callback.  A Deadlock WILL occur because NO Bluetooth    */
   /*          Callbacks will be issued while this function is currently*/
   /*          outstanding.                                             */
static void BTPSAPI GATT_Service_Discovery_Event_Callback(unsigned int BluetoothStackID, GATT_Service_Discovery_Event_Data_t *GATT_Service_Discovery_Event_Data, unsigned long CallbackParameter)
{
   DeviceInfo_t *DeviceInfo;

   /* Verify that the input parameters are semi-valid.                  */
   if((BluetoothStackID) && (GATT_Service_Discovery_Event_Data))
   {
      switch(GATT_Service_Discovery_Event_Data->Event_Data_Type)
      {
         case etGATT_Service_Discovery_Indication:
            /* Verify the event data.                                   */
            if(GATT_Service_Discovery_Event_Data->Event_Data.GATT_Service_Discovery_Indication_Data)
            {
               /* Find the device info for this connection.             */
               if((DeviceInfo = SearchDeviceInfoEntryByBD_ADDR(&DeviceInfoList, ApplicationStateInfo.LEConnectionInfo.AddressType, ApplicationStateInfo.LEConnectionInfo.BD_ADDR)) != NULL)
               {
                  Display(("\r\n"));
                  Display(("Service 0x%04X - 0x%04X, UUID: ", GATT_Service_Discovery_Event_Data->Event_Data.GATT_Service_Discovery_Indication_Data->ServiceInformation.Service_Handle, GATT_Service_Discovery_Event_Data->Event_Data.GATT_Service_Discovery_Indication_Data->ServiceInformation.End_Group_Handle));
                  DisplayUUID(&(GATT_Service_Discovery_Event_Data->Event_Data.GATT_Service_Discovery_Indication_Data->ServiceInformation.UUID));
                  Display(("\r\n"));

                  /* Attempt to populate the handles for the GAP        */
                  /* Service.                                           */
                  GAPSPopulateHandles(&(DeviceInfo->GAPSClientInfo), GATT_Service_Discovery_Event_Data->Event_Data.GATT_Service_Discovery_Indication_Data);
               }
            }
            break;
         case etGATT_Service_Discovery_Complete:
            /* Verify the event data.                                   */
            if(GATT_Service_Discovery_Event_Data->Event_Data.GATT_Service_Discovery_Complete_Data)
            {
               Display(("\r\n"));
               Display(("Service Discovery Operation Complete, Status 0x%02X.\r\n", GATT_Service_Discovery_Event_Data->Event_Data.GATT_Service_Discovery_Complete_Data->Status));

               /* Find the device info for this connection.             */
               if((DeviceInfo = SearchDeviceInfoEntryByBD_ADDR(&DeviceInfoList, ApplicationStateInfo.LEConnectionInfo.AddressType, ApplicationStateInfo.LEConnectionInfo.BD_ADDR)) != NULL)
               {
                  /* Flag that no service discovery operation is        */
                  /* outstanding for this device.                       */
                  DeviceInfo->Flags &= ~DEVICE_INFO_FLAGS_SERVICE_DISCOVERY_OUTSTANDING;
               }
            }
            break;
      }

      DisplayPrompt();
   }
   else
   {
      /* There was an error with one or more of the input parameters.   */
      Display(("\r\n"));

      Display(("GATT Callback Data: Event_Data = NULL.\r\n"));

      DisplayPrompt();
   }
}

   /* The following represents the a BAS Event Callback.  This function */
   /* will be called whenever an BAS Event occurs that is associated    */
   /* with the specified Bluetooth Stack ID.  This function passes to   */
   /* the caller the Bluetooth Stack ID, the BAS Event Data that        */
   /* occurred and the BAS Event Callback Parameter that was specified  */
   /* when this Callback was installed.  The caller is free to use the  */
   /* contents of the BAS Event Data ONLY in the context of this        */
   /* callback.  If the caller requires the Data for a longer period of */
   /* time, then the callback function MUST copy the data into another  */
   /* Data Buffer This function is guaranteed NOT to be invoked more    */
   /* than once simultaneously for the specified installed callback     */
   /* (i.e.  this function DOES NOT have to be re-entrant).It needs to  */
   /* be noted however, that if the same Callback is installed more than*/
   /* once, then the callbacks will be called serially.  Because of     */
   /* this, the processing in this function should be as efficient as   */
   /* possible.  It should also be noted that this function is called in*/
   /* the Thread Context of a Thread that the User does NOT own.        */
   /* Therefore, processing in this function should be as efficient as  */
   /* possible (this argument holds anyway because another BAS Event    */
   /* will not be processed while this function call is outstanding).   */
   /* ** NOTE ** This function MUST NOT Block and wait for events that  */
   /*            can only be satisfied by Receiving BAS Event Packets.  */
   /*            A Deadlock WILL occur because NO BAS Event Callbacks   */
   /*            will be issued while this function is currently        */
   /*            outstanding.                                           */
static void BTPSAPI BAS_Event_Callback(unsigned int BluetoothStackID, BAS_Event_Data_t *BAS_Event_Data, unsigned long CallbackParameter)
{
   int           Result;
   DeviceInfo_t *DeviceInfo;

   /* Verify that all parameters to this callback are Semi-Valid.       */
   if((BluetoothStackID) && (BAS_Event_Data))
   {
      /* Search for the Device entry for our current LE connection.     */
      if((DeviceInfo = SearchDeviceInfoEntryByBD_ADDR(&DeviceInfoList, ApplicationStateInfo.LEConnectionInfo.AddressType, ApplicationStateInfo.LEConnectionInfo.BD_ADDR)) != NULL)
      {
         /* Determine the Battery Service Event that occurred.          */
         switch(BAS_Event_Data->Event_Data_Type)
         {
            case etBAS_Server_Read_Client_Configuration_Request:
               if((BAS_Event_Data->Event_Data.BAS_Read_Client_Configuration_Data) && (BAS_Event_Data->Event_Data.BAS_Read_Client_Configuration_Data->ClientConfigurationType == ctBatteryLevel))
               {
                  Display(("Battery Read Battery Client Configuration Request.\r\n"));

                  Result = BAS_Read_Client_Configuration_Response(BluetoothStackID, ApplicationStateInfo.BASInstanceID, BAS_Event_Data->Event_Data.BAS_Read_Client_Configuration_Data->TransactionID, DeviceInfo->BASServerInformation.Battery_Level_Client_Configuration);
                  if(Result)
                     Display(("Error - BAS_Read_Client_Configuration_Response() %d.\r\n", Result));
               }
               break;
            case etBAS_Server_Client_Configuration_Update:
               if((BAS_Event_Data->Event_Data.BAS_Client_Configuration_Update_Data) && (BAS_Event_Data->Event_Data.BAS_Client_Configuration_Update_Data->ClientConfigurationType == ctBatteryLevel))
               {
                  Display(("Battery Client Configuration Update: %s.\r\n", (BAS_Event_Data->Event_Data.BAS_Client_Configuration_Update_Data->Notify?"ENABLED":"DISABLED")));

                  /* Update the stored configuration for this device.   */
                  if(BAS_Event_Data->Event_Data.BAS_Client_Configuration_Update_Data->Notify)
                     DeviceInfo->BASServerInformation.Battery_Level_Client_Configuration = GATT_CLIENT_CONFIGURATION_CHARACTERISTIC_NOTIFY_ENABLE;
                  else
                     DeviceInfo->BASServerInformation.Battery_Level_Client_Configuration = 0;
               }
               break;
            case etBAS_Server_Read_Battery_Level_Request:
               if(BAS_Event_Data->Event_Data.BAS_Read_Battery_Level_Data)
               {
                  Display(("Battery Read Battery Level Request.\r\n"));

                  /* Just respond with the current Battery Level.       */
                  Result = BAS_Battery_Level_Read_Request_Response(BluetoothStackID, BAS_Event_Data->Event_Data.BAS_Read_Battery_Level_Data->TransactionID, (Byte_t)ApplicationStateInfo.BatteryLevel);
                  if(Result)
                     Display(("Error - BAS_Battery_Level_Read_Request_Response() %d.\r\n", Result));
               }
               break;
         }
      }

      DisplayPrompt();
   }
   else
   {
      /* There was an error with one or more of the input parameters.   */
      Display(("\r\n"));

      Display(("Battery Service Callback Data: Event_Data = NULL.\r\n"));
   }
}

   /* The following declared type represents the application HID Service*/
   /* Event Callback.  This function will be called whenever an HIDS    */
   /* Event occurs that is associated with the specified Bluetooth Stack*/
   /* ID.  This function passes to the caller the Bluetooth Stack ID,   */
   /* the HIDS Event Data that occurred and the HIDS Event Callback     */
   /* Parameter that was specified when this Callback was installed.    */
   /* The caller is free to use the contents of the HIDS Event Data ONLY*/
   /* in the context of this callback.  If the caller requires the Data */
   /* for a longer period of time, then the callback function MUST copy */
   /* the data into another Data Buffer This function is guaranteed NOT */
   /* to be invoked more than once simultaneously for the specified     */
   /* installed callback (i.e.  this function DOES NOT have be          */
   /* re-entrant).  It needs to be noted however, that if the same      */
   /* Callback is installed more than once, then the callbacks will be  */
   /* called serially.  Because of this, the processing in this function*/
   /* should be as efficient as possible.  It should also be noted that */
   /* this function is called in the Thread Context of a Thread that the*/
   /* User does NOT own.  Therefore, processing in this function should */
   /* be as efficient as possible (this argument holds anyway because   */
   /* another HIDS Profile Event will not be processed while this       */
   /* function call is outstanding).                                    */
   /* ** NOTE ** This function MUST NOT Block and wait for events that  */
   /*            can only be satisfied by receiving HIDS Event Packets. */
   /*            A Deadlock WILL occur because NO HIDS Event Callbacks  */
   /*            will be issued while this function is currently        */
   /*            outstanding.                                           */
static void BTPSAPI HIDS_Event_Callback(unsigned int BluetoothStackID, HIDS_Event_Data_t *HIDS_Event_Data, unsigned long CallbackParameter)
{
   int           Result;
   Byte_t        ErrorCode;
   Word_t        Configuration;
   Byte_t       *ReportData;
   DeviceInfo_t *DeviceInfo;
   unsigned int  ReportDataLength;

   /* Verify that all parameters to this callback are Semi-Valid.       */
   if((BluetoothStackID) && (HIDS_Event_Data))
   {
      /* Search for the Device entry for our current LE connection.     */
      if((DeviceInfo = SearchDeviceInfoEntryByBD_ADDR(&DeviceInfoList, ApplicationStateInfo.LEConnectionInfo.AddressType, ApplicationStateInfo.LEConnectionInfo.BD_ADDR)) != NULL)
      {
         /* Determine the HID Service Event that occurred.              */
         switch(HIDS_Event_Data->Event_Data_Type)
         {
            case etHIDS_Server_Read_Client_Configuration_Request:
               if(HIDS_Event_Data->Event_Data.HIDS_Read_Client_Configuration_Data)
               {
                  Display(("HIDS Read Client Configuration Request: %u.\r\n", HIDS_Event_Data->Event_Data.HIDS_Read_Client_Configuration_Data->ReportType));

                  if(HIDS_Event_Data->Event_Data.HIDS_Read_Client_Configuration_Data->ReportType == rtBootKeyboardInputReport)
                     Configuration = DeviceInfo->BootKeyboardInputConfiguration;
                  else
                     Configuration = DeviceInfo->ReportKeyboardInputConfiguration;

                  /* Respond to the read request.                       */
                  Result = HIDS_Read_Client_Configuration_Response(ApplicationStateInfo.BluetoothStackID, ApplicationStateInfo.HIDSInstanceID, HIDS_Event_Data->Event_Data.HIDS_Read_Client_Configuration_Data->TransactionID, Configuration);
                  if(Result)
                     Display(("Error - HIDS_Read_Client_Configuration_Response() %d.\r\n", Result));
               }
               break;
            case etHIDS_Server_Client_Configuration_Update_Request:
               if(HIDS_Event_Data->Event_Data.HIDS_Client_Configuration_Update_Data)
               {
                  Display(("HIDS Client Configuration Update: %u.\r\n", HIDS_Event_Data->Event_Data.HIDS_Client_Configuration_Update_Data->ReportType));

                  if(HIDS_Event_Data->Event_Data.HIDS_Client_Configuration_Update_Data->ReportType == rtBootKeyboardInputReport)
                  {
                     DeviceInfo->BootKeyboardInputConfiguration   = HIDS_Event_Data->Event_Data.HIDS_Client_Configuration_Update_Data->ClientConfiguration;

                     Display(("Boot Keyboard Input Report Configuration 0x%04X.\r\n", DeviceInfo->BootKeyboardInputConfiguration));
                  }
                  else
                  {
                     DeviceInfo->ReportKeyboardInputConfiguration = HIDS_Event_Data->Event_Data.HIDS_Client_Configuration_Update_Data->ClientConfiguration;

                     Display(("Report Keyboard Input Report Configuration 0x%04X.\r\n", DeviceInfo->ReportKeyboardInputConfiguration));
                  }
               }
               break;
            case etHIDS_Server_Get_Protocol_Mode_Request:
               if(HIDS_Event_Data->Event_Data.HIDS_Get_Protocol_Mode_Request_Data)
               {
                  Display(("HIDS Get Protocol Mode Request.\r\n"));

                  /* Note that security is required to read this        */
                  /* characteristic.                                    */
                  if(ApplicationStateInfo.LEConnectionInfo.Flags & CONNECTION_INFO_FLAGS_CONNECTION_ENCRYPTED)
                     ErrorCode = 0;
                  else
                     ErrorCode = ATT_PROTOCOL_ERROR_CODE_INSUFFICIENT_ENCRYPTION;

                  /* Respond the Get Protocol Mode request.             */
                  Result = HIDS_Get_Protocol_Mode_Response(ApplicationStateInfo.BluetoothStackID, ApplicationStateInfo.HIDSInstanceID, HIDS_Event_Data->Event_Data.HIDS_Get_Protocol_Mode_Request_Data->TransactionID, ErrorCode, ApplicationStateInfo.HIDProtocolMode);
                  if(Result)
                     Display(("Error - HIDS_Get_Protocol_Mode_Response() %d.\r\n", Result));
               }
               break;
            case etHIDS_Server_Set_Protocol_Mode_Request:
               if(HIDS_Event_Data->Event_Data.HIDS_Set_Protocol_Mode_Request_Data)
               {
                  Display(("HIDS Set Protocol Mode Request: %s(%u).\r\n", (HIDS_Event_Data->Event_Data.HIDS_Set_Protocol_Mode_Request_Data->ProtocolMode == pmBoot)?"Boot":"Report", (unsigned int)HIDS_Event_Data->Event_Data.HIDS_Set_Protocol_Mode_Request_Data->ProtocolMode));

                  /* Note that security is required to write this       */
                  /* characteristic.                                    */
                  if(ApplicationStateInfo.LEConnectionInfo.Flags & CONNECTION_INFO_FLAGS_CONNECTION_ENCRYPTED)
                     ApplicationStateInfo.HIDProtocolMode = HIDS_Event_Data->Event_Data.HIDS_Set_Protocol_Mode_Request_Data->ProtocolMode;
               }
               break;
            case etHIDS_Server_Get_Report_Map_Request:
               if(HIDS_Event_Data->Event_Data.HIDS_Get_Report_Map_Data)
               {
                  Display(("HIDS Get Report Map Request: Offset = %u.\r\n", HIDS_Event_Data->Event_Data.HIDS_Get_Report_Map_Data->ReportMapOffset));

                  /* Note that security is required to read this        */
                  /* characteristic.                                    */
                  if(ApplicationStateInfo.LEConnectionInfo.Flags & CONNECTION_INFO_FLAGS_CONNECTION_ENCRYPTED)
                  {
                     /* Initialize the return value to success.         */
                     ErrorCode = 0;

                     /* Verify that the offset being read is valid.     */
                     if(HIDS_Event_Data->Event_Data.HIDS_Get_Report_Map_Data->ReportMapOffset < (sizeof(KeyboardReportDescriptor)))
                     {
                        /* Get a pointer to the report map to return.   */
                        ReportDataLength = (sizeof(KeyboardReportDescriptor) - HIDS_Event_Data->Event_Data.HIDS_Get_Report_Map_Data->ReportMapOffset);
                        ReportData       = &KeyboardReportDescriptor[HIDS_Event_Data->Event_Data.HIDS_Get_Report_Map_Data->ReportMapOffset];
                     }
                     else
                        ErrorCode = ATT_PROTOCOL_ERROR_CODE_INVALID_OFFSET;
                  }
                  else
                  {
                     ErrorCode        = ATT_PROTOCOL_ERROR_CODE_INSUFFICIENT_ENCRYPTION;
                     ReportDataLength = 0;
                     ReportData       = NULL;
                  }

                  /* Respond the Get Report Map request.                */
                  Result = HIDS_Get_Report_Map_Response(ApplicationStateInfo.BluetoothStackID, ApplicationStateInfo.HIDSInstanceID, HIDS_Event_Data->Event_Data.HIDS_Get_Report_Map_Data->TransactionID, ErrorCode, ReportDataLength, ReportData);
                  if(Result)
                     Display(("Error - HIDS_Get_Report_Map_Response() %d.\r\n", Result));
               }
               break;
            case etHIDS_Server_Get_Report_Request:
               if(HIDS_Event_Data->Event_Data.HIDS_Get_Report_Request_Data)
               {
                  Display(("HID Get Report Request: %u.\r\n", HIDS_Event_Data->Event_Data.HIDS_Get_Report_Request_Data->ReportType));

                  /* Note that security is required to read this        */
                  /* characteristic.                                    */
                  if(ApplicationStateInfo.LEConnectionInfo.Flags & CONNECTION_INFO_FLAGS_CONNECTION_ENCRYPTED)
                  {
                     /* Flag that no error has occurred.                */
                     ErrorCode = 0;

                     /* Determine what report the Host is attempting to */
                     /* read.                                           */
                     if((HIDS_Event_Data->Event_Data.HIDS_Get_Report_Request_Data->ReportType == rtBootKeyboardInputReport) || ((HIDS_Event_Data->Event_Data.HIDS_Get_Report_Request_Data->ReportType == rtReport) && (HIDS_Event_Data->Event_Data.HIDS_Get_Report_Request_Data->ReportReferenceData.ReportType == HIDS_REPORT_REFERENCE_REPORT_TYPE_INPUT_REPORT)))
                     {
                        /* Respond with the Keyboard Input Report.  Note*/
                        /* that since our Report Mode Report is         */
                        /* identical to the Boot Mode Report we do not  */
                        /* need to differentiate here.                  */
                        ReportDataLength = HID_KEYBOARD_INPUT_REPORT_SIZE;
                        ReportData       = ApplicationStateInfo.CurrentInputReport;
                     }
                     else
                     {
                        if((HIDS_Event_Data->Event_Data.HIDS_Get_Report_Request_Data->ReportType == rtBootKeyboardOutputReport) || ((HIDS_Event_Data->Event_Data.HIDS_Get_Report_Request_Data->ReportType == rtReport) && (HIDS_Event_Data->Event_Data.HIDS_Get_Report_Request_Data->ReportReferenceData.ReportType == HIDS_REPORT_REFERENCE_REPORT_TYPE_OUTPUT_REPORT)))
                        {
                           /* Respond with the Keyboard Output Report.  */
                           /* Note that since our Report Mode Report is */
                           /* identical to the Boot Mode Report we do   */
                           /* not need to differentiate here.           */
                           ReportDataLength = sizeof(ApplicationStateInfo.CurrentOutputReport);
                           ReportData       = &(ApplicationStateInfo.CurrentOutputReport);
                        }
                        else
                        {
                           Display(("Unknown Report %u, (ID,Type) = %u, %u.\r\n", HIDS_Event_Data->Event_Data.HIDS_Get_Report_Request_Data->ReportType, HIDS_Event_Data->Event_Data.HIDS_Get_Report_Request_Data->ReportReferenceData.ReportID, HIDS_Event_Data->Event_Data.HIDS_Get_Report_Request_Data->ReportReferenceData.ReportType));

                           ErrorCode = ATT_PROTOCOL_ERROR_CODE_UNLIKELY_ERROR;
                        }
                     }
                  }
                  else
                  {
                     ErrorCode        = ATT_PROTOCOL_ERROR_CODE_INSUFFICIENT_ENCRYPTION;
                     ReportDataLength = 0;
                     ReportData       = NULL;
                  }

                  /* Respond to the Get Report Request.                 */
                  Result = HIDS_Get_Report_Response(ApplicationStateInfo.BluetoothStackID, ApplicationStateInfo.HIDSInstanceID, HIDS_Event_Data->Event_Data.HIDS_Get_Report_Request_Data->TransactionID, HIDS_Event_Data->Event_Data.HIDS_Get_Report_Request_Data->ReportType, &(HIDS_Event_Data->Event_Data.HIDS_Get_Report_Request_Data->ReportReferenceData), ErrorCode, ReportDataLength, ReportData);
                  if(Result)
                     Display(("Error - HIDS_Get_Report_Response() %d.\r\n", Result));
               }
               break;
            case etHIDS_Server_Set_Report_Request:
               if(HIDS_Event_Data->Event_Data.HIDS_Set_Report_Request_Data)
               {
                  Display(("HID Set Report Request: %u.\r\n", HIDS_Event_Data->Event_Data.HIDS_Set_Report_Request_Data->ReportType));

                  /* Note that security is required to write this       */
                  /* characteristic.                                    */
                  if(ApplicationStateInfo.LEConnectionInfo.Flags & CONNECTION_INFO_FLAGS_CONNECTION_ENCRYPTED)
                  {
                     /* Flag that no error has occurred.                */
                     ErrorCode = 0;

                     /* Determine what report the Host is attempting to */
                     /* write.                                          */
                     if((HIDS_Event_Data->Event_Data.HIDS_Set_Report_Request_Data->ReportType == rtBootKeyboardOutputReport) || ((HIDS_Event_Data->Event_Data.HIDS_Set_Report_Request_Data->ReportType == rtReport) && (HIDS_Event_Data->Event_Data.HIDS_Set_Report_Request_Data->ReportReferenceData.ReportType == HIDS_REPORT_REFERENCE_REPORT_TYPE_OUTPUT_REPORT)))
                     {
                        /* Verify that the length is valid.             */
                        if(HIDS_Event_Data->Event_Data.HIDS_Set_Report_Request_Data->ReportLength == (sizeof(ApplicationStateInfo.CurrentOutputReport)))
                        {
                           /* Set the Output Report Value.              */
                           ApplicationStateInfo.CurrentOutputReport = READ_UNALIGNED_BYTE_LITTLE_ENDIAN(HIDS_Event_Data->Event_Data.HIDS_Set_Report_Request_Data->Report);

                           Display(("Current Output Report Value: 0x%02X.\r\n", ApplicationStateInfo.CurrentOutputReport));
                        }
                        else
                           ErrorCode = ATT_PROTOCOL_ERROR_CODE_INVALID_ATTRIBUTE_VALUE_LENGTH;
                     }
                     else
                     {
                        Display(("Unknown Report %u, (ID,Type) = %u, %u.\r\n", HIDS_Event_Data->Event_Data.HIDS_Set_Report_Request_Data->ReportType, HIDS_Event_Data->Event_Data.HIDS_Set_Report_Request_Data->ReportReferenceData.ReportID, HIDS_Event_Data->Event_Data.HIDS_Set_Report_Request_Data->ReportReferenceData.ReportType));

                        ErrorCode = ATT_PROTOCOL_ERROR_CODE_UNLIKELY_ERROR;
                     }
                  }
                  else
                     ErrorCode = ATT_PROTOCOL_ERROR_CODE_INSUFFICIENT_ENCRYPTION;


                  /* Respond to the Set Report Request.                 */
                  Result = HIDS_Set_Report_Response(ApplicationStateInfo.BluetoothStackID, ApplicationStateInfo.HIDSInstanceID, HIDS_Event_Data->Event_Data.HIDS_Set_Report_Request_Data->TransactionID, HIDS_Event_Data->Event_Data.HIDS_Set_Report_Request_Data->ReportType, &(HIDS_Event_Data->Event_Data.HIDS_Set_Report_Request_Data->ReportReferenceData), ErrorCode);
                  if(Result)
                     Display(("Error - HIDS_Set_Report_Response() %d.\r\n", Result));
               }
               break;
            case etHIDS_Server_Control_Point_Command_Indication:
               if(HIDS_Event_Data->Event_Data.HIDS_Control_Point_Command_Data)
               {
                  Display(("HID Control Point Command: %s (%u).\r\n", ((HIDS_Event_Data->Event_Data.HIDS_Control_Point_Command_Data->ControlPointCommand == pcSuspend)?"Suspend":"Exit Suspend"), (unsigned int)HIDS_Event_Data->Event_Data.HIDS_Control_Point_Command_Data->ControlPointCommand));
               }
               break;
         }
      }

      DisplayPrompt();
   }
   else
   {
      /* There was an error with one or more of the input parameters.   */
      Display(("\r\n"));

      Display(("HIDS Event Callback Data: Event_Data = NULL.\r\n"));
   }
}

   /* The following function represents the Security Timer that is used */
   /* to ensure that the Master re-establishes security after pairing on*/
   /* subsequent connections.                                           */
static void BTPSAPI BSC_TimerCallback(unsigned int BluetoothStackID, unsigned int TimerID, unsigned long CallbackParameter)
{
   /* Verify that the input parameters are semi-valid.                  */
   if((BluetoothStackID) && (TimerID))
   {
      /* Verify that the LE Connection is still active.                 */
      if((ApplicationStateInfo.Flags & APPLICATION_STATE_INFO_FLAGS_LE_CONNECTED) && (ApplicationStateInfo.LEConnectionInfo.SecurityTimerID == TimerID))
      {
         /* Invalidate the Timer ID that just expired.                  */
         ApplicationStateInfo.LEConnectionInfo.SecurityTimerID = 0;

         /* If the connection is not currently encrypted then we will   */
         /* send a Slave Security Request.                              */
         if(!(ApplicationStateInfo.LEConnectionInfo.Flags & CONNECTION_INFO_FLAGS_CONNECTION_ENCRYPTED))
            SlaveSecurityReEstablishment(BluetoothStackID, ApplicationStateInfo.LEConnectionInfo.BD_ADDR);
      }
   }
}

   /* ***************************************************************** */
   /*                    End of Event Callbacks.                        */
   /* ***************************************************************** */

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
         /* Now that the device is discoverable attempt to make it      */
         /* pairable.                                                   */
         ret_val = SetPairable();
         if(!ret_val)
         {
            /* Reset the HID Protocol Mode to the default mode (Report  */
            /* Mode).                                                   */
            ApplicationStateInfo.HIDProtocolMode = pmReport;

            /* Clear the LE Connection Information.                     */
            BTPS_MemInitialize(&(ApplicationStateInfo.LEConnectionInfo), 0, sizeof(ApplicationStateInfo.LEConnectionInfo));

            /* Clear the LE Connection Flag.                            */
            ApplicationStateInfo.Flags &= ~APPLICATION_STATE_INFO_FLAGS_LE_CONNECTED;

            /* Set up the selection interface for the device.           */
            UserInterface_Device();

            /* Return success to the caller.                            */
            ret_val = (int)ApplicationStateInfo.BluetoothStackID;
         }
         else
            DisplayFunctionError("SetPairable", ret_val);

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
   Display(("\r\nHOGP>"));
}

   /* The following function is used to process a command line string.  */
   /* This function takes as it's only parameter the command line string*/
   /* to be parsed and returns TRUE if a command was parsed and executed*/
   /* or FALSE otherwise.                                               */
Boolean_t ProcessCommandLine(char *String)
{
   return(CommandLineInterpreter(String));
}
