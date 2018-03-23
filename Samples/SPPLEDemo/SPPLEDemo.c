/*****< sppledemo.c >**********************************************************/
/*      Copyright 2012 - 2014 Stonestreet One.                                */
/*      All Rights Reserved.                                                  */
/*                                                                            */
/*      Copyright 2015 Texas Instruments Incorporated.                        */
/*      All Rights Reserved.                                                  */
/*                                                                            */
/*  SPPLEDEMO - Embedded Bluetooth SPP Emulation using GATT (LE) application. */
/*                                                                            */
/*  Author:  Tim Cook                                                         */
/*                                                                            */
/*** MODIFICATION HISTORY *****************************************************/
/*                                                                            */
/*   mm/dd/yy  F. Lastname    Description of Modification                     */
/*   --------  -----------    ------------------------------------------------*/
/*   04/16/12  Tim Cook       Initial creation.                               */
/*   01/09/13  M. Buckley     Adapted use of ConnectionID and                 */
/*                            ConnectionBD_ADDR for use with multiple devices */
/*   11/24/14  R. Malovany    Update.                                         */
/*   03/03/15  D. Horowitz    Adding Demo Application version.                */
/******************************************************************************/
#include "Main.h"                /* Application Interface Abstraction.        */
#include "SPPLEDemo.h"           /* Application Header.                       */
#include "SS1BTPS.h"             /* Main SS1 BT Stack Header.                 */
#include "SS1BTGAT.h"            /* Main SS1 GATT Header.                     */
#include "SS1BTGAP.h"            /* Main SS1 GAP Service Header.              */
#include "BTPSKRNL.h"            /* BTPS Kernel Header.                       */
#include "SS1BTVS.h"             /* Vendor Specific Prototypes/Constants.     */

#define MAX_SUPPORTED_COMMANDS                     (65)  /* Denotes the       */
                                                         /* maximum number of */
                                                         /* User Commands that*/
                                                         /* are supported by  */
                                                         /* this application. */

#define MAX_NUM_OF_PARAMETERS                       (5)  /* Denotes the max   */
                                                         /* number of         */
                                                         /* parameters a      */
                                                         /* command can have. */

#define MAX_INQUIRY_RESULTS                        (20)  /* Denotes the max   */
                                                         /* number of inquiry */
                                                         /* results.          */

#define MAX_SUPPORTED_LINK_KEYS                     (1)  /* Max supported Link*/
                                                         /* keys.             */

#define MAX_LE_CONNECTIONS                          (2)  /* Denotes the max   */
                                                         /* number of LE      */
                                                         /* connections that  */
                                                         /* are allowed at    */
                                                         /* the same time.    */

#define MAX_SIMULTANEOUS_SPP_PORTS                  (1) /* Maximum SPP Ports  */
                                                        /* that we support.   */

#define MAXIMUM_SPP_LOOPBACK_BUFFER_SIZE           (64) /* Maximum size of the*/
                                                        /* buffer used in     */
                                                        /* loopback mode.     */

#define SPP_PERFORM_MASTER_ROLE_SWITCH              (1) /* Defines if TRUE    */
                                                        /* that a role switch */
                                                        /* should be performed*/
                                                        /* for all SPP        */
                                                        /* connections.       */

#define DEFAULT_LE_IO_CAPABILITY   (licNoInputNoOutput)  /* Denotes the       */
                                                         /* default I/O       */
                                                         /* Capability that is*/
                                                         /* used with LE      */
                                                         /* Pairing.          */

#define DEFAULT_LE_MITM_PROTECTION              (TRUE)   /* Denotes the       */
                                                         /* default value used*/
                                                         /* for Man in the    */
                                                         /* Middle (MITM)     */
                                                         /* protection used   */
                                                         /* with LE Pairing.  */

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

#define SPPLE_DATA_BUFFER_LENGTH  (BTPS_CONFIGURATION_GATT_MAXIMUM_SUPPORTED_MTU_SIZE)
                                                         /* Defines the length*/
                                                         /* of a SPPLE Data   */
                                                         /* Buffer.           */

#define SPPLE_DATA_CREDITS        (SPPLE_DATA_BUFFER_LENGTH*1) /* Defines the */
                                                         /* number of credits */
                                                         /* in an SPPLE Buffer*/

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

#define EXIT_TEST_MODE                             (-10) /* Flags exit from   */
                                                         /* Test Mode.        */

#define EXIT_MODE                                  (-11) /* Flags exit from   */
                                                         /* any Mode.         */

   /* The following MACRO is used to convert an ASCII character into the*/
   /* equivalent decimal value.  The MACRO converts lower case          */
   /* characters to upper case before the conversion.                   */
#define ToInt(_x)                                  (((_x) > 0x39)?(((_x) & ~0x20)-0x37):((_x)-0x30))

   /* Determine the Name we will use for this compilation.              */
#define LE_APP_DEMO_NAME                        "SPPLEDemo"

   /* Following converts a Sniff Parameter in Milliseconds to frames.   */
#define MILLISECONDS_TO_BASEBAND_SLOTS(_x)         ((_x) / (0.625))

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

   /* The following structure holds status information about a send     */
   /* process.                                                          */
typedef struct _tagSend_Info_t
{
   Boolean_t    BufferFull;
   DWord_t      BytesToSend;
   DWord_t      BytesSent;
   unsigned int DataStrIndex;
} Send_Info_t;

   /* The following defines the structure that is used to hold          */
   /* information about all open SPP Ports.                             */
typedef struct SPP_Context_Info_t
{
   unsigned int  LocalSerialPortID;
   unsigned int  ServerPortNumber;
   Word_t        Connection_Handle;
   BD_ADDR_t     BD_ADDR;
   DWord_t       SPPServerSDPHandle;
   Boolean_t     Connected;
   Send_Info_t   SendInfo;

#if MAXIMUM_SPP_LOOPBACK_BUFFER_SIZE > 0

   unsigned int  BufferLength;
   unsigned char Buffer[MAXIMUM_SPP_LOOPBACK_BUFFER_SIZE];

#endif

} SPP_Context_Info_t;

   /* The following defines the format of a SPPLE Data Buffer.          */
typedef struct _tagSPPLE_Data_Buffer_t
{
   unsigned int InIndex;
   unsigned int OutIndex;
   unsigned int BytesFree;
   unsigned int BufferSize;
   Byte_t       Buffer[SPPLE_DATA_CREDITS];
} SPPLE_Data_Buffer_t;

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

   /* The following structure is used to hold a list of information     */
   /* on all paired devices.                                            */
typedef struct _tagDeviceInfo_t
{
   Byte_t                   Flags;
   Byte_t                   EncryptionKeySize;
   GAP_LE_Address_Type_t    ConnectionAddressType;
   BD_ADDR_t                ConnectionBD_ADDR;
   Long_Term_Key_t          LTK;
   Random_Number_t          Rand;
   Word_t                   EDIV;
   GAPS_Client_Info_t       GAPSClientInfo;
   SPPLE_Client_Info_t      ClientInfo;
   SPPLE_Server_Info_t      ServerInfo;
   struct _tagDeviceInfo_t *NextDeviceInfoInfoPtr;
} DeviceInfo_t;

#define DEVICE_INFO_DATA_SIZE                            (sizeof(DeviceInfo_t))

   /* Defines the bit mask flags that may be set in the DeviceInfo_t    */
   /* structure.                                                        */
#define DEVICE_INFO_FLAGS_CONNECTED                      0x01
#define DEVICE_INFO_FLAGS_LTK_VALID                      0x02
#define DEVICE_INFO_FLAGS_SPPLE_SERVER                   0x04
#define DEVICE_INFO_FLAGS_SERVICE_DISCOVERY_OUTSTANDING  0x08
#define DEVICE_INFO_FLAGS_LINK_ENCRYPTED                 0x10

   /* The following structure is used to hold all of the SPPLE related  */
   /* information pertaining to buffers and credits.                    */
typedef struct _tagSPPLE_Buffer_Info_t
{
   Send_Info_t         SendInfo;
   unsigned int        TransmitCredits;
   Word_t              QueuedCredits;
   SPPLE_Data_Buffer_t ReceiveBuffer;
   SPPLE_Data_Buffer_t TransmitBuffer;
} SPPLE_Buffer_Info_t;

   /* The following structure is used to hold information on a connected*/
   /* LE Device.                                                        */
typedef struct _tagLE_Context_Info_t
{
   BD_ADDR_t           ConnectionBD_ADDR;
   unsigned int        ConnectionID;
   SPPLE_Buffer_Info_t SPPLEBufferInfo;
   Boolean_t           BufferFull;
}  LE_Context_Info_t;

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
static BTPSCONST Encryption_Key_t ER = {0x28, 0xBA, 0xE1, 0x37, 0x13, 0xB2, 0x20, 0x45, 0x16, 0xB2, 0x19, 0xD0, 0x80, 0xEE, 0x4A, 0x51};

                        /* The Identity Root Key should be generated    */
                        /* in such a way as to guarantee 128 bits of    */
                        /* entropy.                                     */
static BTPSCONST Encryption_Key_t IR = {0x41, 0x09, 0xA0, 0x88, 0x09, 0x6B, 0x70, 0xC0, 0x95, 0x23, 0x3C, 0x8C, 0x48, 0xFC, 0xC9, 0xFE};

                        /* The following keys can be regenerated on the */
                        /* fly using the constant IR and ER keys and    */
                        /* are used globally, for all devices.          */
static Encryption_Key_t DHK;
static Encryption_Key_t IRK;

   /* Internal Variables to this Module (Remember that all variables    */
   /* declared static are initialized to 0 automatically by the         */
   /* compiler as part of standard C/C++).                              */

static int                 UI_Mode;                 /* Holds the UI Mode.              */

static Byte_t              SPPLEBuffer[SPPLE_DATA_BUFFER_LENGTH+1];  /* Buffer that is */
                                                    /* used for Sending/Receiving      */
                                                    /* SPPLE Service Data.             */

static unsigned int        SPPLEServiceID;          /* The following holds the SPP LE  */
                                                    /* Service ID that is returned from*/
                                                    /* GATT_Register_Service().        */

static unsigned int        GAPSInstanceID;          /* Holds the Instance ID for the   */
                                                    /* GAP Service.                    */

static GAPLE_Parameters_t  LE_Parameters;           /* Holds GAP Parameters like       */
                                                    /* Discoverability, Connectability */
                                                    /* Modes.                          */

static DeviceInfo_t       *DeviceInfoList;          /* Holds the list head for the     */
                                                    /* device info list.               */

static unsigned int        BluetoothStackID;        /* Variable which holds the Handle */
                                                    /* of the opened Bluetooth Protocol*/
                                                    /* Stack.                          */

static Boolean_t           LocalDeviceIsMaster;     /* Boolean that tells if the local */
                                                    /* device is the master of the     */
                                                    /* current connection.             */

static BD_ADDR_t           CurrentLERemoteBD_ADDR;  /* Variable which holds the        */
                                                    /* current LE BD_ADDR of the device*/
                                                    /* which is currently pairing or   */
                                                    /* authenticating.                 */

static BD_ADDR_t           CurrentCBRemoteBD_ADDR;  /* Variable which holds the        */
                                                    /* current CB BD_ADDR of the device*/
                                                    /* which is currently pairing or   */
                                                    /* authenticating.                 */

static BD_ADDR_t           InquiryResultList[MAX_INQUIRY_RESULTS]; /* Variable which   */
                                                    /* contains the inquiry result     */
                                                    /* received from the most recently */
                                                    /* preformed inquiry.              */

static unsigned int        NumberofValidResponses;  /* Variable which holds the number */
                                                    /* of valid inquiry results within */
                                                    /* the inquiry results array.      */

static LinkKeyInfo_t       LinkKeyInfo[MAX_SUPPORTED_LINK_KEYS]; /* Variable holds     */
                                                    /* BD_ADDR <-> Link Keys for       */
                                                    /* pairing.                        */

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

static Boolean_t           LoopbackActive;          /* Variable which flags whether or */
                                                    /* not the application is currently*/
                                                    /* operating in Loopback Mode      */
                                                    /* (TRUE) or not (FALSE).          */

static Boolean_t           DisplayRawData;          /* Variable which flags whether or */
                                                    /* not the application is to       */
                                                    /* simply display the Raw Data     */
                                                    /* when it is received (when not   */
                                                    /* operating in Loopback Mode).    */

static Boolean_t           AutomaticReadActive;     /* Variable which flags whether or */
                                                    /* not the application is to       */
                                                    /* automatically read all data     */
                                                    /* as it is received.              */

static unsigned int        NumberCommands;          /* Variable which is used to hold  */
                                                    /* the number of Commands that are */
                                                    /* supported by this application.  */
                                                    /* Commands are added individually.*/

static CommandTable_t      CommandTable[MAX_SUPPORTED_COMMANDS]; /* Variable which is  */
                                                    /* used to hold the actual Commands*/
                                                    /* that are supported by this      */
                                                    /* application.                    */

static SPP_Context_Info_t  SPPContextInfo[MAX_SIMULTANEOUS_SPP_PORTS];
                                                    /* Variable that contains          */
                                                    /* information about the current   */
                                                    /* open SPP Ports                  */

static LE_Context_Info_t   LEContextInfo[MAX_LE_CONNECTIONS]; /* Array that contains   */
                                                    /* the connection ID and BD_ADDR   */
                                                    /* of each connected device.       */

static DWord_t             MaxBaudRate;             /* Variable stores the maximum     */
                                                    /* HCI UART baud rate supported by */
                                                    /* this platform.                  */

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
   "No Input/Output",
   "Keyboard/Display"
} ;

   /* The following defines a data sequence that will be used to        */
   /* generate message data.                                            */
static char DataStr[]  = "0123456789";
static int  DataStrLen = (sizeof(DataStr)-1);

   /*********************************************************************/
   /**                     SPPLE Service Table                         **/
   /*********************************************************************/

   /* The SPPLE Service Declaration UUID.                               */
static BTPSCONST GATT_Primary_Service_128_Entry_t SPPLE_Service_UUID =
{
   SPPLE_SERVICE_BLUETOOTH_UUID_CONSTANT
};

   /* The Tx Characteristic Declaration.                                */
static BTPSCONST GATT_Characteristic_Declaration_128_Entry_t SPPLE_Tx_Declaration =
{
   GATT_CHARACTERISTIC_PROPERTIES_NOTIFY,
   SPPLE_TX_CHARACTERISTIC_BLUETOOTH_UUID_CONSTANT
};

   /* The Tx Characteristic Value.                                      */
static BTPSCONST GATT_Characteristic_Value_128_Entry_t  SPPLE_Tx_Value =
{
   SPPLE_TX_CHARACTERISTIC_BLUETOOTH_UUID_CONSTANT,
   0,
   NULL
};

   /* The Tx Credits Characteristic Declaration.                        */
static BTPSCONST GATT_Characteristic_Declaration_128_Entry_t SPPLE_Tx_Credits_Declaration =
{
   (GATT_CHARACTERISTIC_PROPERTIES_READ|GATT_CHARACTERISTIC_PROPERTIES_WRITE_WITHOUT_RESPONSE|GATT_CHARACTERISTIC_PROPERTIES_WRITE),
   SPPLE_TX_CREDITS_CHARACTERISTIC_BLUETOOTH_UUID_CONSTANT
};

   /* The Tx Credits Characteristic Value.                              */
static BTPSCONST GATT_Characteristic_Value_128_Entry_t SPPLE_Tx_Credits_Value =
{
   SPPLE_TX_CREDITS_CHARACTERISTIC_BLUETOOTH_UUID_CONSTANT,
   0,
   NULL
};

   /* The SPPLE RX Characteristic Declaration.                          */
static BTPSCONST GATT_Characteristic_Declaration_128_Entry_t SPPLE_Rx_Declaration =
{
   (GATT_CHARACTERISTIC_PROPERTIES_WRITE_WITHOUT_RESPONSE),
   SPPLE_RX_CHARACTERISTIC_BLUETOOTH_UUID_CONSTANT
};

   /* The SPPLE RX Characteristic Value.                                */
static BTPSCONST GATT_Characteristic_Value_128_Entry_t  SPPLE_Rx_Value =
{
   SPPLE_RX_CHARACTERISTIC_BLUETOOTH_UUID_CONSTANT,
   0,
   NULL
};


   /* The SPPLE Rx Credits Characteristic Declaration.                  */
static BTPSCONST GATT_Characteristic_Declaration_128_Entry_t SPPLE_Rx_Credits_Declaration =
{
   (GATT_CHARACTERISTIC_PROPERTIES_READ|GATT_CHARACTERISTIC_PROPERTIES_NOTIFY),
   SPPLE_RX_CREDITS_CHARACTERISTIC_BLUETOOTH_UUID_CONSTANT
};

   /* The SPPLE Rx Credits Characteristic Value.                        */
static BTPSCONST GATT_Characteristic_Value_128_Entry_t SPPLE_Rx_Credits_Value =
{
   SPPLE_RX_CREDITS_CHARACTERISTIC_BLUETOOTH_UUID_CONSTANT,
   0,
   NULL
};

   /* Client Characteristic Configuration Descriptor.                   */
static GATT_Characteristic_Descriptor_16_Entry_t Client_Characteristic_Configuration =
{
   GATT_CLIENT_CHARACTERISTIC_CONFIGURATION_BLUETOOTH_UUID_CONSTANT,
   GATT_CLIENT_CHARACTERISTIC_CONFIGURATION_LENGTH,
   NULL
};

   /* The following defines the SPPLE service that is registered with   */
   /* the GATT_Register_Service function call.                          */
   /* * NOTE * This array will be registered with GATT in the call to   */
   /*          GATT_Register_Service.                                   */
BTPSCONST GATT_Service_Attribute_Entry_t SPPLE_Service[] =
{
   {GATT_ATTRIBUTE_FLAGS_READABLE,          aetPrimaryService128,            (Byte_t *)&SPPLE_Service_UUID},                  //0
   {GATT_ATTRIBUTE_FLAGS_READABLE,          aetCharacteristicDeclaration128, (Byte_t *)&SPPLE_Tx_Declaration},                //1
   {0,                                      aetCharacteristicValue128,       (Byte_t *)&SPPLE_Tx_Value},                      //2
   {GATT_ATTRIBUTE_FLAGS_READABLE_WRITABLE, aetCharacteristicDescriptor16,   (Byte_t *)&Client_Characteristic_Configuration}, //3
   {GATT_ATTRIBUTE_FLAGS_READABLE,          aetCharacteristicDeclaration128, (Byte_t *)&SPPLE_Tx_Credits_Declaration},        //4
   {GATT_ATTRIBUTE_FLAGS_READABLE_WRITABLE, aetCharacteristicValue128,       (Byte_t *)&SPPLE_Tx_Credits_Value},              //5
   {GATT_ATTRIBUTE_FLAGS_READABLE,          aetCharacteristicDeclaration128, (Byte_t *)&SPPLE_Rx_Declaration},                //6
   {GATT_ATTRIBUTE_FLAGS_WRITABLE,          aetCharacteristicValue128,       (Byte_t *)&SPPLE_Rx_Value},                      //7
   {GATT_ATTRIBUTE_FLAGS_READABLE,          aetCharacteristicDeclaration128, (Byte_t *)&SPPLE_Rx_Credits_Declaration},        //8
   {GATT_ATTRIBUTE_FLAGS_READABLE,          aetCharacteristicValue128,       (Byte_t *)&SPPLE_Rx_Credits_Value},              //9
   {GATT_ATTRIBUTE_FLAGS_READABLE_WRITABLE, aetCharacteristicDescriptor16,   (Byte_t *)&Client_Characteristic_Configuration}, //10
};

#define SPPLE_SERVICE_ATTRIBUTE_COUNT               (sizeof(SPPLE_Service)/sizeof(GATT_Service_Attribute_Entry_t))

#define SPPLE_TX_CHARACTERISTIC_ATTRIBUTE_OFFSET               2
#define SPPLE_TX_CHARACTERISTIC_CCD_ATTRIBUTE_OFFSET           3
#define SPPLE_TX_CREDITS_CHARACTERISTIC_ATTRIBUTE_OFFSET       5
#define SPPLE_RX_CHARACTERISTIC_ATTRIBUTE_OFFSET               7
#define SPPLE_RX_CREDITS_CHARACTERISTIC_ATTRIBUTE_OFFSET       9
#define SPPLE_RX_CREDITS_CHARACTERISTIC_CCD_ATTRIBUTE_OFFSET   10

   /*********************************************************************/
   /**                    END OF SERVICE TABLE                         **/
   /*********************************************************************/

   /* Internal function prototypes.                                     */
static DeviceInfo_t *CreateNewDeviceInfoEntry(DeviceInfo_t **ListHead, GAP_LE_Address_Type_t ConnectionAddressType, BD_ADDR_t ConnectionBD_ADDR);
static DeviceInfo_t *SearchDeviceInfoEntryByBD_ADDR(DeviceInfo_t **ListHead, BD_ADDR_t BD_ADDR);
static DeviceInfo_t *DeleteDeviceInfoEntry(DeviceInfo_t **ListHead, BD_ADDR_t BD_ADDR);
static void FreeDeviceInfoEntryMemory(DeviceInfo_t *EntryToFree);
static void FreeDeviceInfoList(DeviceInfo_t **ListHead);

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
static void StrToBD_ADDR(char *BoardStr, BD_ADDR_t *Board_Address);

static void DisplayIOCapabilities(void);
static void DisplayClassOfDevice(Class_of_Device_t Class_of_Device);
static void DisplayAdvertisingData(GAP_LE_Advertising_Data_t *Advertising_Data);
static void DisplayPairingInformation(GAP_LE_Pairing_Capabilities_t Pairing_Capabilities);
static void DisplayUUID(GATT_UUID_t *UUID);
static void DisplayUsage(char *UsageString);
static void DisplayFunctionError(char *Function,int Status);
static void DisplayFunctionSuccess(char *Function);
static void DisplayConnectLEUsage(char *CharacteristicName);
static void DisplayFWVersion (void);

static int OpenStack(HCI_DriverInformation_t *HCI_DriverInformation, BTPS_Initialization_t *BTPS_Initialization);
static int CloseStack(void);

static int SetDisc(void);
static int SetConnect(void);
static int SetPairable(void);

static void DumpAppearanceMappings(void);
static Boolean_t AppearanceToString(Word_t Appearance, char **String);
static Boolean_t AppearanceIndexToAppearance(unsigned int Index, Word_t *Appearance);

static void GAPSPopulateHandles(GAPS_Client_Info_t *ClientInfo, GATT_Service_Discovery_Indication_Data_t *ServiceInfo);
static void SPPLEPopulateHandles(SPPLE_Client_Info_t *ClientInfo, GATT_Service_Discovery_Indication_Data_t *ServiceInfo);

static unsigned int AddDataToBuffer(SPPLE_Data_Buffer_t *DataBuffer, unsigned int DataLength, Byte_t *Data);
static unsigned int RemoveDataFromBuffer(SPPLE_Data_Buffer_t *DataBuffer, unsigned int BufferLength, Byte_t *Buffer);
static void InitializeBuffer(SPPLE_Data_Buffer_t *DataBuffer);

static int EnableDisableNotificationsIndications(unsigned int ConnectionID, Word_t ClientConfigurationHandle, Word_t ClientConfigurationValue, GATT_Client_Event_Callback_t ClientEventCallback);

static unsigned int FillBufferWithString(SPPLE_Data_Buffer_t *DataBuffer, unsigned *CurrentBufferLength, unsigned int MaxLength, Byte_t *Buffer, unsigned int *DataStrIndex);

static void SPPLESendProcess(LE_Context_Info_t *LEContextInfo, DeviceInfo_t *DeviceInfo);
static void SPPLESendCredits(LE_Context_Info_t *LEContextInfo, DeviceInfo_t *DeviceInfo, unsigned int DataLength);
static void SPPLEReceiveCreditEvent(LE_Context_Info_t *LEContextInfo, DeviceInfo_t *DeviceInfo, unsigned int Credits);
static unsigned int SPPLESendData(LE_Context_Info_t *LEContextInfo, DeviceInfo_t *DeviceInfo, unsigned int DataLength, Byte_t *Data);
static void SPPLEDataIndicationEvent(LE_Context_Info_t *LEContextInfo, DeviceInfo_t *DeviceInfo, unsigned int DataLength, Byte_t *Data);
static int SPPLEReadData(LE_Context_Info_t *LEContextInfo, DeviceInfo_t *DeviceInfo, unsigned int BufferLength, Byte_t *Buffer);

static int StartScan(unsigned int BluetoothStackID);
static int StopScan(unsigned int BluetoothStackID);

static int ConnectLEDevice(unsigned int BluetoothStackID, BD_ADDR_t BD_ADDR, GAP_LE_Address_Type_t RemoteAddressType ,GAP_LE_Address_Type_t OwnAddressType, Boolean_t UseWhiteList);
static int DisconnectLEDevice(unsigned int BluetoothStackID, BD_ADDR_t BD_ADDR);
static int CancelConnectLEDevice(unsigned int BluetoothStackID, BD_ADDR_t BD_ADDR);

static void ConfigureCapabilities(GAP_LE_Pairing_Capabilities_t *Capabilities);
static int SendPairingRequest(BD_ADDR_t BD_ADDR, Boolean_t ConnectionMaster);
static int SlavePairingRequestResponse(BD_ADDR_t BD_ADDR);
static int EncryptionInformationRequestResponse(BD_ADDR_t BD_ADDR, Byte_t KeySize, GAP_LE_Authentication_Response_Information_t *GAP_LE_Authentication_Response_Information);
static int DeleteLinkKey(BD_ADDR_t BD_ADDR);

static int DisplayHelp(ParameterList_t *TempParam);
static int QueryMemory(ParameterList_t *TempParam);
static int SetLEDiscoverabilityMode(ParameterList_t *TempParam);
static int SetLEConnectabilityMode(ParameterList_t *TempParam);
static int SetLEPairabilityMode(ParameterList_t *TempParam);
static int GetLocalAddress(ParameterList_t *TempParam);
static int SetBaudRate(ParameterList_t *TempParam);
static int ChangeLEPairingParameters(ParameterList_t *TempParam);
static int LEPassKeyResponse(ParameterList_t *TempParam);
static int LEQueryEncryption(ParameterList_t *TempParam);
static int LESetPasskey(ParameterList_t *TempParam);
static int AdvertiseLE(ParameterList_t *TempParam);
static int StartScanning(ParameterList_t *TempParam);
static int StopScanning(ParameterList_t *TempParam);
static int ConnectLE(ParameterList_t *TempParam);
static int DisconnectLE(ParameterList_t *TempParam);
static int CancelConnectLE(ParameterList_t *TempParam);
static int PairLE(ParameterList_t *TempParam);
static int DiscoverGAPS(ParameterList_t *TempParam);
static int ReadLocalName(ParameterList_t *TempParam);
static int SetLocalName(ParameterList_t *TempParam);
static int ReadRemoteName(ParameterList_t *TempParam);
static int ReadLocalAppearance(ParameterList_t *TempParam);
static int SetLocalAppearance(ParameterList_t *TempParam);
static int ReadRemoteAppearance(ParameterList_t *TempParam);
static int DiscoverSPPLE(ParameterList_t *TempParam);
static int RegisterSPPLE(ParameterList_t *TempParam);
static int ConfigureSPPLE(ParameterList_t *TempParam);
static int SendDataCommand(ParameterList_t *TempParam);
static int ReadDataCommand(ParameterList_t *TempParam);
static int Loopback(ParameterList_t *TempParam);
static int DisplayRawModeData(ParameterList_t *TempParam);
static int AutomaticReadMode(ParameterList_t *TempParam);

static int Inquiry(ParameterList_t *TempParam);
static int DisplayInquiryList(ParameterList_t *TempParam);
static int SetCBDiscoverabilityMode(ParameterList_t *TempParam);
static int SetCBConnectabilityMode(ParameterList_t *TempParam);
static int SetCBPairabilityMode(ParameterList_t *TempParam);
static int ChangeSimplePairingParameters(ParameterList_t *TempParam);
static void ResolveRemoteAddressHelper(BD_ADDR_t BD_ADDR);
static int Pair(ParameterList_t *TempParam);
static int EndPairing(ParameterList_t *TempParam);
static int PINCodeResponse(ParameterList_t *TempParam);
static int PassKeyResponse(ParameterList_t *TempParam);
static int UserConfirmationResponse(ParameterList_t *TempParam);
static int GetLocalName(ParameterList_t *TempParam);
static int SetClassOfDevice(ParameterList_t *TempParam);
static int GetClassOfDevice(ParameterList_t *TempParam);
static int GetRemoteName(ParameterList_t *TempParam);
static int SniffMode(ParameterList_t *TempParam);
static int ExitSniffMode(ParameterList_t *TempParam);
static int OpenServer(ParameterList_t *TempParam);
static int CloseServer(ParameterList_t *TempParam);
static int OpenRemoteServer(ParameterList_t *TempParam);
static int CloseRemoteServer(ParameterList_t *TempParam);
static int Read(ParameterList_t *TempParam);
static int Write(ParameterList_t *TempParam);
static int GetConfigParams(ParameterList_t *TempParam);
static int SetConfigParams(ParameterList_t *TempParam);
static int GetQueueParams(ParameterList_t *TempParam);
static int SetQueueParams(ParameterList_t *TempParam);
static int SendData(ParameterList_t *TempParam);

static int ServerMode(ParameterList_t *TempParam);
static int ClientMode(ParameterList_t *TempParam);

static int FindSPPPortIndex(unsigned int SerialPortID);
static int FindSPPPortIndexByServerPortNumber(unsigned int ServerPortNumber);
static int FindSPPPortIndexByAddress(BD_ADDR_t BD_ADDR);
static int FindFreeSPPPortIndex(void);
static int ClosePortByNumber(unsigned int LocalServerPort);
static int FindFreeLEIndex(void);
static int FindLEIndexByAddress(BD_ADDR_t BD_ADDR);
static int FindLEIndexByConnectionID(unsigned int ConnectionID);
static int UpdateConnectionID(unsigned int ConnectionID, BD_ADDR_t BD_ADDR);
static void RemoveConnectionInfo(BD_ADDR_t BD_ADDR);

   /* BTPS Callback function prototypes.                                */
static void BTPSAPI GAP_LE_Event_Callback(unsigned int BluetoothStackID,GAP_LE_Event_Data_t *GAP_LE_Event_Data, unsigned long CallbackParameter);
static void BTPSAPI GATT_ServerEventCallback(unsigned int BluetoothStackID, GATT_Server_Event_Data_t *GATT_ServerEventData, unsigned long CallbackParameter);
static void BTPSAPI GATT_ClientEventCallback_SPPLE(unsigned int BluetoothStackID, GATT_Client_Event_Data_t *GATT_Client_Event_Data, unsigned long CallbackParameter);
static void BTPSAPI GATT_ClientEventCallback_GAPS(unsigned int BluetoothStackID, GATT_Client_Event_Data_t *GATT_Client_Event_Data, unsigned long CallbackParameter);
static void BTPSAPI GATT_Connection_Event_Callback(unsigned int BluetoothStackID, GATT_Connection_Event_Data_t *GATT_Connection_Event_Data, unsigned long CallbackParameter);
static void BTPSAPI GATT_Service_Discovery_Event_Callback(unsigned int BluetoothStackID, GATT_Service_Discovery_Event_Data_t *GATT_Service_Discovery_Event_Data, unsigned long CallbackParameter);
static void BTPSAPI GAP_Event_Callback(unsigned int BluetoothStackID, GAP_Event_Data_t *GAP_Event_Data, unsigned long CallbackParameter);
static void BTPSAPI SPP_Event_Callback(unsigned int BluetoothStackID, SPP_Event_Data_t *SPP_Event_Data, unsigned long CallbackParameter);
static void BTPSAPI HCI_Event_Callback(unsigned int BluetoothStackID, HCI_Event_Data_t *HCI_Event_Data, unsigned long CallbackParameter);

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
static DeviceInfo_t *CreateNewDeviceInfoEntry(DeviceInfo_t **ListHead, GAP_LE_Address_Type_t ConnectionAddressType, BD_ADDR_t ConnectionBD_ADDR)
{
   DeviceInfo_t *DeviceInfoPtr = NULL;
   Boolean_t     Result = FALSE;

   /* Verify that the passed in parameters seem semi-valid.             */
   if((ListHead) && (!COMPARE_NULL_BD_ADDR(ConnectionBD_ADDR)))
   {
      /* Allocate the memory for the entry.                             */
      if((DeviceInfoPtr = BTPS_AllocateMemory(sizeof(DeviceInfo_t))) != NULL)
      {
         /* Initialize the entry.                                       */
         BTPS_MemInitialize(DeviceInfoPtr, 0, sizeof(DeviceInfo_t));
         DeviceInfoPtr->ConnectionAddressType = ConnectionAddressType;
         DeviceInfoPtr->ConnectionBD_ADDR     = ConnectionBD_ADDR;

         Result = BSC_AddGenericListEntry_Actual(ekBD_ADDR_t, BTPS_STRUCTURE_OFFSET(DeviceInfo_t, ConnectionBD_ADDR), BTPS_STRUCTURE_OFFSET(DeviceInfo_t, NextDeviceInfoInfoPtr), (void **)(ListHead), (void *)(DeviceInfoPtr));
         if(!Result)
         {
            /* Failed to add to list so we should free the memory that  */
            /* we allocated for the entry.                              */
            BTPS_FreeMemory(DeviceInfoPtr);
         }
      }
   }

   return(DeviceInfoPtr);
}

   /* The following function searches the specified List for the        */
   /* specified Connection BD_ADDR.  This function returns NULL if      */
   /* either the List Head is invalid, the BD_ADDR is invalid, or the   */
   /* Connection BD_ADDR was NOT found.                                 */
static DeviceInfo_t *SearchDeviceInfoEntryByBD_ADDR(DeviceInfo_t **ListHead, BD_ADDR_t BD_ADDR)
{
   return(BSC_SearchGenericListEntry(ekBD_ADDR_t, (void *)(&BD_ADDR), BTPS_STRUCTURE_OFFSET(DeviceInfo_t, ConnectionBD_ADDR), BTPS_STRUCTURE_OFFSET(DeviceInfo_t, NextDeviceInfoInfoPtr), (void **)(ListHead)));
}

   /* The following function searches the specified Key Info List for   */
   /* the specified BD_ADDR and removes it from the List.  This function*/
   /* returns NULL if either the List Head is invalid, the BD_ADDR is   */
   /* invalid, or the specified Entry was NOT present in the list.  The */
   /* entry returned will have the Next Entry field set to NULL, and    */
   /* the caller is responsible for deleting the memory associated with */
   /* this entry by calling the FreeKeyEntryMemory() function.          */
static DeviceInfo_t *DeleteDeviceInfoEntry(DeviceInfo_t **ListHead, BD_ADDR_t BD_ADDR)
{
   return(BSC_DeleteGenericListEntry(ekBD_ADDR_t, (void *)(&BD_ADDR), BTPS_STRUCTURE_OFFSET(DeviceInfo_t, ConnectionBD_ADDR), BTPS_STRUCTURE_OFFSET(DeviceInfo_t, NextDeviceInfoInfoPtr), (void **)(ListHead)));
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
   BSC_FreeGenericListEntryList((void **)(ListHead), BTPS_STRUCTURE_OFFSET(DeviceInfo_t, NextDeviceInfoInfoPtr));
}

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
   AddCommand("SETDISCOVERABILITYMODE", SetCBDiscoverabilityMode);
   AddCommand("SETCONNECTABILITYMODE", SetCBConnectabilityMode);
   AddCommand("SETPAIRABILITYMODE", SetCBPairabilityMode);
   AddCommand("CHANGESIMPLEPAIRINGPARAMETERS", ChangeSimplePairingParameters);
   AddCommand("GETLOCALADDRESS", GetLocalAddress);
   AddCommand("SETLOCALNAME", SetLocalName);
   AddCommand("GETLOCALNAME", GetLocalName);
   AddCommand("SETCLASSOFDEVICE", SetClassOfDevice);
   AddCommand("GETCLASSOFDEVICE", GetClassOfDevice);
   AddCommand("GETREMOTENAME", GetRemoteName);
   AddCommand("SNIFFMODE", SniffMode);
   AddCommand("EXITSNIFFMODE", ExitSniffMode);
   AddCommand("OPEN", OpenRemoteServer);
   AddCommand("CLOSE", CloseRemoteServer);
   AddCommand("READ", Read);
   AddCommand("WRITE", Write);
   AddCommand("GETCONFIGPARAMS", GetConfigParams);
   AddCommand("SETCONFIGPARAMS", SetConfigParams);
   AddCommand("GETQUEUEPARAMS", GetQueueParams);
   AddCommand("SETQUEUEPARAMS", SetQueueParams);
   AddCommand("LOOPBACK", Loopback);
   AddCommand("DISPLAYRAWMODEDATA", DisplayRawModeData);
   AddCommand("AUTOMATICREADMODE", AutomaticReadMode);
   AddCommand("SETBAUDRATE", SetBaudRate);
   AddCommand("CBSEND", SendData);
   AddCommand("SETDISCOVERABILITYMODE", SetLEDiscoverabilityMode);
   AddCommand("SETCONNECTABILITYMODE", SetLEConnectabilityMode);
   AddCommand("SETPAIRABILITYMODE", SetLEPairabilityMode);
   AddCommand("CHANGEPAIRINGPARAMETERS", ChangeLEPairingParameters);
   AddCommand("ADVERTISELE", AdvertiseLE);
   AddCommand("STARTSCANNING", StartScanning);
   AddCommand("STOPSCANNING", StopScanning);
   AddCommand("CONNECTLE", ConnectLE);
   AddCommand("DISCONNECTLE", DisconnectLE);
   AddCommand("CANCELCONNECTLE", CancelConnectLE);
   AddCommand("PAIRLE", PairLE);
   AddCommand("LEPASSKEYRESPONSE", LEPassKeyResponse);
   AddCommand("QUERYENCRYPTIONMODE", LEQueryEncryption);
   AddCommand("SETPASSKEY", LESetPasskey);
   AddCommand("DISCOVERSPPLE", DiscoverSPPLE);
   AddCommand("REGISTERSPPLE", RegisterSPPLE);
   AddCommand("CONFIGURESPPLE", ConfigureSPPLE);
   AddCommand("DISCOVERGAPS", DiscoverGAPS);
   AddCommand("GETLOCALNAME", ReadLocalName);
   AddCommand("GETLEREMOTENAME", ReadRemoteName);
   AddCommand("GETLOCALAPPEARANCE", ReadLocalAppearance);
   AddCommand("SETLOCALAPPEARANCE", SetLocalAppearance);
   AddCommand("GETREMOTEAPPEARANCE", ReadRemoteAppearance);
   AddCommand("LESEND", SendDataCommand);
   AddCommand("LEREAD", ReadDataCommand);
   AddCommand("HELP", DisplayHelp);
   AddCommand("QUERYMEMORY", QueryMemory);
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
   AddCommand("SETCBDISCOVERABILITYMODE", SetCBDiscoverabilityMode);
   AddCommand("SETCBCONNECTABILITYMODE", SetCBConnectabilityMode);
   AddCommand("SETCBPAIRABILITYMODE", SetCBPairabilityMode);
   AddCommand("CHANGESIMPLEPAIRINGPARAMETERS", ChangeSimplePairingParameters);
   AddCommand("GETLOCALADDRESS", GetLocalAddress);
   AddCommand("SETLOCALNAME", SetLocalName);
   AddCommand("GETLOCALNAME", GetLocalName);
   AddCommand("SETCLASSOFDEVICE", SetClassOfDevice);
   AddCommand("GETCLASSOFDEVICE", GetClassOfDevice);
   AddCommand("GETREMOTENAME", GetRemoteName);
   AddCommand("SNIFFMODE", SniffMode);
   AddCommand("EXITSNIFFMODE", ExitSniffMode);
   AddCommand("OPEN", OpenServer);
   AddCommand("CLOSE", CloseServer);
   AddCommand("READ", Read);
   AddCommand("WRITE", Write);
   AddCommand("GETCONFIGPARAMS", GetConfigParams);
   AddCommand("SETCONFIGPARAMS", SetConfigParams);
   AddCommand("GETQUEUEPARAMS", GetQueueParams);
   AddCommand("SETQUEUEPARAMS", SetQueueParams);
   AddCommand("LOOPBACK", Loopback);
   AddCommand("DISPLAYRAWMODEDATA", DisplayRawModeData);
   AddCommand("AUTOMATICREADMODE", AutomaticReadMode);
   AddCommand("SETBAUDRATE", SetBaudRate);
   AddCommand("CBSEND", SendData);
   AddCommand("SETDISCOVERABILITYMODE", SetLEDiscoverabilityMode);
   AddCommand("SETCONNECTABILITYMODE", SetLEConnectabilityMode);
   AddCommand("SETPAIRABILITYMODE", SetLEPairabilityMode);
   AddCommand("CHANGEPAIRINGPARAMETERS", ChangeLEPairingParameters);
   AddCommand("ADVERTISELE", AdvertiseLE);
   AddCommand("STARTSCANNING", StartScanning);
   AddCommand("STOPSCANNING", StopScanning);
   AddCommand("CONNECTLE", ConnectLE);
   AddCommand("DISCONNECTLE", DisconnectLE);
   AddCommand("CANCELCONNECTLE", CancelConnectLE);
   AddCommand("PAIRLE", PairLE);
   AddCommand("LEPASSKEYRESPONSE", LEPassKeyResponse);
   AddCommand("QUERYENCRYPTIONMODE", LEQueryEncryption);
   AddCommand("SETPASSKEY", LESetPasskey);
   AddCommand("DISCOVERSPPLE", DiscoverSPPLE);
   AddCommand("REGISTERSPPLE", RegisterSPPLE);
   AddCommand("CONFIGURESPPLE", ConfigureSPPLE);
   AddCommand("DISCOVERGAPS", DiscoverGAPS);
   AddCommand("GETLOCALNAME", ReadLocalName);
   AddCommand("GETLEREMOTENAME", ReadRemoteName);
   AddCommand("GETLOCALAPPEARANCE", ReadLocalAppearance);
   AddCommand("SETLOCALAPPEARANCE", SetLocalAppearance);
   AddCommand("GETREMOTEAPPEARANCE", ReadRemoteAppearance);
   AddCommand("LESEND", SendDataCommand);
   AddCommand("LEREAD", ReadDataCommand);
   AddCommand("HELP", DisplayHelp);
   AddCommand("QUERYMEMORY", QueryMemory);
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
         case EXIT_CODE:
            /* Close all connected ports.                               */
            if(UI_Mode == UI_MODE_IS_SERVER)
               CloseServer(NULL);
            else
            {
               if(UI_Mode == UI_MODE_IS_CLIENT)
                  CloseRemoteServer(NULL);
            }

            /* Restart the User Interface Selection.                    */
            UI_Mode = UI_MODE_SELECT;

            /* Set up the Selection Interface.                          */
            UserInterface_Selection();
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
            if((ret_val = ((*CommandFunction)(&TempCommand->Parameters))) == 0)
            {
               /* Return success to the caller.                         */
               ret_val = 0;
            }
            else
            {
               if ((ret_val != EXIT_CODE) && (ret_val != EXIT_TEST_MODE))
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
   Display(("BR/EDR: I/O Capabilities: %s, MITM: %s.\r\n", IOCapabilitiesStrings[(unsigned int)(IOCapability - icDisplayOnly)], MITMProtection?"TRUE":"FALSE"));
}

   /* Utility function to display a Class of Device Structure.          */
static void DisplayClassOfDevice(Class_of_Device_t Class_of_Device)
{
   Display(("Class of Device: 0x%02X%02X%02X.\r\n", Class_of_Device.Class_of_Device0, Class_of_Device.Class_of_Device1, Class_of_Device.Class_of_Device2));
}

   /* Utility function to display advertising data.                     */
static void DisplayAdvertisingData(GAP_LE_Advertising_Data_t *Advertising_Data)
{
   unsigned int Index;
   unsigned int Index2;

   /* Verify that the input parameters seem semi-valid.                 */
   if(Advertising_Data)
   {
      for(Index = 0; Index < Advertising_Data->Number_Data_Entries; Index++)
      {
         Display(("  AD Type: 0x%02X.\r\n", Advertising_Data->Data_Entries[Index].AD_Type));
         Display(("  AD Length: 0x%02X.\r\n", Advertising_Data->Data_Entries[Index].AD_Data_Length));
         if(Advertising_Data->Data_Entries[Index].AD_Data_Buffer)
         {
            Display(("  AD Data: "));
            for(Index2 = 0; Index2 < Advertising_Data->Data_Entries[Index].AD_Data_Length; Index2++)
            {
               Display(("0x%02X ", Advertising_Data->Data_Entries[Index].AD_Data_Buffer[Index2]));
            }
            Display(("\r\n"));
         }
      }
   }
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


   /* This function displays the usage of DisplayConnectLEUsage command */
static void DisplayConnectLEUsage(char *CharacteristicName)
{
   Display(("Usage: %s \t", CharacteristicName));
   Display((" [BD_ADDR] (default Public Addresses)\r\n"));
   Display(("[RemoteDeviceAddressType (0 = Public Address, 1 = Random Address )(Optional)]\r\n"));
   Display(("[OwnAddressType          (0 = Public Address, 1 = Random Address )(Optional)].\r\n"));
}

   /* The following function is responsible for opening the SS1         */
   /* Bluetooth Protocol Stack.  This function accepts a pre-populated  */
   /* HCI Driver Information structure that contains the HCI Driver     */
   /* Transport Information.  This function returns zero on successful  */
   /* execution and a negative value on all errors.                     */
static int OpenStack(HCI_DriverInformation_t *HCI_DriverInformation, BTPS_Initialization_t *BTPS_Initialization)
{
   int                           i;
   int                           Result;
   int                           ret_val = 0;
   char                          BluetoothAddress[16];
   Byte_t                        Status;
   Byte_t                        NumberLEPackets;
   Word_t                        LEPacketLength;
   BD_ADDR_t                     BD_ADDR;
   unsigned int                  ServiceID;
   HCI_Version_t                 HCIVersion;
   L2CA_Link_Connect_Params_t    L2CA_Link_Connect_Params;

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

         /* Next, check the return value of the initialization to see   */
         /* if it was successful.                                       */
         if(Result > 0)
         {
            /* The Stack was initialized successfully, inform the user  */
            /* and set the return value of the initialization function  */
            /* to the Bluetooth Stack ID.                               */
            BluetoothStackID = Result;
            Display(("Bluetooth Stack ID: %d\r\n", BluetoothStackID));

            ret_val          = 0;

            /* Attempt to enable the WBS feature.                       */
            Result = BSC_EnableFeature(BluetoothStackID, BSC_FEATURE_BLUETOOTH_LOW_ENERGY);
            if(!Result)
            {
               Display(("LOW ENERGY Support initialized.\r\n"));
            }
            else
            {
               Display(("LOW ENERGY Support not initialized %d.\r\n", Result));
            }


            /* Initialize the Default Pairing Parameters.               */
            LE_Parameters.IOCapability   = DEFAULT_LE_IO_CAPABILITY;
            LE_Parameters.MITMProtection = DEFAULT_LE_MITM_PROTECTION;
            LE_Parameters.OOBDataPresent = FALSE;

            /* Initialize the default Secure Simple Pairing parameters. */
            IOCapability                 = DEFAULT_IO_CAPABILITY;
            OOBSupport                   = FALSE;
            MITMProtection               = DEFAULT_MITM_PROTECTION;

            if(!HCI_Version_Supported(BluetoothStackID, &HCIVersion))
               Display(("Device Chipset: %s\r\n", (HCIVersion <= NUM_SUPPORTED_HCI_VERSIONS)?HCIVersionStrings[HCIVersion]:HCIVersionStrings[NUM_SUPPORTED_HCI_VERSIONS]));

            /* Printing the BTPS version                                */
            Display(("BTPS Version  : %s \r\n", BTPS_VERSION_VERSION_STRING));
            /* Printing the FW version                                  */
            DisplayFWVersion();

            /* Printing the Demo Application name and version           */
            Display(("App Name      : %s \r\n", LE_APP_DEMO_NAME));
            Display(("App Version   : %s \r\n", DEMO_APPLICATION_VERSION_STRING));

            /* Let's output the Bluetooth Device Address so that the    */
            /* user knows what the Device Address is.                   */
            if(!GAP_Query_Local_BD_ADDR(BluetoothStackID, &BD_ADDR))
            {
               BD_ADDRToStr(BD_ADDR, BluetoothAddress);

               Display(("LOCAL BD_ADDR: %s\r\n", BluetoothAddress));
            }

            if(HCI_Command_Supported(BluetoothStackID, HCI_SUPPORTED_COMMAND_WRITE_DEFAULT_LINK_POLICY_BIT_NUMBER) > 0)
               HCI_Write_Default_Link_Policy_Settings(BluetoothStackID, (HCI_LINK_POLICY_SETTINGS_ENABLE_MASTER_SLAVE_SWITCH|HCI_LINK_POLICY_SETTINGS_ENABLE_SNIFF_MODE), &Status);

            /* Go ahead and allow Master/Slave Role Switch.             */
            L2CA_Link_Connect_Params.L2CA_Link_Connect_Request_Config  = cqAllowRoleSwitch;
            L2CA_Link_Connect_Params.L2CA_Link_Connect_Response_Config = csMaintainCurrentRole;

            L2CA_Set_Link_Connection_Configuration(BluetoothStackID, &L2CA_Link_Connect_Params);

            if(HCI_Command_Supported(BluetoothStackID, HCI_SUPPORTED_COMMAND_WRITE_DEFAULT_LINK_POLICY_BIT_NUMBER) > 0)
               HCI_Write_Default_Link_Policy_Settings(BluetoothStackID, (HCI_LINK_POLICY_SETTINGS_ENABLE_MASTER_SLAVE_SWITCH|HCI_LINK_POLICY_SETTINGS_ENABLE_SNIFF_MODE), &Status);

            /* Delete all Stored Link Keys.                             */
            ASSIGN_BD_ADDR(BD_ADDR, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);

            DeleteLinkKey(BD_ADDR);

            /* Flag that no connection is currently active.             */
            ASSIGN_BD_ADDR(CurrentLERemoteBD_ADDR, 0, 0, 0, 0, 0, 0);
            ASSIGN_BD_ADDR(CurrentCBRemoteBD_ADDR, 0, 0, 0, 0, 0, 0);
            LocalDeviceIsMaster = FALSE;

            for(i=0; i<MAX_LE_CONNECTIONS; i++)
            {
               LEContextInfo[i].ConnectionID = 0;
               ASSIGN_BD_ADDR(LEContextInfo[i].ConnectionBD_ADDR, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
            }

            /* Regenerate IRK and DHK from the constant Identity Root   */
            /* Key.                                                     */
            GAP_LE_Diversify_Function(BluetoothStackID, (Encryption_Key_t *)(&IR), 1,0, &IRK);
            GAP_LE_Diversify_Function(BluetoothStackID, (Encryption_Key_t *)(&IR), 3, 0, &DHK);

            /* Flag that we have no Key Information in the Key List.    */
            DeviceInfoList = NULL;

            /* Initialize the GATT Service.                             */
            if((Result = GATT_Initialize(BluetoothStackID, GATT_INITIALIZATION_FLAGS_SUPPORT_LE, GATT_Connection_Event_Callback, 0)) == 0)
            {
               /* Determine the number of LE packets that the controller*/
               /* will accept at a time.                                */
               if((!HCI_LE_Read_Buffer_Size(BluetoothStackID, &Status, &LEPacketLength, &NumberLEPackets)) && (!Status) && (LEPacketLength))
               {
                  NumberLEPackets = (NumberLEPackets/MAX_LE_CONNECTIONS);
                  NumberLEPackets = (NumberLEPackets == 0)?1:NumberLEPackets;
               }
               else
                  NumberLEPackets = 1;

               /* Set a limit on the number of packets that we will     */
               /* queue internally.                                     */
               GATT_Set_Queuing_Parameters(BluetoothStackID, (unsigned int)NumberLEPackets, (unsigned int)(NumberLEPackets-1), FALSE);

               /* Initialize the GAPS Service.                          */
               Result = GAPS_Initialize_Service(BluetoothStackID, &ServiceID);
               if(Result > 0)
               {
                  /* Save the Instance ID of the GAP Service.           */
                  GAPSInstanceID = (unsigned int)Result;

                  /* Set the GAP Device Name and Device Appearance.     */
                  GAP_Set_Local_Device_Name (BluetoothStackID,LE_APP_DEMO_NAME);
                  GAPS_Set_Device_Name(BluetoothStackID, GAPSInstanceID, LE_APP_DEMO_NAME);
                  GAPS_Set_Device_Appearance(BluetoothStackID, GAPSInstanceID, GAP_DEVICE_APPEARENCE_VALUE_GENERIC_COMPUTER);

                  /* Return success to the caller.                      */
                  ret_val        = 0;
               }
               else
               {
                  /* The Stack was NOT initialized successfully, inform */
                  /* the user and set the return value of the           */
                  /* initialization function to an error.               */
                  DisplayFunctionError("GAPS_Initialize_Service", Result);

                  /* Cleanup GATT Module.                               */
                  GATT_Cleanup(BluetoothStackID);

                  BluetoothStackID = 0;

                  ret_val          = UNABLE_TO_INITIALIZE_STACK;
               }
            }
            else
            {
               /* The Stack was NOT initialized successfully, inform the*/
               /* user and set the return value of the initialization   */
               /* function to an error.                                 */
               DisplayFunctionError("GATT_Initialize", Result);

               BluetoothStackID = 0;

               ret_val          = UNABLE_TO_INITIALIZE_STACK;
            }

            /* Initialize SPP context.                                  */
            BTPS_MemInitialize(SPPContextInfo, 0, sizeof(SPPContextInfo));
         }
         else
         {
            /* The Stack was NOT initialized successfully, inform the   */
            /* user and set the return value of the initialization      */
            /* function to an error.                                    */
            DisplayFunctionError("BSC_Initialize", Result);

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
      /* Cleanup GAP Service Module.                                    */
      if(GAPSInstanceID)
         GAPS_Cleanup_Service(BluetoothStackID, GAPSInstanceID);

      /* Un-registered SPP LE Service.                                  */
      if(SPPLEServiceID)
         GATT_Un_Register_Service(BluetoothStackID, SPPLEServiceID);

      /* Cleanup GATT Module.                                           */
      GATT_Cleanup(BluetoothStackID);

      /* Simply close the Stack                                         */
      BSC_Shutdown(BluetoothStackID);

      /* Free BTPSKRNL allocated memory.                                */
      BTPS_DeInit();

      Display(("Stack Shutdown.\r\n"));

      /* Free the Key List.                                             */
      FreeDeviceInfoList(&DeviceInfoList);

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
      if(!ret_val)
      {
         /* * NOTE * Discoverability is only applicable when we are     */
         /*          advertising so save the default Discoverability    */
         /*          Mode for later.                                    */
         LE_Parameters.DiscoverabilityMode = dmGeneralDiscoverableMode;
      }
      else
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
      if(!ret_val)
      {
         /* * NOTE * Connectability is only an applicable when          */
         /*          advertising so we will just save the default       */
         /*          connectability for the next time we enable         */
         /*          advertising.                                       */
         LE_Parameters.ConnectableMode = lcmConnectable;
      }
      else
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
         if(!Result)
         {
            /* Now Set the LE Pairability.                              */

            /* Attempt to set the attached device to be pairable.       */
            Result = GAP_LE_Set_Pairability_Mode(BluetoothStackID, lpmPairableMode);

            /* Next, check the return value of the GAP Set Pairability  */
            /* mode command for successful execution.                   */
            if(!Result)
            {
               /* The device has been set to pairable mode, now register*/
               /* an Authentication Callback to handle the              */
               /* Authentication events if required.                    */
               Result = GAP_LE_Register_Remote_Authentication(BluetoothStackID, GAP_LE_Event_Callback, (unsigned long)0);

               /* Next, check the return value of the GAP Register      */
               /* Remote Authentication command for successful          */
               /* execution.                                            */
               if(Result)
               {
                  /* An error occurred while trying to execute this     */
                  /* function.                                          */
                  DisplayFunctionError("GAP_LE_Register_Remote_Authentication", Result);

                  ret_val = Result;
               }
            }
            else
            {
               /* An error occurred while trying to make the device     */
               /* pairable.                                             */
               DisplayFunctionError("GAP_LE_Set_Pairability_Mode", Result);

               ret_val = Result;
            }
         }
         else
         {
            /* An error occurred while trying to execute this function. */
            DisplayFunctionError("GAP_Register_Remote_Authentication", Result);

            ret_val = Result;
         }
      }
      else
      {
         /* An error occurred while trying to make the device pairable. */
         DisplayFunctionError("GAP_Set_Pairability_Mode", Result);

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

   /* The following function is a utility function that is used to dump */
   /* the Appearance to String Mapping Table.                           */
static void DumpAppearanceMappings(void)
{
   unsigned int Index;

   for(Index=0;Index<NUMBER_OF_APPEARANCE_MAPPINGS;++Index)
      Display(("   %u = %s.\r\n", Index, AppearanceMappings[Index].String));
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

   /* The following function is used to map an Index into the Appearance*/
   /* Mapping table to it's Appearance Value.  This function returns    */
   /* TRUE on success or FALSE otherwise.                               */
static Boolean_t AppearanceIndexToAppearance(unsigned int Index, Word_t *Appearance)
{
   Boolean_t ret_val;

   if((Index < NUMBER_OF_APPEARANCE_MAPPINGS) && (Appearance))
   {
      *Appearance = AppearanceMappings[Index].Appearance;
      ret_val     = TRUE;
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

   /* The following function is a utility function that provides a      */
   /* mechanism of populating a BRSM Client Information structure with  */
   /* the information discovered from a GATT Service Discovery          */
   /* operation.                                                        */
static void SPPLEPopulateHandles(SPPLE_Client_Info_t *ClientInfo, GATT_Service_Discovery_Indication_Data_t *ServiceInfo)
{
   Word_t                                       *ClientConfigurationHandle;
   unsigned int                                  Index1;
   unsigned int                                  Index2;
   GATT_Characteristic_Information_t            *CurrentCharacteristic;
   GATT_Characteristic_Descriptor_Information_t *CurrentDescriptor;

   /* Verify that the input parameters are semi-valid.                  */
   if((ClientInfo) && (ServiceInfo) && (ServiceInfo->ServiceInformation.UUID.UUID_Type == guUUID_128) && (SPPLE_COMPARE_SPPLE_SERVICE_UUID_TO_UUID_128(ServiceInfo->ServiceInformation.UUID.UUID.UUID_128)))
   {
      /* Loop through all characteristics discovered in the service     */
      /* and populate the correct entry.                                */
      CurrentCharacteristic = ServiceInfo->CharacteristicInformationList;
      if(CurrentCharacteristic)
      {
         for(Index1=0;Index1<ServiceInfo->NumberOfCharacteristics;Index1++,CurrentCharacteristic++)
         {
            /* All SPPLE UUIDs are defined to be 128 bit UUIDs.         */
            if(CurrentCharacteristic->Characteristic_UUID.UUID_Type == guUUID_128)
            {
               ClientConfigurationHandle = NULL;

               /* Determine which characteristic this is.               */
               if(!SPPLE_COMPARE_SPPLE_TX_UUID_TO_UUID_128(CurrentCharacteristic->Characteristic_UUID.UUID.UUID_128))
               {
                  if(!SPPLE_COMPARE_SPPLE_TX_CREDITS_UUID_TO_UUID_128(CurrentCharacteristic->Characteristic_UUID.UUID.UUID_128))
                  {
                     if(!SPPLE_COMPARE_SPPLE_RX_UUID_TO_UUID_128(CurrentCharacteristic->Characteristic_UUID.UUID.UUID_128))
                     {
                        if(!SPPLE_COMPARE_SPPLE_RX_CREDITS_UUID_TO_UUID_128(CurrentCharacteristic->Characteristic_UUID.UUID.UUID_128))
                           continue;
                        else
                        {
                           ClientInfo->Rx_Credit_Characteristic = CurrentCharacteristic->Characteristic_Handle;
                           ClientConfigurationHandle            = &(ClientInfo->Rx_Credit_Client_Configuration_Descriptor);
                        }
                     }
                     else
                     {
                        ClientInfo->Rx_Characteristic = CurrentCharacteristic->Characteristic_Handle;
                        continue;
                     }
                  }
                  else
                  {
                     ClientInfo->Tx_Credit_Characteristic = CurrentCharacteristic->Characteristic_Handle;
                     continue;
                  }
               }
               else
               {
                  ClientInfo->Tx_Characteristic = CurrentCharacteristic->Characteristic_Handle;
                  ClientConfigurationHandle     = &(ClientInfo->Tx_Client_Configuration_Descriptor);
               }

               /* Loop through the Descriptor List.                     */
               CurrentDescriptor = CurrentCharacteristic->DescriptorList;
               if((CurrentDescriptor) && (ClientConfigurationHandle))
               {
                  for(Index2=0;Index2<CurrentCharacteristic->NumberOfDescriptors;Index2++,CurrentDescriptor++)
                  {
                     if(CurrentDescriptor->Characteristic_Descriptor_UUID.UUID_Type == guUUID_16)
                     {
                        if(GATT_COMPARE_CLIENT_CHARACTERISTIC_CONFIGURATION_ATTRIBUTE_TYPE_TO_BLUETOOTH_UUID_16(CurrentDescriptor->Characteristic_Descriptor_UUID.UUID.UUID_16))
                        {
                           *ClientConfigurationHandle = CurrentDescriptor->Characteristic_Descriptor_Handle;
                           break;
                        }
                     }
                  }
               }
            }
         }
      }
   }
}

   /* The following function is a utility function that is used to add  */
   /* data (using InIndex as the buffer index) from the buffer specified*/
   /* by the DataBuffer parameter.  The second and third parameters     */
   /* specified the length of the data to add and the pointer to the    */
   /* data to add to the buffer.  This function returns the actual      */
   /* number of bytes that were added to the buffer (or 0 if none were  */
   /* added).                                                           */
static unsigned int AddDataToBuffer(SPPLE_Data_Buffer_t *DataBuffer, unsigned int DataLength, Byte_t *Data)
{
   unsigned int BytesAdded = 0;
   unsigned int Count;

   /* Verify that the input parameters are valid.                       */
   if((DataBuffer) && (DataLength) && (Data))
   {
      /* Loop while we have data AND space in the buffer.               */
      while(DataLength)
      {
         /* Get the number of bytes that can be placed in the buffer    */
         /* until it wraps.                                             */
         Count = DataBuffer->BufferSize - DataBuffer->InIndex;

         /* Determine if the number of bytes free is less than the      */
         /* number of bytes till we wrap and choose the smaller of the  */
         /* numbers.                                                    */
         Count = (DataBuffer->BytesFree < Count)?DataBuffer->BytesFree:Count;

         /* Cap the Count that we add to buffer to the length of the    */
         /* data provided by the caller.                                */
         Count = (Count > DataLength)?DataLength:Count;

         if(Count)
         {
            /* Copy the data into the buffer.                           */
            BTPS_MemCopy(&DataBuffer->Buffer[DataBuffer->InIndex], Data, Count);

            /* Update the counts.                                       */
            DataBuffer->InIndex   += Count;
            DataBuffer->BytesFree -= Count;
            DataLength            -= Count;
            BytesAdded            += Count;
            Data                  += Count;

            /* Wrap the InIndex if necessary.                           */
            if(DataBuffer->InIndex >= DataBuffer->BufferSize)
               DataBuffer->InIndex = 0;
         }
         else
            break;
      }
   }

   return(BytesAdded);
}

   /* The following function is a utility function that is used to      */
   /* removed data (using OutIndex as the buffer index) from the buffer */
   /* specified by the DataBuffer parameter The second parameter        */
   /* specifies the length of the Buffer that is pointed to by the third*/
   /* parameter.  This function returns the actual number of bytes that */
   /* were removed from the DataBuffer (or 0 if none were added).       */
   /* * NOTE * Buffer is optional and if not specified up to            */
   /*          BufferLength bytes will be deleted from the Buffer.      */
static unsigned int RemoveDataFromBuffer(SPPLE_Data_Buffer_t *DataBuffer, unsigned int BufferLength, Byte_t *Buffer)
{
   unsigned int Count;
   unsigned int BytesRemoved = 0;
   unsigned int MaxRemove;

   /* Verify that the input parameters are valid.                       */
   if((DataBuffer) && (BufferLength))
   {
      /* Loop while we have data to remove and space in the buffer to   */
      /* place it.                                                      */
      while(BufferLength)
      {
         /* Determine the number of bytes that are present in the       */
         /* buffer.                                                     */
         Count = DataBuffer->BufferSize - DataBuffer->BytesFree;
         if(Count)
         {
            /* Calculate the maximum number of bytes that I can remove  */
            /* from the buffer before it wraps.                         */
            MaxRemove = DataBuffer->BufferSize - DataBuffer->OutIndex;

            /* Cap max we can remove at the BufferLength of the caller's*/
            /* buffer.                                                  */
            MaxRemove = (MaxRemove > BufferLength)?BufferLength:MaxRemove;

            /* Cap the number of bytes I will remove in this iteration  */
            /* at the maximum I can remove or the number of bytes that  */
            /* are in the buffer.                                       */
            Count = (Count > MaxRemove)?MaxRemove:Count;

            /* Copy the data into the caller's buffer (If specified).   */
            if(Buffer)
            {
               BTPS_MemCopy(Buffer, &DataBuffer->Buffer[DataBuffer->OutIndex], Count);
               Buffer += Count;
            }

            /* Update the counts.                                       */
            DataBuffer->OutIndex  += Count;
            DataBuffer->BytesFree += Count;
            BytesRemoved          += Count;
            BufferLength          -= Count;

            /* Wrap the OutIndex if necessary.                          */
            if(DataBuffer->OutIndex >= DataBuffer->BufferSize)
               DataBuffer->OutIndex = 0;
         }
         else
            break;
      }
   }

   return(BytesRemoved);
}

   /* The following function is used to initialize the specified buffer */
   /* to the defaults.                                                  */
static void InitializeBuffer(SPPLE_Data_Buffer_t *DataBuffer)
{
   /* Verify that the input parameters are valid.                       */
   if(DataBuffer)
   {
      DataBuffer->BufferSize = SPPLE_DATA_CREDITS;
      DataBuffer->BytesFree  = SPPLE_DATA_CREDITS;
      DataBuffer->InIndex    = 0;
      DataBuffer->OutIndex   = 0;
   }
}

   /* The following function function is used to enable/disable         */
   /* notifications on a specified handle.  This function returns the   */
   /* positive non-zero Transaction ID of the Write Request or a        */
   /* negative error code.                                              */
static int EnableDisableNotificationsIndications(unsigned int ConnectionID, Word_t ClientConfigurationHandle, Word_t ClientConfigurationValue, GATT_Client_Event_Callback_t ClientEventCallback)
{
   int              ret_val;
   NonAlignedWord_t Buffer;

   /* Verify the input parameters.                                      */
   if((BluetoothStackID) && (ConnectionID) && (ClientConfigurationHandle))
   {
      ASSIGN_HOST_WORD_TO_LITTLE_ENDIAN_UNALIGNED_WORD(&Buffer, ClientConfigurationValue);

      ret_val = GATT_Write_Request(BluetoothStackID, ConnectionID, ClientConfigurationHandle, sizeof(Buffer), &Buffer, ClientEventCallback, 0);
   }
   else
      ret_val = BTPS_ERROR_INVALID_PARAMETER;

   return(ret_val);
}

   /* The following function is a utility function that exists to fill  */
   /* the specified buffer with the DataStr that is used to send data.  */
   /* This function will fill from the CurrentBufferLength up to Max    */
   /* Length in Buffer.  CurrentBufferLength is used to return the total*/
   /* length of the buffer.  The last parameter specifies the data      */
   /* string index which is used to fill any remainder of the string so */
   /* that there are no breaks in the pattern.  This function returns   */
   /* the number of bytes added to the transmit buffer.                 */
static unsigned int FillBufferWithString(SPPLE_Data_Buffer_t *DataBuffer, unsigned *CurrentBufferLength, unsigned int MaxLength, Byte_t *Buffer, unsigned int *DataStrIndex)
{
   unsigned int DataCount;
   unsigned int Added2Buffer = 0;
   unsigned int RemainingDataStrLen;

   /* Verify that the input parameter is semi-valid.                    */
   if((DataBuffer) && (CurrentBufferLength) && (MaxLength) && (Buffer))
   {
      /* Copy as much of the DataStr into the Transmit buffer as is     */
      /* possible.                                                      */
      while(*CurrentBufferLength < MaxLength)
      {
         /* Cap the data to copy at the maximum of the string length and*/
         /* the remaining amount that can be placed in the buffer.      */
         DataCount = (DataStrLen > (MaxLength - (*CurrentBufferLength))) ? (MaxLength - (*CurrentBufferLength)) : DataStrLen;

         /* Calculate the remaining space in the data string.           */
         RemainingDataStrLen = DataStrLen - (*DataStrIndex);

         /* Possibly reduce the amount of data to add to the buffer by  */
         /* the current index within the data string.                   */
         DataCount = DataCount > RemainingDataStrLen ? RemainingDataStrLen : DataCount;

         /* Build the data string into the SPPLEBuffer.                 */
         BTPS_MemCopy(&Buffer[*CurrentBufferLength], &DataStr[*DataStrIndex], DataCount);

         /* Increment the data string index.                            */
         *DataStrIndex += DataCount;

         if((*DataStrIndex) >= DataStrLen)
            *DataStrIndex = 0;

         /* Increment the index.                                        */
         *CurrentBufferLength += DataCount;
      }
   }

   return(Added2Buffer);
}

   /* The following function is responsible for handling a Send Process.*/
static void SPPLESendProcess(LE_Context_Info_t *LEContextInfo, DeviceInfo_t *DeviceInfo)
{
   int          Result;
   Boolean_t    Done = FALSE;
   unsigned int TransmitIndex;
   unsigned int MaxLength;
   unsigned int SPPLEBufferLength;
   unsigned int Added2Buffer;

   /* Verify that the input parameter is semi-valid.                    */
   if((LEContextInfo) && (DeviceInfo))
   {
      /* Loop while we have data to send and we have not used up all    */
      /* Transmit Credits.                                              */
      TransmitIndex     = 0;
      SPPLEBufferLength = 0;
      Added2Buffer      = 0;
      while((LEContextInfo->SPPLEBufferInfo.SendInfo.BytesToSend) && (LEContextInfo->BufferFull == FALSE) && (LEContextInfo->SPPLEBufferInfo.TransmitCredits) && (!Done))
      {
         /* Get the maximum length of what we can send in this          */
         /* transaction.                                                */
         MaxLength = (LEContextInfo->SPPLEBufferInfo.SendInfo.BytesToSend > LEContextInfo->SPPLEBufferInfo.TransmitCredits)?LEContextInfo->SPPLEBufferInfo.TransmitCredits:LEContextInfo->SPPLEBufferInfo.SendInfo.BytesToSend;
         MaxLength = (MaxLength > SPPLE_DATA_BUFFER_LENGTH)?SPPLE_DATA_BUFFER_LENGTH:MaxLength;

         /* If we do not have any outstanding data get some more data.  */
         if(!SPPLEBufferLength)
         {
            /* Send any buffered data first.                            */
            if(LEContextInfo->SPPLEBufferInfo.TransmitBuffer.BytesFree != LEContextInfo->SPPLEBufferInfo.TransmitBuffer.BufferSize)
            {
               /* Remove the queued data from the Transmit Buffer.      */
               SPPLEBufferLength = RemoveDataFromBuffer(&(LEContextInfo->SPPLEBufferInfo.TransmitBuffer), MaxLength, SPPLEBuffer);

               /* If we added some data to the transmit buffer decrement*/
               /* what we just removed.                                 */
               if(Added2Buffer>=SPPLEBufferLength)
                  Added2Buffer -= SPPLEBufferLength;
            }

            /* Fill up the rest of the buffer with the data string.     */
            Added2Buffer     += FillBufferWithString(&(LEContextInfo->SPPLEBufferInfo.TransmitBuffer), &SPPLEBufferLength, MaxLength, SPPLEBuffer, &(LEContextInfo->SPPLEBufferInfo.SendInfo.DataStrIndex));

            /* Reset the Transmit Index to 0.                           */
            TransmitIndex     = 0;

            /* If we don't have any data to send (i.e. we didn't have   */
            /* enough credits to fill up a string) we should just exit  */
            /* the loop.                                                */
            if(SPPLEBufferLength == 0)
               break;
         }

         /* Use the correct API based on device role for SPPLE.         */
         if(DeviceInfo->Flags & DEVICE_INFO_FLAGS_SPPLE_SERVER)
         {
            /* We are acting as SPPLE Server, so notify the Tx          */
            /* Characteristic.                                          */
            if(DeviceInfo->ServerInfo.Tx_Client_Configuration_Descriptor == GATT_CLIENT_CONFIGURATION_CHARACTERISTIC_NOTIFY_ENABLE)
               Result = GATT_Handle_Value_Notification(BluetoothStackID, SPPLEServiceID, LEContextInfo->ConnectionID, SPPLE_TX_CHARACTERISTIC_ATTRIBUTE_OFFSET, (Word_t)SPPLEBufferLength, &SPPLEBuffer[TransmitIndex]);
            else
            {
               /* Not configured for notifications so exit the loop.    */
               Done = TRUE;
            }
         }
         else
         {
            /* We are acting as SPPLE Client, so write to the Rx        */
            /* Characteristic.                                          */
            if(DeviceInfo->ClientInfo.Tx_Characteristic)
               Result = GATT_Write_Without_Response_Request(BluetoothStackID, LEContextInfo->ConnectionID, DeviceInfo->ClientInfo.Rx_Characteristic, (Word_t)SPPLEBufferLength, &SPPLEBuffer[TransmitIndex]);
            else
            {
               /* We have not discovered the Tx Characteristic, so exit */
               /* the loop.                                             */
               Done = TRUE;
            }
         }

         /* Check to see if any data was written.                       */
         if(!Done)
         {
            /* Check to see if the data was written successfully.       */
            if(Result >= 0)
            {
               /* Adjust the counters.                                  */
               LEContextInfo->SPPLEBufferInfo.SendInfo.BytesToSend -= (unsigned int)Result;
               LEContextInfo->SPPLEBufferInfo.SendInfo.BytesSent   += (unsigned int)Result;
               TransmitIndex                                       += (unsigned int)Result;
               SPPLEBufferLength                                   -= (unsigned int)Result;
               LEContextInfo->SPPLEBufferInfo.TransmitCredits      -= (unsigned int)Result;

               /* If we have no more remaining Tx Credits AND we have   */
               /* data built up to send, we need to queue this in the Tx*/
               /* Buffer.                                               */
               if((!(LEContextInfo->SPPLEBufferInfo.TransmitCredits)) && (SPPLEBufferLength))
               {
                  /* Add the remaining data to the transmit buffer.     */
                  AddDataToBuffer(&(LEContextInfo->SPPLEBufferInfo.TransmitBuffer), SPPLEBufferLength, &SPPLEBuffer[TransmitIndex]);

                  SPPLEBufferLength = 0;
               }
            }
            else
            {
               /* Check to see what error has occurred.                 */
               if(Result == BTPS_ERROR_INSUFFICIENT_BUFFER_SPACE)
               {
                  /* Queue is full so add the data that we have into the*/
                  /* transmit buffer for this device and wait on the    */
                  /* buffer to empty.                                   */
                  AddDataToBuffer(&(LEContextInfo->SPPLEBufferInfo.TransmitBuffer), SPPLEBufferLength, &SPPLEBuffer[TransmitIndex]);

                  SPPLEBufferLength         = 0;

                  /* Flag that the LE buffer is full                    */
                  LEContextInfo->BufferFull = TRUE;
               }
               else
               {
                  Display(("SEND failed with error %d\r\n", Result));

                  LEContextInfo->SPPLEBufferInfo.SendInfo.BytesToSend  = 0;
               }
            }
         }
      }

      /* If we have added more bytes to the transmit buffer than we can */
      /* send in this process remove the extra.                         */
      if(Added2Buffer > LEContextInfo->SPPLEBufferInfo.SendInfo.BytesToSend)
         RemoveDataFromBuffer(&(LEContextInfo->SPPLEBufferInfo.TransmitBuffer), Added2Buffer-LEContextInfo->SPPLEBufferInfo.SendInfo.BytesToSend, NULL);

      /* Display a message if we have sent all required data.           */
      if((!(LEContextInfo->SPPLEBufferInfo.SendInfo.BytesToSend)) && (LEContextInfo->SPPLEBufferInfo.SendInfo.BytesSent))
      {
         Display(("\r\nSend Complete, Sent %u.\r\n", LEContextInfo->SPPLEBufferInfo.SendInfo.BytesSent));
         DisplayPrompt();

         LEContextInfo->SPPLEBufferInfo.SendInfo.BytesSent = 0;
      }
   }
}

   /* The following function is responsible for transmitting the        */
   /* specified number of credits to the remote device.                 */
static void SPPLESendCredits(LE_Context_Info_t *LEContextInfo, DeviceInfo_t *DeviceInfo, unsigned int DataLength)
{
   int              Result;
   unsigned int     ActualCredits;
   NonAlignedWord_t Credits;

   /* Verify that the input parameters are semi-valid.                  */
   if((LEContextInfo) && (DeviceInfo) && ((DataLength) || (LEContextInfo->SPPLEBufferInfo.QueuedCredits)))
   {
      /* Only attempt to send the credits if the LE buffer is not full. */
      if(LEContextInfo->BufferFull == FALSE)
      {
         /* Make sure that we don't credit more than can be filled in   */
         /* our receive buffer.                                         */
         ActualCredits = DataLength + LEContextInfo->SPPLEBufferInfo.QueuedCredits;
         ActualCredits = (ActualCredits > LEContextInfo->SPPLEBufferInfo.ReceiveBuffer.BytesFree)?LEContextInfo->SPPLEBufferInfo.ReceiveBuffer.BytesFree:ActualCredits;

         /* Format the credit packet.                                   */
         ASSIGN_HOST_WORD_TO_LITTLE_ENDIAN_UNALIGNED_WORD(&Credits, ActualCredits);

         /* Determine how to send credits based on the role.            */
         if(DeviceInfo->Flags & DEVICE_INFO_FLAGS_SPPLE_SERVER)
         {
            /* We are acting as a server so notify the Rx Credits       */
            /* characteristic.                                          */
            if(DeviceInfo->ServerInfo.Rx_Credit_Client_Configuration_Descriptor == GATT_CLIENT_CONFIGURATION_CHARACTERISTIC_NOTIFY_ENABLE)
               Result = GATT_Handle_Value_Notification(BluetoothStackID, SPPLEServiceID, LEContextInfo->ConnectionID, SPPLE_RX_CREDITS_CHARACTERISTIC_ATTRIBUTE_OFFSET, WORD_SIZE, (Byte_t *)&Credits);
            else
               Result = 0;
         }
         else
         {
            /* We are acting as a client so send a Write Without        */
            /* Response packet to the Tx Credit Characteristic.         */
            if(DeviceInfo->ClientInfo.Tx_Credit_Characteristic)
               Result = GATT_Write_Without_Response_Request(BluetoothStackID, LEContextInfo->ConnectionID, DeviceInfo->ClientInfo.Tx_Credit_Characteristic, WORD_SIZE, &Credits);
            else
               Result = 0;
         }

         /* If an error occurred we need to queue the credits to try    */
         /* again.                                                      */
         if(Result >= 0)
         {
            /* Clear the queued credit count as if there were any queued*/
            /* credits they have now been sent.                         */
            LEContextInfo->SPPLEBufferInfo.QueuedCredits = 0;
         }
         else
         {
            if(Result == BTPS_ERROR_INSUFFICIENT_BUFFER_SPACE)
            {
               /* Flag that the buffer is full.                         */
               LEContextInfo->BufferFull = TRUE;
            }

            LEContextInfo->SPPLEBufferInfo.QueuedCredits += DataLength;
         }
      }
      else
         LEContextInfo->SPPLEBufferInfo.QueuedCredits += DataLength;
   }
}

   /* The following function is responsible for handling a received     */
   /* credit, event.                                                    */
static void SPPLEReceiveCreditEvent(LE_Context_Info_t *LEContextInfo, DeviceInfo_t *DeviceInfo, unsigned int Credits)
{
   /* Verify that the input parameters are semi-valid.                  */
   if((LEContextInfo) && (DeviceInfo))
   {
      /* If this is a real credit event store the number of credits.    */
      LEContextInfo->SPPLEBufferInfo.TransmitCredits += Credits;

      /* Handle any active send process.                                */
      SPPLESendProcess(LEContextInfo, DeviceInfo);

      /* Send all queued data.                                          */
      SPPLESendData(LEContextInfo, DeviceInfo, 0, NULL);

      /* It is possible that we have received data queued, so call the  */
      /* Data Indication Event to handle this.                          */
      SPPLEDataIndicationEvent(LEContextInfo, DeviceInfo, 0, NULL);
   }
}

   /* The following function sends the specified data to the specified  */
   /* data.  This function will queue any of the data that does not go  */
   /* out.  This function returns the number of bytes sent if all the   */
   /* data was sent, or 0.                                              */
   /* * NOTE * If DataLength is 0 and Data is NULL then all queued data */
   /*          will be sent.                                            */
static unsigned int SPPLESendData(LE_Context_Info_t *LEContextInfo, DeviceInfo_t *DeviceInfo, unsigned int DataLength, Byte_t *Data)
{
   int          Result;
   Boolean_t    Done;
   unsigned int DataCount;
   unsigned int MaxLength;
   unsigned int TransmitIndex;
   unsigned int QueuedBytes;
   unsigned int SPPLEBufferLength;
   unsigned int TotalBytesTransmitted = 0;

   /* Verify that the input parameters are semi-valid.                  */
   if((LEContextInfo) && (DeviceInfo))
   {
      /* Loop while we have data to send and we can send it.            */
      Done              = FALSE;
      TransmitIndex     = 0;
      SPPLEBufferLength = 0;
      while(!Done)
      {
         /* Check to see if we have credits to use to transmit the data */
         /* (and that the buffer is not FULL).                          */
         if((LEContextInfo->SPPLEBufferInfo.TransmitCredits) && (LEContextInfo->BufferFull == FALSE))
         {
            /* Get the maximum length of what we can send in this       */
            /* transaction.                                             */
            MaxLength = (SPPLE_DATA_BUFFER_LENGTH > LEContextInfo->SPPLEBufferInfo.TransmitCredits)?LEContextInfo->SPPLEBufferInfo.TransmitCredits:SPPLE_DATA_BUFFER_LENGTH;

            /* If we do not have any outstanding data get some more     */
            /* data.                                                    */
            if(!SPPLEBufferLength)
            {
               /* Send any buffered data first.                         */
               if(LEContextInfo->SPPLEBufferInfo.TransmitBuffer.BytesFree != LEContextInfo->SPPLEBufferInfo.TransmitBuffer.BufferSize)
               {
                  /* Remove the queued data from the Transmit Buffer.   */
                  SPPLEBufferLength = RemoveDataFromBuffer(&(LEContextInfo->SPPLEBufferInfo.TransmitBuffer), MaxLength, SPPLEBuffer);
               }
               else
               {
                  /* Check to see if we have data to send.              */
                  if((DataLength) && (Data))
                  {
                     /* Copy the data to send into the SPPLEBuffer.     */
                     SPPLEBufferLength = (DataLength > MaxLength)?MaxLength:DataLength;
                     BTPS_MemCopy(SPPLEBuffer, Data, SPPLEBufferLength);

                     DataLength -= SPPLEBufferLength;
                     Data       += SPPLEBufferLength;
                  }
                  else
                  {
                     /* No data queued or data left to send so exit the */
                     /* loop.                                           */
                     Done = TRUE;
                  }
               }

               /* Set the count of data that we can send.               */
               DataCount         = SPPLEBufferLength;

               /* Reset the Transmit Index to 0.                        */
               TransmitIndex     = 0;
            }
            else
            {
               /* We have data to send so cap it at the maximum that can*/
               /* be transmitted.                                       */
               DataCount = (SPPLEBufferLength > MaxLength)?MaxLength:SPPLEBufferLength;
            }

            /* Try to write data if not exiting the loop.               */
            if(!Done)
            {
               /* Use the correct API based on device role for SPPLE.   */
               if(DeviceInfo->Flags & DEVICE_INFO_FLAGS_SPPLE_SERVER)
               {
                  /* We are acting as SPPLE Server, so notify the Tx    */
                  /* Characteristic.                                    */
                  if(DeviceInfo->ServerInfo.Tx_Client_Configuration_Descriptor == GATT_CLIENT_CONFIGURATION_CHARACTERISTIC_NOTIFY_ENABLE)
                     Result = GATT_Handle_Value_Notification(BluetoothStackID, SPPLEServiceID, LEContextInfo->ConnectionID, SPPLE_TX_CHARACTERISTIC_ATTRIBUTE_OFFSET, (Word_t)DataCount, &SPPLEBuffer[TransmitIndex]);
                  else
                  {
                     /* Not configured for notifications so exit the    */
                     /* loop.                                           */
                     Done = TRUE;
                  }
               }
               else
               {
                  /* We are acting as SPPLE Client, so write to the Rx  */
                  /* Characteristic.                                    */
                  if(DeviceInfo->ClientInfo.Tx_Characteristic)
                     Result = GATT_Write_Without_Response_Request(BluetoothStackID, LEContextInfo->ConnectionID, DeviceInfo->ClientInfo.Rx_Characteristic, (Word_t)DataCount, &SPPLEBuffer[TransmitIndex]);
                  else
                  {
                     /* We have not discovered the Tx Characteristic, so*/
                     /* exit the loop.                                  */
                     Done = TRUE;
                  }
               }

               /* Check to see if any data was written.                 */
               if(!Done)
               {
                  /* Check to see if the data was written successfully. */
                  if(Result >= 0)
                  {
                     /* Adjust the counters.                            */
                     TransmitIndex                                  += (unsigned int)Result;
                     SPPLEBufferLength                              -= (unsigned int)Result;
                     LEContextInfo->SPPLEBufferInfo.TransmitCredits -= (unsigned int)Result;

                     /* Flag that data was sent.                        */
                     TotalBytesTransmitted                          += Result;

                     /* If we have no more remaining Tx Credits AND we  */
                     /* have data built up to send, we need to queue    */
                     /* this in the Tx Buffer.                          */
                     if((!(LEContextInfo->SPPLEBufferInfo.TransmitCredits)) && (SPPLEBufferLength))
                     {
                        /* Add the remaining data to the transmit       */
                        /* buffer.                                      */
                        QueuedBytes = AddDataToBuffer(&(LEContextInfo->SPPLEBufferInfo.TransmitBuffer), SPPLEBufferLength, &SPPLEBuffer[TransmitIndex]);
                        TotalBytesTransmitted += QueuedBytes;

                        SPPLEBufferLength = 0;
                     }
                  }
                  else
                  {
                     /* Failed to send data so add the data that we have*/
                     /* into the transmit buffer for this device and    */
                     /* wait on the buffer to empty.                    */
                     QueuedBytes = AddDataToBuffer(&(LEContextInfo->SPPLEBufferInfo.TransmitBuffer), SPPLEBufferLength, &SPPLEBuffer[TransmitIndex]);
                     TotalBytesTransmitted += QueuedBytes;

                     SPPLEBufferLength = 0;

                     /* Flag that we should exit the loop.              */
                     Done              = TRUE;

                     /* Check to see what error has occurred.           */
                     if(Result == BTPS_ERROR_INSUFFICIENT_BUFFER_SPACE)
                     {
                        /* Flag that the LE buffer is full.             */
                        LEContextInfo->BufferFull = TRUE;
                     }
                     else
                        Display(("SEND failed with error %d\r\n", Result));
                  }
               }
            }
         }
         else
         {
            /* We have no transmit credits, so buffer the data.         */
            QueuedBytes = AddDataToBuffer(&(LEContextInfo->SPPLEBufferInfo.TransmitBuffer), DataLength, Data);

            TotalBytesTransmitted += QueuedBytes;

            /* Exit the loop.                                           */
            Done = TRUE;
         }
      }
   }

   return(TotalBytesTransmitted);
}

   /* The following function is responsible for handling a data         */
   /* indication event.                                                 */
static void SPPLEDataIndicationEvent(LE_Context_Info_t *LEContextInfo, DeviceInfo_t *DeviceInfo, unsigned int DataLength, Byte_t *Data)
{
   Boolean_t    Done;
   unsigned int ReadLength;
   unsigned int Length;
   unsigned int Transmitted;

   /* Verify that the input parameters are semi-valid.                  */
   if((LEContextInfo) && (DeviceInfo))
   {
      /* If we are automatically reading the data, go ahead and credit  */
      /* what we just received, as well as reading everything in the    */
      /* buffer.                                                        */
      if((AutomaticReadActive) || (LoopbackActive))
      {
         /* Loop until we read all of the data queued.                  */
         Done = FALSE;
         while(!Done)
         {
            /* If in loopback mode cap what we remove at the max of what*/
            /* we can send or queue.  If in loopback we will also not   */
            /* attempt to read more than can be filled in the transmit  */
            /* buffer.  This is to guard against the case where we read */
            /* data that we could not queue or transmit due to a buffer */
            /* full condition.                                          */
            if(LoopbackActive)
               ReadLength = (SPPLE_DATA_BUFFER_LENGTH > (LEContextInfo->SPPLEBufferInfo.TransmitBuffer.BytesFree))?(LEContextInfo->SPPLEBufferInfo.TransmitBuffer.BytesFree):SPPLE_DATA_BUFFER_LENGTH;
            else
               ReadLength = SPPLE_DATA_BUFFER_LENGTH;

            /* Read all queued data.                                    */
            Length = SPPLEReadData(LEContextInfo, DeviceInfo, ReadLength, SPPLEBuffer);
            if(((int)Length) > 0)
            {
               /* If loopback is active, loopback the data.             */
               if(LoopbackActive)
               {
                  Transmitted = SPPLESendData(LEContextInfo, DeviceInfo, Length, SPPLEBuffer);
                  if((Transmitted != Length) || (LEContextInfo->BufferFull))
                  {
                     /* If we failed to send all that was read (or the  */
                     /* LE buffer is now full) then we should exit the  */
                     /* loop.                                           */
                     Done = TRUE;
                  }
               }

               /* If we are displaying the data then do that here.      */
               if(DisplayRawData)
               {
                  SPPLEBuffer[Length] = '\0';
                  Display(((char *)SPPLEBuffer));
               }
            }
            else
               Done = TRUE;
         }

         /* Only send/display data just received if any is specified in */
         /* the call to this function.                                  */
         if((DataLength) && (Data))
         {
            /* If loopback is active, loopback the data just received.  */
            if((AutomaticReadActive) || (LoopbackActive))
            {
               /* If we are displaying the data then do that here.      */
               if(DisplayRawData)
               {
                  BTPS_MemCopy(SPPLEBuffer, Data, DataLength);
                  SPPLEBuffer[DataLength] = '\0';
                  Display(((char *)SPPLEBuffer));
               }

               /* Check to see if Loopback is active, if it is we will  */
               /* loopback the data we just received.                   */
               if(LoopbackActive)
               {
                  /* Only queue the data in the receive buffer that we  */
                  /* cannot send.  If in loopback we will also not      */
                  /* attempt to read more than can be filled in the     */
                  /* transmit buffer.  This is to guard against the case*/
                  /* where we read data that we could not queue or      */
                  /* transmit due to a buffer full condition.           */
                  ReadLength = (DataLength > (LEContextInfo->SPPLEBufferInfo.TransmitBuffer.BytesFree))?(LEContextInfo->SPPLEBufferInfo.TransmitBuffer.BytesFree):DataLength;

                  /* Send the data.                                     */
                  if((Transmitted = SPPLESendData(LEContextInfo, DeviceInfo, ReadLength, Data)) > 0)
                  {
                     /* Credit the data we just sent.                   */
                     SPPLESendCredits(LEContextInfo, DeviceInfo, Transmitted);

                     /* Increment what was just sent.                   */
                     DataLength -= ReadLength;
                     Data       += ReadLength;
                  }
               }
               else
               {
                  /* Loopback is not active so just credit back the data*/
                  /* we just received.                                  */
                  SPPLESendCredits(LEContextInfo, DeviceInfo, DataLength);

                  DataLength = 0;
               }

               /* If we have data left that cannot be sent, queue this  */
               /* in the receive buffer.                                */
               if((DataLength) && (Data))
               {
                  /* We are not in Loopback or Automatic Read Mode so   */
                  /* just buffer all the data.                          */
                  Length = AddDataToBuffer(&(LEContextInfo->SPPLEBufferInfo.ReceiveBuffer), DataLength, Data);
                  if(Length != DataLength)
                     Display(("Receive Buffer Overflow of %u bytes", DataLength - Length));
               }
            }

            /* If we are displaying the data then do that here.         */
            if(DisplayRawData)
            {
               BTPS_MemCopy(SPPLEBuffer, Data, DataLength);
               SPPLEBuffer[DataLength] = '\0';
               Display(((char *)SPPLEBuffer));
            }
         }
      }
      else
      {
         if((DataLength) && (Data))
         {
            /* Display a Data indication event.                         */
            Display(("\r\nData Indication Event, Connection ID %u, Received %u bytes.\r\n", LEContextInfo->ConnectionID, DataLength));

            /* We are not in Loopback or Automatic Read Mode so just    */
            /* buffer all the data.                                     */
            Length = AddDataToBuffer(&(LEContextInfo->SPPLEBufferInfo.ReceiveBuffer), DataLength, Data);
            if(Length != DataLength)
               Display(("Receive Buffer Overflow of %u bytes.\r\n", DataLength - Length));
         }
      }
   }
}

   /* The following function is used to read data from the specified    */
   /* device.  The final two parameters specify the BufferLength and the*/
   /* Buffer to read the data into.  On success this function returns   */
   /* the number of bytes read.  If an error occurs this will return a  */
   /* negative error code.                                              */
static int SPPLEReadData(LE_Context_Info_t *LEContextInfo, DeviceInfo_t *DeviceInfo, unsigned int BufferLength, Byte_t *Buffer)
{
   int          ret_val;
   Boolean_t    Done;
   unsigned int Length;
   unsigned int TotalLength;

   /* Verify that the input parameters are semi-valid.                  */
   if((LEContextInfo) && (DeviceInfo) && (BufferLength) && (Buffer))
   {
      Done        = FALSE;
      TotalLength = 0;
      while(!Done)
      {
         Length = RemoveDataFromBuffer(&(LEContextInfo->SPPLEBufferInfo.ReceiveBuffer), BufferLength, Buffer);
         if(Length > 0)
         {
            BufferLength -= Length;
            Buffer       += Length;
            TotalLength  += Length;
         }
         else
            Done = TRUE;
      }

      /* Credit what we read.                                           */
      SPPLESendCredits(LEContextInfo, DeviceInfo, TotalLength);

      /* Return the total number of bytes read.                         */
      ret_val = (int)TotalLength;
   }
   else
      ret_val = BTPS_ERROR_INVALID_PARAMETER;

   return(ret_val);
}

   /* The following function is responsible for starting a scan.        */
static int StartScan(unsigned int BluetoothStackID)
{
   int Result;

   /* First, determine if the input parameters appear to be semi-valid. */
   if(BluetoothStackID)
   {
      /* Not currently scanning, go ahead and attempt to perform the    */
      /* scan.                                                          */
      Result = GAP_LE_Perform_Scan(BluetoothStackID, stActive, 10, 10, latPublic, fpNoFilter, TRUE, GAP_LE_Event_Callback, 0);

      if(!Result)
      {
         Display(("Scan started successfully.\r\n"));
      }
      else
      {
         /* Unable to start the scan.                                   */
         Display(("Unable to perform scan: %d\r\n", Result));
      }
   }
   else
      Result = -1;

   return(Result);
}

   /* The following function is responsible for stopping on on-going    */
   /* scan.                                                             */
static int StopScan(unsigned int BluetoothStackID)
{
   int Result;

   /* First, determine if the input parameters appear to be semi-valid. */
   if(BluetoothStackID)
   {
      Result = GAP_LE_Cancel_Scan(BluetoothStackID);
      if(!Result)
      {
         Display(("Scan stopped successfully.\r\n"));
      }
      else
      {
         /* Error stopping scan.                                        */
         Display(("Unable to stop scan: %d\r\n", Result));
      }
   }
   else
      Result = -1;

   return(Result);
}

   /* The following function is responsible to determining that Remote  */
   /* Device address is resolvable private address or not. If it is     */
   /* resolvable address then it resolve this address.                  */
static void ResolveRemoteAddressHelper(BD_ADDR_t BD_ADDR)
{
   if(GAP_LE_TEST_RESOLVABLE_ADDRESS_BITS(BD_ADDR))
   {
      if(GAP_LE_Resolve_Address(BluetoothStackID,&IRK,BD_ADDR))
         Display(("GAP_LE_Resolve_Address Success: \r\n\n"));
      else
         Display(("GAP_LE_Resolve_Address Failure: \r\n\n"));
   }
}

   /* The following function is responsible for creating an LE          */
   /* connection to the specified Remote Device.                        */
static int ConnectLEDevice(unsigned int BluetoothStackID, BD_ADDR_t BD_ADDR, GAP_LE_Address_Type_t RemoteAddressType ,GAP_LE_Address_Type_t OwnAddressType, Boolean_t UseWhiteList)
{
   int                            Result;
   int                            LEConnectionIndex;
   unsigned int                   WhiteListChanged;
   GAP_LE_White_List_Entry_t      WhiteListEntry;
   GAP_LE_Connection_Parameters_t ConnectionParameters;

   /* First, determine if the input parameters appear to be semi-valid. */
   if((BluetoothStackID) && (!COMPARE_NULL_BD_ADDR(BD_ADDR)))
   {
        /* Make sure that there are available connections               */
      if((LEConnectionIndex = FindFreeLEIndex()) >= 0)
      {
          /* Check to see if the device is already connected.           */
          if(FindLEIndexByAddress(BD_ADDR) == -1)
          {
            /* Remove any previous entries for this device from the     */
            /* White List.                                              */
            WhiteListEntry.Address_Type = RemoteAddressType;
            WhiteListEntry.Address      = BD_ADDR;

            GAP_LE_Remove_Device_From_White_List(BluetoothStackID, 1, &WhiteListEntry, &WhiteListChanged);

            if(UseWhiteList)
               Result = GAP_LE_Add_Device_To_White_List(BluetoothStackID, 1, &WhiteListEntry, &WhiteListChanged);
            else
               Result = 1;

            /* If everything has been successful, up until this point,  */
            /* then go ahead and attempt the connection.                */
            if(Result >= 0)
            {
               /* Initialize the connection parameters.                 */
               ConnectionParameters.Connection_Interval_Min    = 50;
               ConnectionParameters.Connection_Interval_Max    = 200;
               ConnectionParameters.Minimum_Connection_Length  = 0;
               ConnectionParameters.Maximum_Connection_Length  = 10000;
               ConnectionParameters.Slave_Latency              = 0;
               ConnectionParameters.Supervision_Timeout        = 20000;

               /* Everything appears correct, go ahead and attempt to   */
               /* make the connection.                                  */
               Result = GAP_LE_Create_Connection(BluetoothStackID, 100, 100, Result?fpNoFilter:fpWhiteList, RemoteAddressType, Result?&BD_ADDR:NULL, OwnAddressType, &ConnectionParameters, GAP_LE_Event_Callback, 0);

               if(!Result)
               {
                  Display(("Connection Request successful.\r\n"));

                  /* Add to an unused position in the connection info   */
                  /* array.                                             */
                  LEContextInfo[LEConnectionIndex].ConnectionBD_ADDR = BD_ADDR;
               }
               else
               {
                  /* Unable to create connection.                       */
                  Display(("Unable to create connection: %d.\r\n", Result));
               }
            }
            else
            {
               /* Unable to add device to White List.                   */
               Display(("Unable to add device to White List.\r\n"));
            }
         }
         else
         {
            /* Device already connected.                                */
            Display(("Device is already connected.\r\n"));

            Result = -2;
         }
      }
      else
      {
         /* MAX_LE_CONNECTIONS reached                                  */
         Display(("Connection limit reached. No connections available.\r\n"));
         Result = -3;
      }
   }
   else
      Result = -1;

   return(Result);
}

   /* The following function is provided to allow a mechanism to        */
   /* disconnect a currently connected device.                          */
static int DisconnectLEDevice(unsigned int BluetoothStackID, BD_ADDR_t BD_ADDR)
{
   int Result;

   /* First, determine if the input parameters appear to be semi-valid. */
   if((BluetoothStackID) && (!COMPARE_NULL_BD_ADDR(BD_ADDR)))
   {
      /* Make sure that a device with address BD_ADDR is connecting or  */
      /* connected.                                                     */
      if(FindLEIndexByAddress(BD_ADDR) >= 0)
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

   /* The following function is provided to allow a mechanism to        */
   /* cancel a pending connection attempt.                              */
static int CancelConnectLEDevice(unsigned int BluetoothStackID, BD_ADDR_t BD_ADDR)
{
   int           Result;
   DeviceInfo_t *DeviceInfoPtr;

   /* First, determine if the input parameters appear to be semi-valid. */
   if((BluetoothStackID) && (!COMPARE_NULL_BD_ADDR(BD_ADDR)))
   {
      /* Make sure that a device with address BD_ADDR is connecting or  */
      /* connected.                                                     */
      if(FindLEIndexByAddress(BD_ADDR) >= 0)
      {
         /* Check that the device is not already connected.             */
         if(((DeviceInfoPtr = SearchDeviceInfoEntryByBD_ADDR(&DeviceInfoList, BD_ADDR)) == NULL) || (!(DeviceInfoPtr->Flags & DEVICE_INFO_FLAGS_CONNECTED)))
         {
            Result = GAP_LE_Cancel_Create_Connection(BluetoothStackID);
            if(!Result)
            {
               /* Connection successfully canceled, note that the       */
               /* BD_ADDR will be removed from the connection list in   */
               /* the etLE_Connection_Complete event.                   */
               Display(("Cancel successful.\r\n"));
            }
            else
            {
               Display(("Error: GAP_LE_Cancel_Create_Connection() failed with return value %d.\r\n", Result));
            }
         }
         else
         {
            Display(("Can't cancel connection request, device already connected.\r\n"));
         }
      }
      else
      {
         /* Device is not connecting or connected.                      */
         Display(("Device is not connecting.\r\n"));

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
      Capabilities->Bonding_Type                    = lbtBonding;
      Capabilities->IO_Capability                   = LE_Parameters.IOCapability;
      Capabilities->MITM                            = LE_Parameters.MITMProtection;
      Capabilities->OOB_Present                     = LE_Parameters.OOBDataPresent;

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
      Capabilities->Maximum_Encryption_Key_Size        = GAP_LE_MAXIMUM_ENCRYPTION_KEY_SIZE;

      /* This application only demonstrates using Long Term Key's (LTK) */
      /* for encryption of a LE Link, however we could request and send */
      /* all possible keys here if we wanted to.                        */
      Capabilities->Receiving_Keys.Encryption_Key     = TRUE;
      Capabilities->Receiving_Keys.Identification_Key = FALSE;
      Capabilities->Receiving_Keys.Signing_Key        = FALSE;

      Capabilities->Sending_Keys.Encryption_Key       = TRUE;
      Capabilities->Sending_Keys.Identification_Key   = FALSE;
      Capabilities->Sending_Keys.Signing_Key          = FALSE;
   }
}

   /* The following function provides a mechanism for sending a pairing */
   /* request to a device that is connected on an LE Link.              */
static int SendPairingRequest(BD_ADDR_t BD_ADDR, Boolean_t ConnectionMaster)
{
   int                           ret_val;
   BoardStr_t                    BoardStr;
   GAP_LE_Pairing_Capabilities_t Capabilities;

   /* Make sure a Bluetooth Stack is open.                              */
   if(BluetoothStackID)
   {
      /* Make sure the BD_ADDR is valid.                                */
      if(!COMPARE_NULL_BD_ADDR(BD_ADDR))
      {
         /* Configure the application pairing parameters.               */
         ConfigureCapabilities(&Capabilities);

         /* Set the BD_ADDR of the device that we are attempting to pair*/
         /* with.                                                       */
         CurrentLERemoteBD_ADDR = BD_ADDR;

         BD_ADDRToStr(BD_ADDR, BoardStr);
         Display(("Attempting to Pair to %s.\r\n", BoardStr));

         /* Attempt to pair to the remote device.                       */
         if(ConnectionMaster)
         {
            /* Start the pairing process.                               */
            ret_val = GAP_LE_Pair_Remote_Device(BluetoothStackID, BD_ADDR, &Capabilities, GAP_LE_Event_Callback, 0);

            Display(("     GAP_LE_Pair_Remote_Device returned %d.\r\n", ret_val));
         }
         else
         {
            /* As a slave we can only request that the Master start     */
            /* the pairing process.                                     */
            ret_val = GAP_LE_Request_Security(BluetoothStackID, BD_ADDR, Capabilities.Bonding_Type, Capabilities.MITM, GAP_LE_Event_Callback, 0);

            Display(("     GAP_LE_Request_Security returned %d.\r\n", ret_val));
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

   /* The following function provides a mechanism of sending a Slave    */
   /* Pairing Response to a Master's Pairing Request.                   */
static int SlavePairingRequestResponse(BD_ADDR_t BD_ADDR)
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

      Display(("GAP_LE_Authentication_Response returned %d.\r\n", ret_val));
   }
   else
   {
      Display(("Stack ID Invalid.\r\n"));

      ret_val = INVALID_STACK_ID_ERROR;
   }

   return(ret_val);
}

   /* The following function is provided to allow a mechanism of        */
   /* responding to a request for Encryption Information to send to a   */
   /* remote device.                                                    */
static int EncryptionInformationRequestResponse(BD_ADDR_t BD_ADDR, Byte_t KeySize, GAP_LE_Authentication_Response_Information_t *GAP_LE_Authentication_Response_Information)
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

   /* The following function is responsible for setting the application */
   /* state to support loopback mode.  This function will return zero on*/
   /* successful execution and a negative value on errors.              */
static int Loopback(ParameterList_t *TempParam)
{
   int ret_val;

   /* First check to see if the parameters required for the execution of*/
   /* this function appear to be semi-valid.                            */
   if(BluetoothStackID)
   {
      /* Next check to see if the parameters required for the execution */
      /* of this function appear to be semi-valid.                      */
      if((TempParam) && (TempParam->NumberofParameters > 0))
      {
         if(TempParam->Params->intParam)
            LoopbackActive = TRUE;
         else
            LoopbackActive = FALSE;
      }
      else
         LoopbackActive = (LoopbackActive?FALSE:TRUE);

      /* Finally output the current Loopback state.                     */
      Display(("Current Loopback Mode set to: %s.\r\n", LoopbackActive?"ACTIVE":"INACTIVE"));

#if MAXIMUM_SPP_LOOPBACK_BUFFER_SIZE == 0

      if(LoopbackActive)
         Display(("SPP Loopback not supported with MAXIMUM_SPP_LOOPBACK_BUFFER_SIZE = 0.\r\n"));

#endif

      /* Flag success.                                                  */
      ret_val = 0;
   }
   else
   {
      /* One or more of the necessary parameters are invalid.           */
      ret_val = INVALID_PARAMETERS_ERROR;
   }

   return(ret_val);
}

   /* The following function is responsible for setting the application */
   /* state to support displaying Raw Data.  This function will return  */
   /* zero on successful execution and a negative value on errors.      */
static int DisplayRawModeData(ParameterList_t *TempParam)
{
   int ret_val;

   /* First check to see if the parameters required for the execution of*/
   /* this function appear to be semi-valid.                            */
   if(BluetoothStackID)
   {
      /* Check to see if Loopback is active.  If it is then we will not */
      /* process this command (and we will inform the user).            */
      if(!LoopbackActive)
      {
         /* Next check to see if the parameters required for the        */
         /* execution of this function appear to be semi-valid.         */
         if((TempParam) && (TempParam->NumberofParameters > 0))
         {
            if(TempParam->Params->intParam)
               DisplayRawData = TRUE;
            else
               DisplayRawData = FALSE;
         }
         else
            DisplayRawData = (DisplayRawData?FALSE:TRUE);

         /* Output the current Raw Data Display Mode state.             */
         Display(("Current Raw Data Display Mode set to: %s.\r\n", DisplayRawData?"ACTIVE":"INACTIVE"));

#if MAXIMUM_SPP_LOOPBACK_BUFFER_SIZE == 0

         if(DisplayRawData)
            Display(("SPP Display Raw not supported with MAXIMUM_SPP_LOOPBACK_BUFFER_SIZE = 0.\r\n"));

#endif

         /* Flag that the function was successful.                      */
         ret_val = 0;
      }
      else
      {
         Display(("Unable to process Raw Mode Display Request when operating in Loopback Mode.\r\n"));

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

   /* The following function is responsible for setting the application */
   /* state to support Automatically reading all data that is received  */
   /* through SPP.  This function will return zero on successful        */
   /* execution and a negative value on errors.                         */
static int AutomaticReadMode(ParameterList_t *TempParam)
{
   int ret_val;

   /* First check to see if the parameters required for the execution of*/
   /* this function appear to be semi-valid.                            */
   if(BluetoothStackID)
   {
      /* Check to see if Loopback is active.  If it is then we will not */
      /* process this command (and we will inform the user).            */
      if(!LoopbackActive)
      {
         /* Next check to see if the parameters required for the        */
         /* execution of this function appear to be semi-valid.         */
         if((TempParam) && (TempParam->NumberofParameters > 0))
         {
            if(TempParam->Params->intParam)
               AutomaticReadActive = TRUE;
            else
               AutomaticReadActive = FALSE;
         }
         else
            AutomaticReadActive = (AutomaticReadActive?FALSE:TRUE);

         /* Output the current Automatic Read Mode state.               */
         Display(("Current Automatic Read Mode set to: %s.\r\n", AutomaticReadActive?"ACTIVE":"INACTIVE"));

#if MAXIMUM_SPP_LOOPBACK_BUFFER_SIZE == 0

         if(AutomaticReadActive)
            Display(("SPP Automatic Read Mode not supported with MAXIMUM_SPP_LOOPBACK_BUFFER_SIZE = 0.\r\n"));

#endif

         /* Flag that the function was successful.                      */
         ret_val = 0;
      }
      else
      {
         Display(("Unable to process Automatic Read Mode Request when operating in Loopback Mode.\r\n"));

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
   /* display the current Inquiry List (with Indexes).  This is useful  */
   /* in case the user has forgotten what Inquiry Index a particular    */
   /* Bluetooth Device was located in.  This function returns zero on   */
   /* successful execution and a negative value on all errors.          */
static int DisplayInquiryList(ParameterList_t *TempParam)
{
   int          ret_val = 0;
   BoardStr_t   Function_BoardStr;
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
static int SetCBDiscoverabilityMode(ParameterList_t *TempParam)
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
static int SetCBConnectabilityMode(ParameterList_t *TempParam)
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
static int SetCBPairabilityMode(ParameterList_t *TempParam)
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

         /* Verify that we are not connected to device already.         */
         if(FindSPPPortIndexByAddress(InquiryResultList[(TempParam->Params[0].intParam - 1)]) < 0)
         {
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
            /* Display an error to the user describing that Pairing can */
            /* only occur when we are not connected.                    */
            Display(("Only valid when not connected.\r\n"));

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
            ASSIGN_BD_ADDR(CurrentCBRemoteBD_ADDR, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
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
      if(!COMPARE_NULL_BD_ADDR(CurrentCBRemoteBD_ADDR))
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
            Result = GAP_Authentication_Response(BluetoothStackID, CurrentCBRemoteBD_ADDR, &GAP_Authentication_Information);

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
            ASSIGN_BD_ADDR(CurrentCBRemoteBD_ADDR, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
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
      if(!COMPARE_NULL_BD_ADDR(CurrentCBRemoteBD_ADDR))
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
            Result = GAP_Authentication_Response(BluetoothStackID, CurrentCBRemoteBD_ADDR, &GAP_Authentication_Information);

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
            ASSIGN_BD_ADDR(CurrentCBRemoteBD_ADDR, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
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
      if(!COMPARE_NULL_BD_ADDR(CurrentCBRemoteBD_ADDR))
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
            Result = GAP_Authentication_Response(BluetoothStackID, CurrentCBRemoteBD_ADDR, &GAP_Authentication_Information);

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
            ASSIGN_BD_ADDR(CurrentCBRemoteBD_ADDR, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
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
   int        Result;
   int        ret_val;
   BD_ADDR_t  BD_ADDR;
   BoardStr_t Function_BoardStr;

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

   /* The following function is responsible for putting a specified     */
   /* connection into HCI Sniff Mode with passed in parameters.         */
static int SniffMode(ParameterList_t *TempParam)
{
   int          Result;
   int          ret_val;
   int          SerialPortIndex;
   Byte_t       Status;
   Word_t       Sniff_Max_Interval;
   Word_t       Sniff_Min_Interval;
   Word_t       Sniff_Attempt;
   Word_t       Sniff_Timeout;
   unsigned int LocalSerialPortID;

   /* First, check that valid Bluetooth Stack ID exists.                */
   if(BluetoothStackID)
   {
      /* Make sure that all of the parameters required for this function*/
      /* appear to be at least semi-valid.                              */
      if((TempParam) && (TempParam->NumberofParameters >= 5) && (TempParam->Params[0].intParam))
      {
         LocalSerialPortID    = (unsigned int)TempParam->Params[0].intParam;
         Sniff_Max_Interval   = (Word_t)MILLISECONDS_TO_BASEBAND_SLOTS(TempParam->Params[1].intParam);
         Sniff_Min_Interval   = (Word_t)MILLISECONDS_TO_BASEBAND_SLOTS(TempParam->Params[2].intParam);
         Sniff_Attempt        = TempParam->Params[3].intParam;
         Sniff_Timeout        = TempParam->Params[4].intParam;

         /* Make sure the Sniff Mode parameters seem semi valid.        */
         if((Sniff_Attempt) && (Sniff_Max_Interval) && (Sniff_Min_Interval) && (Sniff_Min_Interval < Sniff_Max_Interval))
         {
            /* Make sure the connection handle is valid.                */
            if(((SerialPortIndex = FindSPPPortIndex(LocalSerialPortID)) >= 0) && (SPPContextInfo[SerialPortIndex].Connection_Handle))
            {
               /* Now that we have the connection try and go to Sniff.  */
               Result = HCI_Sniff_Mode(BluetoothStackID, SPPContextInfo[SerialPortIndex].Connection_Handle, Sniff_Max_Interval, Sniff_Min_Interval, Sniff_Attempt, Sniff_Timeout, &Status);
               if((!Result) && (Status == HCI_ERROR_CODE_NO_ERROR))
               {
                  DisplayFunctionSuccess("HCI_Sniff_Mode()");

                  /* Return success to the caller.                      */
                  ret_val = 0;
               }
               else
               {
                  Display(("Error - HCI_Sniff_Mode() %d: 0x%02X", Result, Status));

                  ret_val = FUNCTION_ERROR;
               }
            }
            else
            {
               Display(("Invalid Serial Port ID.\r\n"));

               ret_val = FUNCTION_ERROR;
            }
         }
         else
         {
            /* One or more of the necessary parameters is/are invalid.  */
            DisplayUsage("SniffMode [SerialPortID] [MaxInterval] [MinInterval] [Attempt] [Timeout]");

            ret_val = INVALID_PARAMETERS_ERROR;
         }
      }
      else
      {
         /* One or more of the necessary parameters is/are invalid.     */
         DisplayUsage("SniffMode [SerialPortID] [MaxInterval] [MinInterval] [Attempt] [Timeout]");

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

   /* The following function is responsible for attempting to Exit      */
   /* Sniff Mode for a specified connection.                            */
static int ExitSniffMode(ParameterList_t *TempParam)
{
   int          Result;
   int          ret_val;
   int          SerialPortIndex;
   Byte_t       Status;
   unsigned int LocalSerialPortID;

   /* First, check that valid Bluetooth Stack ID exists.                */
   if(BluetoothStackID)
   {
      /* Make sure that all of the parameters required for this function*/
      /* appear to be at least semi-valid.                              */
      if((TempParam) && (TempParam->NumberofParameters >= 1) && (TempParam->Params[0].intParam))
      {
         /* Save the specified Serial Port ID.                          */
         LocalSerialPortID = (unsigned int)TempParam->Params[0].intParam;

         /* Make sure the connection handle is valid.                   */
         if(((SerialPortIndex = FindSPPPortIndex(LocalSerialPortID)) >= 0) && (SPPContextInfo[SerialPortIndex].Connection_Handle))
         {
            /* Attempt to Exit Sniff Mode for the Specified Device.     */
            Result = HCI_Exit_Sniff_Mode(BluetoothStackID, SPPContextInfo[SerialPortIndex].Connection_Handle, &Status);
            if((!Result) && (Status == HCI_ERROR_CODE_NO_ERROR))
            {
               /* Flag that HCI_Exit_Sniff_Mode was successfully.       */
               DisplayFunctionSuccess("HCI_Exit_Sniff_Mode()");

               ret_val = 0;
            }
            else
            {
               /* Failed to get exit sniff mode.                        */
               Display(("Error - HCI_Exit_Sniff_Mode() %d: 0x%02X", Result, Status));

               /* Return function error to the caller.                  */
               ret_val = FUNCTION_ERROR;
            }
         }
         else
         {
            Display(("Invalid Serial Port ID.\r\n"));

            ret_val = FUNCTION_ERROR;
         }
      }
      else
      {
         /* One or more of the necessary parameters is/are invalid.     */
         DisplayUsage("ExitSniffMode [SerialPortID] ");

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

   /* The following function is responsible for opening a Serial Port   */
   /* Server on the Local Device.  This function opens the Serial Port  */
   /* Server on the specified RFCOMM Channel.  This function returns    */
   /* zero if successful, or a negative return value if an error        */
   /* occurred.                                                         */
static int OpenServer(ParameterList_t *TempParam)
{
   int           SerialPortIndex;
   int           ret_val;
   char         *ServiceName;
   DWord_t       SPPServerSDPHandle;
   unsigned int  ServerPortID;
   unsigned int  ServerPortNumber;

   /* First check to see if a valid Bluetooth Stack ID exists.          */
   if(BluetoothStackID)
   {
      /* Next, check to see if the parameters specified are valid.      */
      if((TempParam) && (TempParam->NumberofParameters >= 1) && (TempParam->Params[0].intParam))
      {
         /* Find an empty slot.                                         */
         if((SerialPortIndex = FindFreeSPPPortIndex()) >= 0)
         {
            /* Save the server port number.                             */
            ServerPortNumber = TempParam->Params[0].intParam;

            /* Simply attempt to open an Serial Server, on RFCOMM Server*/
            /* Port 1.                                                  */
            ret_val = SPP_Open_Server_Port(BluetoothStackID, ServerPortNumber, SPP_Event_Callback, (unsigned long)0);

            /* If the Open was successful, then note the Serial Port    */
            /* Server ID.                                               */
            if(ret_val > 0)
            {
               /* Note the Serial Port Server ID of the opened Serial   */
               /* Port Server.                                          */
               ServerPortID = (unsigned int)ret_val;

               /* Create a Buffer to hold the Service Name.             */
               if((ServiceName = BTPS_AllocateMemory(64)) != NULL)
               {
                  /* The Server was opened successfully, now register a */
                  /* SDP Record indicating that an Serial Port Server   */
                  /* exists. Do this by first creating a Service Name.  */
                  BTPS_SprintF(ServiceName, "Serial Port Server Port %d", ServerPortNumber);

                  /* Only register an SDP record if we have not already */
                  /* opened a server on this port number.               */
                  if(FindSPPPortIndexByServerPortNumber(ServerPortNumber) < 0)
                  {
                     /* Now that a Service Name has been created try to */
                     /* Register the SDP Record.                        */
                     ret_val = SPP_Register_Generic_SDP_Record(BluetoothStackID, ServerPortID, ServiceName, &SPPServerSDPHandle);
                  }
                  else
                  {
                     /* We already have opened another SPP Port on that */
                     /* RFCOMM Port Number so there is no need to       */
                     /* register a duplicate SDP Record.                */
                     SPPServerSDPHandle = 0;
                     ret_val            = 0;
                  }

                  /* If there was an error creating the Serial Port     */
                  /* Server's SDP Service Record then go ahead an close */
                  /* down the server an flag an error.                  */
                  if(ret_val < 0)
                  {
                     Display(("Unable to Register Server SDP Record, Error = %d.\r\n", ret_val));

                     SPP_Close_Server_Port(BluetoothStackID, ServerPortID);

                     /* Flag that there is no longer an Serial Port     */
                     /* Server Open.                                    */
                     ServerPortID = 0;

                     ret_val      = UNABLE_TO_REGISTER_SERVER;
                  }
                  else
                  {
                     /* Simply flag to the user that everything         */
                     /* initialized correctly.                          */
                     Display(("Server Opened: Server Port %u, Serial Port ID %u.\r\n", (unsigned int)TempParam->Params[0].intParam, ServerPortID));

                     /* We found an empty slot to store the context     */
                     SPPContextInfo[SerialPortIndex].LocalSerialPortID  = ServerPortID;
                     SPPContextInfo[SerialPortIndex].ServerPortNumber   = ServerPortNumber;
                     SPPContextInfo[SerialPortIndex].SPPServerSDPHandle = SPPServerSDPHandle;
                     SPPContextInfo[SerialPortIndex].Connected          = FALSE;

                     /* If this message is not seen in the terminal log */
                     /* after opening a server, then something went     */
                     /* wrong                                           */
                     Display(("Server Port Context Stored.\r\n"));

                     /* Flag success to the caller.                     */
                     ret_val = 0;
                  }

                  /* Free the Service Name buffer.                      */
                  BTPS_FreeMemory(ServiceName);
               }
               else
               {
                  Display(("Failed to allocate buffer to hold Service Name in SDP Record.\r\n"));
               }
            }
            else
            {
               Display(("Unable to Open Server on: %d, Error = %d.\r\n", TempParam->Params[0].intParam, ret_val));

               ret_val = UNABLE_TO_REGISTER_SERVER;
            }
         }
         else
         {
            /* Maximum number of ports reached.                         */
            Display(("Maximum allowed server ports open.\r\n"));

            ret_val = FUNCTION_ERROR;
         }
      }
      else
      {
         DisplayUsage("Open [Port Number]");

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

   /* The following function is responsible for closing a Serial Port   */
   /* Server that was previously opened via a successful call to the    */
   /* OpenServer() function.  This function returns zero if successful  */
   /* or a negative return error code if there was an error.            */
static int CloseServer(ParameterList_t *TempParam)
{
   int i;
   int ret_val = 0;
   int LocalSerialPortID;

   /* First check to see if a valid Bluetooth Stack ID exists.          */
   if(BluetoothStackID)
   {
      /* Figure out the server port that corresponds to the portID      */
      /* requested by the user.                                         */
      if((TempParam) && (TempParam->NumberofParameters) && (TempParam->Params[0].intParam))
      {
         LocalSerialPortID = TempParam->Params[0].intParam;
         ClosePortByNumber(LocalSerialPortID);
      }
      else
      {
         /* If no port is specified to close then we should close them  */
         /* all.                                                        */
         if((!TempParam) || ((TempParam) && (TempParam->NumberofParameters == 0)))
         {
            for(i=0; i < sizeof(SPPContextInfo)/sizeof(SPP_Context_Info_t); i++)
            {
               /* If a server is register then close the port.          */
               if((SPPContextInfo[i].LocalSerialPortID) && (SPPContextInfo[i].ServerPortNumber))
                  ClosePortByNumber(SPPContextInfo[i].LocalSerialPortID);
            }
         }
      }
   }
   else
   {
      /* No valid Bluetooth Stack ID exists.                            */
      ret_val = INVALID_STACK_ID_ERROR;
   }

   return(ret_val);
}

   /* The following function is responsible for initiating a connection */
   /* with a Remote Serial Port Server.  This function returns zero if  */
   /* successful and a negative value if an error occurred.             */
static int OpenRemoteServer(ParameterList_t *TempParam)
{
   int ret_val;
   int Result;
   int SerialPortIndex;

   /* First, check that valid Bluetooth Stack ID exists.                */
   if(BluetoothStackID)
   {
      /* Next, check that we are in client mode.                        */
      if(UI_Mode == UI_MODE_IS_CLIENT)
      {
         /* Next, let's make sure that the user has specified a Remote  */
         /* Bluetooth Device to open.                                   */
         if((TempParam) && (TempParam->NumberofParameters > 1) && (TempParam->Params[0].intParam) && (NumberofValidResponses) && (TempParam->Params[0].intParam <= NumberofValidResponses) && (!COMPARE_NULL_BD_ADDR(InquiryResultList[(TempParam->Params[0].intParam - 1)])) && (TempParam->Params[1].intParam))
         {
            /* Find a free serial port entry.                           */
            if((SerialPortIndex = FindFreeSPPPortIndex()) >= 0)
            {
               /* Now let's attempt to open the Remote Serial Port      */
               /* Server.                                               */
               Result = SPP_Open_Remote_Port(BluetoothStackID, InquiryResultList[(TempParam->Params[0].intParam - 1)], TempParam->Params[1].intParam, SPP_Event_Callback, (unsigned long)0);
               if(Result > 0)
               {
                  /* Inform the user that the call to open the Remote   */
                  /* Serial Port Server was successful.                 */
                  Display(("SPP_Open_Remote_Port success, Serial Port ID = %u.\r\n", (unsigned int)Result));

                  /* Note the Serial Port Client ID and Bluetooth       */
                  /* Address.                                           */
                  SPPContextInfo[SerialPortIndex].LocalSerialPortID = (unsigned int)Result;
                  SPPContextInfo[SerialPortIndex].BD_ADDR           = InquiryResultList[(TempParam->Params[0].intParam - 1)];

                  /* Flag success to the caller.                        */
                  ret_val                                          = 0;
               }
               else
               {
                  /* Inform the user that the call to Open the Remote   */
                  /* Serial Port Server failed.                         */
                  DisplayFunctionError("SPP_Open_Remote_Port", Result);

                  /* One or more of the necessary parameters is/are     */
                  /* invalid.                                           */
                  ret_val = INVALID_PARAMETERS_ERROR;
               }
            }
            else
            {
               /* Maximum client ports reached.                         */
               Display(("Maximum allowed client ports open.\r\n"));

               ret_val = FUNCTION_ERROR;
            }
         }
         else
         {
            DisplayUsage("Open [Inquiry Index] [RFCOMM Server Port].\r\n");

            /* One or more of the necessary parameters is/are invalid.  */
            ret_val = INVALID_PARAMETERS_ERROR;
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

   /* The following function is responsible for terminating a connection*/
   /* with a Remote Serial Port Server.  This function returns zero if  */
   /* successful and a negative value if an error occurred.             */
static int CloseRemoteServer(ParameterList_t *TempParam)
{
   int          i;
   int          ret_val = 0;
   unsigned int LocalSerialPortID;

   /* First check to see if a valid Bluetooth Stack ID exists.          */
   if(BluetoothStackID)
   {
      /* Figure out the server port that corresponds to the portID      */
      /* requested by the user.                                         */
      if((TempParam) && (TempParam->NumberofParameters) && (TempParam->Params[0].intParam))
      {
         LocalSerialPortID = TempParam->Params[0].intParam;
         ClosePortByNumber(LocalSerialPortID);
      }
      else
      {
         /* If no port is specified to close then we should close them  */
         /* all.                                                        */
         if((!TempParam) || ((TempParam) && (TempParam->NumberofParameters == 0)))
         {
            for(i=0; i < sizeof(SPPContextInfo)/sizeof(SPP_Context_Info_t); i++)
            {
               /* Attempt to close this port.                           */
               if(SPPContextInfo[i].LocalSerialPortID)
                  ClosePortByNumber(SPPContextInfo[i].LocalSerialPortID);
            }
         }
      }
   }
   else
   {
      /* No valid Bluetooth Stack ID exists.                            */
      ret_val = INVALID_STACK_ID_ERROR;
   }

   return(ret_val);
}

   /* The following function is responsible for reading data that was   */
   /* received via an Open SPP port.  The function reads a fixed number */
   /* of bytes at a time from the SPP Port and displays it. If the call */
   /* to the SPP_Data_Read() function is successful but no data is      */
   /* available to read the function displays "No data to read.".  This */
   /* function requires that a valid Bluetooth Stack ID and Serial Port */
   /* ID exist before running.  This function returns zero if successful*/
   /* and a negative value if an error occurred.                        */
static int Read(ParameterList_t *TempParam)
{
   int          ret_val;
   int          Result;
   int          SerialPortIndex;
   char         Buffer[32];
   unsigned int LocalSerialPortID;

   /* First check to see if the parameters required for the execution of*/
   /* this function appear to be semi-valid.                            */
   if(BluetoothStackID)
   {
      /* Only allow the Read Command if we are not in Loopback or       */
      /* Display Raw Data Mode.                                         */
      if((!LoopbackActive) && (!DisplayRawData))
      {
         /* Verify that the parameters to this function are valid.      */
         if((TempParam) && (TempParam->NumberofParameters) && (TempParam->Params[0].intParam))
         {
            /* Save the selected Serial Port ID.                        */
            LocalSerialPortID = (unsigned int)TempParam->Params[0].intParam;

            /* Find the serial port entry for this port.                */
            if(((SerialPortIndex = FindSPPPortIndex(LocalSerialPortID)) >= 0) && (SPPContextInfo[SerialPortIndex].Connected == TRUE))
            {
               /* The required parameters appear to be semi-valid, send */
               /* the command to Read Data from SPP.                    */
               do
               {
                  /* Attempt to read data from the buffer.              */
                  Result = SPP_Data_Read(BluetoothStackID, SPPContextInfo[SerialPortIndex].LocalSerialPortID, (Word_t)(sizeof(Buffer)-1), (Byte_t*)&Buffer);

                  /* Next, check the return value to see if the command */
                  /* was successfully.                                  */
                  if(Result >= 0)
                  {
                     /* Null terminate the read data.                   */
                     Buffer[Result] = 0;

                     /* Data was read successfully, the result indicates*/
                     /* the of bytes that were successfully Read.       */
                     Display(("Read: %d.\r\n", Result));

                     if(Result > 0)
                        Display(("Message: %s\r\n", Buffer));

                     ret_val = 0;
                  }
                  else
                  {
                     /* An error occurred while reading from SPP.       */
                     DisplayFunctionError("SPP_Data_Read Failure", Result);

                     ret_val = Result;
                  }
               } while(Result > 0);
            }
            else
            {
               /* Maximum client ports reached.                         */
               Display(("Port is invalid or not connected.\r\n"));

               ret_val = FUNCTION_ERROR;
            }
         }
         else
         {
            DisplayUsage("Read [SerialPortID].\r\n");

            /* One or more of the necessary parameters is/are invalid.  */
            ret_val = INVALID_PARAMETERS_ERROR;
         }
      }
      else
      {
         /* Simply inform the user that this command is not available in*/
         /* this mode.                                                  */
         Display(("This operation cannot be performed while in Loopback Mode or while Displaying Raw Data.\r\n"));

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

   /* The following function is responsible for Writing Data to an Open */
   /* SPP Port.  The string that is written is defined by the constant  */
   /* TEST_DATA (at the top of this file).  This function requires that */
   /* a valid Bluetooth Stack ID and Serial Port ID exist before        */
   /* running.  This function returns zero is successful or a negative  */
   /* return value if there was an error.                               */
static int Write(ParameterList_t *TempParam)
{
   int          ret_val;
   int          Result;
   int          SerialPortIndex;
   unsigned int LocalSerialPortID;

   /* First check to see if the parameters required for the execution of*/
   /* this function appear to be semi-valid.                            */
   if(BluetoothStackID)
   {
      /* Verify that the parameters to this function are valid.         */
      if((TempParam) && (TempParam->NumberofParameters) && (TempParam->Params[0].intParam) )
      {
         /* Save the selected Serial Port ID.                           */
        LocalSerialPortID = TempParam->Params[0].intParam;

        /* Find the serial port entry for this port.                    */
        if(((SerialPortIndex = FindSPPPortIndex(LocalSerialPortID)) >= 0) && (SPPContextInfo[SerialPortIndex].Connected == TRUE))
        {
           /* Simply write out the default string value.                */
           Result = SPP_Data_Write(BluetoothStackID, SPPContextInfo[SerialPortIndex].LocalSerialPortID, (Word_t)DataStrLen, (Byte_t *)DataStr);

           /* Next, check the return value to see if the command was    */
           /* issued successfully.                                      */
           if(Result >= 0)
           {
              /* The Data was written successfully, Result indicates the*/
              /* number of bytes successfully written.                  */
              Display(("Wrote: %d.\r\n", Result));

              /* Flag success to the caller.                            */
              ret_val = 0;
           }
           else
           {
              /* There was an error writing the Data to the SPP Port.   */
              Display(("Failed: %d.\r\n", Result));

              /* Flag that an error occurred while submitting the       */
              /* command.                                               */
              ret_val = FUNCTION_ERROR;
           }
        }
        else
        {
           /* Maximum client ports reached.                             */
           Display(("Port is invalid or not connected.\r\n"));

           ret_val = FUNCTION_ERROR;
        }
      }
      else
      {
         DisplayUsage("Write [SerialPortID].\r\n");

         /* One or more of the necessary parameters is/are invalid.     */
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

   /* The following function is responsible for querying the current    */
   /* configuration parameters that are used by SPP.  This function will*/
   /* return zero on successful execution and a negative value on       */
   /* errors.                                                           */
static int GetConfigParams(ParameterList_t *TempParam)
{
   int                        ret_val;
   SPP_Configuration_Params_t SPPConfigurationParams;

   /* First check to see if the parameters required for the execution of*/
   /* this function appear to be semi-valid.                            */
   if(BluetoothStackID)
   {
      /* Simply query the configuration parameters.                     */
      ret_val = SPP_Get_Configuration_Parameters(BluetoothStackID, &SPPConfigurationParams);

      if(ret_val >= 0)
      {
         /* Parameters have been queried successfully, go ahead and     */
         /* notify the user.                                            */
         Display(("SPP_Get_Configuration_Parameters(): Success\r\n", ret_val));
         Display(("   MaximumFrameSize   : %d (0x%X)\r\n", SPPConfigurationParams.MaximumFrameSize, SPPConfigurationParams.MaximumFrameSize));
         Display(("   TransmitBufferSize : %d (0x%X)\r\n", SPPConfigurationParams.TransmitBufferSize, SPPConfigurationParams.TransmitBufferSize));
         Display(("   ReceiveBufferSize  : %d (0x%X)\r\n", SPPConfigurationParams.ReceiveBufferSize, SPPConfigurationParams.ReceiveBufferSize));

         /* Flag success.                                               */
         ret_val = 0;
      }
      else
      {
         /* Error querying the current parameters.                      */
         Display(("SPP_Get_Configuration_Parameters(): Error %d.\r\n", ret_val));

         ret_val = FUNCTION_ERROR;
      }
   }
   else
   {
      /* One or more of the necessary parameters are invalid.           */
      ret_val = INVALID_PARAMETERS_ERROR;
   }

   return(ret_val);
}

   /* The following function is responsible for setting the current     */
   /* configuration parameters that are used by SPP.  This function will*/
   /* return zero on successful execution and a negative value on       */
   /* errors.                                                           */
static int SetConfigParams(ParameterList_t *TempParam)
{
   int                        ret_val;
   SPP_Configuration_Params_t SPPConfigurationParams;

   /* First check to see if the parameters required for the execution of*/
   /* this function appear to be semi-valid.                            */
   if(BluetoothStackID)
   {
      /* Next check to see if the parameters required for the execution */
      /* of this function appear to be semi-valid.                      */
      if((TempParam) && (TempParam->NumberofParameters > 2))
      {
         /* Parameters have been specified, go ahead and write them to  */
         /* the stack.                                                  */
         SPPConfigurationParams.MaximumFrameSize   = (unsigned int)(TempParam->Params[0].intParam);
         SPPConfigurationParams.TransmitBufferSize = (unsigned int)(TempParam->Params[1].intParam);
         SPPConfigurationParams.ReceiveBufferSize  = (unsigned int)(TempParam->Params[2].intParam);

         ret_val = SPP_Set_Configuration_Parameters(BluetoothStackID, &SPPConfigurationParams);

         if(ret_val >= 0)
         {
            Display(("SPP_Set_Configuration_Parameters(): Success\r\n", ret_val));
            Display(("   MaximumFrameSize   : %d (0x%X)\r\n", SPPConfigurationParams.MaximumFrameSize, SPPConfigurationParams.MaximumFrameSize));
            Display(("   TransmitBufferSize : %d (0x%X)\r\n", SPPConfigurationParams.TransmitBufferSize, SPPConfigurationParams.TransmitBufferSize));
            Display(("   ReceiveBufferSize  : %d (0x%X)\r\n", SPPConfigurationParams.ReceiveBufferSize, SPPConfigurationParams.ReceiveBufferSize));

            /* Flag success.                                            */
            ret_val = 0;
         }
         else
         {
            /* Error setting the current parameters.                    */
            Display(("SPP_Set_Configuration_Parameters(): Error %d.\r\n", ret_val));

            ret_val = FUNCTION_ERROR;
         }
      }
      else
      {
         DisplayUsage("SetConfigParams [MaximumFrameSize] [TransmitBufferSize (0: don't change)] [ReceiveBufferSize (0: don't change)]");

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

   /* The following function is responsible for querying the current    */
   /* queuing parameters that are used by SPP/RFCOMM (into L2CAP).  This*/
   /* function will return zero on successful execution and a negative  */
   /* value on errors.                                                  */
static int GetQueueParams(ParameterList_t *TempParam)
{
   int          ret_val;
   unsigned int MaximumNumberDataPackets;
   unsigned int QueuedDataPacketsThreshold;

   /* First check to see if the parameters required for the execution of*/
   /* this function appear to be semi-valid.                            */
   if(BluetoothStackID)
   {
      /* Simply query the queuing parameters.                           */
      ret_val = SPP_Get_Queuing_Parameters(BluetoothStackID, &MaximumNumberDataPackets, &QueuedDataPacketsThreshold);

      if(ret_val >= 0)
      {
         /* Parameters have been queried successfully, go ahead and     */
         /* notify the user.                                            */
         Display(("SPP_Get_Queuing_Parameters(): Success.\r\n"));
         Display(("   MaximumNumberDataPackets   : %d (0x%X)\r\n", MaximumNumberDataPackets, MaximumNumberDataPackets));
         Display(("   QueuedDataPacketsThreshold : %d (0x%X)\r\n", QueuedDataPacketsThreshold, QueuedDataPacketsThreshold));

         /* Flag success.                                               */
         ret_val = 0;
      }
      else
      {
         /* Error querying the current parameters.                      */
         Display(("SPP_Get_Queuing_Parameters(): Error %d.\r\n", ret_val));

         ret_val = FUNCTION_ERROR;
      }
   }
   else
   {
      /* One or more of the necessary parameters are invalid.           */
      ret_val = INVALID_PARAMETERS_ERROR;
   }

   return(ret_val);
}

   /* The following function is responsible for setting the current     */
   /* queuing parameters that are used by SPP/RFCOMM (into L2CAP).  This*/
   /* function will return zero on successful execution and a negative  */
   /* value on errors.                                                  */
static int SetQueueParams(ParameterList_t *TempParam)
{
   int ret_val;

   /* First check to see if the parameters required for the execution of*/
   /* this function appear to be semi-valid.                            */
   if(BluetoothStackID)
   {
      /* Next check to see if the parameters required for the execution */
      /* of this function appear to be semi-valid.                      */
      if((TempParam) && (TempParam->NumberofParameters > 1))
      {
         /* Parameters have been specified, go ahead and write them to  */
         /* the stack.                                                  */
         ret_val = SPP_Set_Queuing_Parameters(BluetoothStackID, (unsigned int)(TempParam->Params[0].intParam), (unsigned int)(TempParam->Params[1].intParam));

         if(ret_val >= 0)
         {
            Display(("SPP_Set_Queuing_Parameters(): Success.\r\n"));
            Display(("   MaximumNumberDataPackets   : %d (0x%X)\r\n", (unsigned int)(TempParam->Params[0].intParam), (unsigned int)(TempParam->Params[0].intParam)));
            Display(("   QueuedDataPacketsThreshold : %d (0x%X)\r\n", (unsigned int)(TempParam->Params[1].intParam), (unsigned int)(TempParam->Params[1].intParam)));

            /* Flag success.                                            */
            ret_val = 0;
         }
         else
         {
            /* Error setting the current parameters.                    */
            Display(("SPP_Set_Queuing_Parameters(): Error %d.\r\n", ret_val));

            ret_val = FUNCTION_ERROR;
         }
      }
      else
      {
         DisplayUsage("SetQueueParams [MaximumNumberDataPackets] [QueuedDataPacketsThreshold]");

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

   /* The following function is responsible for sending a number of     */
   /* characters to a remote device to which a connection exists.  The  */
   /* function receives a parameter that indicates the number of byte to*/
   /* be transferred.  This function will return zero on successful     */
   /* execution and a negative value on errors.                         */
static int SendData(ParameterList_t *TempParam)
{
   int          SerialPortIndex;
   int          Ndx;
   Word_t       DataCount;
   Boolean_t    Done;
   unsigned int LocalSerialPortID;

   /* Make sure that all of the parameters required for this function   */
   /* appear to be at least semi-valid.                                 */
   if((TempParam) && (TempParam->NumberofParameters >= 2) && (TempParam->Params[0].intParam > 0) && (TempParam->Params[1].intParam))
   {
      /* Store the Serial Port ID in a local variable.                  */
      LocalSerialPortID = TempParam->Params[1].intParam;

      /* Verify that there is a connection that is established.         */
      if((SerialPortIndex = FindSPPPortIndex(LocalSerialPortID)) >= 0)
      {
         /* Check to see if we are sending to another port.             */
         if(!SPPContextInfo[SerialPortIndex].SendInfo.BytesToSend)
         {
            /* Get the count of the number of bytes to send.            */
            SPPContextInfo[SerialPortIndex].SendInfo.BytesToSend  = (DWord_t)TempParam->Params[0].intParam;
            SPPContextInfo[SerialPortIndex].SendInfo.BytesSent    = 0;

            Done = FALSE;
            while((SPPContextInfo[SerialPortIndex].SendInfo.BytesToSend) && (!Done))
            {
               /* Set the Number of bytes to send in the first packet.  */
               if(SPPContextInfo[SerialPortIndex].SendInfo.BytesToSend > DataStrLen)
                  DataCount = DataStrLen;
               else
                  DataCount = SPPContextInfo[SerialPortIndex].SendInfo.BytesToSend;

               Ndx = SPP_Data_Write(BluetoothStackID, LocalSerialPortID, DataCount, (unsigned char *)DataStr);
               if(Ndx >= 0)
               {
                  /* Adjust the counters.                               */
                  SPPContextInfo[SerialPortIndex].SendInfo.BytesToSend -= Ndx;
                  if(Ndx < DataCount)
                  {
                     SPPContextInfo[SerialPortIndex].SendInfo.BytesSent  = Ndx;
                     SPPContextInfo[SerialPortIndex].SendInfo.BufferFull = TRUE;
                     Done                                                = TRUE;
                  }
               }
               else
               {
                  Display(("SEND failed with error %d\r\n", Ndx));
                  SPPContextInfo[SerialPortIndex].SendInfo.BytesToSend  = 0;
               }
            }
         }
         else
            Display(("Send Currently in progress.\r\n"));
      }
      else
         Display(("Context not found\r\n"));
   }
   else
      DisplayUsage("SEND [Number of Bytes to send] [Serial Port ID]\r\n");

   return(0);
}

   /* The following function is responsible for changing the User       */
   /* Interface Mode to Server.                                         */
static int ServerMode(ParameterList_t *TempParam)
{
   UI_Mode = UI_MODE_IS_SERVER;

   UserInterface_Server();

   return(0);
}

   /* The following function is responsible for changing the User       */
   /* Interface Mode to Client.                                         */
static int ClientMode(ParameterList_t *TempParam)
{
   UI_Mode = UI_MODE_IS_CLIENT;

   UserInterface_Client();

   return(0);
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
      Display(("* Command Options General: Help, GetLocalAddress, SetBaudRate    *\r\n"));
      Display(("*                          Quit,                                 *\r\n"));
      Display(("* Command Options BR/EDR:  Inquiry, DisplayInquiryList, Pair,    *\r\n"));
      Display(("*                          EndPairing, PINCodeResponse,          *\r\n"));
      Display(("*                          PassKeyResponse,                      *\r\n"));
      Display(("*                          UserConfirmationResponse,             *\r\n"));
      Display(("*                          SetDiscoverabilityMode,               *\r\n"));
      Display(("*                          SetConnectabilityMode,                *\r\n"));
      Display(("*                          SetPairabilityMode,                   *\r\n"));
      Display(("*                          ChangeSimplePairingParameters,        *\r\n"));
      Display(("*                          GetLocalName, SetLocalName,           *\r\n"));
      Display(("*                          GetClassOfDevice, SetClassOfDevice,   *\r\n"));
      Display(("*                          GetRemoteName, SniffMode,             *\r\n"));
      Display(("*                          ExitSniffMode, Open, Close, Read,     *\r\n"));
      Display(("*                          Write, GetConfigParams,               *\r\n"));
      Display(("*                          SetConfigParams, GetQueueParams,      *\r\n"));
      Display(("*                          DisplayRawModeData, AutomaticReadMode,*\r\n"));
      Display(("*                          SetQueueParams, Loopback,             *\r\n"));
      Display(("*                          CBSend                                *\r\n"));
   }
   else
   {
      if(UI_Mode == UI_MODE_IS_SERVER)
      {
         Display(("\r\n"));
         Display(("******************************************************************\r\n"));
         Display(("* Command Options General: Help, GetLocalAddress, SetBaudRate    *\r\n"));
         Display(("*                          Quit,                                 *\r\n"));
         Display(("* Command Options BR/EDR:  Inquiry, DisplayInquiryList, Pair,    *\r\n"));
         Display(("*                          EndPairing, PINCodeResponse,          *\r\n"));
         Display(("*                          PassKeyResponse,                      *\r\n"));
         Display(("*                          UserConfirmationResponse,             *\r\n"));
         Display(("*                          SetDiscoverabilityMode,               *\r\n"));
         Display(("*                          SetConnectabilityMode,                *\r\n"));
         Display(("*                          SetPairabilityMode,                   *\r\n"));
         Display(("*                          ChangeSimplePairingParameters,        *\r\n"));
         Display(("*                          GetLocalName, SetLocalName,           *\r\n"));
         Display(("*                          GetClassOfDevice, SetClassOfDevice,   *\r\n"));
         Display(("*                          GetRemoteName, SniffMode,             *\r\n"));
         Display(("*                          ExitSniffMode, Open, Close, Read,     *\r\n"));
         Display(("*                          Write, GetConfigParams,               *\r\n"));
         Display(("*                          SetConfigParams, GetQueueParams,      *\r\n"));
         Display(("*                          SetQueueParams, Loopback,             *\r\n"));
         Display(("*                          DisplayRawModeData, AutomaticReadMode,*\r\n"));
         Display(("*                          CBSend                                *\r\n"));
      }
      else
      {
         Display(("\r\n"));
         Display(("******************************************************************\r\n"));
         Display(("* Command Options: Server, Client, Help                          *\r\n"));
         Display(("******************************************************************\r\n"));
      }
   }

   if(UI_Mode != UI_MODE_SELECT)
   {
      Display(("* Command Options GAPLE:   SetDiscoverabilityMode,               *\r\n"));
      Display(("*                          SetConnectabilityMode,                *\r\n"));
      Display(("*                          SetPairabilityMode,                   *\r\n"));
      Display(("*                          ChangePairingParameters,              *\r\n"));
      Display(("*                          AdvertiseLE, StartScanning,           *\r\n"));
      Display(("*                          StopScanning, ConnectLE,              *\r\n"));
      Display(("*                          DisconnectLE, CancelConnectLE,        *\r\n"));
      Display(("*                          PairLE, LEPasskeyResponse,            *\r\n"));
      Display(("*                          QueryEncryptionMode, SetPasskey,      *\r\n"));
      Display(("*                          DiscoverGAPS, GetLocalName,           *\r\n"));
      Display(("*                          SetLocalName, GetLERemoteName,        *\r\n"));
      Display(("*                          SetLocalAppearance,                   *\r\n"));
      Display(("*                          GetLocalAppearance,                   *\r\n"));
      Display(("*                          GetRemoteAppearance,                  *\r\n"));
      Display(("* Command Options SPPLE:   DiscoverSPPLE, RegisterSPPLE, LESend, *\r\n"));
      Display(("*                          ConfigureSPPLE, LERead, Loopback,     *\r\n"));
      Display(("*                          DisplayRawModeData, AutomaticReadMode *\r\n"));
      Display(("******************************************************************\r\n"));
   }

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
   if(BluetoothStackID)
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
   if(BluetoothStackID)
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
         Result = GAP_LE_Set_Pairability_Mode(BluetoothStackID, PairabilityMode);

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

   /* The following function is responsible for changing the Secure     */
   /* Simple Pairing Parameters that are exchanged during the Pairing   */
   /* procedure when Secure Simple Pairing (Security Level 4) is used.  */
   /* This function returns zero on successful execution and a negative */
   /* value on all errors.                                              */
static int ChangeLEPairingParameters(ParameterList_t *TempParam)
{
   int ret_val;

   /* First, check that valid Bluetooth Stack ID exists.                */
   if(BluetoothStackID)
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
   int                              Result;
   int                              ret_val;
   GAP_LE_Authentication_Response_Information_t  GAP_LE_Authentication_Response_Information;

   /* First, check that valid Bluetooth Stack ID exists.                */
   if(BluetoothStackID)
   {
      /* First, check to see if there is an on-going Pairing operation  */
      /* active.                                                        */
      if(!COMPARE_NULL_BD_ADDR(CurrentLERemoteBD_ADDR))
      {
         /* Make sure that all of the parameters required for this      */
         /* function appear to be at least semi-valid.                  */
         if((TempParam) && (TempParam->NumberofParameters > 0) && (BTPS_StringLength(TempParam->Params[0].strParam) <= GAP_LE_PASSKEY_MAXIMUM_NUMBER_OF_DIGITS))
         {
            /* Parameters appear to be valid, go ahead and populate the */
            /* response structure.                                      */
            GAP_LE_Authentication_Response_Information.GAP_LE_Authentication_Type  = larPasskey;
            GAP_LE_Authentication_Response_Information.Authentication_Data_Length  = (Byte_t)(sizeof(DWord_t));
            GAP_LE_Authentication_Response_Information.Authentication_Data.Passkey = (DWord_t)(TempParam->Params[0].intParam);

            /* Submit the Authentication Response.                      */
            Result = GAP_LE_Authentication_Response(BluetoothStackID, CurrentLERemoteBD_ADDR, &GAP_LE_Authentication_Response_Information);

            /* Check the return value for the submitted command for     */
            /* success.                                                 */
            if(!Result)
            {
               /* Operation was successful, inform the user.            */
               DisplayFunctionSuccess("Passkey Response");

               /* Flag success to the caller.                           */
               ret_val = 0;
            }
            else
            {
               /* Inform the user that the Authentication Response was  */
               /* not successful.                                       */
               DisplayFunctionError("GAP_LE_Authentication_Response", Result);

               ret_val = FUNCTION_ERROR;
            }

            /* Flag that there is no longer a current Authentication    */
            /* procedure in progress.                                   */
            ASSIGN_BD_ADDR(CurrentLERemoteBD_ADDR, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
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
   /* The following function is responsible for querying the Encryption */
   /* Mode for an LE Connection.  This function returns zero on         */
   /* successful execution and a negative value on all errors.          */
static int LEQueryEncryption(ParameterList_t *TempParam)
{
   int                   ret_val;
   int                   LEConnectionIndex;
   BD_ADDR_t             BD_ADDR;
   GAP_Encryption_Mode_t GAP_Encryption_Mode;

   /* First, check that valid Bluetooth Stack ID exists.                */
   if(BluetoothStackID)
   {
      /*Next, make sure that a valid device address exists.             */
      if((TempParam) && (TempParam->NumberofParameters > 0) && (TempParam->Params[0].strParam) && (BTPS_StringLength(TempParam->Params[0].strParam) >= (sizeof(BD_ADDR_t)*2)))
      {
         /* Convert the parameter to a Bluetooth Device Address.        */
         StrToBD_ADDR(TempParam->Params[0].strParam, &BD_ADDR);

         /* Find the LE Connection Index for this connection.           */
         if((LEConnectionIndex = FindLEIndexByAddress(BD_ADDR)) >= 0)
         {
            /* Query the current Encryption Mode for this Connection.   */
            ret_val = GAP_LE_Query_Encryption_Mode(BluetoothStackID, LEContextInfo[LEConnectionIndex].ConnectionBD_ADDR, &GAP_Encryption_Mode);
            if(!ret_val)
               Display(("Current Encryption Mode: %s.\r\n", (GAP_Encryption_Mode == emEnabled)?"Enabled":"Disabled"));
            else
            {
               Display(("Error - GAP_LE_Query_Encryption_Mode returned %d.\r\n", ret_val));

               ret_val = FUNCTION_ERROR;
            }
         }
         else
         {
            /* There is not currently an on-going authentication        */
            /* operation, inform the user of this error condition.      */
            Display(("No connections established.\r\n"));

            ret_val = FUNCTION_ERROR;
         }
      }
      else
      {
         /* Invalid parameters specified so flag an error to the user.  */
         Display(("Usage: QueryEncryptionMode [BD_ADDR].\r\n"));

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

   /* The following function is responsible for querying the Encryption */
   /* Mode for an LE Connection.  This function returns zero on         */
   /* successful execution and a negative value on all errors.          */
static int LESetPasskey(ParameterList_t *TempParam)
{
   int     ret_val;
   DWord_t Passkey;

   /* First, check that valid Bluetooth Stack ID exists.                */
   if(BluetoothStackID)
   {
      /* Make sure that all of the parameters required for this         */
      /* function appear to be at least semi-valid.                     */
      if((TempParam) && (TempParam->NumberofParameters >= 1) && ((TempParam->Params[0].intParam == 0) || (TempParam->Params[0].intParam == 1)))
      {
         if(TempParam->Params[0].intParam == 1)
         {
            /* We are setting the passkey so make sure it is valid.     */
            if(BTPS_StringLength(TempParam->Params[1].strParam) <= GAP_LE_PASSKEY_MAXIMUM_NUMBER_OF_DIGITS)
            {
               Passkey = (DWord_t)(TempParam->Params[1].intParam);

               ret_val = GAP_LE_Set_Fixed_Passkey(BluetoothStackID, &Passkey);
               if(!ret_val)
                  Display(("Fixed Passkey set to %06l.\r\n", Passkey));
            }
            else
            {
               Display(("Error - Invalid Passkey.\r\n"));

               ret_val = INVALID_PARAMETERS_ERROR;
            }
         }
         else
         {
            /* Un-set the fixed passkey that we previously configured.  */
            ret_val = GAP_LE_Set_Fixed_Passkey(BluetoothStackID, NULL);
            if(!ret_val)
               Display(("Fixed Passkey no longer configured.\r\n"));
         }

         /* If GAP_LE_Set_Fixed_Passkey returned an error display this. */
         if((ret_val) && (ret_val != INVALID_PARAMETERS_ERROR))
         {
            Display(("Error - GAP_LE_Set_Fixed_Passkey returned %d.\r\n", ret_val));

            ret_val = FUNCTION_ERROR;
         }
      }
      else
      {
         /* One or more of the necessary parameters is/are invalid.     */
         Display(("SetPasskey [(0 = UnSet Passkey, 1 = Set Fixed Passkey)] [6 Digit Passkey (optional)].\r\n"));

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

   /* The following function is responsible for enabling LE             */
   /* Advertisements.  This function returns zero on successful         */
   /* execution and a negative value on all errors.                     */
static int AdvertiseLE(ParameterList_t *TempParam)
{
   int                                 ret_val = 0;
   int                                 Length;
   GAP_LE_Advertising_Parameters_t     AdvertisingParameters;
   GAP_LE_Connectability_Parameters_t  ConnectabilityParameters;
   GAP_LE_Address_Type_t              OwnAddressType = latPublic;
   BD_ADDR_t                          BD_ADDR;
   Byte_t                             StatusResult;
   union
   {
      Advertising_Data_t               AdvertisingData;
      Scan_Response_Data_t             ScanResponseData;
   } Advertisement_Data_Buffer;

   /* First, check that valid Bluetooth Stack ID exists.                */
   if(BluetoothStackID)
   {
      /* Make sure that all of the parameters required for this function*/
      /* appear to be at least semi-valid.                              */
      if(TempParam && (TempParam->Params[0].strParam))
      {
         if  (((TempParam->NumberofParameters == 1) && ((TempParam->Params[0].intParam == 0) || (TempParam->Params[0].intParam == 1)))
         || ((TempParam->NumberofParameters >= 2) && ((TempParam->Params[1].intParam == 0) || (TempParam->Params[1].intParam == 1))))
         {

             /* Determine whether to enable or disable Advertising.     */
             if(TempParam->Params[0].intParam == 0)
             {
                /* Disable Advertising.                                 */
                ret_val = GAP_LE_Advertising_Disable(BluetoothStackID);
                if(!ret_val)
                {
                   Display(("   GAP_LE_Advertising_Disable success.\r\n"));
                }
                else
                {
                   DisplayFunctionError("GAP_LE_Advertising_Disable", ret_val);

                   ret_val = FUNCTION_ERROR;
                }
             }
             else
             {
                /* Verifying enable Advertising.parameters              */
                if(TempParam->NumberofParameters >= 2)
                {
                   if (TempParam->Params[1].intParam == 1)
                   {
                       OwnAddressType = latRandom;
                       if (BTPS_StringLength(TempParam->Params[2].strParam) >= (sizeof(BD_ADDR_t)*2))
                       {
                            /* Convert the parameter to a Bluetooth     */
                            /* Device Address.                          */
                            StrToBD_ADDR(TempParam->Params[2].strParam, &BD_ADDR);
                       }
                       else
                        ret_val = INVALID_PARAMETERS_ERROR;
                   }
                }

                if (!ret_val)
                {
                    /* Enable Advertising.  Set the Advertising Data.   */
                    BTPS_MemInitialize(&(Advertisement_Data_Buffer.AdvertisingData), 0, sizeof(Advertising_Data_t));

                    /* Set the Flags A/D Field (1 byte type and 1 byte  */
                    /* Flags.                                           */
                    Advertisement_Data_Buffer.AdvertisingData.Advertising_Data[0] = 2;
                    Advertisement_Data_Buffer.AdvertisingData.Advertising_Data[1] = HCI_LE_ADVERTISING_REPORT_DATA_TYPE_FLAGS;
                    Advertisement_Data_Buffer.AdvertisingData.Advertising_Data[2] = 0;

                    /* Configure the flags field based on the           */
                    /* Discoverability Mode.                            */
                    if(LE_Parameters.DiscoverabilityMode == dmGeneralDiscoverableMode)
                       Advertisement_Data_Buffer.AdvertisingData.Advertising_Data[2] = HCI_LE_ADVERTISING_FLAGS_GENERAL_DISCOVERABLE_MODE_FLAGS_BIT_MASK;
                    else
                    {
                       if(LE_Parameters.DiscoverabilityMode == dmLimitedDiscoverableMode)
                          Advertisement_Data_Buffer.AdvertisingData.Advertising_Data[2] = HCI_LE_ADVERTISING_FLAGS_LIMITED_DISCOVERABLE_MODE_FLAGS_BIT_MASK;
                    }

                    /* Write thee advertising data to the chip.         */
                    ret_val = GAP_LE_Set_Advertising_Data(BluetoothStackID, (Advertisement_Data_Buffer.AdvertisingData.Advertising_Data[0] + 1), &(Advertisement_Data_Buffer.AdvertisingData));
                    if(!ret_val)
                    {
                       BTPS_MemInitialize(&(Advertisement_Data_Buffer.ScanResponseData), 0, sizeof(Scan_Response_Data_t));

                       /* Set the Scan Response Data.                   */
                       Length = BTPS_StringLength(LE_APP_DEMO_NAME);
                       if(Length < (ADVERTISING_DATA_MAXIMUM_SIZE - 2))
                       {
                          Advertisement_Data_Buffer.ScanResponseData.Scan_Response_Data[1] = HCI_LE_ADVERTISING_REPORT_DATA_TYPE_LOCAL_NAME_COMPLETE;
                       }
                       else
                       {
                          Advertisement_Data_Buffer.ScanResponseData.Scan_Response_Data[1] = HCI_LE_ADVERTISING_REPORT_DATA_TYPE_LOCAL_NAME_SHORTENED;
                          Length = (ADVERTISING_DATA_MAXIMUM_SIZE - 2);
                       }

                       Advertisement_Data_Buffer.ScanResponseData.Scan_Response_Data[0] = (Byte_t)(1 + Length);
                       BTPS_MemCopy(&(Advertisement_Data_Buffer.ScanResponseData.Scan_Response_Data[2]),LE_APP_DEMO_NAME,Length);

                       ret_val = GAP_LE_Set_Scan_Response_Data(BluetoothStackID, (Advertisement_Data_Buffer.ScanResponseData.Scan_Response_Data[0] + 1), &(Advertisement_Data_Buffer.ScanResponseData));
                       if(!ret_val)
                       {
                          /* Set up the advertising parameters.         */
                          AdvertisingParameters.Advertising_Channel_Map   = HCI_LE_ADVERTISING_CHANNEL_MAP_DEFAULT;
                          AdvertisingParameters.Scan_Request_Filter       = fpNoFilter;
                          AdvertisingParameters.Connect_Request_Filter    = fpNoFilter;
                          AdvertisingParameters.Advertising_Interval_Min  = 100;
                          AdvertisingParameters.Advertising_Interval_Max  = 200;

                          /* Configure the Connectability Parameters.   */
                          /* * NOTE * Since we do not ever put ourselves*/
                          /*          to be direct connectable then we  */
                          /*          will set the DirectAddress to all */
                          /*          0s.                               */
                          ConnectabilityParameters.Connectability_Mode   = LE_Parameters.ConnectableMode;
                          ConnectabilityParameters.Own_Address_Type      = OwnAddressType;
                          ConnectabilityParameters.Direct_Address_Type   = latPublic;
                          ASSIGN_BD_ADDR(ConnectabilityParameters.Direct_Address, 0, 0, 0, 0, 0, 0);

                         /* If its a Random Address if So set the Random*/
                         /* address first                               */
                        if((OwnAddressType) && (!ret_val))
                        {
                            ret_val = HCI_LE_Set_Random_Address(BluetoothStackID, BD_ADDR, &StatusResult);
                            ret_val += StatusResult;
                        }

                         /* Now enable advertising.                     */
                        if(!ret_val)
                        {
                             ret_val = GAP_LE_Advertising_Enable(BluetoothStackID, TRUE, &AdvertisingParameters, &ConnectabilityParameters, GAP_LE_Event_Callback, 0);
                             if(!ret_val)
                             {
                                Display(("   GAP_LE_Advertising_Enable success.\r\n"));
                             }
                             else
                             {
                                DisplayFunctionError("GAP_LE_Advertising_Enable", ret_val);

                                ret_val = FUNCTION_ERROR;
                             }
                        }
                        else
                        {
                            DisplayFunctionError("HCI_LE_Set_Random_Address", ret_val);
                            ret_val = FUNCTION_ERROR;
                        }

                      }
                      else
                      {
                         DisplayFunctionError("GAP_LE_Set_Advertising_Data(dtScanResponse)", ret_val);

                         ret_val = FUNCTION_ERROR;
                      }

                   }
                   else
                   {
                      DisplayFunctionError("GAP_LE_Set_Advertising_Data(dtAdvertising)", ret_val);

                      ret_val = FUNCTION_ERROR;
                   }
                }
                else
                {
                   DisplayUsage("AdvertiseLE [0 = Disable] ");
                   DisplayUsage("AdvertiseLE [1 = Enable] [0 = Public  Address(Optional)]");
                   DisplayUsage("AdvertiseLE [1 = Enable] [1 = Random Address] [Random BD Address]");
                   ret_val = INVALID_PARAMETERS_ERROR;
                }
             }
         }
         else
         {
             DisplayUsage("AdvertiseLE [0 = Disable] ");
             DisplayUsage("AdvertiseLE [1 = Enable] [0 = Public  Address(Optional)]");
             DisplayUsage("AdvertiseLE [1 = Enable] [1 = Random Address] [Random BD Address]");
             ret_val = INVALID_PARAMETERS_ERROR;
         }
      }
      else
      {
          DisplayUsage("AdvertiseLE [0 = Disable] ");
          DisplayUsage("AdvertiseLE [1 = Enable] [0 = Public  Address(Optional)]");
          DisplayUsage("AdvertiseLE [1 = Enable] [1 = Random Address] [Random BD Address]");
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


   /* The following function is responsible for starting an LE scan     */
   /* procedure.  This function returns zero if successful and a        */
   /* negative value if an error occurred.                              */
static int StartScanning(ParameterList_t *TempParam)
{
   int ret_val;

   /* First, check that valid Bluetooth Stack ID exists.                */
   if(BluetoothStackID)
   {
      /* Simply start scanning.                                         */
      if(!StartScan(BluetoothStackID))
         ret_val = 0;
      else
         ret_val = FUNCTION_ERROR;
   }
   else
   {
      /* No valid Bluetooth Stack ID exists.                            */
      ret_val = INVALID_STACK_ID_ERROR;
   }

   return(ret_val);
}

   /* The following function is responsible for stopping an LE scan     */
   /* procedure.  This function returns zero if successful and a        */
   /* negative value if an error occurred.                              */
static int StopScanning(ParameterList_t *TempParam)
{
   int ret_val;

   /* First, check that valid Bluetooth Stack ID exists.                */
   if(BluetoothStackID)
   {
      /* Simply stop scanning.                                          */
      if(!StopScan(BluetoothStackID))
         ret_val = 0;
      else
         ret_val = FUNCTION_ERROR;
   }
   else
   {
      /* No valid Bluetooth Stack ID exists.                            */
      ret_val = INVALID_STACK_ID_ERROR;
   }

   return(ret_val);
}

   /* The following function is responsible for connecting to an LE     */
   /* device. This function need to receive at least a parameter of     */
   /* BD_ADDRESS [0xBD_Addr or BD_Addr], the next two parameters are    */
   /* Optional Remote Device Address Type and Own Address Type, The     */
   /* default value for this parameters is Public address type.  This   */
   /* function returns zero if successful and a negative value if an    */
   /* error occurred.                                                   */
static int ConnectLE(ParameterList_t *TempParam)
{
   int                   ret_val;
   BD_ADDR_t             BD_ADDR;
   GAP_LE_Address_Type_t OwnAddressType = latPublic;
   GAP_LE_Address_Type_t RemoteAddressType = latPublic;

   /* First, check that valid Bluetooth Stack ID exists.                */
   if(BluetoothStackID)
   {
      /* Next, make sure that a valid device address exists.            */
      if (TempParam && (BTPS_StringLength(TempParam->Params[0].strParam) >= (sizeof(BD_ADDR_t)*2)) && (TempParam->Params[0].strParam))
      {
          StrToBD_ADDR(TempParam->Params[0].strParam, &BD_ADDR);
          switch(TempParam->NumberofParameters)
          {
              case 3:
                      if ((TempParam->Params[2].intParam >= 0) && (TempParam->Params[2].intParam <= 1))
                          OwnAddressType  = (TempParam->Params[2].intParam == 0) ?latPublic :latRandom;
                      else
                      {
                          ret_val = INVALID_PARAMETERS_ERROR;
                          break;
                      }
              case 2:
                      if ((TempParam->Params[1].intParam >= 0) && (TempParam->Params[1].intParam <= 1))
                      {
                          if (TempParam->Params[1].intParam  == 1)
                          {
                              RemoteAddressType  = latRandom;
                              ResolveRemoteAddressHelper(BD_ADDR);
                          }
                      }
                      else
                      {
                          ret_val = INVALID_PARAMETERS_ERROR;
                          break;
                      }
              case 1:
                      if(!ConnectLEDevice(BluetoothStackID, BD_ADDR, RemoteAddressType, OwnAddressType, FALSE))
                          ret_val = 0;
                      else
                          ret_val = FUNCTION_ERROR;
                      break;
              default:
                      ret_val = INVALID_PARAMETERS_ERROR;
                      break;
          }
          if (ret_val)
          {
              /* Invalid parameters specified so flag an error to the   */
              /* user.                                                  */
              DisplayConnectLEUsage("ConnectLE");

              /* Flag that an error occurred while submitting the       */
              /* command.                                               */
              ret_val = INVALID_PARAMETERS_ERROR;
          }
      }
      else
      {
          /* Invalid parameters specified so flag an error to the user. */
          DisplayConnectLEUsage("ConnectLE");

          /* Flag that an error occurred while submitting the command.  */
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
   int       LEConnectionIndex;
   BD_ADDR_t BD_ADDR;

   /* First, check that valid Bluetooth Stack ID exists.                */
   if(BluetoothStackID)
   {
      /*Next, make sure that a valid device address exists.             */
      if((TempParam) && (TempParam->NumberofParameters > 0) && (TempParam->Params[0].strParam) && (BTPS_StringLength(TempParam->Params[0].strParam) >= (sizeof(BD_ADDR_t)*2)))
      {
         /* Convert the parameter to a Bluetooth Device Address.        */
         StrToBD_ADDR(TempParam->Params[0].strParam, &BD_ADDR);

         /* Find the LE Connection Index for this connection.           */
         if((LEConnectionIndex = FindLEIndexByAddress(BD_ADDR)) >= 0)
         {
            /* Attempt to disconnect the device.                        */
            if(!DisconnectLEDevice(BluetoothStackID, LEContextInfo[LEConnectionIndex].ConnectionBD_ADDR))
               ret_val = 0;
            else
               ret_val = FUNCTION_ERROR;
         }
         else
         {
            /* No matching ConnectionBD_ADDR.                           */
            Display(("No connection with BD_ADDR %s exists.\r\n", TempParam->Params[0].strParam));

            ret_val = FUNCTION_ERROR;
         }
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

   /* The following function is responsible for canceling an LE         */
   /* connection request. This function returns zero if successful and  */
   /* a negative value if an error occurred.                            */
static int CancelConnectLE(ParameterList_t *TempParam)
{
   int       ret_val;
   int       LEConnectionIndex;
   BD_ADDR_t BD_ADDR;

   /* First, check that valid Bluetooth Stack ID exists.                */
   if(BluetoothStackID)
   {
      /*Next, make sure that a valid device address exists.             */
      if((TempParam) && (TempParam->NumberofParameters > 0) && (TempParam->Params[0].strParam) && (BTPS_StringLength(TempParam->Params[0].strParam) >= (sizeof(BD_ADDR_t)*2)))
      {
         /* Convert the parameter to a Bluetooth Device Address.        */
         StrToBD_ADDR(TempParam->Params[0].strParam, &BD_ADDR);

         /* Find the LE Connection Index for this connection.           */
         if((LEConnectionIndex = FindLEIndexByAddress(BD_ADDR)) >= 0)
         {
            /* Attempt to cancel the connection to the device.          */
            if(!CancelConnectLEDevice(BluetoothStackID, LEContextInfo[LEConnectionIndex].ConnectionBD_ADDR))
               ret_val = 0;
            else
               ret_val = FUNCTION_ERROR;
         }
         else
         {
            /* We are not connecting or connected to this device.       */
            Display(("Not currently connecting to BD_ADDR %s.\r\n", TempParam->Params[0].strParam));

            ret_val = FUNCTION_ERROR;
         }
      }
      else
      {
         /* Invalid parameters specified so flag an error to the user.  */
         Display(("Usage: CancelConnectLE [BD_ADDR].\r\n"));

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

   /* The following function is provided to allow a mechanism of        */
   /* Pairing (or requesting security if a slave) to the connected      */
   /* device.                                                           */
static int PairLE(ParameterList_t *TempParam)
{
   int       ret_val;
   int       LEConnectionIndex;
   BD_ADDR_t BD_ADDR;

   /* First, check that valid Bluetooth Stack ID exists.                */
   if(BluetoothStackID)
   {
      /*Next, make sure that a valid device address exists.             */
      if((TempParam) && (TempParam->NumberofParameters > 0) && (TempParam->Params[0].strParam) && (BTPS_StringLength(TempParam->Params[0].strParam) >= (sizeof(BD_ADDR_t)*2)))
      {
         /* Convert the parameter to a Bluetooth Device Address.        */
         StrToBD_ADDR(TempParam->Params[0].strParam, &BD_ADDR);

         /* Find the LE Connection Index for this connection.           */
         if((LEConnectionIndex = FindLEIndexByAddress(BD_ADDR)) >= 0)
         {
            /* Attempt to send a pairing request to the specified       */
            /* device.                                                  */
            if(!SendPairingRequest(LEContextInfo[LEConnectionIndex].ConnectionBD_ADDR, LocalDeviceIsMaster))
               ret_val = 0;
            else
               ret_val = FUNCTION_ERROR;
         }
         else
         {
            /* No matching ConnectionBD_ADDR.                           */
            Display(("No connection with BD_ADDR %s exists.\r\n", TempParam->Params[0].strParam));

            ret_val = FUNCTION_ERROR;
         }
      }
      else
      {
         /* Invalid parameters specified so flag an error to the user.  */
         Display(("Usage: PairLE [BD_ADDR].\r\n"));

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
   int           LEConnectionIndex;
   BD_ADDR_t     BD_ADDR;
   GATT_UUID_t   UUID[1];
   DeviceInfo_t *DeviceInfo;

   /* Next, make sure that a valid device address exists.               */
   if((TempParam) && (TempParam->NumberofParameters > 0) && (TempParam->Params[0].strParam) && (BTPS_StringLength(TempParam->Params[0].strParam) >= (sizeof(BD_ADDR_t)*2)))
   {
      /* Convert the parameter to a Bluetooth Device Address.           */
      StrToBD_ADDR(TempParam->Params[0].strParam, &BD_ADDR);

      /* Find the LE Connection Index for this connection.              */
      if((LEConnectionIndex = FindLEIndexByAddress(BD_ADDR)) >= 0)
      {
         /* Get the device info for the connection device.              */
         if((DeviceInfo = SearchDeviceInfoEntryByBD_ADDR(&DeviceInfoList, LEContextInfo[LEConnectionIndex].ConnectionBD_ADDR)) != NULL)
         {
            /* Verify that no service discovery is outstanding for this */
            /* device.                                                  */
            if(!(DeviceInfo->Flags & DEVICE_INFO_FLAGS_SERVICE_DISCOVERY_OUTSTANDING))
            {
               /* Configure the filter so that only the SPP LE Service  */
               /* is discovered.                                        */
               UUID[0].UUID_Type = guUUID_16;
               GAP_ASSIGN_GAP_SERVICE_UUID_16(UUID[0].UUID.UUID_16);

               /* Start the service discovery process.                  */
               ret_val = GATT_Start_Service_Discovery(BluetoothStackID, LEContextInfo[LEConnectionIndex].ConnectionID, (sizeof(UUID)/sizeof(GATT_UUID_t)), UUID, GATT_Service_Discovery_Event_Callback, 0);
               if(!ret_val)
               {
                  /* Display success message.                           */
                  Display(("GATT_Start_Service_Discovery success.\r\n"));

                  /* Flag that a Service Discovery Operation is         */
                  /* outstanding.                                       */
                  DeviceInfo->Flags |= DEVICE_INFO_FLAGS_SERVICE_DISCOVERY_OUTSTANDING;
               }
               else
               {
                  /* An error occur so just clean-up.                   */
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
         /* No matching ConnectionBD_ADDR.                              */
         Display(("No connection with BD_ADDR %s exists.\r\n", TempParam->Params[0].strParam));

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
   if(GAPSInstanceID)
   {
      /* Initialize the Name Buffer to all zeros.                       */
      BTPS_MemInitialize(NameBuffer, 0, sizeof(NameBuffer));

      /* Query the Local Name.                                          */
      ret_val = GAPS_Query_Device_Name(BluetoothStackID, GAPSInstanceID, NameBuffer);
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

   /* The following function is responsible for setting the current     */
   /* Local Device Name.  This function will return zero on successful  */
   /* execution and a negative value on errors.                         */
static int SetLocalName(ParameterList_t *TempParam)
{
   int  ret_val;

   /* Verify that the input parameters are semi-valid.                  */
   if((TempParam) && (TempParam->NumberofParameters > 0) && (BTPS_StringLength(TempParam->Params[0].strParam) > 0) && (BTPS_StringLength(TempParam->Params[0].strParam) <= BTPS_CONFIGURATION_GAPS_MAXIMUM_SUPPORTED_DEVICE_NAME))
   {
      /* Verify that the GAP Service is registered.                     */
      if(GAPSInstanceID)
      {
         /* Attempt to submit the command.                              */
         ret_val = GAP_Set_Local_Device_Name(BluetoothStackID, TempParam->Params[0].strParam);
         if(!ret_val)
         {
            /* Query the Local Name.                                    */
            ret_val = GAPS_Set_Device_Name(BluetoothStackID, GAPSInstanceID, TempParam->Params[0].strParam);
            if(!ret_val)
               Display(("Local Device Name set to %s.\r\n", TempParam->Params[0].strParam));
            else
            {
               Display(("Error - GAPS_Query_Device_Name returned %d.\r\n", ret_val));

               ret_val = FUNCTION_ERROR;
            }
         }
         else
         {
            Display(("Error - GAP_Set_Local_Device_Name returned %d.\r\n", ret_val));

            ret_val = FUNCTION_ERROR;
         }
      }
      else
      {
         Display(("GAP Service not registered.\r\n"));

         ret_val = FUNCTION_ERROR;
      }
   }
   else
   {
      Display(("Usage: SetLocalName [NameString].\r\n"));

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
   int           LEConnectionIndex;
   BD_ADDR_t     BD_ADDR;
   DeviceInfo_t *DeviceInfo;

   /* Next, make sure that a valid device address exists.               */
   if((TempParam) && (TempParam->NumberofParameters > 0) && (TempParam->Params[0].strParam) && (BTPS_StringLength(TempParam->Params[0].strParam) >= (sizeof(BD_ADDR_t)*2)))
   {
      /* Convert the parameter to a Bluetooth Device Address.           */
      StrToBD_ADDR(TempParam->Params[0].strParam, &BD_ADDR);

      /* Find the LE Connection Index for this connection.              */
      if((LEConnectionIndex = FindLEIndexByAddress(BD_ADDR)) >= 0)
      {
         /* Get the device info for the connection device.              */
         if((DeviceInfo = SearchDeviceInfoEntryByBD_ADDR(&DeviceInfoList, LEContextInfo[LEConnectionIndex].ConnectionBD_ADDR)) != NULL)
         {
            /* Verify that we discovered the Device Name Handle.        */
            if(DeviceInfo->GAPSClientInfo.DeviceNameHandle)
            {
               /* Attempt to read the remote device name.               */
               ret_val = GATT_Read_Value_Request(BluetoothStackID, LEContextInfo[LEConnectionIndex].ConnectionID, DeviceInfo->GAPSClientInfo.DeviceNameHandle, GATT_ClientEventCallback_GAPS, (unsigned long)DeviceInfo->GAPSClientInfo.DeviceNameHandle);
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
         /* No matching ConnectionBD_ADDR.                              */
         Display(("No connection with BD_ADDR %s exists.\r\n", TempParam->Params[0].strParam));

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
   if(GAPSInstanceID)
   {
      /* Query the Local Name.                                          */
      ret_val = GAPS_Query_Device_Appearance(BluetoothStackID, GAPSInstanceID, &Appearance);
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

   /* The following function is responsible for setting the Local Device*/
   /* Appearance value.  This function will return zero on successful   */
   /* execution and a negative value on errors.                         */
static int SetLocalAppearance(ParameterList_t *TempParam)
{
   int    ret_val;
   Word_t Appearance;

   /* Verify that the input parameters are semi-valid.                  */
   if((TempParam) && (TempParam->NumberofParameters > 0) && (TempParam->Params[0].intParam >= 0) && (TempParam->Params[0].intParam < NUMBER_OF_APPEARANCE_MAPPINGS))
   {
      /* Verify that the GAP Service is registered.                     */
      if(GAPSInstanceID)
      {
         /* Map the Appearance Index to the GAP Appearance Value.       */
         if(AppearanceIndexToAppearance(TempParam->Params[0].intParam, &Appearance))
         {
            /* Set the Local Appearance.                                */
            ret_val = GAPS_Set_Device_Appearance(BluetoothStackID, GAPSInstanceID, Appearance);
            if(!ret_val)
               Display(("GAPS_Set_Device_Appearance success.\r\n"));
            else
            {
               Display(("Error - GAPS_Set_Device_Appearance returned %d.\r\n", ret_val));

               ret_val = FUNCTION_ERROR;
            }
         }
         else
         {
            Display(("Invalid Appearance Index.\r\n"));

            ret_val = FUNCTION_ERROR;
         }
      }
      else
      {
         Display(("GAP Service not registered.\r\n"));

         ret_val = FUNCTION_ERROR;
      }
   }
   else
   {
      Display(("Usage: SetLocalAppearance [Index].\r\n"));
      Display(("Where Index = \r\n"));
      DumpAppearanceMappings();

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
   int           LEConnectionIndex;
   BD_ADDR_t     BD_ADDR;
   DeviceInfo_t *DeviceInfo;

   /* Next, make sure that a valid device address exists.               */
   if((TempParam) && (TempParam->NumberofParameters > 0) && (TempParam->Params[0].strParam) && (BTPS_StringLength(TempParam->Params[0].strParam) >= (sizeof(BD_ADDR_t)*2)))
   {
      /* Convert the parameter to a Bluetooth Device Address.           */
      StrToBD_ADDR(TempParam->Params[0].strParam, &BD_ADDR);

      /* Find the LE Connection Index for this connection.              */
      if((LEConnectionIndex = FindLEIndexByAddress(BD_ADDR)) >= 0)
      {
         /* Get the device info for the connection device.              */
         if((DeviceInfo = SearchDeviceInfoEntryByBD_ADDR(&DeviceInfoList, LEContextInfo[LEConnectionIndex].ConnectionBD_ADDR)) != NULL)
         {
            /* Verify that we discovered the Device Name Handle.        */
            if(DeviceInfo->GAPSClientInfo.DeviceAppearanceHandle)
            {
               /* Attempt to read the remote device name.               */
               ret_val = GATT_Read_Value_Request(BluetoothStackID, LEContextInfo[LEConnectionIndex].ConnectionID, DeviceInfo->GAPSClientInfo.DeviceAppearanceHandle, GATT_ClientEventCallback_GAPS, (unsigned long)DeviceInfo->GAPSClientInfo.DeviceAppearanceHandle);
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
         /* No matching ConnectionBD_ADDR.                              */
         Display(("No connection with BD_ADDR %s exists.\r\n", TempParam->Params[0].strParam));

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

   /* The following function is responsible for performing a SPPLE      */
   /* Service Discovery Operation.  This function will return zero on   */
   /* successful execution and a negative value on errors.              */
static int DiscoverSPPLE(ParameterList_t *TempParam)
{
   int           ret_val;
   int           LEConnectionIndex;
   BD_ADDR_t     BD_ADDR;
   GATT_UUID_t   UUID[1];
   DeviceInfo_t *DeviceInfo;

   /* Next, make sure that a valid device address exists.               */
   if((TempParam) && (TempParam->NumberofParameters > 0) && (TempParam->Params[0].strParam) && (BTPS_StringLength(TempParam->Params[0].strParam) >= (sizeof(BD_ADDR_t)*2)))
   {
      /* Convert the parameter to a Bluetooth Device Address.           */
      StrToBD_ADDR(TempParam->Params[0].strParam, &BD_ADDR);

      /* Find the LE Connection Index for this connection.              */
      if((LEConnectionIndex = FindLEIndexByAddress(BD_ADDR)) >= 0)
      {
         /* Get the device info for the connection device.              */
         if((DeviceInfo = SearchDeviceInfoEntryByBD_ADDR(&DeviceInfoList, LEContextInfo[LEConnectionIndex].ConnectionBD_ADDR)) != NULL)
         {
            /* Verify that no service discovery is outstanding for this */
            /* device.                                                  */
            if(!(DeviceInfo->Flags & DEVICE_INFO_FLAGS_SERVICE_DISCOVERY_OUTSTANDING))
            {
               /* Configure the filter so that only the SPP LE Service  */
               /* is discovered.                                        */
               UUID[0].UUID_Type = guUUID_128;
               SPPLE_ASSIGN_SPPLE_SERVICE_UUID_128(&(UUID[0].UUID.UUID_128));

               /* Start the service discovery process.                  */
               ret_val = GATT_Start_Service_Discovery(BluetoothStackID, LEContextInfo[LEConnectionIndex].ConnectionID, (sizeof(UUID)/sizeof(GATT_UUID_t)), UUID, GATT_Service_Discovery_Event_Callback, 0);
               if(!ret_val)
               {
                  /* Display success message.                           */
                  Display(("GATT_Start_Service_Discovery success.\r\n"));

                  /* Flag that a Service Discovery Operation is         */
                  /* outstanding.                                       */
                  DeviceInfo->Flags |= DEVICE_INFO_FLAGS_SERVICE_DISCOVERY_OUTSTANDING;
               }
               else
               {
                  /* An error occur so just clean-up.                   */
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
         /* No matching ConnectionBD_ADDR.                              */
         Display(("No connection with BD_ADDR %s exists.\r\n", TempParam->Params[0].strParam));

         ret_val = FUNCTION_ERROR;
      }
   }
   else
   {
      /* Invalid parameters specified so flag an error to the user.     */
      Display(("Usage: DiscoverSPPLE [BD_ADDR].\r\n"));

      /* Flag that an error occurred while submitting the command.      */
      ret_val = INVALID_PARAMETERS_ERROR;
   }

   return(ret_val);
}

   /* The following function is responsible for registering a SPPLE     */
   /* Service.  This function will return zero on successful execution  */
   /* and a negative value on errors.                                   */
static int RegisterSPPLE(ParameterList_t *TempParam)
{
   int                           ret_val;
   GATT_Attribute_Handle_Group_t ServiceHandleGroup;

   /* Verify that there is no active connection.                        */
   if(FindFreeLEIndex() != -1)
   {
      /* Verify that the Service is not already registered.             */
      if(!SPPLEServiceID)
      {
         /* Initialize the handle group to 0 .                          */
         ServiceHandleGroup.Starting_Handle = 0;
         ServiceHandleGroup.Ending_Handle   = 0;

         /* Register the SPPLE Service.                                 */
         ret_val = GATT_Register_Service(BluetoothStackID, SPPLE_SERVICE_FLAGS, SPPLE_SERVICE_ATTRIBUTE_COUNT, (GATT_Service_Attribute_Entry_t *)SPPLE_Service, &ServiceHandleGroup, GATT_ServerEventCallback, 0);
         if(ret_val > 0)
         {
            /* Display success message.                                 */
            Display(("Successfully registered SPPLE Service.\r\n"));

            /* Save the ServiceID of the registered service.            */
            SPPLEServiceID = (unsigned int)ret_val;

            /* Return success to the caller.                            */
            ret_val        = 0;
         }
      }
      else
      {
         Display(("SPPLE Service already registered.\r\n"));

         ret_val = FUNCTION_ERROR;
      }
   }
   else
   {
      Display(("Connection currently active.\r\n"));

      ret_val = FUNCTION_ERROR;
   }

   return(ret_val);
}

   /* The following function is responsible for configure a SPPLE       */
   /* Service on a remote device.  This function will return zero on    */
   /* successful execution and a negative value on errors.              */
static int ConfigureSPPLE(ParameterList_t *TempParam)
{
   int           ret_val;
   int           LEConnectionIndex;
   BD_ADDR_t     BD_ADDR;
   DeviceInfo_t *DeviceInfo;

   /* Next, make sure that a valid device address exists.               */
   if((TempParam) && (TempParam->NumberofParameters > 0) && (TempParam->Params[0].strParam) && (BTPS_StringLength(TempParam->Params[0].strParam) >= (sizeof(BD_ADDR_t)*2)))
   {
      /* Convert the parameter to a Bluetooth Device Address.           */
      StrToBD_ADDR(TempParam->Params[0].strParam, &BD_ADDR);

      /* Find the LE Connection Index for this connection.              */
      if((LEConnectionIndex = FindLEIndexByAddress(BD_ADDR)) >= 0)
      {
         /* Get the device info for the connection device.              */
         if((DeviceInfo = SearchDeviceInfoEntryByBD_ADDR(&DeviceInfoList, LEContextInfo[LEConnectionIndex].ConnectionBD_ADDR)) != NULL)
         {
            /* Determine if a service discovery operation has been      */
            /* previously done.                                         */
            if(SPPLE_CLIENT_INFORMATION_VALID(DeviceInfo->ClientInfo))
            {
               Display(("SPPLE Service found on remote device, attempting to read Transmit Credits, and configured CCCDs.\r\n"));

               /* Send the Initial Credits to the remote device.        */
               SPPLESendCredits(&(LEContextInfo[LEConnectionIndex]), DeviceInfo, LEContextInfo[LEConnectionIndex].SPPLEBufferInfo.ReceiveBuffer.BytesFree);

               /* Enable Notifications on the proper characteristics.   */
               EnableDisableNotificationsIndications(LEContextInfo[LEConnectionIndex].ConnectionID, DeviceInfo->ClientInfo.Rx_Credit_Client_Configuration_Descriptor, GATT_CLIENT_CONFIGURATION_CHARACTERISTIC_NOTIFY_ENABLE, GATT_ClientEventCallback_SPPLE);
               EnableDisableNotificationsIndications(LEContextInfo[LEConnectionIndex].ConnectionID, DeviceInfo->ClientInfo.Tx_Client_Configuration_Descriptor, GATT_CLIENT_CONFIGURATION_CHARACTERISTIC_NOTIFY_ENABLE, GATT_ClientEventCallback_SPPLE);

               ret_val = 0;
            }
            else
            {
               Display(("No SPPLE Service discovered on device.\r\n"));

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
         /* No matching ConnectionBD_ADDR.                              */
         Display(("No connection with BD_ADDR %s exists.\r\n", TempParam->Params[0].strParam));

         ret_val = FUNCTION_ERROR;
      }
   }
   else
   {
      /* Invalid parameters specified so flag an error to the user.     */
      Display(("Usage: ConfigureSPPLE [BD_ADDR].\r\n"));

      /* Flag that an error occurred while submitting the command.      */
      ret_val = INVALID_PARAMETERS_ERROR;
   }

   return(ret_val);
}

   /* The following function is responsible for sending a number of     */
   /* characters to a remote device to which a connection exists.  The  */
   /* function receives a parameter that indicates the number of byte to*/
   /* be transferred.  This function will return zero on successful     */
   /* execution and a negative value on errors.                         */
static int SendDataCommand(ParameterList_t *TempParam)
{
   int           LEConnectionIndex;
   BD_ADDR_t     BD_ADDR;
   DeviceInfo_t *DeviceInfo;

   /* Make sure that all of the parameters required for this function   */
   /* appear to be at least semi-valid.                                 */
   if((TempParam) && (TempParam->NumberofParameters >= 2) && (TempParam->Params[1].intParam > 0) && (TempParam->Params[0].strParam) && (BTPS_StringLength(TempParam->Params[0].strParam) >= (sizeof(BD_ADDR_t)*2)))
   {
      /* Convert the parameter to a Bluetooth Device Address.           */
      StrToBD_ADDR(TempParam->Params[0].strParam, &BD_ADDR);

      /* Find the LE Connection Index for this connection.              */
      if((LEConnectionIndex = FindLEIndexByAddress(BD_ADDR)) >= 0)
      {
         /* Check to see if we are sending to another port.             */
         if(!(LEContextInfo[LEConnectionIndex].SPPLEBufferInfo.SendInfo.BytesToSend))
         {
            if(LEContextInfo[LEConnectionIndex].SPPLEBufferInfo.TransmitCredits > 0)
            {
               /* Get the device info for the connection device.        */
               if((DeviceInfo = SearchDeviceInfoEntryByBD_ADDR(&DeviceInfoList, LEContextInfo[LEConnectionIndex].ConnectionBD_ADDR)) != NULL)
               {
                  /* Get the count of the number of bytes to send.      */
                  LEContextInfo[LEConnectionIndex].SPPLEBufferInfo.SendInfo.BytesToSend  = (DWord_t)TempParam->Params[1].intParam;
                  LEContextInfo[LEConnectionIndex].SPPLEBufferInfo.SendInfo.BytesSent    = 0;

                  /* Kick start the send process.                       */
                  SPPLESendProcess(&(LEContextInfo[LEConnectionIndex]), DeviceInfo);
               }
               else
                  Display(("No device info.\r\n"));
            }
            else
               Display(("Cannot send data, no transmit credits.\r\n"));
         }
         else
            Display(("Send currently in progress.\r\n"));
      }
      else
      {
         /* No matching ConnectionBD_ADDR.                              */
         Display(("No connection with BD_ADDR %s exists.\r\n", TempParam->Params[0].strParam));
      }
   }
   else
      DisplayUsage("LESend [BD_ADDR] [Number of bytes to send]\r\n");

   return(0);
}

   /* The following function is responsible for reading data sent by a  */
   /* remote device to which a connection exists.  This function will   */
   /* return zero on successful execution and a negative value on       */
   /* errors.                                                           */
static int ReadDataCommand(ParameterList_t *TempParam)
{
   int           LEConnectionIndex;
   Boolean_t     Done;
   BD_ADDR_t     BD_ADDR;
   unsigned int  Temp;
   DeviceInfo_t *DeviceInfo;

   /* Next, make sure that a valid device address exists.               */
   if((TempParam) && (TempParam->NumberofParameters > 0) && (TempParam->Params[0].strParam) && (BTPS_StringLength(TempParam->Params[0].strParam) >= (sizeof(BD_ADDR_t)*2)))
   {
      /* Convert the parameter to a Bluetooth Device Address.           */
      StrToBD_ADDR(TempParam->Params[0].strParam, &BD_ADDR);

      /* Find the LE Connection Index for this connection.              */
      if((LEConnectionIndex = FindLEIndexByAddress(BD_ADDR)) >= 0)
      {
         /* Get the device info for the connection device.              */
         if((DeviceInfo = SearchDeviceInfoEntryByBD_ADDR(&DeviceInfoList, LEContextInfo[LEConnectionIndex].ConnectionBD_ADDR)) != NULL)
         {
            /* Determine the number of bytes we are going to read.      */
            Temp = LEContextInfo[LEConnectionIndex].SPPLEBufferInfo.ReceiveBuffer.BufferSize - LEContextInfo[LEConnectionIndex].SPPLEBufferInfo.ReceiveBuffer.BytesFree;

            Display(("Read: %u.\r\n", Temp));

            /* Loop and read all of the data.                           */
            Done = FALSE;
            while(!Done)
            {
               /* Read the data.                                        */
               Temp = SPPLEReadData(&(LEContextInfo[LEConnectionIndex]), DeviceInfo, SPPLE_DATA_BUFFER_LENGTH, SPPLEBuffer);
               if(Temp > 0)
               {
                  /* Display the data.                                  */
                  SPPLEBuffer[Temp] = '\0';
                  Display(((char *)SPPLEBuffer));
               }
               else
                  Done = TRUE;
            }
            Display(("\r\n"));
         }
         else
            Display(("No Device Info.\r\n"));
      }
      else
      {
         /* No matching ConnectionBD_ADDR.                              */
         Display(("No connection with BD_ADDR %s exists.\r\n", TempParam->Params[0].strParam));
      }
   }
   else
   {
      /* Invalid parameters specified so flag an error to the user.     */
      Display(("Usage: LEREAD [BD_ADDR].\r\n"));
   }

   return(0);
}

   /* The following function is a utility function that is used to find */
   /* the SPP Port Index by the specified Serial Port ID.  This function*/
   /* returns the index of the Serial Port or -1 on failure.            */
static int FindSPPPortIndex(unsigned int SerialPortID)
{
   int          ret_val = -1;
   unsigned int i;

   /* Search the list for the Serial Port Info.                         */
   for(i=0; i < sizeof(SPPContextInfo)/sizeof(SPP_Context_Info_t); i++)
   {
      /* Check to see if this entry matches the entry we are to search  */
      /* for.                                                           */
      if(SPPContextInfo[i].LocalSerialPortID == SerialPortID)
      {
         ret_val = (int)i;
         break;
      }
   }

   return(ret_val);
}

   /* The following function is a utility function that is used to find */
   /* the SPP Port Index by the specified Server Port Number.  This     */
   /* function returns the index of the Serial Port or -1 on failure.   */
static int FindSPPPortIndexByServerPortNumber(unsigned int ServerPortNumber)
{
   int          ret_val = -1;
   unsigned int i;

   /* Search the list for the Serial Port Info.                         */
   for(i=0; i < sizeof(SPPContextInfo)/sizeof(SPP_Context_Info_t); i++)
   {
      /* Check to see if this entry matches the entry we are to search  */
      /* for.                                                           */
      if(SPPContextInfo[i].ServerPortNumber == ServerPortNumber)
      {
         ret_val = (int)i;
         break;
      }
   }

   return(ret_val);
}

   /* The following function is a utility function that is used to find */
   /* the SPP Port Index by the specified BD_ADDR.  This function       */
   /* returns the index of the Serial Port or -1 on failure.            */
static int FindSPPPortIndexByAddress(BD_ADDR_t BD_ADDR)
{
   int          ret_val = -1;
   unsigned int i;

   /* Search the list for the Serial Port Info.                         */
   for(i=0; i < sizeof(SPPContextInfo)/sizeof(SPP_Context_Info_t); i++)
   {
      /* Check to see if this entry matches the entry we are to search  */
      /* for.                                                           */
      if(COMPARE_BD_ADDR(SPPContextInfo[i].BD_ADDR, BD_ADDR))
      {
         ret_val = (int)i;
         break;
      }
   }

   return(ret_val);
}

   /* The following function is a utility function that is used to find */
   /* a SPP Port Index that is not currently in use.  This function     */
   /* returns the index of the Serial Port or -1 on failure.            */
static int FindFreeSPPPortIndex(void)
{
   return(FindSPPPortIndex(0));
}

   /* The following function is a utility function which is used to     */
   /* close the specified port by the specified Serial Port ID.         */
static int ClosePortByNumber(unsigned int LocalSerialPortID)
{
   int       i;
   int       ret_val;
   Boolean_t ServerPort;

   /* Find an empty slot.                                               */
   if((i = FindSPPPortIndex(LocalSerialPortID)) >= 0)
   {
      /* Un-register the SDP Record registered for this server port.    */
      if(SPPContextInfo[i].ServerPortNumber)
      {
         if(SPPContextInfo[i].SPPServerSDPHandle)
            SPP_Un_Register_SDP_Record(BluetoothStackID, SPPContextInfo[i].LocalSerialPortID, SPPContextInfo[i].SPPServerSDPHandle);

         ServerPort = TRUE;
      }
      else
         ServerPort = FALSE;

      /* Close the specified server port.                               */
      if(ServerPort)
         ret_val = SPP_Close_Server_Port(BluetoothStackID, SPPContextInfo[i].LocalSerialPortID);
      else
         ret_val = SPP_Close_Port(BluetoothStackID, SPPContextInfo[i].LocalSerialPortID);

      if(ret_val < 0)
      {
         if(ServerPort)
            DisplayFunctionError("SPP_Close_Server_Port", ret_val);
         else
            DisplayFunctionError("SPP_Close_Port", ret_val);

         ret_val = FUNCTION_ERROR;
      }
      else
         ret_val = 0;

      /* Clear SPP Server info for that port.                           */
      BTPS_MemInitialize(&SPPContextInfo[i], 0, sizeof(SPPContextInfo[i]));

      Display(("Port Context Cleared.\r\n"));
   }

   return(ret_val);
}

   /* The following function is responsible for iterating through the   */
   /* array BDInfoArray[MAX_LE_CONNECTIONS], which contains the         */
   /* connection information for connected LE devices.  It returns -1 if*/
   /* the a free connection index is not found.  If a free index is     */
   /* found, it returns the free index which can be used for another    */
   /* connection.                                                       */
static int FindFreeLEIndex(void)
{
   BD_ADDR_t NullBD_ADDR;

   ASSIGN_BD_ADDR(NullBD_ADDR, 0, 0, 0, 0, 0, 0);

   return(FindLEIndexByAddress(NullBD_ADDR));
}

   /* The following function is responsible for iterating through the   */
   /* array BDInfoArray[MAX_LE_CONNECTIONS], which contains the         */
   /* connection information for connected LE devices.  It returns -1 if*/
   /* the BD_ADDR is not found.  If the BD_ADDR is found, it returns the*/
   /* index at which the BD_ADDR was found in the array.                */
static int FindLEIndexByAddress(BD_ADDR_t BD_ADDR)
{
   int i;
   int ret_val = -1;

   for(i=0; i<MAX_LE_CONNECTIONS; i++)
   {
      if(COMPARE_BD_ADDR(BD_ADDR, LEContextInfo[i].ConnectionBD_ADDR))
      {
         ret_val = i;
         break;
      }
   }

   return(ret_val);
}

   /* The following function is responsible for iterating through the   */
   /* array BDInfoArray[MAX_LE_CONNECTIONS], which contains the         */
   /* connection information for connected LE devices.  It returns -1 if*/
   /* the Connection ID is not found.  If the Connection ID is found, it*/
   /* returns the index at which the Connection ID was found in the     */
   /* array.                                                            */
static int FindLEIndexByConnectionID(unsigned int ConnectionID)
{
   int i;
   int ret_val = -1;

   for(i=0; i<MAX_LE_CONNECTIONS; i++)
   {
      if(LEContextInfo[i].ConnectionID == ConnectionID)
      {
         ret_val = i;
         break;
      }
   }

   return(ret_val);
}

   /* The following function is responsible for updating the connection */
   /* identifier for a given BD_ADDR.  If an entry in the array         */
   /* BDInfoArray[MAX_LE_CONNECTIONS] has a matching BD_ADDR, then its  */
   /* ConnectionID is set to the passed value of ConnectionID and the   */
   /* function returns its index in the array.  If no matching BD_ADDR  */
   /* is found, the function returns -1.                                */
static int UpdateConnectionID(unsigned int ConnectionID, BD_ADDR_t BD_ADDR)
{
   int LEConnectionIndex;

   /* Check for the index of the entry for this connection.             */
   LEConnectionIndex = FindLEIndexByAddress(BD_ADDR);
   if(LEConnectionIndex >= 0)
      LEContextInfo[LEConnectionIndex].ConnectionID = ConnectionID;
   else
   {
      Display(("Error in updating ConnectionID.\r\n"));
   }

   return(LEConnectionIndex);
}

   /* The following function is responsible for clearing the values of  */
   /* an entry in BDInfoArray if its ConnectionBD_ADDR matches BD_ADDR. */
static void RemoveConnectionInfo(BD_ADDR_t BD_ADDR)
{
   int LEConnectionIndex;

   /* If an index is returned (any but -1), then found                  */
   LEConnectionIndex = FindLEIndexByAddress(BD_ADDR);
   if(LEConnectionIndex >= 0)
   {
      ASSIGN_BD_ADDR(LEContextInfo[LEConnectionIndex].ConnectionBD_ADDR, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
      LEContextInfo[LEConnectionIndex].ConnectionID = 0;

      /* Re-initialize the Transmit and Receive Buffers, as well as the */
      /* transmit credits.                                              */
      InitializeBuffer(&(LEContextInfo[LEConnectionIndex].SPPLEBufferInfo.TransmitBuffer));
      InitializeBuffer(&(LEContextInfo[LEConnectionIndex].SPPLEBufferInfo.ReceiveBuffer));

      /* Clear the Transmit Credits count.                              */
      LEContextInfo[LEConnectionIndex].SPPLEBufferInfo.TransmitCredits      = 0;

      /* Flag that no credits are queued.                               */
      LEContextInfo[LEConnectionIndex].SPPLEBufferInfo.QueuedCredits        = 0;

      /* Clear the SPPLE Send Information.                              */
      LEContextInfo[LEConnectionIndex].SPPLEBufferInfo.SendInfo.BytesToSend  = 0;
      LEContextInfo[LEConnectionIndex].SPPLEBufferInfo.SendInfo.BytesSent    = 0;
      LEContextInfo[LEConnectionIndex].SPPLEBufferInfo.SendInfo.BufferFull   = FALSE;
      LEContextInfo[LEConnectionIndex].SPPLEBufferInfo.SendInfo.DataStrIndex = 0;
   }
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
   int                                           LEConnectionInfo;
   BoardStr_t                                    BoardStr;
   unsigned int                                  Index;
   DeviceInfo_t                                 *DeviceInfo;
   Long_Term_Key_t                               GeneratedLTK;
   GAP_LE_Security_Information_t                 GAP_LE_Security_Information;
   GAP_LE_Connection_Parameters_t                ConnectionParameters;
   GAP_LE_Advertising_Report_Data_t             *DeviceEntryPtr;
   GAP_LE_Authentication_Event_Data_t           *Authentication_Event_Data;
   GAP_LE_Authentication_Response_Information_t  GAP_LE_Authentication_Response_Information;

   /* Verify that all parameters to this callback are Semi-Valid.       */
   if((BluetoothStackID) && (GAP_LE_Event_Data))
   {
      switch(GAP_LE_Event_Data->Event_Data_Type)
      {
         case etLE_Advertising_Report:
            Display(("\r\netLE_Advertising_Report with size %d.\r\n",(int)GAP_LE_Event_Data->Event_Data_Size));
            Display(("  %d Responses.\r\n",GAP_LE_Event_Data->Event_Data.GAP_LE_Advertising_Report_Event_Data->Number_Device_Entries));

            for(Index = 0; Index < GAP_LE_Event_Data->Event_Data.GAP_LE_Advertising_Report_Event_Data->Number_Device_Entries; Index++)
            {
               DeviceEntryPtr = &(GAP_LE_Event_Data->Event_Data.GAP_LE_Advertising_Report_Event_Data->Advertising_Data[Index]);

               /* Display the packet type for the device                */
               switch(DeviceEntryPtr->Advertising_Report_Type)
               {
                  case rtConnectableUndirected:
                     Display(("  Advertising Type: %s.\r\n", "rtConnectableUndirected"));
                     break;
                  case rtConnectableDirected:
                     Display(("  Advertising Type: %s.\r\n", "rtConnectableDirected"));
                     break;
                  case rtScannableUndirected:
                     Display(("  Advertising Type: %s.\r\n", "rtScannableUndirected"));
                     break;
                  case rtNonConnectableUndirected:
                     Display(("  Advertising Type: %s.\r\n", "rtNonConnectableUndirected"));
                     break;
                  case rtScanResponse:
                     Display(("  Advertising Type: %s.\r\n", "rtScanResponse"));
                     break;
               }

               /* Display the Address Type.                             */
               if(DeviceEntryPtr->Address_Type == latPublic)
               {
                  Display(("  Address Type: %s.\r\n","atPublic"));
               }
               else
               {
                  Display(("  Address Type: %s.\r\n","atRandom"));
               }

               /* Display the Device Address.                           */
               Display(("  Address: 0x%02X%02X%02X%02X%02X%02X.\r\n", DeviceEntryPtr->BD_ADDR.BD_ADDR5, DeviceEntryPtr->BD_ADDR.BD_ADDR4, DeviceEntryPtr->BD_ADDR.BD_ADDR3, DeviceEntryPtr->BD_ADDR.BD_ADDR2, DeviceEntryPtr->BD_ADDR.BD_ADDR1, DeviceEntryPtr->BD_ADDR.BD_ADDR0));
               Display(("  RSSI: %d.\r\n", (int)DeviceEntryPtr->RSSI));
               Display(("  Data Length: %d.\r\n", DeviceEntryPtr->Raw_Report_Length));

               DisplayAdvertisingData(&(DeviceEntryPtr->Advertising_Data));
            }
            break;
         case etLE_Connection_Complete:
            Display(("\r\netLE_Connection_Complete with size %d.\r\n",(int)GAP_LE_Event_Data->Event_Data_Size));

            if(GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Complete_Event_Data)
            {
               BD_ADDRToStr(GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Complete_Event_Data->Peer_Address, BoardStr);

               Display(("   Status:       0x%02X.\r\n", GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Complete_Event_Data->Status));
               Display(("   Role:         %s.\r\n", (GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Complete_Event_Data->Master)?"Master":"Slave"));
               Display(("   Address Type: %s.\r\n", (GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Complete_Event_Data->Peer_Address_Type == latPublic)?"Public":"Random"));
               Display(("   BD_ADDR:      %s.\r\n", BoardStr));

               if(GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Complete_Event_Data->Status == HCI_ERROR_CODE_NO_ERROR)
               {
                  /* If not already in the connection info array, add   */
                  /* it.                                                */
                  if(FindLEIndexByAddress(GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Complete_Event_Data->Peer_Address) < 0)
                  {
                     /* Find an unused position in the connection info  */
                     /* array.                                          */
                     LEConnectionInfo = FindFreeLEIndex();
                     if(LEConnectionInfo >= 0)
                        LEContextInfo[LEConnectionInfo].ConnectionBD_ADDR = GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Complete_Event_Data->Peer_Address;
                  }

                  /* Set a global flag to indicate if we are the        */
                  /* connection master.                                 */
                  LocalDeviceIsMaster = GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Complete_Event_Data->Master;

                  /* Make sure that no entry already exists.            */
                  if((DeviceInfo = SearchDeviceInfoEntryByBD_ADDR(&DeviceInfoList, GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Complete_Event_Data->Peer_Address)) == NULL)
                  {
                     /* No entry exists so create one.                  */
                     if((DeviceInfo = CreateNewDeviceInfoEntry(&DeviceInfoList, GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Complete_Event_Data->Peer_Address_Type, GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Complete_Event_Data->Peer_Address)) != NULL)
                     {
                        /* Flag that we are now connected.              */
                        DeviceInfo->Flags |= DEVICE_INFO_FLAGS_CONNECTED;
                     }
                     else
                     {
                        Display(("Failed to add device to Device Info List.\r\n"));
                     }
                  }
                  else
                  {
                     /* Flag that we are now connected.                 */
                     DeviceInfo->Flags |= DEVICE_INFO_FLAGS_CONNECTED;

                     /* If we are the Master of the connection we will  */
                     /* attempt to Re-Establish Security if a LTK for   */
                     /* this device exists (i.e.  we previously paired).*/
                     if(LocalDeviceIsMaster)
                     {
                        /* Re-Establish Security if there is a LTK that */
                        /* is stored for this device.                   */
                        if(DeviceInfo->Flags & DEVICE_INFO_FLAGS_LTK_VALID)
                        {
                           /* Re-Establish Security with this LTK.      */
                           Display(("Attempting to Re-Establish Security.\r\n"));

                           /* Attempt to re-establish security to this  */
                           /* device.                                   */
                           GAP_LE_Security_Information.Local_Device_Is_Master                                      = TRUE;
                           BTPS_MemCopy(&(GAP_LE_Security_Information.Security_Information.Master_Information.LTK), &(DeviceInfo->LTK), LONG_TERM_KEY_SIZE);
                           BTPS_MemCopy(&(GAP_LE_Security_Information.Security_Information.Master_Information.Rand), &(DeviceInfo->Rand), RANDOM_NUMBER_DATA_SIZE);

                           GAP_LE_Security_Information.Security_Information.Master_Information.EDIV                = DeviceInfo->EDIV;
                           GAP_LE_Security_Information.Security_Information.Master_Information.Encryption_Key_Size = DeviceInfo->EncryptionKeySize;

                           Result = GAP_LE_Reestablish_Security(BluetoothStackID, DeviceInfo->ConnectionBD_ADDR, &GAP_LE_Security_Information, GAP_LE_Event_Callback, 0);
                           if(Result)
                           {
                              Display(("GAP_LE_Reestablish_Security returned %d.\r\n",Result));
                           }
                        }
                     }
                  }
               }
               else
               {
                  /* Clear the Connection ID.                           */
                  RemoveConnectionInfo(GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Complete_Event_Data->Peer_Address);
               }
            }
            break;
         case etLE_Disconnection_Complete:
            Display(("\r\netLE_Disconnection_Complete with size %d.\r\n", (int)GAP_LE_Event_Data->Event_Data_Size));

            if(GAP_LE_Event_Data->Event_Data.GAP_LE_Disconnection_Complete_Event_Data)
            {
               Display(("   Status: 0x%02X.\r\n", GAP_LE_Event_Data->Event_Data.GAP_LE_Disconnection_Complete_Event_Data->Status));
               Display(("   Reason: 0x%02X.\r\n", GAP_LE_Event_Data->Event_Data.GAP_LE_Disconnection_Complete_Event_Data->Reason));

               BD_ADDRToStr(GAP_LE_Event_Data->Event_Data.GAP_LE_Disconnection_Complete_Event_Data->Peer_Address, BoardStr);

               Display(("   BD_ADDR: %s.\r\n", BoardStr));

               /* Check to see if the device info is present in the     */
               /* list.                                                 */
               if((DeviceInfo = SearchDeviceInfoEntryByBD_ADDR(&DeviceInfoList, GAP_LE_Event_Data->Event_Data.GAP_LE_Disconnection_Complete_Event_Data->Peer_Address)) != NULL)
               {
                  /* Flag that we are no longer connected.              */
                  DeviceInfo->Flags &= ~DEVICE_INFO_FLAGS_CONNECTED;

                  /* Flag that no service discovery operation is        */
                  /* outstanding for this device.                       */
                  DeviceInfo->Flags &= ~DEVICE_INFO_FLAGS_SERVICE_DISCOVERY_OUTSTANDING;

                  /* Clear the CCCDs stored for this device.            */
                  DeviceInfo->ServerInfo.Rx_Credit_Client_Configuration_Descriptor = 0;
                  DeviceInfo->ServerInfo.Tx_Client_Configuration_Descriptor        = 0;

                  /* If this device is not paired, then delete it.  The */
                  /* link will be encrypted if the device is paired.    */
                  if(!(DeviceInfo->Flags & DEVICE_INFO_FLAGS_LINK_ENCRYPTED))
                  {
                     if((DeviceInfo = DeleteDeviceInfoEntry(&DeviceInfoList, GAP_LE_Event_Data->Event_Data.GAP_LE_Disconnection_Complete_Event_Data->Peer_Address)) != NULL)
                        FreeDeviceInfoEntryMemory(DeviceInfo);
                  }
                  else
                  {
                     /* Flag that the Link is no longer encrypted since */
                     /* we have disconnected.                           */
                     DeviceInfo->Flags &= ~DEVICE_INFO_FLAGS_LINK_ENCRYPTED;
                  }
               }
            }
            break;
         case etLE_Connection_Parameter_Update_Request:
            Display(("\r\netLE_Connection_Parameter_Update_Request with size %d.\r\n", (int)GAP_LE_Event_Data->Event_Data_Size));

            if(GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Parameter_Update_Request_Event_Data)
            {
               BD_ADDRToStr(GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Parameter_Update_Request_Event_Data->BD_ADDR, BoardStr);
               Display(("   BD_ADDR:             %s.\r\n", BoardStr));
               Display(("   Minimum Interval:    %u.\r\n", (unsigned int)GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Parameter_Update_Request_Event_Data->Conn_Interval_Min));
               Display(("   Maximum Interval:    %u.\r\n", (unsigned int)GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Parameter_Update_Request_Event_Data->Conn_Interval_Max));
               Display(("   Slave Latency:       %u.\r\n", (unsigned int)GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Parameter_Update_Request_Event_Data->Slave_Latency));
               Display(("   Supervision Timeout: %u.\r\n", (unsigned int)GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Parameter_Update_Request_Event_Data->Conn_Supervision_Timeout));

               /* Initialize the connection parameters.                 */
               ConnectionParameters.Connection_Interval_Min    = GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Parameter_Update_Request_Event_Data->Conn_Interval_Min;
               ConnectionParameters.Connection_Interval_Max    = GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Parameter_Update_Request_Event_Data->Conn_Interval_Max;
               ConnectionParameters.Slave_Latency              = GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Parameter_Update_Request_Event_Data->Slave_Latency;
               ConnectionParameters.Supervision_Timeout        = GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Parameter_Update_Request_Event_Data->Conn_Supervision_Timeout;
               ConnectionParameters.Minimum_Connection_Length  = 0;
               ConnectionParameters.Maximum_Connection_Length  = 10000;

               Display(("\r\nAttempting to accept connection parameter update request.\r\n"));

               /* Go ahead and accept whatever the slave has requested. */
               Result = GAP_LE_Connection_Parameter_Update_Response(BluetoothStackID, GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Parameter_Update_Request_Event_Data->BD_ADDR, TRUE, &ConnectionParameters);
               if(!Result)
               {
                  Display(("      GAP_LE_Connection_Parameter_Update_Response() success.\r\n"));
               }
               else
               {
                  Display(("      GAP_LE_Connection_Parameter_Update_Response() error %d.\r\n", Result));
               }
            }
            break;
         case etLE_Connection_Parameter_Updated:
            Display(("\r\netLE_Connection_Parameter_Updated with size %d.\r\n", (int)GAP_LE_Event_Data->Event_Data_Size));

            if(GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Parameter_Updated_Event_Data)
            {
               BD_ADDRToStr(GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Parameter_Updated_Event_Data->BD_ADDR, BoardStr);
               Display(("   Status:              0x%02X.\r\n", GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Parameter_Updated_Event_Data->Status));
               Display(("   BD_ADDR:             %s.\r\n", BoardStr));

               if(GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Parameter_Updated_Event_Data->Status == HCI_ERROR_CODE_NO_ERROR)
               {
                  Display(("   Connection Interval: %u.\r\n", (unsigned int)GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Parameter_Updated_Event_Data->Current_Connection_Parameters.Connection_Interval));
                  Display(("   Slave Latency:       %u.\r\n", (unsigned int)GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Parameter_Updated_Event_Data->Current_Connection_Parameters.Slave_Latency));
                  Display(("   Supervision Timeout: %u.\r\n", (unsigned int)GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Parameter_Updated_Event_Data->Current_Connection_Parameters.Supervision_Timeout));
               }
            }
            break;
         case etLE_Encryption_Change:
            Display(("\r\netLE_Encryption_Change with size %d.\r\n",(int)GAP_LE_Event_Data->Event_Data_Size));

            /* Search for the device entry to see flag if the link is   */
            /* encrypted.                                               */
            if((DeviceInfo = SearchDeviceInfoEntryByBD_ADDR(&DeviceInfoList, GAP_LE_Event_Data->Event_Data.GAP_LE_Encryption_Change_Event_Data->BD_ADDR)) != NULL)
            {
               /* Check to see if the encryption change was successful. */
               if((GAP_LE_Event_Data->Event_Data.GAP_LE_Encryption_Change_Event_Data->Encryption_Change_Status == HCI_ERROR_CODE_NO_ERROR) && (GAP_LE_Event_Data->Event_Data.GAP_LE_Encryption_Change_Event_Data->Encryption_Mode == emEnabled))
                  DeviceInfo->Flags |= DEVICE_INFO_FLAGS_LINK_ENCRYPTED;
               else
                  DeviceInfo->Flags &= ~DEVICE_INFO_FLAGS_LINK_ENCRYPTED;
            }
            break;
         case etLE_Encryption_Refresh_Complete:
            Display(("\r\netLE_Encryption_Refresh_Complete with size %d.\r\n", (int)GAP_LE_Event_Data->Event_Data_Size));

            /* Search for the device entry to see flag if the link is   */
            /* encrypted.                                               */
            if((DeviceInfo = SearchDeviceInfoEntryByBD_ADDR(&DeviceInfoList, GAP_LE_Event_Data->Event_Data.GAP_LE_Encryption_Refresh_Complete_Event_Data->BD_ADDR)) != NULL)
            {
               /* Check to see if the refresh was successful.           */
               if(GAP_LE_Event_Data->Event_Data.GAP_LE_Encryption_Refresh_Complete_Event_Data->Status == HCI_ERROR_CODE_NO_ERROR)
                  DeviceInfo->Flags |= DEVICE_INFO_FLAGS_LINK_ENCRYPTED;
               else
                  DeviceInfo->Flags &= ~DEVICE_INFO_FLAGS_LINK_ENCRYPTED;
            }
            break;
         case etLE_Authentication:
            Display(("\r\netLE_Authentication with size %d.\r\n", (int)GAP_LE_Event_Data->Event_Data_Size));

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

                        BTPS_MemCopy(&(GAP_LE_Authentication_Response_Information.Authentication_Data.Long_Term_Key_Information.Long_Term_Key), &GeneratedLTK, LONG_TERM_KEY_SIZE);
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
                     if(Result)
                     {
                        Display(("      GAP_LE_Authentication_Response returned %d.\r\n",Result));
                     }
                     break;
                  case latSecurityRequest:
                     /* Display the data for this event.                */
                     /* * NOTE * This is only sent from Slave to Master.*/
                     /*          Thus we must be the Master in this     */
                     /*          connection.                            */
                     Display(("    latSecurityRequest:.\r\n"));
                     Display(("      BD_ADDR: %s.\r\n", BoardStr));
                     Display(("      Bonding Type: %s.\r\n", ((Authentication_Event_Data->Authentication_Event_Data.Security_Request.Bonding_Type == lbtBonding)?"Bonding":"No Bonding")));
                     Display(("      MITM: %s.\r\n", ((Authentication_Event_Data->Authentication_Event_Data.Security_Request.MITM == TRUE)?"YES":"NO")));

                     /* Determine if we have previously paired with the */
                     /* device. If we have paired we will attempt to    */
                     /* re-establish security using a previously        */
                     /* exchanged LTK.                                  */
                     if((DeviceInfo = SearchDeviceInfoEntryByBD_ADDR(&DeviceInfoList, Authentication_Event_Data->BD_ADDR)) != NULL)
                     {
                        /* Determine if a Valid Long Term Key is stored */
                        /* for this device.                             */
                        if(DeviceInfo->Flags & DEVICE_INFO_FLAGS_LTK_VALID)
                        {
                           Display(("Attempting to Re-Establish Security.\r\n"));

                           /* Attempt to re-establish security to this  */
                           /* device.                                   */
                           GAP_LE_Security_Information.Local_Device_Is_Master                                      = TRUE;

                           BTPS_MemCopy(&(GAP_LE_Security_Information.Security_Information.Master_Information.LTK), &(DeviceInfo->LTK), LONG_TERM_KEY_SIZE);
                           BTPS_MemCopy(&(GAP_LE_Security_Information.Security_Information.Master_Information.Rand), &(DeviceInfo->Rand), RANDOM_NUMBER_DATA_SIZE);

                           GAP_LE_Security_Information.Security_Information.Master_Information.EDIV                = DeviceInfo->EDIV;
                           GAP_LE_Security_Information.Security_Information.Master_Information.Encryption_Key_Size = DeviceInfo->EncryptionKeySize;

                           Result = GAP_LE_Reestablish_Security(BluetoothStackID, Authentication_Event_Data->BD_ADDR, &GAP_LE_Security_Information, GAP_LE_Event_Callback, 0);
                           if(Result)
                           {
                              Display(("GAP_LE_Reestablish_Security returned %d.\r\n",Result));
                           }
                        }
                        else
                        {
                           CurrentLERemoteBD_ADDR = Authentication_Event_Data->BD_ADDR;

                           /* We do not have a stored Link Key for this */
                           /* device so go ahead and pair to this       */
                           /* device.                                   */
                           SendPairingRequest(Authentication_Event_Data->BD_ADDR, TRUE);
                        }
                     }
                     else
                     {
                        CurrentLERemoteBD_ADDR = Authentication_Event_Data->BD_ADDR;

                        /* There is no Key Info Entry for this device   */
                        /* so we will just treat this as a slave        */
                        /* request and initiate pairing.                */
                        SendPairingRequest(Authentication_Event_Data->BD_ADDR, TRUE);
                     }

                     break;
                  case latPairingRequest:
                     CurrentLERemoteBD_ADDR = Authentication_Event_Data->BD_ADDR;

                     Display(("Pairing Request: %s.\r\n",BoardStr));
                     DisplayPairingInformation(Authentication_Event_Data->Authentication_Event_Data.Pairing_Request);

                     /* This is a pairing request. Respond with a       */
                     /* Pairing Response.                               */
                     /* * NOTE * This is only sent from Master to Slave.*/
                     /*          Thus we must be the Slave in this      */
                     /*          connection.                            */

                     /* Send the Pairing Response.                      */
                     SlavePairingRequestResponse(Authentication_Event_Data->BD_ADDR);
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
                        {
                           Display(("GAP_LE_Authentication_Response returned %d.\r\n",Result));
                        }
                     }
                     else
                     {
                        if(Authentication_Event_Data->Authentication_Event_Data.Confirmation_Request.Request_Type == crtPasskey)
                        {
                           Display(("Call LEPasskeyResponse [PASSCODE].\r\n"));
                        }
                        else
                        {
                           if(Authentication_Event_Data->Authentication_Event_Data.Confirmation_Request.Request_Type == crtDisplay)
                           {
                              Display(("Passkey: %06ld.\r\n", Authentication_Event_Data->Authentication_Event_Data.Confirmation_Request.Display_Passkey));
                           }
                        }
                     }
                     break;
                  case latSecurityEstablishmentComplete:
                     Display(("Security Re-Establishment Complete: %s.\r\n", BoardStr));
                     Display(("                            Status: 0x%02X.\r\n", Authentication_Event_Data->Authentication_Event_Data.Security_Establishment_Complete.Status));
                     break;
                  case latPairingStatus:
                     ASSIGN_BD_ADDR(CurrentLERemoteBD_ADDR, 0, 0, 0, 0, 0, 0);

                     Display(("Pairing Status: %s.\r\n", BoardStr));
                     Display(("        Status: 0x%02X.\r\n", Authentication_Event_Data->Authentication_Event_Data.Pairing_Status.Status));

                     if(Authentication_Event_Data->Authentication_Event_Data.Pairing_Status.Status == GAP_LE_PAIRING_STATUS_NO_ERROR)
                     {
                        Display(("        Key Size: %d.\r\n", Authentication_Event_Data->Authentication_Event_Data.Pairing_Status.Negotiated_Encryption_Key_Size));
                     }
                     else
                     {
                        /* Failed to pair so delete the key entry for   */
                        /* this device and disconnect the link.         */
                        if((DeviceInfo = DeleteDeviceInfoEntry(&DeviceInfoList, Authentication_Event_Data->BD_ADDR)) != NULL)
                           FreeDeviceInfoEntryMemory(DeviceInfo);

                        /* Disconnect the Link.                         */
                        GAP_LE_Disconnect(BluetoothStackID, Authentication_Event_Data->BD_ADDR);
                     }
                     break;
                  case latEncryptionInformationRequest:
                     Display(("Encryption Information Request %s.\r\n", BoardStr));

                     /* Generate new LTK,EDIV and Rand and respond with */
                     /* them.                                           */
                     EncryptionInformationRequestResponse(Authentication_Event_Data->BD_ADDR, Authentication_Event_Data->Authentication_Event_Data.Encryption_Request_Information.Encryption_Key_Size, &GAP_LE_Authentication_Response_Information);
                     break;
                  case latEncryptionInformation:
                     /* Display the information from the event.         */
                     Display((" Encryption Information from RemoteDevice: %s.\r\n", BoardStr));
                     Display(("                             Key Size: %d.\r\n", Authentication_Event_Data->Authentication_Event_Data.Encryption_Information.Encryption_Key_Size));

                     /* ** NOTE ** If we are the Slave we will NOT      */
                     /*            store the LTK that is sent to us by  */
                     /*            the Master.  However if it was ever  */
                     /*            desired that the Master and Slave    */
                     /*            switch roles in a later connection   */
                     /*            we could store that information at   */
                     /*            this point.                          */
                     if(LocalDeviceIsMaster)
                     {
                        /* Search for the entry for this slave to store */
                        /* the information into.                        */
                        if((DeviceInfo = SearchDeviceInfoEntryByBD_ADDR(&DeviceInfoList, Authentication_Event_Data->BD_ADDR)) != NULL)
                        {
                           BTPS_MemCopy(&(DeviceInfo->LTK), &(Authentication_Event_Data->Authentication_Event_Data.Encryption_Information.LTK), LONG_TERM_KEY_SIZE);
                           BTPS_MemCopy(&(DeviceInfo->Rand), &(Authentication_Event_Data->Authentication_Event_Data.Encryption_Information.Rand), RANDOM_NUMBER_DATA_SIZE);

                           DeviceInfo->EDIV              = Authentication_Event_Data->Authentication_Event_Data.Encryption_Information.EDIV;
                           DeviceInfo->EncryptionKeySize = Authentication_Event_Data->Authentication_Event_Data.Encryption_Information.Encryption_Key_Size;
                           DeviceInfo->Flags            |= DEVICE_INFO_FLAGS_LTK_VALID;
                        }
                        else
                        {
                           Display(("No Key Info Entry for this Slave.\r\n"));
                        }
                     }
                     break;
               }
            }
            break;
      }

      /* Display the command prompt.                                    */
      DisplayPrompt();
   }
}

   /* The following function is for an GATT Server Event Callback.  This*/
   /* function will be called whenever a GATT Request is made to the    */
   /* server who registers this function that cannot be handled         */
   /* internally by GATT.  This function passes to the caller the GATT  */
   /* Server Event Data that occurred and the GATT Server Event Callback*/
   /* Parameter that was specified when this Callback was installed.    */
   /* The caller is free to use the contents of the GATT Server Event   */
   /* Data ONLY in the context of this callback.  If the caller requires*/
   /* the Data for a longer period of time, then the callback function  */
   /* MUST copy the data into another Data Buffer.  This function is    */
   /* guaranteed NOT to be invoked more than once simultaneously for the*/
   /* specified installed callback (i.e.  this function DOES NOT have be*/
   /* reentrant).  It Needs to be noted however, that if the same       */
   /* Callback is installed more than once, then the callbacks will be  */
   /* called serially.  Because of this, the processing in this function*/
   /* should be as efficient as possible.  It should also be noted that */
   /* this function is called in the Thread Context of a Thread that the*/
   /* User does NOT own.  Therefore, processing in this function should */
   /* be as efficient as possible (this argument holds anyway because   */
   /* another GATT Event (Server/Client or Connection) will not be      */
   /* processed while this function call is outstanding).               */
   /* * NOTE * This function MUST NOT Block and wait for Events that can*/
   /*          only be satisfied by Receiving a Bluetooth Event         */
   /*          Callback.  A Deadlock WILL occur because NO Bluetooth    */
   /*          Callbacks will be issued while this function is currently*/
   /*          outstanding.                                             */
static void BTPSAPI GATT_ServerEventCallback(unsigned int BluetoothStackID, GATT_Server_Event_Data_t *GATT_ServerEventData, unsigned long CallbackParameter)
{
   int           LEConnectionIndex;
   Byte_t        Temp[2];
   Word_t        Value;
   Word_t        PreviousValue;
   Word_t        AttributeOffset;
   DeviceInfo_t *DeviceInfo;

   /* Verify that all parameters to this callback are Semi-Valid.       */
   if((BluetoothStackID) && (GATT_ServerEventData))
   {
      switch(GATT_ServerEventData->Event_Data_Type)
      {
         case etGATT_Server_Read_Request:
            /* Verify that the Event Data is valid.                     */
            if(GATT_ServerEventData->Event_Data.GATT_Read_Request_Data)
            {
               /* Verify that the read isn't a read blob (no SPPLE      */
               /* readable characteristics are long).                   */
               if(GATT_ServerEventData->Event_Data.GATT_Read_Request_Data->AttributeValueOffset == 0)
               {
                  /* Find the LE Connection Index for this connection.  */
                  if((LEConnectionIndex = FindLEIndexByAddress(GATT_ServerEventData->Event_Data.GATT_Read_Request_Data->RemoteDevice)) >= 0)
                  {
                     /* Grab the device info for the currently          */
                     /* connected device.                               */
                     if((DeviceInfo = SearchDeviceInfoEntryByBD_ADDR(&DeviceInfoList, LEContextInfo[LEConnectionIndex].ConnectionBD_ADDR)) != NULL)
                     {
                        /* Determine which request this read is coming  */
                        /* for.                                         */
                        switch(GATT_ServerEventData->Event_Data.GATT_Read_Request_Data->AttributeOffset)
                        {
                           case SPPLE_TX_CHARACTERISTIC_CCD_ATTRIBUTE_OFFSET:
                              ASSIGN_HOST_WORD_TO_LITTLE_ENDIAN_UNALIGNED_WORD(Temp, DeviceInfo->ServerInfo.Tx_Client_Configuration_Descriptor);
                              break;
                           case SPPLE_TX_CREDITS_CHARACTERISTIC_ATTRIBUTE_OFFSET:
                              ASSIGN_HOST_WORD_TO_LITTLE_ENDIAN_UNALIGNED_WORD(Temp, LEContextInfo[LEConnectionIndex].SPPLEBufferInfo.TransmitCredits);
                              break;
                           case SPPLE_RX_CREDITS_CHARACTERISTIC_ATTRIBUTE_OFFSET:
                              ASSIGN_HOST_WORD_TO_LITTLE_ENDIAN_UNALIGNED_WORD(Temp, LEContextInfo[LEConnectionIndex].SPPLEBufferInfo.ReceiveBuffer.BytesFree);
                              break;
                           case SPPLE_RX_CREDITS_CHARACTERISTIC_CCD_ATTRIBUTE_OFFSET:
                              ASSIGN_HOST_WORD_TO_LITTLE_ENDIAN_UNALIGNED_WORD(Temp, DeviceInfo->ServerInfo.Rx_Credit_Client_Configuration_Descriptor);
                              break;
                        }

                        GATT_Read_Response(BluetoothStackID, GATT_ServerEventData->Event_Data.GATT_Read_Request_Data->TransactionID, WORD_SIZE, Temp);
                     }
                     else
                     {
                        Display(("Error - No device info entry for this device.\r\n"));
                     }
                  }
                  else
                  {
                     Display(("Error - No such device connected.\r\n"));
                  }
               }
               else
                  GATT_Error_Response(BluetoothStackID, GATT_ServerEventData->Event_Data.GATT_Read_Request_Data->TransactionID, GATT_ServerEventData->Event_Data.GATT_Read_Request_Data->AttributeOffset, ATT_PROTOCOL_ERROR_CODE_ATTRIBUTE_NOT_LONG);
            }
            else
               Display(("Invalid Read Request Event Data.\r\n"));
            break;
         case etGATT_Server_Write_Request:
            /* Verify that the Event Data is valid.                     */
            if(GATT_ServerEventData->Event_Data.GATT_Write_Request_Data)
            {
               if(GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValueOffset == 0)
               {
                  /* Cache the Attribute Offset.                        */
                  AttributeOffset = GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeOffset;

                  /* Verify that the value is of the correct length.    */
                  if((AttributeOffset == SPPLE_RX_CHARACTERISTIC_ATTRIBUTE_OFFSET) || ((GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValueLength) && (GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValueLength <= WORD_SIZE)))
                  {
                     /* Find the LE Connection Index for this           */
                     /* connection.                                     */
                     if((LEConnectionIndex = FindLEIndexByAddress(GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->RemoteDevice)) >= 0)
                     {
                        /* Grab the device info for the currently       */
                        /* connected device.                            */
                        if((DeviceInfo = SearchDeviceInfoEntryByBD_ADDR(&DeviceInfoList, LEContextInfo[LEConnectionIndex].ConnectionBD_ADDR)) != NULL)
                        {
                           /* Since the value appears valid go ahead and*/
                           /* accept the write request.                 */
                           GATT_Write_Response(BluetoothStackID, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->TransactionID);

                           /* If this is not a write to the Rx          */
                           /* Characteristic we will read the data here.*/
                           if(AttributeOffset != SPPLE_RX_CHARACTERISTIC_ATTRIBUTE_OFFSET)
                           {
                              if(GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValueLength == WORD_SIZE)
                                 Value = READ_UNALIGNED_WORD_LITTLE_ENDIAN(GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValue);
                              else
                                 Value = READ_UNALIGNED_BYTE_LITTLE_ENDIAN(GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValue);
                           }

                           /* Determine which attribute this write      */
                           /* request is for.                           */
                           switch(AttributeOffset)
                           {
                              case SPPLE_TX_CHARACTERISTIC_CCD_ATTRIBUTE_OFFSET:
                                 /* Client has updated the Tx CCCD.  Now*/
                                 /* we need to check if we have any data*/
                                 /* to send.                            */
                                 DeviceInfo->ServerInfo.Tx_Client_Configuration_Descriptor = Value;

                                 /* If may be possible for transmit     */
                                 /* queued data now.  So fake a Receive */
                                 /* Credit event with 0 as the received */
                                 /* credits.                            */
                                 SPPLEReceiveCreditEvent(&(LEContextInfo[LEConnectionIndex]), DeviceInfo, 0);
                                 break;
                              case SPPLE_TX_CREDITS_CHARACTERISTIC_ATTRIBUTE_OFFSET:
                                 /* Client has sent updated credits.    */
                                 SPPLEReceiveCreditEvent(&(LEContextInfo[LEConnectionIndex]), DeviceInfo, Value);
                                 break;
                              case SPPLE_RX_CHARACTERISTIC_ATTRIBUTE_OFFSET:
                                 /* Client has sent data, so we should  */
                                 /* handle this as a data indication    */
                                 /* event.                              */
                                 SPPLEDataIndicationEvent(&(LEContextInfo[LEConnectionIndex]), DeviceInfo, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValueLength, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValue);

                                 if((!AutomaticReadActive) && (!LoopbackActive))
                                    DisplayPrompt();
                                 break;
                              case SPPLE_RX_CREDITS_CHARACTERISTIC_CCD_ATTRIBUTE_OFFSET:
                                 /* Cache the previous CCD Value.       */
                                 PreviousValue = DeviceInfo->ServerInfo.Rx_Credit_Client_Configuration_Descriptor;

                                 /* Note the updated Rx CCCD Value.     */
                                 DeviceInfo->ServerInfo.Rx_Credit_Client_Configuration_Descriptor = Value;

                                 /* If we were not previously configured*/
                                 /* for notifications send the initial  */
                                 /* credits to the device.              */
                                 if(PreviousValue != GATT_CLIENT_CONFIGURATION_CHARACTERISTIC_NOTIFY_ENABLE)
                                 {
                                    /* Send the initial credits to the  */
                                    /* device.                          */
                                    SPPLESendCredits(&(LEContextInfo[LEConnectionIndex]), DeviceInfo, LEContextInfo[LEConnectionIndex].SPPLEBufferInfo.ReceiveBuffer.BytesFree);
                                 }
                                 break;
                           }
                        }
                        else
                        {
                           Display(("Error - No device info entry for this device.\r\n"));
                        }
                     }
                     else
                     {
                        Display(("Error - No such device connected."));
                     }
                  }
                  else
                     GATT_Error_Response(BluetoothStackID, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->TransactionID, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeOffset, ATT_PROTOCOL_ERROR_CODE_INVALID_ATTRIBUTE_VALUE_LENGTH);
               }
               else
                  GATT_Error_Response(BluetoothStackID, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->TransactionID, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeOffset, ATT_PROTOCOL_ERROR_CODE_ATTRIBUTE_NOT_LONG);
            }
            else
               Display(("Invalid Write Request Event Data.\r\n"));
            break;
      }
   }
   else
   {
      /* There was an error with one or more of the input parameters.   */
      Display(("\r\n"));

      Display(("GATT Callback Data: Event_Data = NULL.\r\n"));

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
static void BTPSAPI GATT_ClientEventCallback_SPPLE(unsigned int BluetoothStackID, GATT_Client_Event_Data_t *GATT_Client_Event_Data, unsigned long CallbackParameter)
{
   int           LEConnectionIndex;
   Word_t        Credits;
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
               /* Find the LE Connection Index for this connection.     */
               if((LEConnectionIndex = FindLEIndexByAddress(GATT_Client_Event_Data->Event_Data.GATT_Read_Response_Data->RemoteDevice)) >= 0)
               {
                  /* Grab the device info for the currently connected   */
                  /* device.                                            */
                  if((DeviceInfo = SearchDeviceInfoEntryByBD_ADDR(&DeviceInfoList, LEContextInfo[LEConnectionIndex].ConnectionBD_ADDR)) != NULL)
                  {
                     if((Word_t)CallbackParameter == DeviceInfo->ClientInfo.Rx_Credit_Characteristic)
                     {
                        /* Make sure this is the correct size for a Rx  */
                        /* Credit Characteristic.                       */
                        if(GATT_Client_Event_Data->Event_Data.GATT_Read_Response_Data->AttributeValueLength == WORD_SIZE)
                        {
                           /* Display the credits we just received.     */
                           Credits = READ_UNALIGNED_WORD_LITTLE_ENDIAN(GATT_Client_Event_Data->Event_Data.GATT_Read_Response_Data->AttributeValue);
                           Display(("\r\nReceived %u Initial Credits.\r\n", Credits));

                           /* We have received the initial credits from */
                           /* the device so go ahead and handle a       */
                           /* Receive Credit Event.                     */
                           SPPLEReceiveCreditEvent(&(LEContextInfo[LEConnectionIndex]), DeviceInfo, Credits);
                        }
                     }
                  }
               }
            }
            else
               Display(("\r\nError - Null Read Response Data.\r\n"));
            break;
         case etGATT_Client_Exchange_MTU_Response:
            if(GATT_Client_Event_Data->Event_Data.GATT_Exchange_MTU_Response_Data)
            {
               Display(("\r\nExchange MTU Response.\r\n"));
               BD_ADDRToStr(GATT_Client_Event_Data->Event_Data.GATT_Exchange_MTU_Response_Data->RemoteDevice, BoardStr);
               Display(("Connection ID:   %u.\r\n", GATT_Client_Event_Data->Event_Data.GATT_Exchange_MTU_Response_Data->ConnectionID));
               Display(("Transaction ID:  %u.\r\n", GATT_Client_Event_Data->Event_Data.GATT_Exchange_MTU_Response_Data->TransactionID));
               Display(("Connection Type: %s.\r\n", (GATT_Client_Event_Data->Event_Data.GATT_Exchange_MTU_Response_Data->ConnectionType == gctLE)?"LE":"BR/EDR"));
               Display(("BD_ADDR:         %s.\r\n", BoardStr));
               Display(("MTU:             %u.\r\n", GATT_Client_Event_Data->Event_Data.GATT_Exchange_MTU_Response_Data->ServerMTU));
            }
            else
               Display(("\r\nError - Null Write Response Data.\r\n"));
            break;
         case etGATT_Client_Write_Response:
            if(GATT_Client_Event_Data->Event_Data.GATT_Write_Response_Data)
            {
               Display(("\r\nWrite Response.\r\n"));
               BD_ADDRToStr(GATT_Client_Event_Data->Event_Data.GATT_Write_Response_Data->RemoteDevice, BoardStr);
               Display(("Connection ID:   %u.\r\n", GATT_Client_Event_Data->Event_Data.GATT_Write_Response_Data->ConnectionID));
               Display(("Transaction ID:  %u.\r\n", GATT_Client_Event_Data->Event_Data.GATT_Write_Response_Data->TransactionID));
               Display(("Connection Type: %s.\r\n", (GATT_Client_Event_Data->Event_Data.GATT_Write_Response_Data->ConnectionType == gctLE)?"LE":"BR/EDR"));
               Display(("BD_ADDR:         %s.\r\n", BoardStr));
               Display(("Bytes Written:   %u.\r\n", GATT_Client_Event_Data->Event_Data.GATT_Write_Response_Data->BytesWritten));
            }
            else
               Display(("\r\nError - Null Write Response Data.\r\n"));
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
               if((DeviceInfo = SearchDeviceInfoEntryByBD_ADDR(&DeviceInfoList, GATT_Client_Event_Data->Event_Data.GATT_Read_Response_Data->RemoteDevice)) != NULL)
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
   int           LEConnectionIndex;
   Word_t        Credits;
   Boolean_t     SuppressResponse = FALSE;
   BoardStr_t    BoardStr;
   DeviceInfo_t *DeviceInfo;

   /* Verify that all parameters to this callback are Semi-Valid.       */
   if((BluetoothStackID) && (GATT_Connection_Event_Data))
   {
      /* Determine the Connection Event that occurred.                  */
      switch(GATT_Connection_Event_Data->Event_Data_Type)
      {
         case etGATT_Connection_Device_Connection:
            if(GATT_Connection_Event_Data->Event_Data.GATT_Device_Connection_Data)
            {
               /* Update the ConnectionID associated with the BD_ADDR   */
               /* If UpdateConnectionID returns -1, then it failed.     */
               if(UpdateConnectionID(GATT_Connection_Event_Data->Event_Data.GATT_Device_Connection_Data->ConnectionID, GATT_Connection_Event_Data->Event_Data.GATT_Device_Connection_Data->RemoteDevice) < 0)
                   Display(("Error - No matching ConnectionBD_ADDR found."));

               Display(("\r\netGATT_Connection_Device_Connection with size %u: \r\n", GATT_Connection_Event_Data->Event_Data_Size));
               BD_ADDRToStr(GATT_Connection_Event_Data->Event_Data.GATT_Device_Connection_Data->RemoteDevice, BoardStr);
               Display(("   Connection ID:   %u.\r\n", GATT_Connection_Event_Data->Event_Data.GATT_Device_Connection_Data->ConnectionID));
               Display(("   Connection Type: %s.\r\n", ((GATT_Connection_Event_Data->Event_Data.GATT_Device_Connection_Data->ConnectionType == gctLE)?"LE":"BR/EDR")));
               Display(("   Remote Device:   %s.\r\n", BoardStr));
               Display(("   Connection MTU:  %u.\r\n", GATT_Connection_Event_Data->Event_Data.GATT_Device_Connection_Data->MTU));

               /* Find the LE Connection Index for this connection.     */
               if((LEConnectionIndex = FindLEIndexByAddress(GATT_Connection_Event_Data->Event_Data.GATT_Device_Connection_Data->RemoteDevice)) >= 0)
               {
                  /* Search for the device info for the connection.     */
                  if((DeviceInfo = SearchDeviceInfoEntryByBD_ADDR(&DeviceInfoList, LEContextInfo[LEConnectionIndex].ConnectionBD_ADDR)) != NULL)
                  {
                     /* Clear the SPPLE Role Flag.                      */
                     DeviceInfo->Flags &= ~DEVICE_INFO_FLAGS_SPPLE_SERVER;

                     /* Initialize the Transmit and Receive Buffers.    */
                     InitializeBuffer(&(LEContextInfo[LEConnectionIndex].SPPLEBufferInfo.ReceiveBuffer));
                     InitializeBuffer(&(LEContextInfo[LEConnectionIndex].SPPLEBufferInfo.TransmitBuffer));

                     /* Flag that we do not have any transmit credits   */
                     /* yet.                                            */
                     LEContextInfo[LEConnectionIndex].SPPLEBufferInfo.TransmitCredits = 0;

                     /* Flag that no credits are queued.                */
                     LEContextInfo[LEConnectionIndex].SPPLEBufferInfo.QueuedCredits   = 0;

                     if(!LocalDeviceIsMaster)
                     {
                        /* Flag that we will act as the Server.         */
                        DeviceInfo->Flags |= DEVICE_INFO_FLAGS_SPPLE_SERVER;

                        /* Send the Initial Credits if the Rx Credit CCD*/
                        /* is already configured (for a bonded device   */
                        /* this could be the case).                     */
                        SPPLESendCredits(&(LEContextInfo[LEConnectionIndex]), DeviceInfo, LEContextInfo[LEConnectionIndex].SPPLEBufferInfo.ReceiveBuffer.BytesFree);
                     }
                     else
                     {
                        /* Attempt to update the MTU to the maximum     */
                        /* supported.                                   */
                        GATT_Exchange_MTU_Request(BluetoothStackID, GATT_Connection_Event_Data->Event_Data.GATT_Device_Connection_Data->ConnectionID, BTPS_CONFIGURATION_GATT_MAXIMUM_SUPPORTED_MTU_SIZE, GATT_ClientEventCallback_SPPLE, 0);
                     }
                  }
               }
            }
            else
               Display(("Error - Null Connection Data.\r\n"));
            break;
         case etGATT_Connection_Device_Disconnection:
            if(GATT_Connection_Event_Data->Event_Data.GATT_Device_Disconnection_Data)
            {
               /* Clear the Connection ID.                              */
               RemoveConnectionInfo(GATT_Connection_Event_Data->Event_Data.GATT_Device_Disconnection_Data->RemoteDevice);

               Display(("\r\netGATT_Connection_Device_Disconnection with size %u: \r\n", GATT_Connection_Event_Data->Event_Data_Size));
               BD_ADDRToStr(GATT_Connection_Event_Data->Event_Data.GATT_Device_Disconnection_Data->RemoteDevice, BoardStr);
               Display(("   Connection ID:   %u.\r\n", GATT_Connection_Event_Data->Event_Data.GATT_Device_Disconnection_Data->ConnectionID));
               Display(("   Connection Type: %s.\r\n", ((GATT_Connection_Event_Data->Event_Data.GATT_Device_Disconnection_Data->ConnectionType == gctLE)?"LE":"BR/EDR")));
               Display(("   Remote Device:   %s.\r\n", BoardStr));
            }
            else
               Display(("Error - Null Disconnection Data.\r\n"));
            break;
         case etGATT_Connection_Device_Buffer_Empty:
            if(GATT_Connection_Event_Data->Event_Data.GATT_Device_Buffer_Empty_Data)
            {
               /* Find the LE Connection Index for this connection.     */
               if((LEConnectionIndex = FindLEIndexByAddress(GATT_Connection_Event_Data->Event_Data.GATT_Device_Buffer_Empty_Data->RemoteDevice)) >= 0)
               {
                  /* Grab the device info for the currently connected   */
                  /* device.                                            */
                  if((DeviceInfo = SearchDeviceInfoEntryByBD_ADDR(&DeviceInfoList, LEContextInfo[LEConnectionIndex].ConnectionBD_ADDR)) != NULL)
                  {
                     /* Flag that the buffer is no longer empty.        */
                     LEContextInfo[LEConnectionIndex].BufferFull = FALSE;

                     /* Attempt to send any queued credits that we may  */
                     /* have.                                           */
                     SPPLESendCredits(&(LEContextInfo[LEConnectionIndex]), DeviceInfo, 0);

                     /* If may be possible for transmit queued data now.*/
                     /* So fake a Receive Credit event with 0 as the    */
                     /* received credits.                               */
                     SPPLEReceiveCreditEvent(&(LEContextInfo[LEConnectionIndex]), DeviceInfo, 0);

                     /* Suppress the command prompt.                    */
                     SuppressResponse   = TRUE;
                  }
               }
            }
            break;
         case etGATT_Connection_Server_Notification:
            if(GATT_Connection_Event_Data->Event_Data.GATT_Server_Notification_Data)
            {
               /* Find the LE Connection Index for this connection.     */
               if((LEConnectionIndex = FindLEIndexByAddress(GATT_Connection_Event_Data->Event_Data.GATT_Server_Notification_Data->RemoteDevice)) >= 0)
               {
                  /* Find the Device Info for the device that has sent  */
                  /* us the notification.                               */
                  if((DeviceInfo = SearchDeviceInfoEntryByBD_ADDR(&DeviceInfoList, LEContextInfo[LEConnectionIndex].ConnectionBD_ADDR)) != NULL)
                  {
                     /* Determine the characteristic that is being      */
                     /* notified.                                       */
                     if(GATT_Connection_Event_Data->Event_Data.GATT_Server_Notification_Data->AttributeHandle == DeviceInfo->ClientInfo.Rx_Credit_Characteristic)
                     {
                        /* Verify that the length of the Rx Credit      */
                        /* Notification is correct.                     */
                        if(GATT_Connection_Event_Data->Event_Data.GATT_Server_Notification_Data->AttributeValueLength == WORD_SIZE)
                        {
                           Credits = READ_UNALIGNED_WORD_LITTLE_ENDIAN(GATT_Connection_Event_Data->Event_Data.GATT_Server_Notification_Data->AttributeValue);

                           /* Handle the received credits event.        */
                           SPPLEReceiveCreditEvent(&(LEContextInfo[LEConnectionIndex]), DeviceInfo, Credits);

                           /* Suppress the command prompt.              */
                           SuppressResponse   = TRUE;
                        }
                     }
                     else
                     {
                        if(GATT_Connection_Event_Data->Event_Data.GATT_Server_Notification_Data->AttributeHandle == DeviceInfo->ClientInfo.Tx_Characteristic)
                        {
                           /* This is a Tx Characteristic Event.  So    */
                           /* call the function to handle the data      */
                           /* indication event.                         */
                           SPPLEDataIndicationEvent(&(LEContextInfo[LEConnectionIndex]), DeviceInfo, GATT_Connection_Event_Data->Event_Data.GATT_Server_Notification_Data->AttributeValueLength, GATT_Connection_Event_Data->Event_Data.GATT_Server_Notification_Data->AttributeValue);

                           /* If we are not looping back or doing       */
                           /* automatic reads we will display the       */
                           /* prompt.                                   */
                           if((!AutomaticReadActive) && (!LoopbackActive))
                              SuppressResponse = FALSE;
                           else
                              SuppressResponse = TRUE;
                        }
                     }
                  }
               }
            }
            else
               Display(("Error - Null Server Notification Data.\r\n"));
            break;
      }

      /* Print the command line prompt.                                 */
      if(!SuppressResponse)
         DisplayPrompt();
   }
   else
   {
      /* There was an error with one or more of the input parameters.   */
      Display(("\r\n"));

      Display(("GATT Connection Callback Data: Event_Data = NULL.\r\n"));

      DisplayPrompt();
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
   int           LEConnectionIndex;
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
               /* Find the LE Connection Index for this connection.     */
               if((LEConnectionIndex = FindLEIndexByConnectionID(GATT_Service_Discovery_Event_Data->Event_Data.GATT_Service_Discovery_Indication_Data->ConnectionID)) >= 0)
               {
                  /* Find the device info for this connection.          */
                  if((DeviceInfo = SearchDeviceInfoEntryByBD_ADDR(&DeviceInfoList, LEContextInfo[LEConnectionIndex].ConnectionBD_ADDR)) != NULL)
                  {
                     Display(("\r\n"));
                     Display(("Service 0x%04X - 0x%04X, UUID: ", GATT_Service_Discovery_Event_Data->Event_Data.GATT_Service_Discovery_Indication_Data->ServiceInformation.Service_Handle, GATT_Service_Discovery_Event_Data->Event_Data.GATT_Service_Discovery_Indication_Data->ServiceInformation.End_Group_Handle));
                     DisplayUUID(&(GATT_Service_Discovery_Event_Data->Event_Data.GATT_Service_Discovery_Indication_Data->ServiceInformation.UUID));
                     Display(("\r\n"));

                     /* Attempt to populate the handles for the GAP     */
                     /* Service.                                        */
                     GAPSPopulateHandles(&(DeviceInfo->GAPSClientInfo), GATT_Service_Discovery_Event_Data->Event_Data.GATT_Service_Discovery_Indication_Data);

                     /* Attempt to populate the handles for the SPPLE   */
                     /* Service.                                        */
                     SPPLEPopulateHandles(&(DeviceInfo->ClientInfo), GATT_Service_Discovery_Event_Data->Event_Data.GATT_Service_Discovery_Indication_Data);
                  }
               }
            }
            break;
         case etGATT_Service_Discovery_Complete:
            /* Verify the event data.                                   */
            if(GATT_Service_Discovery_Event_Data->Event_Data.GATT_Service_Discovery_Complete_Data)
            {
               Display(("\r\n"));
               Display(("Service Discovery Operation Complete, Status 0x%02X.\r\n", GATT_Service_Discovery_Event_Data->Event_Data.GATT_Service_Discovery_Complete_Data->Status));

               /* Find the LE Connection Index for this connection.     */
               if((LEConnectionIndex = FindLEIndexByConnectionID(GATT_Service_Discovery_Event_Data->Event_Data.GATT_Service_Discovery_Complete_Data->ConnectionID)) >= 0)
               {
                  /* Find the device info for this connection.          */
                  if((DeviceInfo = SearchDeviceInfoEntryByBD_ADDR(&DeviceInfoList, LEContextInfo[LEConnectionIndex].ConnectionBD_ADDR)) != NULL)
                  {
                     /* Flag that no service discovery operation is     */
                     /* outstanding for this device.                    */
                     DeviceInfo->Flags &= ~DEVICE_INFO_FLAGS_SERVICE_DISCOVERY_OUTSTANDING;
                  }
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
   BD_ADDR_t                         NULL_BD_ADDR;
   Boolean_t                         OOB_Data;
   Boolean_t                         MITM;
   BoardStr_t                        Callback_BoardStr;
   GAP_IO_Capability_t               RemoteIOCapability;
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
                  CurrentCBRemoteBD_ADDR = GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Remote_Device;

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
                  ASSIGN_BD_ADDR(CurrentCBRemoteBD_ADDR, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
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

                  CurrentCBRemoteBD_ADDR = GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Remote_Device;

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
                     ASSIGN_BD_ADDR(CurrentCBRemoteBD_ADDR, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
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
                  CurrentCBRemoteBD_ADDR = GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Remote_Device;

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

                  Display(("Passkey Value: %u\r\n", GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Authentication_Event_Data.Numeric_Value));
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

   /* The following function is for an SPP Event Callback.  This        */
   /* function will be called whenever a SPP Event occurs that is       */
   /* associated with the Bluetooth Stack.  This function passes to the */
   /* caller the SPP Event Data that occurred and the SPP Event Callback*/
   /* Parameter that was specified when this Callback was installed.    */
   /* The caller is free to use the contents of the SPP SPP Event Data  */
   /* ONLY in the context of this callback.  If the caller requires the */
   /* Data for a longer period of time, then the callback function MUST */
   /* copy the data into another Data Buffer.  This function is         */
   /* guaranteed NOT to be invoked more than once simultaneously for the*/
   /* specified installed callback (i.e.  this function DOES NOT have be*/
   /* reentrant).  It Needs to be noted however, that if the same       */
   /* Callback is installed more than once, then the callbacks will be  */
   /* called serially.  Because of this, the processing in this function*/
   /* should be as efficient as possible.  It should also be noted that */
   /* this function is called in the Thread Context of a Thread that the*/
   /* User does NOT own.  Therefore, processing in this function should */
   /* be as efficient as possible (this argument holds anyway because   */
   /* another SPP Event will not be processed while this function call  */
   /* is outstanding).                                                  */
   /* * NOTE * This function MUST NOT Block and wait for Events that    */
   /*          can only be satisfied by Receiving SPP Event Packets.  A */
   /*          Deadlock WILL occur because NO SPP Event Callbacks will  */
   /*          be issued while this function is currently outstanding.  */
static void BTPSAPI SPP_Event_Callback(unsigned int BluetoothStackID, SPP_Event_Data_t *SPP_Event_Data, unsigned long CallbackParameter)
{
   int          SerialPortIndex;
   int          ret_val = 0;
   int          Index;
   int          Index1;
   int          TempLength;

#if ((SPP_PERFORM_MASTER_ROLE_SWITCH) && (MAX_SIMULTANEOUS_SPP_PORTS > 1))
   Byte_t       StatusResult;
   Word_t       Connection_HandleResult;
   Byte_t       Current_Role;
#endif

   Word_t       ConnectionHandle;
   Boolean_t    _DisplayPrompt = TRUE;
   Boolean_t    Done;
   BoardStr_t   Callback_BoardStr;
   unsigned int LocalSerialPortID;

   /* **** SEE SPPAPI.H for a list of all possible event types.  This   */
   /* program only services its required events.                   **** */

   /* First, check to see if the required parameters appear to be       */
   /* semi-valid.                                                       */
   if((SPP_Event_Data) && (BluetoothStackID))
   {
      /* The parameters appear to be semi-valid, now check to see what  */
      /* type the incoming event is.                                    */
      switch(SPP_Event_Data->Event_Data_Type)
      {
         case etPort_Open_Indication:
            /* A remote port is requesting a connection.                */
            BD_ADDRToStr(SPP_Event_Data->Event_Data.SPP_Open_Port_Indication_Data->BD_ADDR, Callback_BoardStr);

            Display(("\r\n"));
            Display(("SPP Open Indication, ID: 0x%04X, Board: %s.\r\n", SPP_Event_Data->Event_Data.SPP_Open_Port_Indication_Data->SerialPortID, Callback_BoardStr));

            /* Find the index of the SPP Port Information.              */
            if((SerialPortIndex = FindSPPPortIndex(SPP_Event_Data->Event_Data.SPP_Open_Port_Indication_Data->SerialPortID)) >= 0)
            {
               /* Flag that we are connected to the device.             */
               SPPContextInfo[SerialPortIndex].Connected = TRUE;
               SPPContextInfo[SerialPortIndex].BD_ADDR   = SPP_Event_Data->Event_Data.SPP_Open_Port_Indication_Data->BD_ADDR;

               /* Query the connection handle.                          */
               ret_val = GAP_Query_Connection_Handle(BluetoothStackID, SPPContextInfo[SerialPortIndex].BD_ADDR, &ConnectionHandle);
               if(ret_val)
               {
                  /* Failed to Query the Connection Handle.             */
                  DisplayFunctionError("GAP_Query_Connection_Handle()",ret_val);
               }
               else
               {
                  /* Save the connection handle of this connection.     */
                  SPPContextInfo[SerialPortIndex].Connection_Handle = ConnectionHandle;

#if ((SPP_PERFORM_MASTER_ROLE_SWITCH) && (MAX_SIMULTANEOUS_SPP_PORTS > 1))

                  /* First determine the current role to determine if we*/
                  /* are already the master.                            */
                  StatusResult = 0;
                  ret_val = HCI_Role_Discovery(BluetoothStackID, SPPContextInfo[SerialPortIndex].Connection_Handle, &StatusResult, &Connection_HandleResult, &Current_Role);
                  if((ret_val == 0) && (StatusResult == HCI_ERROR_CODE_NO_ERROR))
                  {
                     /* Check to see if we aren't currently the master. */
                     if(Current_Role != HCI_CURRENT_ROLE_MASTER)
                     {
                        /* Attempt to switch to the master role.        */
                        StatusResult = 0;
                        ret_val = HCI_Switch_Role(BluetoothStackID, SPPContextInfo[SerialPortIndex].BD_ADDR, HCI_ROLE_SWITCH_BECOME_MASTER, &StatusResult);
                        if((ret_val == 0) && (StatusResult == HCI_ERROR_CODE_NO_ERROR))
                        {
                            Display(("\r\nInitiating Role Switch.\r\n"));
                        }
                        else
                        {
                            Display(("HCI Switch Role failed. %d: 0x%02X", ret_val, StatusResult));
                        }
                     }
                  }
                  else
                  {
                      Display(("HCI Role Discovery failed. %d: 0x%02X", ret_val, StatusResult));
                  }

#endif

               }
            }
            break;
         case etPort_Open_Confirmation:
            /* A Client Port was opened.  The Status indicates the      */
            /* Status of the Open.                                      */
            Display(("\r\n"));
            Display(("SPP Open Confirmation, ID: 0x%04X, Status 0x%04X.\r\n", SPP_Event_Data->Event_Data.SPP_Open_Port_Confirmation_Data->SerialPortID,
                                                                              SPP_Event_Data->Event_Data.SPP_Open_Port_Confirmation_Data->PortOpenStatus));


            /* Find the index of the SPP Port Information.              */
            if((SerialPortIndex = FindSPPPortIndex(SPP_Event_Data->Event_Data.SPP_Open_Port_Confirmation_Data->SerialPortID)) >= 0)
            {
               /* Check the Status to make sure that an error did not   */
               /* occur.                                                */
               if(SPP_Event_Data->Event_Data.SPP_Open_Port_Confirmation_Data->PortOpenStatus)
               {
                  /* An error occurred while opening the Serial Port so */
                  /* invalidate the Serial Port ID.                     */
                  BTPS_MemInitialize(&SPPContextInfo[SerialPortIndex], 0, sizeof(SPPContextInfo[SerialPortIndex]));
               }
               else
               {
                  /* Flag that we are connected to the device.          */
                  SPPContextInfo[SerialPortIndex].Connected = TRUE;

                  /* Query the connection Handle.                       */
                  ret_val = GAP_Query_Connection_Handle(BluetoothStackID, SPPContextInfo[SerialPortIndex].BD_ADDR, &ConnectionHandle);
                  if(ret_val)
                  {
                     /* Failed to Query the Connection Handle.          */
                     DisplayFunctionError("GAP_Query_Connection_Handle()",ret_val);
                  }
                  else
                  {
                     /* Save the connection handle of this connection.  */
                     SPPContextInfo[SerialPortIndex].Connection_Handle = ConnectionHandle;

#if ((SPP_PERFORM_MASTER_ROLE_SWITCH) && (MAX_SIMULTANEOUS_SPP_PORTS > 1))

                     /* First determine the current role to determine if*/
                     /* we are already the master.                      */
                     StatusResult = 0;
                     ret_val = HCI_Role_Discovery(BluetoothStackID, SPPContextInfo[SerialPortIndex].Connection_Handle, &StatusResult, &Connection_HandleResult, &Current_Role);
                     if((ret_val == 0) && (StatusResult == HCI_ERROR_CODE_NO_ERROR))
                     {
                        /* Check to see if we aren't currently the      */
                        /* master.                                      */
                        if(Current_Role != HCI_CURRENT_ROLE_MASTER)
                        {
                           /* Attempt to switch to the master role.     */
                           StatusResult = 0;
                           ret_val = HCI_Switch_Role(BluetoothStackID, SPPContextInfo[SerialPortIndex].BD_ADDR, HCI_ROLE_SWITCH_BECOME_MASTER, &StatusResult);
                           if((ret_val == 0) && (StatusResult == HCI_ERROR_CODE_NO_ERROR))
                           {
                               Display(("\r\nInitiating Role Switch.\r\n"));
                           }
                           else
                           {
                               Display(("HCI Switch Role failed. %d: 0x%02X", ret_val, StatusResult));
                           }
                        }
                     }
                     else
                     {
                         Display(("HCI Role Discovery failed. %d: 0x%02X", ret_val, StatusResult));
                     }

#endif

                  }
               }
            }
            break;
         case etPort_Close_Port_Indication:
            /* The Remote Port was Disconnected.                        */
            LocalSerialPortID = SPP_Event_Data->Event_Data.SPP_Close_Port_Indication_Data->SerialPortID;

            Display(("\r\n"));
            Display(("SPP Close Port, ID: 0x%04X\r\n", LocalSerialPortID));

            /* Find the port index of the SPP Port that just closed.    */
            if((SerialPortIndex = FindSPPPortIndex(LocalSerialPortID)) >= 0)
            {
               ASSIGN_BD_ADDR(SPPContextInfo[SerialPortIndex].BD_ADDR, 0, 0, 0, 0, 0, 0);
               SPPContextInfo[SerialPortIndex].SendInfo.BytesToSend = 0;
               SPPContextInfo[SerialPortIndex].Connected            = FALSE;

               /* If this is a client port we also need to clear the    */
               /* Serial Port ID since it is no longer in use.          */
               if(SPPContextInfo[SerialPortIndex].ServerPortNumber == 0)
                  SPPContextInfo[SerialPortIndex].LocalSerialPortID = 0;
            }
            break;
         case etPort_Status_Indication:
            /* Display Information about the new Port Status.           */
            Display(("\r\n"));
            Display(("SPP Port Status Indication: 0x%04X, Status: 0x%04X, Break Status: 0x%04X, Length: 0x%04X.\r\n", SPP_Event_Data->Event_Data.SPP_Port_Status_Indication_Data->SerialPortID,
                                                                                                                    SPP_Event_Data->Event_Data.SPP_Port_Status_Indication_Data->PortStatus,
                                                                                                                    SPP_Event_Data->Event_Data.SPP_Port_Status_Indication_Data->BreakStatus,
                                                                                                                    SPP_Event_Data->Event_Data.SPP_Port_Status_Indication_Data->BreakTimeout));

            break;
         case etPort_Data_Indication:
            /* Data was received.  Process it differently based upon the*/
            /* current state of the Loopback Mode.                      */
            LocalSerialPortID = SPP_Event_Data->Event_Data.SPP_Data_Indication_Data->SerialPortID;

            /* Find the port index of the correct SPP Port.             */
            if((SerialPortIndex = FindSPPPortIndex(LocalSerialPortID)) >= 0)
            {

#if MAXIMUM_SPP_LOOPBACK_BUFFER_SIZE > 0

               /* Determine what data mode we are in.                   */
               if(LoopbackActive)
               {
                  /* Initialize Done to false.                          */
                  Done = FALSE;

                  /* Loop until the write buffer is full or there is not*/
                  /* more data to read.                                 */
                  while((Done == FALSE) && (SPPContextInfo[SerialPortIndex].SendInfo.BufferFull == FALSE))
                  {
                     /* The application state is currently in the loop  */
                     /* back state.  Read as much data as we can read.  */
                     if((TempLength = SPP_Data_Read(BluetoothStackID, LocalSerialPortID, (Word_t)sizeof(SPPContextInfo[SerialPortIndex].Buffer), (Byte_t *)SPPContextInfo[SerialPortIndex].Buffer)) > 0)
                     {
                        /* Adjust the Current Buffer Length by the      */
                        /* number of bytes which were successfully read.*/
                        SPPContextInfo[SerialPortIndex].BufferLength = TempLength;

                        /* Next attempt to write all of the data which  */
                        /* is currently in the buffer.                  */
                        if((TempLength = SPP_Data_Write(BluetoothStackID, LocalSerialPortID, (Word_t)SPPContextInfo[SerialPortIndex].BufferLength, (Byte_t *)SPPContextInfo[SerialPortIndex].Buffer)) < (int)SPPContextInfo[SerialPortIndex].BufferLength)
                        {
                           /* Not all of the data was successfully      */
                           /* written or an error occurred, first check */
                           /* to see if an error occurred.              */
                           if(TempLength >= 0)
                           {
                              /* An error did not occur therefore the   */
                              /* Transmit Buffer must be full.  Adjust  */
                              /* the Buffer and Buffer Length by the    */
                              /* amount which as successfully written.  */
                              if(TempLength)
                              {
                                 for(Index=0,Index1=TempLength;Index1<SPPContextInfo[SerialPortIndex].BufferLength;Index++,Index1++)
                                    SPPContextInfo[SerialPortIndex].Buffer[Index] = SPPContextInfo[SerialPortIndex].Buffer[Index1];

                                 SPPContextInfo[SerialPortIndex].BufferLength -= TempLength;
                              }

                              /* Set the flag indicating that the SPP   */
                              /* Write Buffer is full.                  */
                              SPPContextInfo[SerialPortIndex].SendInfo.BufferFull = TRUE;
                           }
                           else
                              Done = TRUE;
                        }
                     }
                     else
                        Done = TRUE;
                  }

                  _DisplayPrompt = FALSE;
               }
               else
               {
                  /* If we are operating in Raw Data Display Mode then  */
                  /* simply display the data that was give to use.      */
                  if((DisplayRawData) || (AutomaticReadActive))
                  {
                     /* Initialize Done to false.                       */
                     Done = FALSE;

                     /* Loop through and read all data that is present  */
                     /* in the buffer.                                  */
                     while(!Done)
                     {

                        /* Read as much data as possible.               */
                        if((TempLength = SPP_Data_Read(BluetoothStackID, LocalSerialPortID, (Word_t)sizeof(SPPContextInfo[SerialPortIndex].Buffer)-1, (Byte_t *)SPPContextInfo[SerialPortIndex].Buffer)) > 0)
                        {
                           /* Now simply display each character that we */
                           /* have just read.                           */
                           if(DisplayRawData)
                           {
                              SPPContextInfo[SerialPortIndex].Buffer[TempLength] = '\0';

                              Display(((char *)SPPContextInfo[SerialPortIndex].Buffer));
                           }
                        }
                        else
                        {
                           /* Either an error occurred or there is no   */
                           /* more data to be read.                     */
                           if(TempLength < 0)
                           {
                              /* Error occurred.                        */
                              Display(("SPP_Data_Read(): Error %d.\r\n", TempLength));
                           }

                           /* Regardless if an error occurred, we are   */
                           /* finished with the current loop.           */
                           Done = TRUE;
                        }
                     }

                     _DisplayPrompt = FALSE;
                  }
                  else
                  {
                     /* Simply inform the user that data has arrived.   */
                     Display(("\r\n"));
                     Display(("SPP Data Indication, ID: 0x%04X, Length: 0x%04X.\r\n", SPP_Event_Data->Event_Data.SPP_Data_Indication_Data->SerialPortID,
                                                                                      SPP_Event_Data->Event_Data.SPP_Data_Indication_Data->DataLength));
                  }
               }

#else

               /* Simply inform the user that data has arrived.         */
               Display(("\r\n"));
               Display(("SPP Data Indication, ID: 0x%04X, Length: 0x%04X.\r\n", SPP_Event_Data->Event_Data.SPP_Data_Indication_Data->SerialPortID,
                                                                                SPP_Event_Data->Event_Data.SPP_Data_Indication_Data->DataLength));

#endif

            }
            break;
         case etPort_Send_Port_Information_Indication:
            /* Simply Respond with the information that was sent to us. */
            ret_val = SPP_Respond_Port_Information(BluetoothStackID, SPP_Event_Data->Event_Data.SPP_Send_Port_Information_Indication_Data->SerialPortID, &SPP_Event_Data->Event_Data.SPP_Send_Port_Information_Indication_Data->SPPPortInformation);
            break;
         case etPort_Transmit_Buffer_Empty_Indication:
            /* Locate the serial port for the SPP Port that now has a   */
            /* transmit buffer empty.                                   */
            LocalSerialPortID = SPP_Event_Data->Event_Data.SPP_Transmit_Buffer_Empty_Indication_Data->SerialPortID;

            /* Attempt to find the index of the SPP Port entry.         */
            if((SerialPortIndex = FindSPPPortIndex(LocalSerialPortID)) >= 0)
            {
               /* Flag that this buffer is no longer full.              */
               SPPContextInfo[SerialPortIndex].SendInfo.BufferFull = FALSE;

               /* The transmit buffer is now empty after being full.    */
               /* Next check the current application state.             */
               if(SPPContextInfo[SerialPortIndex].SendInfo.BytesToSend)
               {
                  /* Send the remainder of the last attempt.            */
                  TempLength                    = (DataStrLen-SPPContextInfo[SerialPortIndex].SendInfo.BytesSent);

                  SPPContextInfo[SerialPortIndex].SendInfo.BytesSent    = SPP_Data_Write(BluetoothStackID, LocalSerialPortID, TempLength, (unsigned char *)&(DataStr[SPPContextInfo[SerialPortIndex].SendInfo.BytesSent]));
                  if((int)(SPPContextInfo[SerialPortIndex].SendInfo.BytesSent) >= 0)
                  {
                     if(SPPContextInfo[SerialPortIndex].SendInfo.BytesSent <= SPPContextInfo[SerialPortIndex].SendInfo.BytesToSend)
                        SPPContextInfo[SerialPortIndex].SendInfo.BytesToSend -= SPPContextInfo[SerialPortIndex].SendInfo.BytesSent;
                     else
                        SPPContextInfo[SerialPortIndex].SendInfo.BytesToSend  = 0;

                     while(SPPContextInfo[SerialPortIndex].SendInfo.BytesToSend)
                     {
                        /* Set the Number of bytes to send in the next  */
                        /* packet.                                      */
                        if(SPPContextInfo[SerialPortIndex].SendInfo.BytesToSend > DataStrLen)
                           TempLength = DataStrLen;
                        else
                           TempLength = SPPContextInfo[SerialPortIndex].SendInfo.BytesToSend;

                        SPPContextInfo[SerialPortIndex].SendInfo.BytesSent = SPP_Data_Write(BluetoothStackID, LocalSerialPortID, TempLength, (unsigned char *)DataStr);
                        if((int)(SPPContextInfo[SerialPortIndex].SendInfo.BytesSent) >= 0)
                        {
                           SPPContextInfo[SerialPortIndex].SendInfo.BytesToSend -= SPPContextInfo[SerialPortIndex].SendInfo.BytesSent;
                           if(SPPContextInfo[SerialPortIndex].SendInfo.BytesSent < TempLength)
                              break;
                        }
                        else
                        {
                           Display(("SPP_Data_Write returned %d.\r\n", (int)SPPContextInfo[SerialPortIndex].SendInfo.BytesSent));

                           SPPContextInfo[SerialPortIndex].SendInfo.BytesToSend = 0;
                        }
                     }
                  }
                  else
                  {
                     Display(("SPP_Data_Write returned %d.\r\n", (int)SPPContextInfo[SerialPortIndex].SendInfo.BytesSent));

                     SPPContextInfo[SerialPortIndex].SendInfo.BytesToSend = 0;
                  }
               }
               else
               {

#if MAXIMUM_SPP_LOOPBACK_BUFFER_SIZE > 0

                  if(LoopbackActive)
                  {
                     /* Initialize Done to false.                       */
                     Done = FALSE;

                     /* Loop until the write buffer is full or there is */
                     /* not more data to read.                          */
                     while(Done == FALSE)
                     {
                        /* The application state is currently in the    */
                        /* loop back state.  Read as much data as we can*/
                        /* read.                                        */
                        if(((TempLength = SPP_Data_Read(BluetoothStackID, LocalSerialPortID, (Word_t)(sizeof(SPPContextInfo[SerialPortIndex].Buffer)-SPPContextInfo[SerialPortIndex].BufferLength), (Byte_t *)&(SPPContextInfo[SerialPortIndex].Buffer[SPPContextInfo[SerialPortIndex].BufferLength]))) > 0) || (SPPContextInfo[SerialPortIndex].BufferLength > 0))
                        {
                           /* Adjust the Current Buffer Length by the   */
                           /* number of bytes which were successfully   */
                           /* read.                                     */
                           if(TempLength > 0)
                              SPPContextInfo[SerialPortIndex].BufferLength += TempLength;

                           /* Next attempt to write all of the data     */
                           /* which is currently in the buffer.         */
                           if((TempLength = SPP_Data_Write(BluetoothStackID, LocalSerialPortID, (Word_t)SPPContextInfo[SerialPortIndex].BufferLength, (Byte_t *)SPPContextInfo[SerialPortIndex].Buffer)) < (int)SPPContextInfo[SerialPortIndex].BufferLength)
                           {
                              /* Not all of the data was successfully   */
                              /* written or an error occurred, first    */
                              /* check to see if an error occurred.     */
                              if(TempLength >= 0)
                              {
                                 /* An error did not occur therefore the*/
                                 /* Transmit Buffer must be full.       */
                                 /* Adjust the Buffer and Buffer Length */
                                 /* by the amount which was successfully*/
                                 /* written.                            */
                                 if(TempLength)
                                 {
                                    for(Index=0,Index1=TempLength;Index1<SPPContextInfo[SerialPortIndex].BufferLength;Index++,Index1++)
                                       SPPContextInfo[SerialPortIndex].Buffer[Index] = SPPContextInfo[SerialPortIndex].Buffer[Index1];

                                    SPPContextInfo[SerialPortIndex].BufferLength -= TempLength;
                                 }
                                 else
                                    Done = TRUE;

                                 /* Set the flag indicating that the SPP*/
                                 /* Write Buffer is full.               */
                                 SPPContextInfo[SerialPortIndex].SendInfo.BufferFull = TRUE;
                              }
                              else
                                 Done = TRUE;
                           }
                           else
                           {
                              SPPContextInfo[SerialPortIndex].BufferLength          = 0;

                              SPPContextInfo[SerialPortIndex].SendInfo.BufferFull   = FALSE;
                           }
                        }
                        else
                           Done = TRUE;
                     }
                  }
                  else
                  {
                     /* Only print the event indication to the user if  */
                     /* we are NOT operating in Raw Data Display Mode.  */
                     if(!DisplayRawData)
                     {
                        Display(("\r\nTransmit Buffer Empty Indication, ID: 0x%04X\r\n", SPP_Event_Data->Event_Data.SPP_Transmit_Buffer_Empty_Indication_Data->SerialPortID));
                     }
                  }

#else

                  Display(("\r\nTransmit Buffer Empty Indication, ID: 0x%04X\r\n", SPP_Event_Data->Event_Data.SPP_Transmit_Buffer_Empty_Indication_Data->SerialPortID));

#endif

               }
            }
            else
            {
               Display(("Could not find SPP server Context after Buffer Empty Indication.\r\n"));
            }

            _DisplayPrompt = FALSE;
            break;
         default:
            /* An unknown/unexpected SPP event was received.            */
            Display(("\r\n"));
            Display(("Unknown Event.\r\n"));
            break;
      }

      /* Check the return value of any function that might have been    */
      /* executed in the callback.                                      */
      if(ret_val)
      {
         /* An error occurred, so output an error message.              */
         Display(("\r\n"));
         Display(("Error %d.\r\n", ret_val));
      }
   }
   else
   {
      /* There was an error with one or more of the input parameters.   */
      Display(("Null Event\r\n"));
   }

   if(_DisplayPrompt)
      DisplayPrompt();
}

   /* The following function is responsible for processing HCI Mode     */
   /* change events.                                                    */
static void BTPSAPI HCI_Event_Callback(unsigned int BluetoothStackID, HCI_Event_Data_t *HCI_Event_Data, unsigned long CallbackParameter)
{
   char *Mode;

#if ((SPP_PERFORM_MASTER_ROLE_SWITCH) && (MAX_SIMULTANEOUS_SPP_PORTS > 1))
   int   SerialPortIndex;
#endif

   /* Make sure that the input parameters that were passed to us are    */
   /* semi-valid.                                                       */
   if((BluetoothStackID) && (HCI_Event_Data))
   {
      /* Process the Event Data.                                        */
      switch(HCI_Event_Data->Event_Data_Type)
      {

#if ((SPP_PERFORM_MASTER_ROLE_SWITCH) && (MAX_SIMULTANEOUS_SPP_PORTS > 1))

         case etRole_Change_Event:
            if(HCI_Event_Data->Event_Data.HCI_Role_Change_Event_Data)
            {
               /* Find the Serial Port entry for this event.            */
               if((SerialPortIndex = FindSPPPortIndexByAddress(HCI_Event_Data->Event_Data.HCI_Role_Change_Event_Data->BD_ADDR)) >= 0)
               {
                  if((HCI_Event_Data->Event_Data.HCI_Role_Change_Event_Data->Status == HCI_ERROR_CODE_NO_ERROR) && (HCI_Event_Data->Event_Data.HCI_Role_Change_Event_Data->New_Role == HCI_CURRENT_ROLE_MASTER))
                  {
                     Display(("\r\nSPP Port %u: Role Change Success.\r\n", SPPContextInfo[SerialPortIndex].LocalSerialPortID));
                  }
                  else
                  {
                     Display(("\r\nSPP Port %u: Role Change Failure (Status 0x%02X, Role 0x%02X).\r\n", SPPContextInfo[SerialPortIndex].LocalSerialPortID, HCI_Event_Data->Event_Data.HCI_Role_Change_Event_Data->Status, HCI_Event_Data->Event_Data.HCI_Role_Change_Event_Data->New_Role));
                  }

                  DisplayPrompt();
               }
            }
            break;

#endif

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
                     /* Restart the User Interface Selection.           */
                     UI_Mode = UI_MODE_SELECT;

                     /* Save the maximum supported baud rate.           */
                     MaxBaudRate = (DWord_t)(HCI_DriverInformation->DriverInformation.COMMDriverInformation.BaudRate);

                     /* Set up the Selection Interface.                 */
                     UserInterface_Selection();

                     /* Display the first command prompt.               */
                     DisplayPrompt();

                     /* Return success to the caller.                   */
                     ret_val = (int)BluetoothStackID;
                  }
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
   Display(("\r\nSPP+LE>"));
}

   /* The following function is used to process a command line string.  */
   /* This function takes as it's only parameter the command line string*/
   /* to be parsed and returns TRUE if a command was parsed and executed*/
   /* or FALSE otherwise.                                               */
Boolean_t ProcessCommandLine(char *String)
{
   return(CommandLineInterpreter(String));
}

