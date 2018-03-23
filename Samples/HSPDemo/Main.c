/*****< main.c >***************************************************************/
/*      Copyright 2012 - 2014 Stonestreet One.                                */
/*      All Rights Reserved.                                                  */
/*                                                                            */
/*      Copyright 2015 Texas Instruments Incorporated.                        */
/*      All Rights Reserved.                                                  */
/*                                                                            */
/*  MAIN - Main application implementation.                                   */
/*                                                                            */
/*  Author:  Tim Cook                                                         */
/*                                                                            */
/*** MODIFICATION HISTORY *****************************************************/
/*                                                                            */
/*   mm/dd/yy  F. Lastname    Description of Modification                     */
/*   --------  -----------    ------------------------------------------------*/
/*   01/28/12  T. Cook        Initial creation.                               */
/******************************************************************************/
#include "Main.h"                /* Main application header.                  */
#include "HAL.h"                 /* Function for Hardware Abstraction.        */
#include "HCITRANS.h"            /* HCI Transport Prototypes/Constants.       */

   /*********************************************************************/
   /* Defines, Enumerations, & Type Definitions                         */
   /*********************************************************************/

   /* The following constant denotes the max buffer size used for user  */
   /* commands input via the User Interface.                            */
#define MAX_COMMAND_LENGTH    (64)

   /*********************************************************************/
   /* Local/Static Variables                                            */
   /*********************************************************************/

   /* Internal Variables to this Module (Remember that all variables    */
   /* declared static are initialized to 0 automatically by the         */
   /* compiler as part of standard C/C++).                              */

static unsigned int BluetoothStackID;
static unsigned int InputIndex;
static Boolean_t SleepAllowed;

   /* The following buffer is used to store console input.  An          */
   /* additional 2 bytes are added to account for 2 null characters that*/
   /* must be added to each command line before the lines are processed.*/
static char Input[MAX_COMMAND_LENGTH + 2];

   /*********************************************************************/
   /* Local/Static Functions                                            */
   /*********************************************************************/

static void ProcessCharactersTask(void *UserParameter);
static void IdleTask(void *UserParameter);
static void ToggleLEDTask(void *UserParameter);
static void BTPSAPI HCI_Sleep_Callback(Boolean_t _SleepAllowed, unsigned long CallbackParameter);
static void PollErrorFlags(void);

   /* The following function is responsible for retrieving commands from*/
   /* the user console.                                                 */
static void ProcessCharactersTask(void *UserParameter)
{
   char      Char;
   Boolean_t CompleteLine;

   /* Initialize the variable indicating a complete line has been parsed*/
   /* to false.                                                         */
   CompleteLine = FALSE;

   /* Attempt to read data from the console.                            */
   while((!CompleteLine) && (HAL_ConsoleRead(1, &Char)))
   {
      switch(Char)
      {
         case '\r':
         case '\n':
            if(InputIndex > 0)
            {
               /* We have received an end of line character, set the    */
               /* complete line variable to true.                       */
               CompleteLine = TRUE;
            }
            else
            {
               /* The user pressed 'Enter' without typing a command,    */
               /* display the application's prompt.                     */
               DisplayPrompt();
            }
            break;
         case 0x08:
            /* Backspace has been pressed, so now decrement the number  */
            /* of bytes in the buffer (if there are bytes in the        */
            /* buffer).                                                 */
            if(InputIndex)
            {
               InputIndex--;
               HAL_ConsoleWrite(3, "\b \b");
            }
            break;
         default:
            /* Accept any other printable characters.                   */
            if((Char >= ' ') && (Char <= '~'))
            {
               /* Add the Data Byte to the Input Buffer, and make sure  */
               /* that we do not overwrite the Input Buffer.            */
               Input[InputIndex++] = Char;
               HAL_ConsoleWrite(1, &Char);

               /* Check to see if we have reached the end of the buffer.*/
               if(InputIndex >= MAX_COMMAND_LENGTH)
               {
                  /* We have received all of the data that we can       */
                  /* handle, set the complete line variable to true.    */
                  CompleteLine = TRUE;
               }
            }
            break;
      }
   }

   /* Check if we have received a complete line.                        */
   if(CompleteLine)
   {
      /* We have received a complete line, null-terminate the string,   */
      /* adding an extra null character for interopability with the     */
      /* command line processing performed in the application.          */
      Input[InputIndex]   = '\0';
      Input[InputIndex+1] = '\0';

      /* Set the input index back to the start of the buffer.           */
      InputIndex = 0;

      /* Process the command line.                                      */
      ProcessCommandLine(Input);
   }
}

   /* The following function is responsible for checking the idle state */
   /* and possibly entering LPM3 mode.                                  */
static void IdleTask(void *UserParameter)
{
   /* If the stack is idle and we are in HCILL sleep, then we may enter */
   /* a Low Power Mode (with Timer Interrupts disabled).                */
   if((BSC_QueryStackIdle(BluetoothStackID)) && (SleepAllowed))
   {
      /* The stack is idle and we are in HCILL sleep, attempt to suspend*/
      /* the UART.                                                      */
      if(!HCITR_COMSuspend())
      {
         /* Check to see if a wakeup is in progress (by the controller).*/
         /* If so we will disable sleep mode so that we complete the    */
         /* process.                                                    */
         if(!HCITR_COMSuspended())
            SleepAllowed = FALSE;

         /* Go ahead and process any characters we may have received on */
         /* the console UART.                                           */
         ProcessCharactersTask(NULL);
      }
      else
      {
         /* Failed to suspend the UART which must mean that the         */
         /* controller is attempting to do a wakeup.  Therefore we will */
         /* flag that sleep mode is disabled.                           */
         SleepAllowed = FALSE;
      }
   }
   else
   {
      /* Poll the error flags.                                          */
      PollErrorFlags();
   }
}

   /* The following function is responsible for toggling the LED.       */
static void ToggleLEDTask(void *UserParameter)
{
   HAL_ToggleLED();
}

   /* The following is the HCI Sleep Callback.  This is registered with */
   /* the stack to note when the Host processor may enter into a sleep  */
   /* mode.                                                             */
static void BTPSAPI HCI_Sleep_Callback(Boolean_t _SleepAllowed, unsigned long CallbackParameter)
{
   /* Simply store the state internally.                                */
   SleepAllowed = _SleepAllowed;

   /* Check if sleep is allowed.                                        */
   if(SleepAllowed)
   {
      /* Sleep is allowed, set the LED color to blue to notify the user.*/
      HAL_SetLEDColor(hlcBlue);
   }
   else
   {
      /* Sleep is not allowed, set the LED color to green to notify the */
      /* user.                                                          */
      HAL_SetLEDColor(hlcGreen);
   }
}

   /* The following function polls all error flags and displays an      */
   /* appropriate message if any error flags are set.                   */
static void PollErrorFlags(void)
{
   unsigned int ErrorFlags;

   /* Query the HCI transport error flags to determine if any errors    */
   /* have occurred.                                                    */
   ErrorFlags = HCITR_COMQueryErrorFlags();

   if(ErrorFlags & HCITR_ERROR_FLAG_EUSCI_UART_RXBUF_OVERRUN)
      Display(("Error: HCITRANS eUSCI UART RXBUF register overrun.\r\n"));

   if(ErrorFlags & HCITR_ERROR_FLAG_UART_RX_BUFFER_OVERRUN)
      Display(("Error: HCITRANS UART Rx buffer overrun.\r\n"));
}

   /*********************************************************************/
   /* Global/Non-Static Functions                                       */
   /*********************************************************************/

   /* The following is the main application entry point.  This function */
   /* will configure the hardware and initialize the OS Abstraction     */
   /* layer, create the main application thread and start the scheduler.*/
int main(void)
{
   int                           Result;
   BTPS_Initialization_t         BTPS_Initialization;
   HCI_DriverInformation_t       HCI_DriverInformation;
   HCI_HCILLConfiguration_t      HCILLConfig;
   HCI_Driver_Reconfigure_Data_t DriverReconfigureData;

   /* Configure the hardware for its intended use.                      */
   HAL_ConfigureHardware();

   /* Flag that sleep is not currently enabled.                         */
   SleepAllowed = FALSE;

   /* Configure the UART Parameters.                                    */
   HCI_DRIVER_SET_COMM_INFORMATION(&HCI_DriverInformation, 1, HAL_HCI_UART_MAX_BAUD_RATE, cpHCILL_RTS_CTS);

   /* Set up the application callbacks.                                 */
   BTPS_Initialization.GetTickCountCallback  = HAL_GetTickCount;
   BTPS_Initialization.MessageOutputCallback = HAL_ConsoleWrite;

   /* Initialize the application.                                       */
   if((Result = InitializeApplication(&HCI_DriverInformation, &BTPS_Initialization)) > 0)
   {
      /* Save the Bluetooth Stack ID.                                   */
      BluetoothStackID = (unsigned int)Result;

      /* Register a sleep mode callback if we are using HCILL Mode.     */
      if((HCI_DriverInformation.DriverInformation.COMMDriverInformation.Protocol == cpHCILL) || (HCI_DriverInformation.DriverInformation.COMMDriverInformation.Protocol == cpHCILL_RTS_CTS))
      {
         HCILLConfig.SleepCallbackFunction        = HCI_Sleep_Callback;
         HCILLConfig.SleepCallbackParameter       = 0;
         DriverReconfigureData.ReconfigureCommand = HCI_COMM_DRIVER_RECONFIGURE_DATA_COMMAND_CHANGE_HCILL_PARAMETERS;
         DriverReconfigureData.ReconfigureData    = (void *)&HCILLConfig;

         /* Register the sleep mode callback.  Note that if this        */
         /* function returns greater than 0 then sleep is currently     */
         /* enabled.                                                    */
         Result = HCI_Reconfigure_Driver(BluetoothStackID, FALSE, &DriverReconfigureData);
         if(Result > 0)
         {
            Display(("Sleep is allowed.\r\n"));

            /* Flag that it is safe to go into sleep mode.              */
            SleepAllowed = TRUE;
         }
      }

      /* We need to execute Add a function to process the command line  */
      /* to the BTPS Scheduler.                                         */
      if(BTPS_AddFunctionToScheduler(ProcessCharactersTask, NULL, 100))
      {
         /* Add the idle task (which determines if LPM3 may be entered) */
         /* to the scheduler.                                           */
         if(BTPS_AddFunctionToScheduler(IdleTask, NULL, 100))
         {
            if(BTPS_AddFunctionToScheduler(ToggleLEDTask, NULL, 750))
            {
               HAL_SetLEDColor(hlcGreen);

               /* Execute the scheduler, note that this function does   */
               /* not return.                                           */
               BTPS_ExecuteScheduler();
            }
         }
      }
   }

   /* If we've gotten to this point then an error has occurred, set the */
   /* LED to red to signify that there is a problem.                    */
   HAL_SetLEDColor(hlcRed);

   /* Poll the error flags to see if we can determine the reason for the*/
   /* failure.                                                          */
   PollErrorFlags();

   /* Scheduler above should run continuously, if it exits an error     */
   /* occurred.                                                         */
   while(1)
   {
      HAL_ToggleLED();

      BTPS_Delay(100);
   }
}
