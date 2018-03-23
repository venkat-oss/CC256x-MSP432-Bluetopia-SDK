/*****< ibeacondemo.c >********************************************************/
/*                                                                            */
/* Copyright (C) 2015 Texas Instruments Incorporated - http://www.ti.com/     */
/*      All Rights Reserved.                                                  */
/*                                                                            */
/*  IBEACONDEMO - Demo Application that simulate Apple iBeacon using LE       */
/*                Advertise.                                                  */
/*                                                                            */
/*  Author:  Ram Malovany                                                     */
/*                                                                            */
/*** MODIFICATION HISTORY *****************************************************/
/*                                                                            */
/*   mm/dd/yy  F. Lastname    Description of Modification                     */
/*   --------  -----------    ------------------------------------------------*/
/*   01/28/15  R. Malovany    Initial creation.                               */
/*   03/03/15  D. Horowitz    Adding Demo Application version.                */
/******************************************************************************/
#include "Main.h"                /* Application Interface Abstraction.        */
#include "iBeaconDemo.h"         /* Application Header.                       */
#include "SS1BTPS.h"             /* Main SS1 BT Stack Header.                 */
#include "BTPSKRNL.h"            /* BTPS Kernel Header.                       */
#include "SS1BTVS.h"             /* Vendor Specific Prototypes/Constants.     */

#define MAX_SUPPORTED_COMMANDS                     (13)  /* Denotes the       */
                                                         /* maximum number of */
                                                         /* User Commands that*/
                                                         /* are supported by  */
                                                         /* this application. */

#define MAX_NUM_OF_PARAMETERS                      (16)  /* Denotes the max   */
                                                         /* number of         */
                                                         /* parameters a      */
                                                         /* command can have. */

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

#define EXIT_TEST_MODE                             (-9)  /* Flags exit from   */
                                                         /* Test Mode.        */

#define EXIT_MODE                                  (-10) /* Flags exit from   */
                                                         /* any Mode.         */

#define UUID_INPUT_LENGTH                           (35) /* Denotes the length*/
                                                         /* of the user input */
                                                         /* parameter for the */
                                                         /* UUID parameter.   */

   /* Determine the Name we will use for this compilation.              */
#define LE_APP_DEMO_NAME                        "iBeaconDemo"


   /* The following MACRO is used to convert an ASCII character into the*/
   /* equivalent decimal value.  The MACRO converts lower case          */
   /* characters to upper case before the conversion.                   */
#define ToInt(_x)                                  (((_x) > 0x39)?(((_x) & ~0x20)-0x37):((_x)-0x30))


   /* Following converts a Sniff Parameter in Milliseconds to frames.   */
#define MILLISECONDS_TO_BASEBAND_SLOTS(_x)         ((_x) / (0.625))

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



static Byte_t iBeacon_Prefeix[] = {0x02,0x01,0x1a,0x1a,0xff,0x4c,0x00,0x02,0x15};
#define IBEACON_PREFIX_LEN   (sizeof(iBeacon_Prefeix)/sizeof(char))

static Byte_t iBeacon_Uuid[] = {0xe2,0xc5,0x6d,0xb5,0xdf,0xfb,0x48,0x48,0xd2,0xb0,0x60,0xd0,0xf5,0xa7,0x10,0x96};
#define IBEACON_UUID_LEN   (sizeof(iBeacon_Uuid)/sizeof(char))

static Byte_t iBeacon_Major[] = {0x00,0x01};
#define IBEACON_MAJOR_LEN   (sizeof(iBeacon_Major)/sizeof(char))

static Byte_t iBeacon_Minor[] = {0x00,0x01};
#define IBEACON_MINOR_LEN   (sizeof(iBeacon_Minor)/sizeof(char))

static signed char iBeacon_Tx_Power = 0xc5;


#define IBEACON_ADRETISE_DATA_SIZE_LEN          (0x1e)

   /* Internal Variables to this Module (Remember that all variables    */
   /* declared static are initialized to 0 automatically by the         */
   /* compiler as part of standard C/C++).                              */

static unsigned int        BluetoothStackID;        /* Variable which holds the Handle */
                                                    /* of the opened Bluetooth Protocol*/
                                                    /* Stack.                          */

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

#define NUM_SUPPORTED_HCI_VERSIONS              (sizeof(HCIVersionStrings)/sizeof(char *) - 1)

   /*********************************************************************/
   /**                    END OF SERVICE TABLE                         **/
   /*********************************************************************/

   /* Internal function prototypes.                                     */
static void UserInterface(void);

static Boolean_t CommandLineInterpreter(char *Command);
static long StringTosignedInteger(char *StringInteger);
static char *StringParser(char *String);
static int CommandParser(UserCommand_t *TempCommand, char *Input);
static int CommandInterpreter(UserCommand_t *TempCommand);
static int AddCommand(char *CommandName, CommandFunction_t CommandFunction);
static CommandFunction_t FindCommand(char *Command);
static void ClearCommands(void);

static void BD_ADDRToStr(BD_ADDR_t Board_Address, BoardStr_t BoardStr);

static void DisplayUsage(char *UsageString);
static void DisplayFunctionError(char *Function,int Status);
static void DisplayFWVersion (void);
static void DisplayFunctionSuccess(char *Function);

static int OpenStack(HCI_DriverInformation_t *HCI_DriverInformation, BTPS_Initialization_t *BTPS_Initialization);
static int CloseStack(void);

static int DisplayHelp(ParameterList_t *TempParam);
static int QueryMemory(ParameterList_t *TempParam);
static int AdvertizeIbeacon(ParameterList_t *TempParam);
static int DisableAdvertizeIbeacon(ParameterList_t *TempParam);
static int SetIbeaconUUID(ParameterList_t *TempParam);
static int QueryIbeaconUUID(ParameterList_t *TempParam);
static int SetIbeaconMinor(ParameterList_t *TempParam);
static int QueryIbeaconMinor(ParameterList_t *TempParam);
static int SetIbeaconMajor(ParameterList_t *TempParam);
static int QueryIbeaconMajor(ParameterList_t *TempParam);
static int SetIbeaconTxPower(ParameterList_t *TempParam);
static int QueryIbeaconTxPower(ParameterList_t *TempParam);

static void BTPSAPI GAP_LE_Event_Callback(unsigned int BluetoothStackID, GAP_LE_Event_Data_t *GAP_LE_Event_Data, unsigned long CallbackParameter);

   /* This function is responsible for taking the input from the user   */
   /* and dispatching the appropriate Command Function.  First, this    */
   /* function retrieves a String of user input, parses the user input  */
   /* into Command and Parameters, and finally executes the Command or  */
   /* Displays an Error Message if the input is not a valid Command.    */
static void UserInterface(void)
{
   /* Next display the available commands.                              */
   DisplayHelp(NULL);
   ClearCommands();

   AddCommand("SETIBEACONUUID", SetIbeaconUUID);
   AddCommand("QUERYIBEACONUUID", QueryIbeaconUUID);
   AddCommand("SETIBEACONMINOR", SetIbeaconMinor);
   AddCommand("QUERYIBEACONMINOR", QueryIbeaconMinor);
   AddCommand("SETIBEACONMAJOR", SetIbeaconMajor);
   AddCommand("QUERYIBEACONMAJOR", QueryIbeaconMajor);
   AddCommand("SETIBEACONTXPOWER", SetIbeaconTxPower);
   AddCommand("QUERYIBEACONTXPOWER", QueryIbeaconTxPower);
   AddCommand("ADVERTIZEIBEACON", AdvertizeIbeacon);
   AddCommand("DISABLEADVERTIZEIBEACON", DisableAdvertizeIbeacon);
   AddCommand("HELP", DisplayHelp);
   AddCommand("QUERYMEMORY", QueryMemory);
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
      Display(("\r\n"));
      Display(("************************************************************\r\n"));
      Display(("* Command Options : SetIbeaconUUID, QueryIbeaconUUID,      *\r\n"));
      Display(("*                   SetIbeaconMajor, QueryIbeaconMajor,    *\r\n"));
      Display(("*                   SetIbeaconMinor, QueryIbeaconMinor,    *\r\n"));
      Display(("*                   SetIbeaconTxPower, QueryIbeaconTxPower,*\r\n"));
      Display(("*                   AdvertizeIbeacon,                      *\r\n"));
      Display(("*                   DisableAdvertizeIbeacon,               *\r\n"));
      Display(("*                   Help                                   *\r\n"));
      Display(("************************************************************\r\n"));
      Display(("\r\n"));


    return(0);
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

            Display(("Exit App \r\n"));

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
static long StringTosignedInteger(char *StringInteger)
{
   int           IsHex;
   unsigned long Index;
   long          ret_val = 0;
   Byte_t        Minus_Flag = 0;

   /* Before proceeding make sure that the parameter that was passed as */
   /* an input appears to be at least semi-valid.                       */
   if((StringInteger) && (BTPS_StringLength(StringInteger)))
   {
      /* Initialize the variable.                                       */
      Index = 0;

      /* Next check to see if this is a hexadecimal number.             */
      if(BTPS_StringLength(StringInteger) > 2 && (StringInteger[0] == '0') &&
        ((StringInteger[1] == 'x') || (StringInteger[1] == 'X')) )
      {
        IsHex = 1;

        /* Increment the String passed the Hexadecimal prefix.          */
        StringInteger += 2;
      }
      else
      {
        /* Check the number for a minus sign - Negative Number          */
        if(StringInteger[0] == '-')
        {
            Minus_Flag = 1;
        }
        IsHex = 0;
      }


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
         /* If we had Minus sign, we need to create negative number, so */
         /* we need to multiply the number with Minus 1                 */
         if (Minus_Flag)
            ret_val *= -1;
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
               TempCommand->Parameters.Params[Count].intParam = StringTosignedInteger(LastParameter);

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

   /* Displays a function error message.                                */
static void DisplayFunctionError(char *Function,int Status)
{
   Display(("%s Failed: %d.\r\n", Function, Status));
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
   int                           Result;
   int                           ret_val = 0;
   char                          BluetoothAddress[16];
   BD_ADDR_t                     BD_ADDR;
   HCI_Version_t                 HCIVersion;

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

static int SetIbeaconUUID(ParameterList_t *TempParam)
{
   int           ret_val     = 0;
   int           index       = 0;
   char          temp_hex[4] = {'0','x','0','0'};
   char         *temp_ptr    = NULL;

    /* First, check that valid Bluetooth Stack ID exists.               */
    if(BluetoothStackID)
    {
        /* Next, let's make sure that the user has specified a Remote   */
        /* Bluetooth Device to open.                                    */
        if((TempParam) && (BTPS_StringLength(TempParam->Params[0].strParam)) == UUID_INPUT_LENGTH)
        {
            /* Puts '-' in the end of the string in order to exit the   */
            /* inner loop in the end of the string                      */
            TempParam->Params[0].strParam[UUID_INPUT_LENGTH]='-';

            /* Holds the location of the input string                   */
            temp_ptr = TempParam->Params[0].strParam;

            /* This loops translates the string into Hex numbers        */
            while (index<16)
            {
                while (*temp_ptr != '-')
                {
                    /* Creates string with Hex format                   */
                    temp_hex[2] = *temp_ptr;
                    temp_ptr++;
                    temp_hex[3] = *temp_ptr;
                    temp_ptr++;

                    /* Translate the Hex string into Hex integer        */
                    iBeacon_Uuid[index] = StringTosignedInteger(temp_hex);
                    index++;
                }

                /* When we encounter '-' we need to increase the        */
                /* pointer location                                     */
                temp_ptr++;
            }
            DisplayFunctionSuccess("SetIbeaconUUID");
            Display(("In order to enable this changes, call DisableAdvertizeIbeacon and call\r\n" ));
            Display(("AdvertizeIbeacon again to enable the beacon with the updated values\r\n"));
        }
        else
        {
            DisplayUsage("Example SetIbeaconUUID: E2C56DB5-DFFB-4848-D2B060D0F5A71096");
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

static int QueryIbeaconUUID(ParameterList_t *TempParam)
{
   int           ret_val = 0;
   int           index   = 0;

   /* First, check that valid Bluetooth Stack ID exists.                */
    if(BluetoothStackID)
    {
       Display(("IBeacon UUID: "));
       for (index = 0; index<4; index++)
       {
           Display(("%02X",iBeacon_Uuid[index]));
       }
       Display(("-%02X%02X",iBeacon_Uuid[4],iBeacon_Uuid[5]));
       Display(("-%02X%02X-",iBeacon_Uuid[6],iBeacon_Uuid[7]));
       for (index = 8; index<16; index++)
       {
           Display(("%02X",iBeacon_Uuid[index]));
       }
       Display(("\r\n"));
    }
    else
    {
        /* No valid Bluetooth Stack ID exists.                          */
        ret_val = INVALID_STACK_ID_ERROR;
    }
    return(ret_val);
}

static int SetIbeaconMinor(ParameterList_t *TempParam)
{
   int           ret_val = 0;

   /* First, check that valid Bluetooth Stack ID exists.                */
    if(BluetoothStackID)
    {
        /* Next, let's make sure that the user has specified a Remote   */
        /* Bluetooth Device to open.                                    */
        if((TempParam) && (TempParam->NumberofParameters == 2))
        {
            iBeacon_Minor[0] = TempParam->Params[0].intParam ;
            iBeacon_Minor[1] = TempParam->Params[1].intParam ;
            DisplayFunctionSuccess("SetIbeaconMinor");
            Display(("In order to enable this changes, call DisableAdvertizeIbeacon and call\r\n" ));
            Display(("AdvertizeIbeacon again to enable the beacon with the updated values\r\n"));
        }
        else
        {
            DisplayUsage("Example SetIbeaconMinor: 0xAB 0xAB");
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

static int QueryIbeaconMinor(ParameterList_t *TempParam)
{
   int           ret_val = 0;

    /* First, check that valid Bluetooth Stack ID exists.               */
    if(BluetoothStackID)
    {
        Display(("Ibeacon Minor: 0x%02x 0x%02x\r\n", iBeacon_Minor[0],iBeacon_Minor[1]));
    }
    else
    {
        /* No valid Bluetooth Stack ID exists.                          */
        ret_val = INVALID_STACK_ID_ERROR;
    }
    return(ret_val);
}


static int SetIbeaconMajor(ParameterList_t *TempParam)
{
   int           ret_val = 0;

   /* First, check that valid Bluetooth Stack ID exists.                */
    if(BluetoothStackID)
    {
        /* Next, let's make sure that the user has specified a Remote   */
        /* Bluetooth Device to open.                                    */
        if((TempParam) && (TempParam->NumberofParameters == 2))
        {
            iBeacon_Major[0] = TempParam->Params[0].intParam ;
            iBeacon_Major[1] = TempParam->Params[1].intParam ;
            DisplayFunctionSuccess("SetIbeaconMajor");
            Display(("In order to enable this changes, call DisableAdvertizeIbeacon and call\r\n" ));
            Display(("AdvertizeIbeacon again to enable the beacon with the updated values\r\n"));
        }
        else
        {
            DisplayUsage("Example SetIbeaconMajor: 0xAB 0xAB");
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


static int QueryIbeaconMajor(ParameterList_t *TempParam)
{
   int           ret_val = 0;

    /* First, check that valid Bluetooth Stack ID exists.               */
    if(BluetoothStackID)
    {
        Display(("Ibeacon Major: 0x%02x 0x%02x\r\n", iBeacon_Major[0],iBeacon_Major[1]));
    }
    else
    {
        /* No valid Bluetooth Stack ID exists.                          */
        ret_val = INVALID_STACK_ID_ERROR;
    }
    return(ret_val);
}


static int SetIbeaconTxPower(ParameterList_t *TempParam)
{
   int           ret_val = 0;

   /* First, check that valid Bluetooth Stack ID exists.                */
    if(BluetoothStackID)
    {
        /* Next, let's make sure that the user has specified a Remote   */
        /* Bluetooth Device to open.                                    */
        if((TempParam) && (TempParam->NumberofParameters == 1) && (TempParam->Params[0].intParam))
        {
            iBeacon_Tx_Power = TempParam->Params[0].intParam ;
            DisplayFunctionSuccess("SetIbeaconTxPower");
            Display(("In order to enable this changes, call DisableAdvertizeIbeacon and call\r\n" ));
            Display(("AdvertizeIbeacon again to enable the beacon with the updated values\r\n"));
        }
        else
        {
            DisplayUsage("SetIbeaconTxPower [Byte]");
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


static int QueryIbeaconTxPower(ParameterList_t *TempParam)
{
   int           ret_val = 0;

    /* First, check that valid Bluetooth Stack ID exists.               */
    if(BluetoothStackID)
    {
        Display(("Ibeacon TxPower: %d\r\n", iBeacon_Tx_Power));
    }
    else
    {
        /* No valid Bluetooth Stack ID exists.                          */
        ret_val = INVALID_STACK_ID_ERROR;
    }
    return(ret_val);
}

static int AdvertizeIbeacon(ParameterList_t *TempParam)
{
   int           ret_val = 0;
   int           index;
   GAP_LE_Advertising_Parameters_t     AdvertisingParameters;
   Advertising_Data_t               AdvertisingData;
   GAP_LE_Connectability_Parameters_t  ConnectabilityParameters;

    /* First, check that valid Bluetooth Stack ID exists.               */
    if(BluetoothStackID)
    {
        /* Enable Advertising.  Set the Advertising Data.               */
        BTPS_MemInitialize(&(AdvertisingData), 0, sizeof(Advertising_Data_t));


        /* Set the IBEACON prefix to the Advertising Data               */
        for(index=0; index < IBEACON_PREFIX_LEN; index++)
        {
            AdvertisingData.Advertising_Data[index] =iBeacon_Prefeix[index] ;
        }

        /* set the index to the IBEACON UUID location at the Advertising*/
        /* Data struct and Set the IBEACON UUID to the Advertising Data */
        for(index = IBEACON_PREFIX_LEN; index < (IBEACON_PREFIX_LEN + IBEACON_UUID_LEN); index++)
        {
            AdvertisingData.Advertising_Data[index] =iBeacon_Uuid[index - IBEACON_PREFIX_LEN] ;
        }

        /* Setting index IBEACON Major location                         */
        index = IBEACON_PREFIX_LEN +IBEACON_UUID_LEN;

        /* Set the iBeacon Major                                        */
        AdvertisingData.Advertising_Data[index++] = iBeacon_Major[0];
        AdvertisingData.Advertising_Data[index++] = iBeacon_Major[1];

        /* Set the iBeacon Minor                                        */
        AdvertisingData.Advertising_Data[index++] = iBeacon_Minor[0];
        AdvertisingData.Advertising_Data[index++] = iBeacon_Minor[1];

        /* Set the iBeacon TX Power                                     */
        AdvertisingData.Advertising_Data[index] = iBeacon_Tx_Power;

        /* Write thee advertising data to the chip.                     */
        ret_val = GAP_LE_Set_Advertising_Data(BluetoothStackID, IBEACON_ADRETISE_DATA_SIZE_LEN, &(AdvertisingData));
        if(0 == ret_val)
        {
            /* Set up the advertising parameters.                       */
            AdvertisingParameters.Advertising_Channel_Map   = HCI_LE_ADVERTISING_CHANNEL_MAP_DEFAULT;
            AdvertisingParameters.Scan_Request_Filter       = fpNoFilter;
            AdvertisingParameters.Connect_Request_Filter    = fpNoFilter;
            AdvertisingParameters.Advertising_Interval_Min  = 100;
            AdvertisingParameters.Advertising_Interval_Max  = 200;

            /* Set up the Connectability parameters.                    */
            ConnectabilityParameters.Connectability_Mode   = lcmNonConnectable;
            ConnectabilityParameters.Own_Address_Type      = latPublic;
            ConnectabilityParameters.Direct_Address_Type   = latPublic;
            ASSIGN_BD_ADDR(ConnectabilityParameters.Direct_Address, 0, 0, 0, 0, 0, 0);

            ret_val = GAP_LE_Advertising_Enable(BluetoothStackID, TRUE, &AdvertisingParameters, &ConnectabilityParameters, GAP_LE_Event_Callback, 0);
            if (ret_val != 0)
            {
                DisplayFunctionError("GAP_LE_Advertising_Enable", ret_val);
            }
            else
            {
                DisplayFunctionSuccess("AdvertizeIbeacon");
            }
        }
        else
        {
          DisplayFunctionError("GAP_LE_Advertising_Enable", ret_val);
        }

    }
    else
    {
        /* No valid Bluetooth Stack ID exists.                          */
        ret_val = INVALID_STACK_ID_ERROR;
    }

   return(ret_val);
}


static int DisableAdvertizeIbeacon(ParameterList_t *TempParam)
{

   int  ret_val = 0;

    /* Disable Advertising.                                             */
    ret_val = GAP_LE_Advertising_Disable(BluetoothStackID);
    if(!ret_val)
    {
       Display(("GAP_LE_Advertising_Disable success.\r\n"));
    }
    else
    {
       DisplayFunctionError("GAP_LE_Advertising_Disable", ret_val);
       ret_val = FUNCTION_ERROR;
    }

  return(ret_val);
 }

   /* Displays a usage string..                                         */
static void DisplayUsage(char *UsageString)
{
   Display(("\nUsage: %s.\r\n", UsageString));
}


static void DisplayFunctionSuccess(char *Function)
{
   Display(("\n%s success.\r\n", Function));
}


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
         /* Advertise Default values at Beginning                       */
         if ((ret_val = AdvertizeIbeacon(NULL)) == 0)
         {
             Display(("************************************************************\r\n"));
             Display((" Application is already Advertising default values,         \r\n"));
             Display((" In order to change them call  DisableAdvertizeIbeacon and  \r\n"));
             Display((" update the values to your needs then call AdvertizeIbeacon \r\n"));
             Display((" again to enable the beacon with the updated values         \r\n"));
             Display(("************************************************************\r\n"));
             Display(("The default values are:                                     \r\n"));
             QueryIbeaconUUID(NULL);
             QueryIbeaconMinor(NULL);
             QueryIbeaconMajor(NULL);
             QueryIbeaconTxPower(NULL);

             /* Attempt to register a HCI Event Callback.  Set up the   */
             /* Selection Interface.                                    */
             UserInterface();

             /* Display the first command prompt.                       */
             DisplayPrompt();

             /* Return success to the caller.                           */
             ret_val = (int)BluetoothStackID;
         }
         else
         {
             DisplayFunctionError("AdvertizeIbeacon",ret_val);
             /* Close the Bluetooth Stack.                              */
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
   Display(("\r\niBeacon>"));
}

   /* The following function is used to process a command line string.  */
   /* This function takes as it's only parameter the command line string*/
   /* to be parsed and returns TRUE if a command was parsed and executed*/
   /* or FALSE otherwise.                                               */
Boolean_t ProcessCommandLine(char *String)
{
   return(CommandLineInterpreter(String));
}

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

}


