/*****< btvs.c >***************************************************************/
/*      Copyright 2011 - 2014 Stonestreet One.                                */
/*      All Rights Reserved.                                                  */
/*                                                                            */
/*  BTVS - Vendor specific implementation of a set of vendor specific         */
/*         functions supported for a specific hardware platform.              */
/*                                                                            */
/*  Author:  Damon Lange                                                      */
/*                                                                            */
/*** MODIFICATION HISTORY *****************************************************/
/*                                                                            */
/*   mm/dd/yy  F. Lastname    Description of Modification                     */
/*   --------  -----------    ------------------------------------------------*/
/*   05/20/11  D. Lange       Initial creation.                               */
/******************************************************************************/
#include "SS1BTPS.h"          /* Bluetopia API Prototypes/Constants.          */
#include "HCIDRV.h"

#include "BTPSKRNL.h"

#include "SS1BTVS.h"          /* Vendor Specific Prototypes/Constants.        */

   /* Defines the size of buffer needed to hold the parameters for      */
   /* the VS Set Power Vector Command.                                  */
#define POWER_VECTOR_COMMAND_SIZE                                20

   /* Total Number of Power Vectors.                                    */
#define POWER_VECTOR_SIZE                                        (POWER_VECTOR_COMMAND_SIZE - 5)

   /* Number of steps in the Power Vector Section.                      */
#define POWER_VECTOR_STEP_SIZE                                   5

   /* Defines the size of buffer needed to hold the DRPB Tester Con Tx  */
   /* Command.                                                          */
#define DRPB_TESTER_CON_TX_SIZE                                  12

   /* Defines the size of the buffer needed to hold the HCILL Parameters*/
   /* command.                                                          */
#define HCILL_PARAMETERS_COMMAND_SIZE                            5

   /* Defines the size of the buffer needed to hold the Sleep Mode      */
   /* Configurations command.                                           */
#define SLEEP_MODE_CONFIGURATIONS_COMMAND_SIZE                   9

   /* The following MACRO is a utility MACRO that exists to aid in      */
   /* converting an unsigned long in milliseconds to 2 Baseband slots.  */
#define CONVERT_TO_TWO_BASEBAND_SLOTS(_x)                         ((unsigned long)((((4000L * (_x)) / 500L) + 5L)/10L))

   /* Vendor Specific Command Opcodes.                                  */
#define VS_DRPB_SET_POWER_VECTOR_COMMAND_OPCODE                  ((Word_t)(0xFD82))
#define VS_DRPB_SET_SET_CLASS2_SINGLE_POWER_COMMAND_OPCODE       ((Word_t)(0xFD87))
#define VS_DRPB_SET_SET_RF_CALIBRATION_INFO_COMMAND_OPCODE       ((Word_t)(0xFD76))
#define VS_DRPB_SET_ENABLE_RF_CALIBRATION_COMMAND_OPCODE         ((Word_t)(0xFD80))
#define VS_DRPB_TESTER_CON_TX_COMMAND_OPCODE                     ((Word_t)(0xFD84))
#define VS_WRITE_HARDWARE_REGISTER_COMMAND_OPCODE                ((Word_t)(0xFF01))
#define VS_HCILL_PARAMETERS_COMMAND_OPCODE                       ((Word_t)(0xFD2B))
#define VS_SLEEP_MODE_CONFIGURATIONS_COMMAND_OPCODE              ((Word_t)(0xFD0C))
#define VS_UPDATE_UART_HCI_BAUDRATE_COMMAND_OPCODE               ((Word_t)(0xFF36))
#define VS_WRITE_BD_ADDRESS_COMMAND_OPCODE                       ((Word_t)(0xFC06))
#define VS_CONFIGURE_DDIP_COMMAND_OPCODE                         ((Word_t)(0xFD55))
#define VS_LE_ENABLE                                             ((Word_t)(0xFD5B))
#define VS_WRITE_CODEC_CONFIG_COMMAND_OPCODE                     ((Word_t)(0xFD06))
#define VS_WRITE_CODEC_CONFIG_ENHANCED_COMMAND_OPCODE            ((Word_t)(0xFD07))
#define VS_PCM_LOOPBACK_OPCODE                                   ((Word_t)(0xFE28))
#define VS_WBS_ASSOCIATE_COMMAND_OPCODE                          ((Word_t)(0xFD78))
#define VS_WBS_DISASSOCIATE_COMMAND_OPCODE                       ((Word_t)(0xFD79))
#define VS_A3DP_OPEN_STREAM_COMMAND_OPCODE                       ((Word_t)(0xFD8C))
#define VS_A3DP_CLOSE_STREAM_COMMAND_OPCODE                      ((Word_t)(0xFD8D))
#define VS_A3DP_CODEC_CONFIGURATION_COMMAND_OPCODE               ((Word_t)(0xFD8E))
#define VS_A3DP_START_STREAM_COMMAND_OPCODE                      ((Word_t)(0xFD8F))
#define VS_A3DP_STOP_STREAM_COMMAND_OPCODE                       ((Word_t)(0xFD90))
#define VS_AVPR_DEBUG_COMMAND_OPCODE                             ((Word_t)(0xFD91))
#define VS_AVPR_ENABLE_COMMAND_OPCODE                            ((Word_t)(0xFD92))
#define VS_A3DP_SINK_OPEN_STREAM_COMMAND_OPCODE                  ((Word_t)(0xFD9A))
#define VS_A3DP_SINK_CLOSE_STREAM_COMMAND_OPCODE                 ((Word_t)(0xFD9B))
#define VS_A3DP_SINK_CODEC_CONFIGURATION_COMMAND_OPCODE          ((Word_t)(0xFD9C))
#define VS_A3DP_SINK_START_STREAM_COMMAND_OPCODE                 ((Word_t)(0xFD9D))
#define VS_A3DP_SINK_STOP_STREAM_COMMAND_OPCODE                  ((Word_t)(0xFD9E))
#define VS_A3DP_MULTI_ROOM_MODE_COMMAND_OPCODE                   ((Word_t)(0xFDA2))

#define VS_COMMAND_OGF(_CommandOpcode)                           ((Byte_t)((_CommandOpcode) >> 10))
#define VS_COMMAND_OCF(_CommandOpcode)                           ((Word_t)((_CommandOpcode) & (0x3FF)))

   /* Miscellaneous Type Declarations.                                  */

   /* Internal Variables to this Module (Remember that all variables    */
   /* declared static are initialized to 0 automatically by the         */
   /* compiler as part of standard C/C++).                              */

   /* Internal constant arrays used to hold parameters to the Power     */
   /* Vector commands.                                                  */

   /* Magical command for RF Calibration Command.                       */
static BTPSCONST Byte_t RF_Calibration_Info[] =
{
   0x01,
   0x21, 0x54, 0x00, 0x00,
   0x61, 0x57, 0x00, 0x00,

   20, 5, 10, 5, 0, 7, 6, 10, 4, 5, 8, 9,
   11, 12, 13, 14, 16, 16, 16, 16, 16, 16,
   16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
   16, 16, 16, 16, 0
};

   /* Internal Function Prototypes.                                     */
static int MapSendRawResults(int Result, Byte_t Status, Byte_t LengthResult, Byte_t *ReturnData);

   /* The following function is a utility function that is used to map  */
   /* the return results that we returned from Send Raw (ONLY for       */
   /* commands that return command complete event) to a negative error  */
   /* code.                                                             */
   /* * NOTE * As an internal function no check is done on the return   */
   /*          parameters.                                              */
static int MapSendRawResults(int Result, Byte_t Status, Byte_t LengthResult, Byte_t *ReturnData)
{
   int ret_val;

   /* Check to see if the API returned an error.                        */
   if((!Result) && (!Status) && (LengthResult >= 1))
   {
      /* Check to see if the chip returned an error.                    */
      if(ReturnData[0] == HCI_ERROR_CODE_NO_ERROR)
         ret_val = 0;
      else
         ret_val = BTPS_ERROR_CODE_HCI_STATUS_BASE - ReturnData[0];
   }
   else
      ret_val = BTPS_ERROR_DEVICE_HCI_ERROR;

   return(ret_val);
}

   /* The following function prototype represents the vendor specific   */
   /* function which is used to change the Bluetooth UART for the Local */
   /* Bluetooth Device specified by the Bluetooth Protocol Stack that   */
   /* is specified by the Bluetooth Protocol Stack ID. The second       */
   /* parameter specifies the new baud rate to set.  This change        */
   /* encompasses both changing the speed of the Bluetooth chip (by     */
   /* issuing the correct commands) and then, if successful, informing  */
   /* the HCI Driver of the change (so the driver can communicate with  */
   /* the Bluetooth device at the new baud rate).  This function returns*/
   /* zero if successful or a negative return error code if there was   */
   /* an error.                                                         */
int BTPSAPI VS_Update_UART_Baud_Rate(unsigned int BluetoothStackID, DWord_t BaudRate)
{
   int                              ret_val;
   Byte_t                           Length;
   Byte_t                           Status;
   Byte_t                           Buffer[16];
   Byte_t                           OGF;
   Word_t                           OCF;
   NonAlignedDWord_t                _BaudRate;
   HCI_Driver_Reconfigure_Data_t    DriverReconfigureData;
   HCI_COMMReconfigureInformation_t COMMReconfigureInformation;

   /* Before continuing, make sure the input parameters appear to be    */
   /* semi-valid.                                                       */
   if((BluetoothStackID) && (BaudRate) && (BaudRate <= CONTROLLER_MAX_HCI_BAUD_RATE))
   {
      /* Write the Baud Rate.                                           */
      ASSIGN_HOST_DWORD_TO_LITTLE_ENDIAN_UNALIGNED_DWORD(&_BaudRate, BaudRate);

      /* Next, write the command to the device.                         */
      Length  = sizeof(Buffer);
      OGF     = VS_COMMAND_OGF(VS_UPDATE_UART_HCI_BAUDRATE_COMMAND_OPCODE);
      OCF     = VS_COMMAND_OCF(VS_UPDATE_UART_HCI_BAUDRATE_COMMAND_OPCODE);

      ret_val = HCI_Send_Raw_Command(BluetoothStackID, OGF, OCF, sizeof(NonAlignedDWord_t), (Byte_t *)&_BaudRate, &Status, &Length, Buffer, TRUE);

      /* Map the return results into an error code.                     */
      if((ret_val = MapSendRawResults(ret_val, Status, Length, Buffer)) == 0)
      {
         /* We were successful, now we need to change the baud rate of  */
         /* the driver.                                                 */
         BTPS_MemInitialize(&(DriverReconfigureData), 0, sizeof(DriverReconfigureData));
         BTPS_MemInitialize(&(COMMReconfigureInformation), 0, sizeof(COMMReconfigureInformation));

         /* Now change the baud rate of the driver.                     */
         DriverReconfigureData.ReconfigureCommand    = HCI_COMM_DRIVER_RECONFIGURE_DATA_COMMAND_CHANGE_COMM_PARAMETERS;
         DriverReconfigureData.ReconfigureData       = &COMMReconfigureInformation;
         COMMReconfigureInformation.ReconfigureFlags = HCI_COMM_RECONFIGURE_INFORMATION_RECONFIGURE_FLAGS_CHANGE_BAUDRATE;
         COMMReconfigureInformation.BaudRate         = BaudRate;

         ret_val = HCI_Reconfigure_Driver(BluetoothStackID, FALSE, &(DriverReconfigureData));
      }
   }
   else
      ret_val = BTPS_ERROR_INVALID_PARAMETER;

   /* Return the result the caller.                                     */
   return(ret_val);
}

   /* The following function prototype represents the vendor specific   */
   /* function which is used to change the HCILL parameters that are    */
   /* used for the HCILL Low Power protocol for the Local Bluetooth     */
   /* Device specified by the Bluetooth Protocol Stack that is specified*/
   /* by the Bluetooth Protocol Stack ID. The second is the             */
   /* InactivityTimeout on the UART in ms.  If no traffic on UART lines */
   /* after this time the Controller sends a Sleep Indication.  The     */
   /* third is the RetransmitTimeout (ms) for the Sleep Indication if no*/
   /* Sleep Acknowledgment (from the Host) is received.  The fourth is  */
   /* the Controller RTS pulse width during Controller wakeup (specified*/
   /* in us). This function returns zero if successful or a negative    */
   /* return error code if there was an error.                          */
int BTPSAPI VS_Update_HCILL_Parameters(unsigned int BluetoothStackID, Word_t InactivityTimeout, Word_t RetransmitTimeout, Byte_t CTSPulseWidth)
{
   int    ret_val;
   Byte_t Length;
   Byte_t Status;
   Byte_t ReturnBuffer[8];
   Byte_t CommandBuffer[HCILL_PARAMETERS_COMMAND_SIZE];
   Byte_t OGF;
   Word_t OCF;

   /* Before continuing, make sure the input parameters appear to be    */
   /* semi-valid.                                                       */
   if(BluetoothStackID)
   {
      /* Format the command to send.                                    */
      ASSIGN_HOST_WORD_TO_LITTLE_ENDIAN_UNALIGNED_WORD(&(CommandBuffer[0]), CONVERT_TO_TWO_BASEBAND_SLOTS(InactivityTimeout));
      ASSIGN_HOST_WORD_TO_LITTLE_ENDIAN_UNALIGNED_WORD(&(CommandBuffer[NON_ALIGNED_WORD_SIZE]), CONVERT_TO_TWO_BASEBAND_SLOTS(RetransmitTimeout));
      ASSIGN_HOST_BYTE_TO_LITTLE_ENDIAN_UNALIGNED_BYTE(&(CommandBuffer[NON_ALIGNED_WORD_SIZE*2]), CTSPulseWidth);

      /* Send the command to update the HCILL Parameters.               */
      Length  = sizeof(ReturnBuffer);
      OGF     = VS_COMMAND_OGF(VS_HCILL_PARAMETERS_COMMAND_OPCODE);
      OCF     = VS_COMMAND_OCF(VS_HCILL_PARAMETERS_COMMAND_OPCODE);

      ret_val = HCI_Send_Raw_Command(BluetoothStackID, OGF, OCF, HCILL_PARAMETERS_COMMAND_SIZE, (Byte_t *)(CommandBuffer), &Status, &Length, ReturnBuffer, TRUE);

      /* Map the return results into an error code.                     */
      ret_val = MapSendRawResults(ret_val, Status, Length, ReturnBuffer);
   }
   else
      ret_val = BTPS_ERROR_INVALID_PARAMETER;

   /* Return the result the caller.                                     */
   return(ret_val);
}

   /* The following function prototype represents the vendor specific   */
   /* function which is used to enable/disable the HCILL low power      */
   /* protocol for the Local Bluetooth Device specified by the Bluetooth*/
   /* Protocol Stack that is specified by the Bluetooth Protocol Stack  */
   /* ID.  The second parameter specifies whether to enable (TRUE) or   */
   /* disable (FALSE) the HCILL low power protocol.  This function      */
   /* returns zero if successful or a negative return error code if     */
   /* there was an error.                                               */
int BTPSAPI VS_Enable_Sleep_Mode(unsigned int BluetoothStackID, Boolean_t Enable)
{
   int    ret_val;
   Byte_t Length;
   Byte_t Status;
   Byte_t ReturnBuffer[8];
   Byte_t CommandBuffer[SLEEP_MODE_CONFIGURATIONS_COMMAND_SIZE];
   Byte_t OGF;
   Word_t OCF;

   /* Before continuing, make sure the input parameters appear to be    */
   /* semi-valid.                                                       */
   if(BluetoothStackID)
   {
      /* Format the command to send.                                    */
      ASSIGN_HOST_BYTE_TO_LITTLE_ENDIAN_UNALIGNED_BYTE(&(CommandBuffer[0]), 1);
      ASSIGN_HOST_BYTE_TO_LITTLE_ENDIAN_UNALIGNED_BYTE(&(CommandBuffer[1]), ((Enable == TRUE)?1:0));
      ASSIGN_HOST_BYTE_TO_LITTLE_ENDIAN_UNALIGNED_BYTE(&(CommandBuffer[2]), 0);
      ASSIGN_HOST_BYTE_TO_LITTLE_ENDIAN_UNALIGNED_BYTE(&(CommandBuffer[3]), 0xFF);
      ASSIGN_HOST_BYTE_TO_LITTLE_ENDIAN_UNALIGNED_BYTE(&(CommandBuffer[4]), 0xFF);
      ASSIGN_HOST_BYTE_TO_LITTLE_ENDIAN_UNALIGNED_BYTE(&(CommandBuffer[5]), 0xFF);
      ASSIGN_HOST_BYTE_TO_LITTLE_ENDIAN_UNALIGNED_BYTE(&(CommandBuffer[6]), 0xFF);
      ASSIGN_HOST_WORD_TO_LITTLE_ENDIAN_UNALIGNED_WORD(&(CommandBuffer[7]), 0x0000);

      /* Send the command to set the sleep mode configuration.          */
      Length  = sizeof(ReturnBuffer);
      OGF     = VS_COMMAND_OGF(VS_SLEEP_MODE_CONFIGURATIONS_COMMAND_OPCODE);
      OCF     = VS_COMMAND_OCF(VS_SLEEP_MODE_CONFIGURATIONS_COMMAND_OPCODE);

      ret_val = HCI_Send_Raw_Command(BluetoothStackID, OGF, OCF, SLEEP_MODE_CONFIGURATIONS_COMMAND_SIZE, (Byte_t *)(CommandBuffer), &Status, &Length, ReturnBuffer, TRUE);

      /* Map the return results into an error code.                     */
      ret_val = MapSendRawResults(ret_val, Status, Length, ReturnBuffer);
   }
   else
      ret_val = BTPS_ERROR_INVALID_PARAMETER;

   /* Return the result the caller.                                     */
   return(ret_val);
}

   /* The following function prototype represents the vendor specific   */
   /* function which is used to change the public Bluetooth Address     */
   /* (also known as a MAC address) for the local Bluetooth Device      */
   /* specified by the Bluetooth Protocol Stack ID.  The second         */
   /* parameter contains the Bluetooth Address to set.  This function   */
   /* returns zero if successful or a negative return error code if     */
   /* there was an error.                                               */
int BTPSAPI VS_Set_Bluetooth_Address(unsigned int BluetoothStackID, BD_ADDR_t BD_ADDR)
{
   int          ret_val;
   Byte_t       Length;
   Byte_t       Status;
   Byte_t       ReturnBuffer[8];
   Byte_t       OGF;
   Word_t       OCF;
   unsigned int Index;

   /* Before continuing, make sure the input parameters appear to be    */
   /* semi-valid.                                                       */
   if(BluetoothStackID)
   {
      /* The TI chipset expects the BD_ADDR to be in big endian format  */
      /* so convert from little-endian since BD_ADDR_t is in little     */
      /* endian format.                                                 */
      for(Index=0;Index<(BD_ADDR_SIZE/2);Index++)
      {
         Length                                     = ((Byte_t *)&BD_ADDR)[BD_ADDR_SIZE-Index-1];
         ((Byte_t *)&BD_ADDR)[BD_ADDR_SIZE-Index-1] = ((Byte_t *)&BD_ADDR)[Index];
         ((Byte_t *)&BD_ADDR)[Index]                = Length;
      }

      /* Send the command to set the sleep mode configuration.          */
      Length  = sizeof(ReturnBuffer);
      OGF     = VS_COMMAND_OGF(VS_WRITE_BD_ADDRESS_COMMAND_OPCODE);
      OCF     = VS_COMMAND_OCF(VS_WRITE_BD_ADDRESS_COMMAND_OPCODE);

      ret_val = HCI_Send_Raw_Command(BluetoothStackID, OGF, OCF, BD_ADDR_SIZE, (Byte_t *)(&BD_ADDR), &Status, &Length, ReturnBuffer, TRUE);

      /* Map the return results into an error code.                     */
      ret_val = MapSendRawResults(ret_val, Status, Length, ReturnBuffer);
   }
   else
      ret_val = BTPS_ERROR_INVALID_PARAMETER;

   /* Return the result the caller.                                     */
   return(ret_val);
}

   /* The following function sends the VS DDIP command in order to      */
   /* change ACL priority over scans.  (Data During Inquiry Page) Send  */
   /* the command when A2DP is streaming for better quality The two     */
   /* parameters are the Best-effort percentage and the Guaranteed      */
   /* percentage when doing scans with ACL traffic Inputs: 1- The       */
   /* BluetoothStackID. 2- Best-effort percentage.  3- Guaranteed       */
   /* percentage.  This function returns zero if successful or a        */
   /* negative return error code if there was an error.                 */
int BTPSAPI VS_Send_DDIP(unsigned int BluetoothStackID, unsigned int BestEffortPercentage, unsigned int GuaranteedPercentage)
{
   int    Result;
   Word_t OCF;
   Byte_t OGF;
   Byte_t CommandLength;
   Byte_t Length;
   Byte_t Status;
   Byte_t CommandBuffer[] = {0x00, 0x00, 0x02, 0x07, 0x02, 0x01, 0x01, 0xFF, 0xFF};

   /* The first 2 DDIP vendor specific parameters (the first 2 bytes)   */
   /* are important and are set below, the remaining use the default    */
   /* values set above.                                                 */
   CommandBuffer[0] = BestEffortPercentage;
   CommandBuffer[1] = GuaranteedPercentage;

   Length        = sizeof(CommandBuffer);
   CommandLength = sizeof(CommandBuffer);
   OGF           = VS_COMMAND_OGF(VS_CONFIGURE_DDIP_COMMAND_OPCODE);
   OCF           = VS_COMMAND_OCF(VS_CONFIGURE_DDIP_COMMAND_OPCODE);
   Result        = HCI_Send_Raw_Command(BluetoothStackID, OGF, OCF, CommandLength, CommandBuffer, &Status, &Length, CommandBuffer, TRUE);

   return(MapSendRawResults(Result, Status, Length, CommandBuffer));
}

   /* The following function enables or disable Bluetooth Low Energy on */
   /* the controller.  This function returns zero if successful or a    */
   /* negative value if an error occurred.                              */
int BTPSAPI VS_LE_Enable(unsigned int BluetoothStackID, Boolean_t Enable, Boolean_t LoadCode)
{
   int    Result;
   Word_t OCF;
   Byte_t OGF;
   Byte_t CommandLength;
   Byte_t Length;
   Byte_t Status;
   Byte_t CommandBuffer[sizeof(Enable) + sizeof(LoadCode)];

   CommandBuffer[0] = Enable ? 1 : 0;
   CommandBuffer[1] = LoadCode ? 1 : 0;

   Length        = sizeof(CommandBuffer);
   CommandLength = sizeof(CommandBuffer);
   OGF           = VS_COMMAND_OGF(VS_LE_ENABLE);
   OCF           = VS_COMMAND_OCF(VS_LE_ENABLE);
   Result        = HCI_Send_Raw_Command(BluetoothStackID, OGF, OCF, CommandLength, CommandBuffer, &Status, &Length, CommandBuffer, TRUE);

   return(MapSendRawResults(Result, Status, Length, CommandBuffer));
}

   /* The following function prototype represents the vendor specific   */
   /* function which is used to change the output power for the Local   */
   /* Bluetooth Device specified by the Bluetooth Protocol Stack that   */
   /* is specified by the Bluetooth Protocol Stack ID. The second       */
   /* parameter is the max output power to set. This function returns   */
   /* zero if successful or a negative return error code if there was   */
   /* an error.                                                         */
   /* * NOTE * The maximum output power is specified from 0 to 12 and   */
   /*          it specifies 4 dBm steps.                                */
int BTPSAPI VS_Set_Max_Output_Power(unsigned int BluetoothStackID, Byte_t MaxPower)
{
   int          ret_val;
   int          PowerVector;
   int          FourDbmDifference;
   int          SmallestDifference;
   Byte_t       Length;
   Byte_t       Status;
   Byte_t       ReturnBuffer[8];
   Byte_t       CommandBuffer[POWER_VECTOR_COMMAND_SIZE];
   Byte_t       OGF;
   Word_t       OCF;
   unsigned int Class2PowerLevel[3];
   unsigned int Index;
   unsigned int FormatIndex;

   /* Before continuing, make sure the input parameters appear to be    */
   /* semi-valid.                                                       */
   if((BluetoothStackID) && (MaxPower <= 12))
   {
      /* Send the HCI_DRPb_Set_Power_Vector commands for each           */
      /* modulation type.                                               */
      for(Index = 0; Index < 3; Index++)
      {
         /* For EDR2 & EDR3 cap the max power at 10 dBm.                */
         if((Index >= 1) && (MaxPower > 10))
            MaxPower = 10;

         /* Format the Command.                                         */
         ASSIGN_HOST_BYTE_TO_LITTLE_ENDIAN_UNALIGNED_BYTE(&CommandBuffer[0], (Byte_t)(Index));
         ASSIGN_HOST_BYTE_TO_LITTLE_ENDIAN_UNALIGNED_BYTE(&CommandBuffer[1], (Byte_t)(-50 * 2));

         /* Format the data that is dependent on the requested max      */
         /* power.                                                      */

         /* Set the smallest difference equal to the max power - 4 dBm. */
         SmallestDifference      = ((MaxPower - 4) >= 0) ? (MaxPower - 4):(-(MaxPower - 4));
         Class2PowerLevel[Index] = POWER_VECTOR_SIZE;
         for(FormatIndex = 0; FormatIndex < POWER_VECTOR_SIZE; FormatIndex++)
         {
            if(FormatIndex < (POWER_VECTOR_SIZE - POWER_VECTOR_STEP_SIZE))
            {
               /* We are only going to program the chip to use          */
               /* POWER_VECTOR_STEP_SIZE discrete power levels, thus    */
               /* for 12 dBm max power we have 12, 7,2,-3,-8 levels     */
               /* so the first 10 slots will be -8.                     */
               PowerVector       =  (((int)(MaxPower)) - (5 * (POWER_VECTOR_STEP_SIZE - 1)));
               FourDbmDifference = ((PowerVector - 4) >= 0)? (PowerVector - 4): (-(PowerVector - 4));
            }
            else
            {
               /* Calculate the vector in the next region.              */
               PowerVector       = ((int)(MaxPower)) - (((POWER_VECTOR_SIZE - FormatIndex)-1)*5);
               FourDbmDifference = ((PowerVector - 4) >= 0)? (PowerVector - 4): (-(PowerVector - 4));
            }

            /* If the current difference for the current vector is      */
            /* smaller than the current difference then we should save  */
            /* the index of this level for usage in the Set Class2      */
            /* Single Power command.                                    */
            if(FourDbmDifference < SmallestDifference)
            {
               SmallestDifference      = FourDbmDifference;
               Class2PowerLevel[Index] = FormatIndex + 1;
            }

            /* Assign the Power Vector into the command.                */
            if((Index == 0) && (FormatIndex == 0))
            {
               ASSIGN_HOST_BYTE_TO_LITTLE_ENDIAN_UNALIGNED_BYTE(&CommandBuffer[2 + FormatIndex], (Byte_t)(MaxPower * 2));
            }
            else
            {
               ASSIGN_HOST_BYTE_TO_LITTLE_ENDIAN_UNALIGNED_BYTE(&CommandBuffer[2 + FormatIndex], (Byte_t)(PowerVector * 2));
            }
         }

         ASSIGN_HOST_BYTE_TO_LITTLE_ENDIAN_UNALIGNED_BYTE(&CommandBuffer[FormatIndex + 2], (Byte_t)(0xFF));
         ASSIGN_HOST_WORD_TO_LITTLE_ENDIAN_UNALIGNED_WORD(&CommandBuffer[FormatIndex + 3], 0);

         /* Now actually send the command now that it is formatted.     */
         Length  = sizeof(ReturnBuffer);
         OGF     = VS_COMMAND_OGF(VS_DRPB_SET_POWER_VECTOR_COMMAND_OPCODE);
         OCF     = VS_COMMAND_OCF(VS_DRPB_SET_POWER_VECTOR_COMMAND_OPCODE);

         ret_val = HCI_Send_Raw_Command(BluetoothStackID, OGF, OCF, sizeof(CommandBuffer), (Byte_t *)(CommandBuffer), &Status, &Length, ReturnBuffer, TRUE);

         /* Map the return results into an error code.                  */
         if((ret_val = MapSendRawResults(ret_val, Status, Length, ReturnBuffer)) < 0)
         {
            /* If Send Raw returns an error return, or an error status  */
            /* OR the chip returns less than the expected number of     */
            /* bytes OR the chip returns a Non-Zero status              */
            /* (ReturnBuffer[0] will contain the command status from the*/
            /* HCI Command Complete Event) we should return an error    */
            /* code.                                                    */
            break;
         }
      }

      /* Only continue if no error occurred.                            */
      if(!ret_val)
      {
         /* Format the VS_DRPb_Set_Class2_Single_Power Command.         */
         ASSIGN_HOST_BYTE_TO_LITTLE_ENDIAN_UNALIGNED_BYTE(&CommandBuffer[0],(Byte_t)(Class2PowerLevel[0]));
         ASSIGN_HOST_BYTE_TO_LITTLE_ENDIAN_UNALIGNED_BYTE(&CommandBuffer[1],(Byte_t)(Class2PowerLevel[1]));
         ASSIGN_HOST_BYTE_TO_LITTLE_ENDIAN_UNALIGNED_BYTE(&CommandBuffer[2],(Byte_t)(Class2PowerLevel[2]));

         /* Send the VS_DRPb_Set_Class2_Single_Power Vendor Specific    */
         /* Command.                                                    */
         Length  = sizeof(ReturnBuffer);
         OGF     = VS_COMMAND_OGF(VS_DRPB_SET_SET_CLASS2_SINGLE_POWER_COMMAND_OPCODE);
         OCF     = VS_COMMAND_OCF(VS_DRPB_SET_SET_CLASS2_SINGLE_POWER_COMMAND_OPCODE);

         ret_val = HCI_Send_Raw_Command(BluetoothStackID, OGF, OCF, 3, (Byte_t *)(CommandBuffer), &Status, &Length, ReturnBuffer, TRUE);
         if((ret_val = MapSendRawResults(ret_val, Status, Length, ReturnBuffer)) == 0)
         {
            /* Send the VS_DRPb_Set_RF_Calibration_Info Vendor Specific */
            /* Command.                                                 */
            Length  = sizeof(ReturnBuffer);
            OGF     = VS_COMMAND_OGF(VS_DRPB_SET_SET_RF_CALIBRATION_INFO_COMMAND_OPCODE);
            OCF     = VS_COMMAND_OCF(VS_DRPB_SET_SET_RF_CALIBRATION_INFO_COMMAND_OPCODE);

            ret_val = HCI_Send_Raw_Command(BluetoothStackID, OGF, OCF, sizeof(RF_Calibration_Info), (Byte_t *)(RF_Calibration_Info), &Status, &Length, ReturnBuffer, TRUE);
            if((ret_val = MapSendRawResults(ret_val, Status, Length, ReturnBuffer)) == 0)
            {
               /* Send the first of two VS_DRPb_Enable_Calibration      */
               /* Vendor Specific Commands.                             */
               ASSIGN_HOST_BYTE_TO_LITTLE_ENDIAN_UNALIGNED_BYTE(&CommandBuffer[0],0x00);
               ASSIGN_HOST_DWORD_TO_LITTLE_ENDIAN_UNALIGNED_DWORD(&CommandBuffer[1],0x00000001);
               ASSIGN_HOST_BYTE_TO_LITTLE_ENDIAN_UNALIGNED_BYTE(&CommandBuffer[5],0x01);

               Length  = sizeof(ReturnBuffer);
               OGF     = VS_COMMAND_OGF(VS_DRPB_SET_ENABLE_RF_CALIBRATION_COMMAND_OPCODE);
               OCF     = VS_COMMAND_OCF(VS_DRPB_SET_ENABLE_RF_CALIBRATION_COMMAND_OPCODE);

               ret_val = HCI_Send_Raw_Command(BluetoothStackID, OGF, OCF, 6, (Byte_t *)(CommandBuffer), &Status, &Length, ReturnBuffer, TRUE);
               if((ret_val = MapSendRawResults(ret_val, Status, Length, ReturnBuffer)) == 0)
               {
                  /* Send the second of two VS_DRPb_Enable_ Calibration */
                  /* Vendor Specific Commands.                          */
                  ASSIGN_HOST_BYTE_TO_LITTLE_ENDIAN_UNALIGNED_BYTE(&CommandBuffer[0],60);
                  ASSIGN_HOST_DWORD_TO_LITTLE_ENDIAN_UNALIGNED_DWORD(&CommandBuffer[1],0x00005ff0);
                  ASSIGN_HOST_BYTE_TO_LITTLE_ENDIAN_UNALIGNED_BYTE(&CommandBuffer[5],0x00);

                  Length  = sizeof(ReturnBuffer);
                  OGF     = VS_COMMAND_OGF(VS_DRPB_SET_ENABLE_RF_CALIBRATION_COMMAND_OPCODE);
                  OCF     = VS_COMMAND_OCF(VS_DRPB_SET_ENABLE_RF_CALIBRATION_COMMAND_OPCODE);

                  ret_val = HCI_Send_Raw_Command(BluetoothStackID, OGF, OCF, 6, (Byte_t *)(CommandBuffer), &Status, &Length, ReturnBuffer, TRUE);

                  /* Map the Send Raw Results.                          */
                  ret_val = MapSendRawResults(ret_val, Status, Length, ReturnBuffer);
               }
            }
         }
      }
   }
   else
      ret_val = BTPS_ERROR_INVALID_PARAMETER;

   /* Return the result the caller.                                     */
   return(ret_val);
}

   /* The following function prototype represents the function which is */
   /* used to put the Local Bluetooth Device specified by the Bluetooth */
   /* Protocol Stack that is specified by the Bluetooth Protocol Stack  */
   /* ID into RF Signal Test Mode.  This function returns zero if       */
   /* successful or a negative return error code if there was an error. */
   /* * NOTE * Once the Local Bluetooth Device is in RF Signal Test Mode*/
   /*          it will remain in this mode until an HCI Reset is issued.*/
int BTPSAPI VS_Enable_RF_SIG_Test_Mode(unsigned int BluetoothStackID)
{
   int         ret_val;
   Byte_t      StatusResult;
   Condition_t ConditionStructure;

   /* Before continuing, make sure the input parameter appears to be    */
   /* semi-valid.                                                       */
   if(BluetoothStackID)
   {
      /* Send HCI Write Scan Enable Command.                            */
      if((!HCI_Write_Scan_Enable(BluetoothStackID, HCI_SCAN_ENABLE_INQUIRY_SCAN_ENABLED_PAGE_SCAN_ENABLED, &StatusResult)) && (StatusResult == HCI_ERROR_CODE_NO_ERROR))
      {
         /* Configure and send the HCI Set Event Filter Command.        */
         ConditionStructure.Condition.Connection_Setup_Filter_Type_All_Devices_Condition.Auto_Accept_Flag = HCI_AUTO_ACCEPT_FLAG_DO_AUTO_ACCEPT;

         if((!HCI_Set_Event_Filter(BluetoothStackID, HCI_FILTER_TYPE_CONNECTION_SETUP, HCI_FILTER_CONDITION_TYPE_CONNECTION_SETUP_ALL_DEVICES, ConditionStructure, &StatusResult)) && (StatusResult == HCI_ERROR_CODE_NO_ERROR))
         {
            /* Finally send the HCI Enable Device Under Test Command.   */
            if((!HCI_Enable_Device_Under_Test_Mode(BluetoothStackID, &StatusResult)) && (StatusResult == HCI_ERROR_CODE_NO_ERROR))
               ret_val = 0;
            else
               ret_val = BTPS_ERROR_DEVICE_HCI_ERROR;
         }
         else
            ret_val = BTPS_ERROR_DEVICE_HCI_ERROR;
      }
      else
         ret_val = BTPS_ERROR_DEVICE_HCI_ERROR;
   }
   else
      ret_val = BTPS_ERROR_INVALID_PARAMETER;

   /* Return the result the caller.                                     */
   return(ret_val);
}

   /* The following function prototype represents the vendor function   */
   /* which is used to put the specified by the Bluetooth Protocol Stack*/
   /* that is specified by the Bluetooth Protocol Stack ID into FCC Test*/
   /* Mode.  The second parameter specifies the Modulation Type to use  */
   /* in the test mode, the third parameter specifies the Test Pattern  */
   /* to transmit in the test mode, the fourth parameter specifies the  */
   /* Frequency Channel to transmit on, and the fifth parameter         */
   /* specifies the Power Level to use while transmitting.  The final   */
   /* two parameters are only used when the Test_Pattern parameter is   */
   /* set to VS_TEST_PATTERN_USER_DEFINED. This function returns zero if*/
   /* successful or a negative return error code if there was an error. */
int BTPSAPI VS_Enable_FCC_Test_Mode(unsigned int BluetoothStackID, VS_Modulation_Type_t Modulation_Type, Byte_t Test_Pattern, Byte_t Frequency_Channel, Byte_t Power_Level, DWord_t Generator_Init_Value, DWord_t EDR_Generator_Mask)
{
   int    ret_val;
   Byte_t Length;
   Byte_t Status;
   Byte_t OGF;
   Word_t OCF;
   Byte_t CommandBuffer[DRPB_TESTER_CON_TX_SIZE];
   Byte_t ReturnBuffer[1];

   /* Before continuing, make sure the input parameters appears to be   */
   /* semi-valid.                                                       */
   if((BluetoothStackID) && (Modulation_Type <= mtEDR3) && (((SByte_t)Test_Pattern) >= VS_TEST_PATTERN_PN9) && (Test_Pattern <= VS_TEST_PATTERN_USER_DEFINED) && (Frequency_Channel <= VS_MAXIMUM_BT_FREQUENCY_CHANNEL) && (Power_Level <= VS_MAXIMUM_POWER_LEVEL))
   {
      /* Format the HCI VS DRPB Tester Con Tx Command.                  */
      BTPS_MemInitialize(CommandBuffer, 0, DRPB_TESTER_CON_TX_SIZE);

      ASSIGN_HOST_BYTE_TO_LITTLE_ENDIAN_UNALIGNED_BYTE(&CommandBuffer[0], Modulation_Type);
      ASSIGN_HOST_BYTE_TO_LITTLE_ENDIAN_UNALIGNED_BYTE(&CommandBuffer[1], Test_Pattern);
      ASSIGN_HOST_BYTE_TO_LITTLE_ENDIAN_UNALIGNED_BYTE(&CommandBuffer[2], Frequency_Channel);
      ASSIGN_HOST_BYTE_TO_LITTLE_ENDIAN_UNALIGNED_BYTE(&CommandBuffer[3], Power_Level);

      if(Test_Pattern == VS_TEST_PATTERN_USER_DEFINED)
      {
         ASSIGN_HOST_DWORD_TO_LITTLE_ENDIAN_UNALIGNED_DWORD(&CommandBuffer[4], Generator_Init_Value);
         ASSIGN_HOST_DWORD_TO_LITTLE_ENDIAN_UNALIGNED_DWORD(&CommandBuffer[8], EDR_Generator_Mask);
      }

      Length  = sizeof(ReturnBuffer);
      OGF     = VS_COMMAND_OGF(VS_DRPB_TESTER_CON_TX_COMMAND_OPCODE);
      OCF     = VS_COMMAND_OCF(VS_DRPB_TESTER_CON_TX_COMMAND_OPCODE);
      ret_val = HCI_Send_Raw_Command(BluetoothStackID, OGF, OCF, DRPB_TESTER_CON_TX_SIZE, (Byte_t *)CommandBuffer, &Status, &Length, ReturnBuffer, TRUE);
      if((ret_val = MapSendRawResults(ret_val, Status, Length, ReturnBuffer)) == 0)
      {
         /* The next step is to send a HCI VS Write Hardware Register   */
         /* Command.  If the Modulation Type is mtContinuousWave we will*/
         /* skip this step.                                             */
         if(Modulation_Type != mtContinuousWave)
         {
            /* Format the HCI VS Write Hardware Register Command.       */
            ASSIGN_HOST_DWORD_TO_LITTLE_ENDIAN_UNALIGNED_DWORD(&CommandBuffer[0], 0x0019180C);
            ASSIGN_HOST_WORD_TO_LITTLE_ENDIAN_UNALIGNED_WORD(&CommandBuffer[4], 0x0101);

            Length  = sizeof(ReturnBuffer);
            OGF     = VS_COMMAND_OGF(VS_WRITE_HARDWARE_REGISTER_COMMAND_OPCODE);
            OCF     = VS_COMMAND_OCF(VS_WRITE_HARDWARE_REGISTER_COMMAND_OPCODE);
            ret_val = HCI_Send_Raw_Command(BluetoothStackID, OGF, OCF, (DWORD_SIZE+WORD_SIZE), (Byte_t *)CommandBuffer, &Status, &Length, ReturnBuffer, TRUE);

            /* Map the Send Raw Results.                                */
            ret_val = MapSendRawResults(ret_val, Status, Length, ReturnBuffer);
         }

         /* We will move to the next step only if no error has occurred.*/
         /* If no error occurred then the next step is to Send the HCI  */
         /* VS Enable RF Calibration Command.                           */
         if(!ret_val)
         {
            /* Format the HCI VS DRPB Enable RF Calibration Command.    */
            ASSIGN_HOST_BYTE_TO_LITTLE_ENDIAN_UNALIGNED_BYTE(&CommandBuffer[0], 0xFF);
            ASSIGN_HOST_DWORD_TO_LITTLE_ENDIAN_UNALIGNED_DWORD(&CommandBuffer[1], 0xFFFFFFFF);
            ASSIGN_HOST_BYTE_TO_LITTLE_ENDIAN_UNALIGNED_BYTE(&CommandBuffer[5], 0x01);

            Length  = sizeof(ReturnBuffer);
            OGF     = VS_COMMAND_OGF(VS_DRPB_SET_ENABLE_RF_CALIBRATION_COMMAND_OPCODE);
            OCF     = VS_COMMAND_OCF(VS_DRPB_SET_ENABLE_RF_CALIBRATION_COMMAND_OPCODE);
            ret_val = HCI_Send_Raw_Command(BluetoothStackID, OGF, OCF, (BYTE_SIZE+DWORD_SIZE+BYTE_SIZE), (Byte_t *)CommandBuffer, &Status, &Length, ReturnBuffer, TRUE);

            /* Map the Send Raw Results.                                */
            ret_val = MapSendRawResults(ret_val, Status, Length, ReturnBuffer);
         }
      }
   }
   else
      ret_val = BTPS_ERROR_INVALID_PARAMETER;

   /* Return the result the caller.                                     */
   return(ret_val);
}

   /* The following function is used to associate a connection handle   */
   /* with WBS audio.  This function returns zero if successful or a    */
   /* negative return error code if there was an error.                 */
   /* * NOTE * The controller's codec configuration should be set before*/
   /*          calling this function.                                   */
int BTPSAPI VS_WBS_Associate(unsigned int BluetoothStackID, Word_t ConnectionHandle)
{
   int    ret_val;
   Byte_t CommandBuffer[48];
   Byte_t Length;
   Byte_t Status;
   Byte_t OGF;
   Word_t OCF;
   Byte_t ReturnBuffer[1];

   /* Verify that the parameters that were passed in appear valid.      */
   if(HCI_CONNECTION_HANDLE_VALID_CONNECTION_HANDLE(((SWord_t)ConnectionHandle)))
   {
      ASSIGN_HOST_WORD_TO_LITTLE_ENDIAN_UNALIGNED_WORD(&CommandBuffer[0], ConnectionHandle);

      Length  = sizeof(ReturnBuffer);
      OGF     = VS_COMMAND_OGF(VS_WBS_ASSOCIATE_COMMAND_OPCODE);
      OCF     = VS_COMMAND_OCF(VS_WBS_ASSOCIATE_COMMAND_OPCODE);
      ret_val = HCI_Send_Raw_Command(BluetoothStackID, OGF, OCF, NON_ALIGNED_WORD_SIZE, CommandBuffer, &Status, &Length, ReturnBuffer, TRUE);
      ret_val = MapSendRawResults(ret_val, Status, Length, ReturnBuffer);
   }
   else
      ret_val = BTPS_ERROR_INVALID_PARAMETER;

   /* Return the result the caller.                                     */
   return(ret_val);
}

   /* The following function is used to disassociate the current WBS    */
   /* connection.  This function returns zero if successful or a        */
   /* negative return error code if there was an error.                 */
int BTPSAPI VS_WBS_Disassociate(unsigned int BluetoothStackID)
{
   int    ret_val;
   Byte_t OGF;
   Word_t OCF;
   Byte_t Length;
   Byte_t Status;
   Byte_t ReturnBuffer[1];

   Length  = sizeof(ReturnBuffer);
   OGF     = VS_COMMAND_OGF(VS_WBS_DISASSOCIATE_COMMAND_OPCODE);
   OCF     = VS_COMMAND_OCF(VS_WBS_DISASSOCIATE_COMMAND_OPCODE);
   ret_val = HCI_Send_Raw_Command(BluetoothStackID, OGF, OCF, 0, NULL, &Status, &Length, ReturnBuffer, TRUE);
   ret_val = MapSendRawResults(ret_val, Status, Length, ReturnBuffer);

   /* Return the result the caller.                                     */
   return(ret_val);
}

   /* The following function is used to write the codec configuration   */
   /* parameters.  This function returns zero if successful or a        */
   /* negative return error code if there was an error.                 */
int BTPSAPI VS_Write_Codec_Config(unsigned int BluetoothStackID, VS_Write_Codec_Config_Params_t *Params)
{
   int          ret_val;
   Byte_t       CommandBuffer[sizeof(*Params)];
   Byte_t       CommandLength;
   Word_t       OCF;
   Byte_t       OGF;
   Byte_t       Length;
   Byte_t       Status;
   unsigned int Index;

   /* Verify that the parameters that were passed in appear valid.      */
   if(Params != NULL)
   {
      BTPS_MemInitialize(CommandBuffer, 0, sizeof(CommandBuffer));

      Index = 0;

      ASSIGN_HOST_WORD_TO_LITTLE_ENDIAN_UNALIGNED_WORD(&CommandBuffer[Index], Params->PCMClockRate_KHz);
      Index += 2;

      ASSIGN_HOST_BYTE_TO_LITTLE_ENDIAN_UNALIGNED_BYTE(&CommandBuffer[Index], Params->PCMDirection_Role);
      Index += 1;

      ASSIGN_HOST_DWORD_TO_LITTLE_ENDIAN_UNALIGNED_DWORD(&CommandBuffer[Index], Params->FrameSyncFrequency_Hz);
      Index += 4;

      ASSIGN_HOST_WORD_TO_LITTLE_ENDIAN_UNALIGNED_WORD(&CommandBuffer[Index], Params->FrameSyncDutyCycle);
      Index += 2;

      ASSIGN_HOST_BYTE_TO_LITTLE_ENDIAN_UNALIGNED_BYTE(&CommandBuffer[Index], Params->FrameSyncEdge);
      Index += 1;

      ASSIGN_HOST_BYTE_TO_LITTLE_ENDIAN_UNALIGNED_BYTE(&CommandBuffer[Index], Params->FrameSyncPolarity);
      Index += 1;

      ASSIGN_HOST_BYTE_TO_LITTLE_ENDIAN_UNALIGNED_BYTE(&CommandBuffer[Index], 0);
      Index += 1;

      ASSIGN_HOST_WORD_TO_LITTLE_ENDIAN_UNALIGNED_WORD(&CommandBuffer[Index], Params->CH1DataOutSize);
      Index += 2;

      ASSIGN_HOST_WORD_TO_LITTLE_ENDIAN_UNALIGNED_WORD(&CommandBuffer[Index], Params->CH1DataOutOffset);
      Index += 2;

      ASSIGN_HOST_BYTE_TO_LITTLE_ENDIAN_UNALIGNED_BYTE(&CommandBuffer[Index], Params->CH1OutEdge);
      Index += 1;

      ASSIGN_HOST_WORD_TO_LITTLE_ENDIAN_UNALIGNED_WORD(&CommandBuffer[Index], Params->CH1DataInSize);
      Index += 2;

      ASSIGN_HOST_WORD_TO_LITTLE_ENDIAN_UNALIGNED_WORD(&CommandBuffer[Index], Params->CH1DataInOffset);
      Index += 2;

      ASSIGN_HOST_BYTE_TO_LITTLE_ENDIAN_UNALIGNED_BYTE(&CommandBuffer[Index], Params->CH1InEdge);
      Index += 1;

      ASSIGN_HOST_BYTE_TO_LITTLE_ENDIAN_UNALIGNED_BYTE(&CommandBuffer[Index], Params->FsynchMultiplier);
      Index += 1;

      ASSIGN_HOST_WORD_TO_LITTLE_ENDIAN_UNALIGNED_WORD(&CommandBuffer[Index], Params->CH2DataOutSize);
      Index += 2;

      ASSIGN_HOST_WORD_TO_LITTLE_ENDIAN_UNALIGNED_WORD(&CommandBuffer[Index], Params->CH2DataOutOffset);
      Index += 2;

      ASSIGN_HOST_BYTE_TO_LITTLE_ENDIAN_UNALIGNED_BYTE(&CommandBuffer[Index], Params->CH2OutEdge);
      Index += 1;

      ASSIGN_HOST_WORD_TO_LITTLE_ENDIAN_UNALIGNED_WORD(&CommandBuffer[Index], Params->CH2DataInSize);
      Index += 2;

      ASSIGN_HOST_WORD_TO_LITTLE_ENDIAN_UNALIGNED_WORD(&CommandBuffer[Index], Params->CH2DataInOffset);
      Index += 2;

      ASSIGN_HOST_BYTE_TO_LITTLE_ENDIAN_UNALIGNED_BYTE(&CommandBuffer[Index], Params->CH2InEdge);
      Index += 1;

      ASSIGN_HOST_BYTE_TO_LITTLE_ENDIAN_UNALIGNED_BYTE(&CommandBuffer[Index], 0);
      Index += 1;

      Length        = sizeof(CommandBuffer);
      CommandLength = Index;
      OGF           = VS_COMMAND_OGF(VS_WRITE_CODEC_CONFIG_COMMAND_OPCODE);
      OCF           = VS_COMMAND_OCF(VS_WRITE_CODEC_CONFIG_COMMAND_OPCODE);
      ret_val       = HCI_Send_Raw_Command(BluetoothStackID, OGF, OCF, CommandLength, CommandBuffer, &Status, &Length, CommandBuffer, TRUE);

      /* Map the Send Raw return results.                               */
      ret_val = MapSendRawResults(ret_val, Status, Length, CommandBuffer);
   }
   else
      ret_val = BTPS_ERROR_INVALID_PARAMETER;

   /* Return the result the caller.                                     */
   return(ret_val);
}

   /* The following function is used to write the enhanced codec        */
   /* configuration parameters.  This function returns zero if          */
   /* successful or a negative return error code if there was an error. */
int BTPSAPI VS_Write_Codec_Config_Enhanced(unsigned int BluetoothStackID, VS_Write_Codec_Config_Enhanced_Params_t *Params)
{
   int          ret_val;
   Byte_t       CommandBuffer[sizeof(*Params)];
   Byte_t       CommandLength;
   Word_t       OCF;
   Byte_t       OGF;
   Byte_t       Length;
   Byte_t       Status;
   unsigned int Index;

   /* Verify that the parameters that were passed in appear valid.      */
   if(BluetoothStackID)
   {
      BTPS_MemInitialize(CommandBuffer, 0, sizeof(CommandBuffer));

      Index = 0;

      ASSIGN_HOST_BYTE_TO_LITTLE_ENDIAN_UNALIGNED_BYTE(&CommandBuffer[Index], Params->PCMClockShutdown);
      Index += 1;

      ASSIGN_HOST_WORD_TO_LITTLE_ENDIAN_UNALIGNED_WORD(&CommandBuffer[Index], Params->PCMClockStart);
      Index += 2;

      ASSIGN_HOST_WORD_TO_LITTLE_ENDIAN_UNALIGNED_WORD(&CommandBuffer[Index], Params->PCMClockStop);
      Index += 2;

      ASSIGN_HOST_BYTE_TO_LITTLE_ENDIAN_UNALIGNED_BYTE(&CommandBuffer[Index], 0);
      Index += 1;

      ASSIGN_HOST_BYTE_TO_LITTLE_ENDIAN_UNALIGNED_BYTE(&CommandBuffer[Index], Params->CH1DataInOrder);
      Index += 1;

      ASSIGN_HOST_BYTE_TO_LITTLE_ENDIAN_UNALIGNED_BYTE(&CommandBuffer[Index], Params->CH1DataOutOrder);
      Index += 1;

      ASSIGN_HOST_BYTE_TO_LITTLE_ENDIAN_UNALIGNED_BYTE(&CommandBuffer[Index], Params->CH1DataOutMode);
      Index += 1;

      ASSIGN_HOST_BYTE_TO_LITTLE_ENDIAN_UNALIGNED_BYTE(&CommandBuffer[Index], Params->CH1DataOutDuplication);
      Index += 1;

      ASSIGN_HOST_DWORD_TO_LITTLE_ENDIAN_UNALIGNED_DWORD(&CommandBuffer[Index], Params->CH1TxDuplicateValue);
      Index += 4;

      ASSIGN_HOST_BYTE_TO_LITTLE_ENDIAN_UNALIGNED_BYTE(&CommandBuffer[Index], Params->CH1DataQuant);
      Index += 1;

      ASSIGN_HOST_BYTE_TO_LITTLE_ENDIAN_UNALIGNED_BYTE(&CommandBuffer[Index], 0);
      Index += 1;

      ASSIGN_HOST_BYTE_TO_LITTLE_ENDIAN_UNALIGNED_BYTE(&CommandBuffer[Index], Params->CH2DataInOrder);
      Index += 1;

      ASSIGN_HOST_BYTE_TO_LITTLE_ENDIAN_UNALIGNED_BYTE(&CommandBuffer[Index], Params->CH2DataOutOrder);
      Index += 1;

      ASSIGN_HOST_BYTE_TO_LITTLE_ENDIAN_UNALIGNED_BYTE(&CommandBuffer[Index], Params->CH2DataOutMode);
      Index += 1;

      ASSIGN_HOST_BYTE_TO_LITTLE_ENDIAN_UNALIGNED_BYTE(&CommandBuffer[Index], Params->CH2DataOutDuplication);
      Index += 1;

      ASSIGN_HOST_DWORD_TO_LITTLE_ENDIAN_UNALIGNED_DWORD(&CommandBuffer[Index], Params->CH2TxDuplicateValue);
      Index += 4;

      ASSIGN_HOST_BYTE_TO_LITTLE_ENDIAN_UNALIGNED_BYTE(&CommandBuffer[Index], Params->CH2DataQuant);
      Index += 1;

      ASSIGN_HOST_BYTE_TO_LITTLE_ENDIAN_UNALIGNED_BYTE(&CommandBuffer[Index], 0);
      Index += 1;

      Length        = sizeof(CommandBuffer);
      CommandLength = Index;
      OGF           = VS_COMMAND_OGF(VS_WRITE_CODEC_CONFIG_ENHANCED_COMMAND_OPCODE);
      OCF           = VS_COMMAND_OCF(VS_WRITE_CODEC_CONFIG_ENHANCED_COMMAND_OPCODE);
      ret_val       = HCI_Send_Raw_Command(BluetoothStackID, OGF, OCF, CommandLength, CommandBuffer, &Status, &Length, CommandBuffer, TRUE);

      /* Map the Send Raw return results.                               */
      ret_val = MapSendRawResults(ret_val, Status, Length, CommandBuffer);
   }
   else
      ret_val = BTPS_ERROR_INVALID_PARAMETER;

   /* Return the result the caller.                                     */
   return(ret_val);
}

   /* The following function is used to start/stop the CC256x PCM       */
   /* interface in loopback mode, The input "int Enable" is TRUE for    */
   /* loopback on.                                                      */
int BTPSAPI VS_Set_PCM_Loopback(unsigned int BluetoothStackID, int Enable)
{
   int    ret_val;
   Word_t OCF;
   Byte_t OGF;
   Byte_t Length;
   Byte_t CommandLength;
   Byte_t Status;
   Byte_t CommandBuffer[1];

   Length           = 1;
   CommandLength    = 1;
   OGF              = VS_COMMAND_OGF(VS_PCM_LOOPBACK_OPCODE);
   OCF              = VS_COMMAND_OCF(VS_PCM_LOOPBACK_OPCODE);
   CommandBuffer[0] = (Byte_t) Enable;

   /* Map the return results.                                           */
   ret_val           = HCI_Send_Raw_Command(BluetoothStackID, OGF, OCF, CommandLength, CommandBuffer, &Status, &Length, CommandBuffer, FALSE);

   /* Return the result the caller.                                     */
    return(ret_val);
}

   /* The following function is used to enable or disable the AVPR      */
   /* features.  This function returns zero if successful or a negative */
   /* return error code if there was an error.                          */
int BTPSAPI VS_AVPR_Enable(unsigned int BluetoothStackID, Boolean_t AVPREnable, Boolean_t LoadCode, Byte_t A3DPRole)
{
   int    ret_val;
   Byte_t CommandBuffer[5];
   Byte_t CommandLength;
   Word_t OCF;
   Byte_t OGF;
   Byte_t Length;
   Byte_t Status;
   Byte_t ReturnBuffer[1];

   /* Verify that the parameters that were passed in appear valid.      */
   if(BluetoothStackID)
   {
      /* Initialize the command buffer.                                 */
      BTPS_MemInitialize(CommandBuffer, 0, sizeof(CommandBuffer));

      /* Determine whether we should enable or disable AVPR.            */
      CommandBuffer[0] = (Byte_t)((AVPREnable)?1:0);
      CommandBuffer[1] = A3DPRole;
      CommandBuffer[2] = (Byte_t)((LoadCode)?1:0);

      CommandLength    = sizeof(CommandBuffer);
      Length           = sizeof(ReturnBuffer);
      OGF              = VS_COMMAND_OGF(VS_AVPR_ENABLE_COMMAND_OPCODE);
      OCF              = VS_COMMAND_OCF(VS_AVPR_ENABLE_COMMAND_OPCODE);

      ret_val          = HCI_Send_Raw_Command(BluetoothStackID, OGF, OCF, CommandLength, CommandBuffer, &Status, &Length, ReturnBuffer, TRUE);

      /* Map the Send Raw return results.                               */
      ret_val = MapSendRawResults(ret_val, Status, Length, ReturnBuffer);
   }
   else
      ret_val = BTPS_ERROR_INVALID_PARAMETER;

   /* Return the result the caller.                                     */
   return(ret_val);
}

   /* The following function is called when an A2DP connection is in the*/
   /* open state to tell the controller the L2CAP parameters of the     */
   /* channel.  This function returns zero if successful or a negative  */
   /* return error code if there was an error.                          */
int BTPSAPI VS_A3DP_Open_Stream(unsigned int BluetoothStackID, Byte_t Connection_Handle, Word_t CID, Word_t MTU)
{
   int    ret_val;
   Byte_t CommandBuffer[15];
   Byte_t CommandLength;
   Word_t OCF;
   Byte_t OGF;
   Byte_t Length;
   Byte_t Status;
   Byte_t ReturnBuffer[1];

   /* Verify that the parameters that were passed in appear valid.      */
   if((BluetoothStackID) && (Connection_Handle))
   {
      BTPS_MemInitialize(CommandBuffer, 0, sizeof(CommandBuffer));
      CommandBuffer[0] = Connection_Handle;

      /* AVDTP version parameter [0x00 - 0x03].                         */
      CommandBuffer[5] = 0x02;

      /* AVDTP payload parameter [0x30 - 0xFF].                         */
      CommandBuffer[6] = 0x65;

      ASSIGN_HOST_WORD_TO_LITTLE_ENDIAN_UNALIGNED_WORD(&CommandBuffer[1], CID);
      ASSIGN_HOST_WORD_TO_LITTLE_ENDIAN_UNALIGNED_WORD(&CommandBuffer[3], MTU);

      CommandLength = sizeof(CommandBuffer);
      Length        = sizeof(ReturnBuffer);
      OGF           = VS_COMMAND_OGF(VS_A3DP_OPEN_STREAM_COMMAND_OPCODE);
      OCF           = VS_COMMAND_OCF(VS_A3DP_OPEN_STREAM_COMMAND_OPCODE);
      ret_val       = HCI_Send_Raw_Command(BluetoothStackID, OGF, OCF, CommandLength, CommandBuffer, &Status, &Length, ReturnBuffer, TRUE);

      /* Map the Send Raw return results.                               */
      ret_val = MapSendRawResults(ret_val, Status, Length, ReturnBuffer);
   }
   else
      ret_val = BTPS_ERROR_INVALID_PARAMETER;

   /* Return the result the caller.                                     */
   return(ret_val);
}

   /* The following function is used to inform the controller when A2DP */
   /* connection closes.  This function returns zero if successful or a */
   /* negative return error code if there was an error.                 */
int BTPSAPI VS_A3DP_Close_Stream(unsigned int BluetoothStackID, Byte_t Connection_Handle)
{
   int    ret_val;
   Byte_t CommandBuffer[5];
   Byte_t CommandLength;
   Word_t OCF;
   Byte_t OGF;
   Byte_t Length;
   Byte_t Status;
   Byte_t ReturnBuffer[1];

   /* Verify that the parameters that were passed in appear valid.      */
   if((BluetoothStackID) && (Connection_Handle))
   {
      BTPS_MemInitialize(CommandBuffer, 0, sizeof(CommandBuffer));
      CommandBuffer[0] = Connection_Handle;

      CommandLength    = sizeof(CommandBuffer);
      Length           = sizeof(ReturnBuffer);
      OGF              = VS_COMMAND_OGF(VS_A3DP_CLOSE_STREAM_COMMAND_OPCODE);
      OCF              = VS_COMMAND_OCF(VS_A3DP_CLOSE_STREAM_COMMAND_OPCODE);
      ret_val          = HCI_Send_Raw_Command(BluetoothStackID, OGF, OCF, CommandLength, CommandBuffer, &Status, &Length, ReturnBuffer, TRUE);

      /* Map the Send Raw return results.                               */
      ret_val = MapSendRawResults(ret_val, Status, Length, ReturnBuffer);
   }
   else
      ret_val = BTPS_ERROR_INVALID_PARAMETER;

   /* Return the result the caller.                                     */
   return(ret_val);
}

   /* The following function is used to configure the SBC Encoder or    */
   /* Decoder parameters.  This function returns zero if successful or a*/
   /* negative return error code if there was an error.                 */
int BTPSAPI VS_A3DP_Codec_Configuration(unsigned int BluetoothStackID, Byte_t AudioFormat, Byte_t SBCFormat, Byte_t BitPoolSize)
{
   int    ret_val;
   Byte_t CommandBuffer[19];
   Byte_t CommandLength;
   Word_t OCF;
   Byte_t OGF;
   Byte_t Length;
   Byte_t Status;
   Byte_t ReturnBuffer[1];

   /* Verify that the parameters that were passed in appear valid.      */
   if(BluetoothStackID)
   {
      BTPS_MemInitialize(CommandBuffer, 0, sizeof(CommandBuffer));

      CommandBuffer[0] = AVRP_SOURCE_PCM;
      CommandBuffer[1] = (AudioFormat & AVRP_AUDIO_FORMAT_PCM_SAMPLE_RATE_MASK);
      CommandBuffer[2] = (Byte_t)(((AudioFormat & AVRP_AUDIO_FORMAT_SBC_MODE_MASK) == AVRP_AUDIO_FORMAT_SBC_MODE_MONO)?1:2);
      CommandBuffer[3] = ((AudioFormat & AVRP_AUDIO_FORMAT_SBC_SAMPLE_RATE_MASK) >> 4);
      CommandBuffer[4] = ((AudioFormat & AVRP_AUDIO_FORMAT_SBC_MODE_MASK) >> 6);

      /* Convert from 0,1,2,3 to 4,8,12,16 for the block length.        */
      CommandBuffer[5] = ((SBCFormat & AVRP_SBC_FORMAT_BLOCK_LENGTH_MASK) + 1) << 2;
      CommandBuffer[6] = 8;
      CommandBuffer[7] = ((SBCFormat & AVRP_SBC_FORMAT_ALLOCATION_METHOD_MASK) >> 2);

      /* old value used to be 12 minimum bit pool.                      */
      CommandBuffer[8] = 20;
      CommandBuffer[9] = BitPoolSize;

      CommandLength    = sizeof(CommandBuffer);
      Length           = sizeof(ReturnBuffer);
      OGF              = VS_COMMAND_OGF(VS_A3DP_CODEC_CONFIGURATION_COMMAND_OPCODE);
      OCF              = VS_COMMAND_OCF(VS_A3DP_CODEC_CONFIGURATION_COMMAND_OPCODE);
      ret_val          = HCI_Send_Raw_Command(BluetoothStackID, OGF, OCF, CommandLength, CommandBuffer, &Status, &Length, ReturnBuffer, TRUE);

      /* Map the Send Raw return results.                               */
      ret_val = MapSendRawResults(ret_val, Status, Length, ReturnBuffer);
   }
   else
      ret_val = BTPS_ERROR_INVALID_PARAMETER;

   /* Return the result the caller.                                     */
   return(ret_val);
}

   /* The following function is used to start assisted A2DP streaming.  */
   /* This function returns zero if successful or a negative return     */
   /* error code if there was an error.                                 */
int BTPSAPI VS_A3DP_Start_Stream(unsigned int BluetoothStackID, Byte_t Connection_Handle)
{
   int    ret_val;
   Byte_t CommandBuffer[5];
   Byte_t CommandLength;
   Word_t OCF;
   Byte_t OGF;
   Byte_t Length;
   Byte_t Status;
   Byte_t ReturnBuffer[1];

   /* Verify that the parameters that were passed in appear valid.      */
   if((BluetoothStackID) && (Connection_Handle))
   {
      BTPS_MemInitialize(CommandBuffer, 0, sizeof(CommandBuffer));
      CommandBuffer[0] = Connection_Handle;

      CommandLength    = sizeof(CommandBuffer);
      Length           = sizeof(ReturnBuffer);
      OGF              = VS_COMMAND_OGF(VS_A3DP_START_STREAM_COMMAND_OPCODE);
      OCF              = VS_COMMAND_OCF(VS_A3DP_START_STREAM_COMMAND_OPCODE);
      ret_val          = HCI_Send_Raw_Command(BluetoothStackID, OGF, OCF, CommandLength, CommandBuffer, &Status, &Length, ReturnBuffer, TRUE);

      /* Map the Send Raw return results.                               */
      ret_val = MapSendRawResults(ret_val, Status, Length, ReturnBuffer);
   }
   else
      ret_val = BTPS_ERROR_INVALID_PARAMETER;

   /* Return the result the caller.                                     */
   return(ret_val);
}

   /* The following function is used to stop assisted A2DP streaming.   */
   /* This function returns zero if successful or a negative return     */
   /* error code if there was an error.                                 */
int BTPSAPI VS_A3DP_Stop_Stream(unsigned int BluetoothStackID, Byte_t Connection_Handle, Byte_t Flags)
{
   int    ret_val;
   Byte_t CommandBuffer[7];
   Byte_t CommandLength;
   Word_t OCF;
   Byte_t OGF;
   Byte_t Length;
   Byte_t Status;
   Byte_t ReturnBuffer[1];

   /* Verify that the parameters that were passed in appear valid.      */
   if((BluetoothStackID) && (Connection_Handle))
   {
      BTPS_MemInitialize(CommandBuffer, 0, sizeof(CommandBuffer));
      CommandBuffer[0] = Connection_Handle;
      CommandBuffer[1] = (Byte_t)((Flags & STOP_STREAM_FLAG_FLUSH_DATA)?1:0);
      CommandBuffer[2] = (Byte_t)((Flags & STOP_STREAM_FLAG_GENERATE_STOP_EVENT)?1:0);

      CommandLength    = sizeof(CommandBuffer);
      Length           = sizeof(ReturnBuffer);
      OGF              = VS_COMMAND_OGF(VS_A3DP_STOP_STREAM_COMMAND_OPCODE);
      OCF              = VS_COMMAND_OCF(VS_A3DP_STOP_STREAM_COMMAND_OPCODE);
      ret_val          = HCI_Send_Raw_Command(BluetoothStackID, OGF, OCF, CommandLength, CommandBuffer, &Status, &Length, ReturnBuffer, TRUE);

      /* Map the Send Raw return results.                               */
      ret_val = MapSendRawResults(ret_val, Status, Length, ReturnBuffer);
   }
   else
      ret_val = BTPS_ERROR_INVALID_PARAMETER;

   /* Return the result the caller.                                     */
   return(ret_val);
}

   /* The following function is used to open a stream as an A2DP SNK    */
   /* device. This function returns zero if successful or a negative    */
   /* return error code if there was an error.                          */
int BTPSAPI VS_A3DP_Sink_Open_Stream(unsigned int BluetoothStackID, Byte_t Connection_Handle, Word_t CID)
{
   int    ret_val;
   Byte_t CommandBuffer[11];
   Byte_t CommandLength;
   Word_t OCF;
   Byte_t OGF;
   Byte_t Length;
   Byte_t Status;
   Byte_t ReturnBuffer[1];

   /* Verify that the parameters that were passed in appear valid.      */
   if((BluetoothStackID) && (Connection_Handle) && (CID))
   {
      BTPS_MemInitialize(CommandBuffer, 0, sizeof(CommandBuffer));

      CommandBuffer[0] = Connection_Handle;
      ASSIGN_HOST_WORD_TO_LITTLE_ENDIAN_UNALIGNED_WORD(&CommandBuffer[1], CID);

      CommandLength    = sizeof(CommandBuffer);
      Length           = sizeof(ReturnBuffer);
      OGF              = VS_COMMAND_OGF(VS_A3DP_SINK_OPEN_STREAM_COMMAND_OPCODE);
      OCF              = VS_COMMAND_OCF(VS_A3DP_SINK_OPEN_STREAM_COMMAND_OPCODE);
      ret_val          = HCI_Send_Raw_Command(BluetoothStackID, OGF, OCF, CommandLength, CommandBuffer, &Status, &Length, ReturnBuffer, TRUE);

      /* Map the Send Raw return results.                               */
      ret_val = MapSendRawResults(ret_val, Status, Length, ReturnBuffer);
   }
   else
      ret_val = BTPS_ERROR_INVALID_PARAMETER;

   /* Return the result the caller.                                     */
   return(ret_val);
}

   /* The following function is used to close a SNK stream previously   */
   /* opened using VS_A3DP_Sink_Open_Stream. This function returns zero */
   /* if successful or a negative return error code if there was an     */
   /* error.                                                            */
int BTPSAPI VS_A3DP_Sink_Close_Stream(unsigned int BluetoothStackID)
{
   int    ret_val;
   Byte_t CommandBuffer[4];
   Byte_t CommandLength;
   Word_t OCF;
   Byte_t OGF;
   Byte_t Length;
   Byte_t Status;
   Byte_t ReturnBuffer[1];

   /* Verify that the parameters that were passed in appear valid.      */
   if(BluetoothStackID)
   {
      BTPS_MemInitialize(CommandBuffer, 0, sizeof(CommandBuffer));

      CommandLength    = sizeof(CommandBuffer);
      Length           = sizeof(ReturnBuffer);
      OGF              = VS_COMMAND_OGF(VS_A3DP_SINK_CLOSE_STREAM_COMMAND_OPCODE);
      OCF              = VS_COMMAND_OCF(VS_A3DP_SINK_CLOSE_STREAM_COMMAND_OPCODE);
      ret_val          = HCI_Send_Raw_Command(BluetoothStackID, OGF, OCF, CommandLength, CommandBuffer, &Status, &Length, ReturnBuffer, TRUE);

      /* Map the Send Raw return results.                               */
      ret_val = MapSendRawResults(ret_val, Status, Length, ReturnBuffer);
   }
   else
      ret_val = BTPS_ERROR_INVALID_PARAMETER;

   /* Return the result the caller.                                     */
   return(ret_val);
}

   /* The following function is used to configure an A3DP device as an  */
   /* A2DP SNK, giving it PCM and SBC parameters. This configuration    */
   /* should be performed after VS_PCM_CodecConfig and VS_AVPR_Enable.  */
   /* This function returns zero if successful or a negative return     */
   /* error code if there was an error.                                 */
int BTPSAPI VS_A3DP_Sink_Codec_Configuration(unsigned int BluetoothStackID, Byte_t AudioFormat, Byte_t SBCFormat)
{
   int    ret_val;
   Byte_t CommandBuffer[18];
   Byte_t CommandLength;
   Word_t OCF;
   Byte_t OGF;
   Byte_t Length;
   Byte_t Status;
   Byte_t ReturnBuffer[1];

   /* Verify that the parameters that were passed in appear valid.      */
   if(BluetoothStackID)
   {
      BTPS_MemInitialize(CommandBuffer, 0, sizeof(CommandBuffer));

      CommandBuffer[0] = (Byte_t)(((AudioFormat & AVRP_AUDIO_FORMAT_SBC_MODE_MASK) == AVRP_AUDIO_FORMAT_SBC_MODE_MONO)?1:2);
      CommandBuffer[1] = ((AudioFormat & AVRP_AUDIO_FORMAT_SBC_SAMPLE_RATE_MASK) >> 4);
      CommandBuffer[2] = ((AudioFormat & AVRP_AUDIO_FORMAT_SBC_MODE_MASK) >> 6);
      CommandBuffer[3] = ((SBCFormat & AVRP_SBC_FORMAT_BLOCK_LENGTH_MASK) + 1) << 2;
      CommandBuffer[4] = ((SBCFormat & AVRP_SBC_FORMAT_SUBBANDS_MASK) == AVRP_SBC_FORMAT_SUBBANDS_4) ? 4 : 8;
      CommandBuffer[5] = ((SBCFormat & AVRP_SBC_FORMAT_ALLOCATION_METHOD_MASK) >> 2);

      CommandLength    = sizeof(CommandBuffer);
      Length           = sizeof(ReturnBuffer);
      OGF              = VS_COMMAND_OGF(VS_A3DP_SINK_CODEC_CONFIGURATION_COMMAND_OPCODE);
      OCF              = VS_COMMAND_OCF(VS_A3DP_SINK_CODEC_CONFIGURATION_COMMAND_OPCODE);
      ret_val          = HCI_Send_Raw_Command(BluetoothStackID, OGF, OCF, CommandLength, CommandBuffer, &Status, &Length, ReturnBuffer, TRUE);

      /* Map the Send Raw return results.                               */
      ret_val = MapSendRawResults(ret_val, Status, Length, ReturnBuffer);
   }
   else
      ret_val = BTPS_ERROR_INVALID_PARAMETER;

   /* Return the result the caller.                                     */
   return(ret_val);
}

   /* The following function is used to change a stream previously      */
   /* opened via VS_A3DP_Sink_Open_Stream to the "Playing" state. This  */
   /* function returns zero if successful or a negative return error    */
   /* code if there was an error.                                       */
int BTPSAPI VS_A3DP_Sink_Start_Stream(unsigned int BluetoothStackID)
{
   int    ret_val;
   Byte_t CommandBuffer[4];
   Byte_t CommandLength;
   Word_t OCF;
   Byte_t OGF;
   Byte_t Length;
   Byte_t Status;
   Byte_t ReturnBuffer[1];

   /* Verify that the parameters that were passed in appear valid.      */
   if(BluetoothStackID)
   {
      BTPS_MemInitialize(CommandBuffer, 0, sizeof(CommandBuffer));

      CommandLength    = sizeof(CommandBuffer);
      Length           = sizeof(ReturnBuffer);
      OGF              = VS_COMMAND_OGF(VS_A3DP_SINK_START_STREAM_COMMAND_OPCODE);
      OCF              = VS_COMMAND_OCF(VS_A3DP_SINK_START_STREAM_COMMAND_OPCODE);
      ret_val          = HCI_Send_Raw_Command(BluetoothStackID, OGF, OCF, CommandLength, CommandBuffer, &Status, &Length, ReturnBuffer, TRUE);

      /* Map the Send Raw return results.                               */
      ret_val = MapSendRawResults(ret_val, Status, Length, ReturnBuffer);
   }
   else
      ret_val = BTPS_ERROR_INVALID_PARAMETER;

   /* Return the result the caller.                                     */
   return(ret_val);
}

   /* The following function is used to change a stream previously      */
   /* opened via VS_A3DP_Sink_Open_stream to the "Stopped" state.  This */
   /* function returns zero if successful or a negative return error    */
   /* code if there was an error.                                       */
int BTPSAPI VS_A3DP_Sink_Stop_Stream(unsigned int BluetoothStackID)
{
   int    ret_val;
   Byte_t CommandBuffer[4];
   Byte_t CommandLength;
   Word_t OCF;
   Byte_t OGF;
   Byte_t Length;
   Byte_t Status;
   Byte_t ReturnBuffer[1];

   /* Verify that the parameters that were passed in appear valid.      */
   if(BluetoothStackID)
   {
      BTPS_MemInitialize(CommandBuffer, 0, sizeof(CommandBuffer));

      CommandLength    = sizeof(CommandBuffer);
      Length           = sizeof(ReturnBuffer);
      OGF              = VS_COMMAND_OGF(VS_A3DP_SINK_STOP_STREAM_COMMAND_OPCODE);
      OCF              = VS_COMMAND_OCF(VS_A3DP_SINK_STOP_STREAM_COMMAND_OPCODE);
      ret_val          = HCI_Send_Raw_Command(BluetoothStackID, OGF, OCF, CommandLength, CommandBuffer, &Status, &Length, ReturnBuffer, TRUE);

      /* Map the Send Raw return results.                               */
      ret_val = MapSendRawResults(ret_val, Status, Length, ReturnBuffer);
   }
   else
      ret_val = BTPS_ERROR_INVALID_PARAMETER;

   /* Return the result the caller.                                     */
   return(ret_val);
}

   /* The following function is used to enable debug messages from the  */
   /* AVPR coprocessor used in A3DP and WBS. These messages are sent on */
   /* the existing TX Debug line for the CC256X radio.                  */
   /* * NOTE * Once enabled, the radio must be reset to disable the     */
   /*          debug messages.                                          */
int BTPSAPI VS_AVPR_Debug(unsigned int BluetoothStackID)
{
   int     ret_val;
   Byte_t  Buffer[5];
   Word_t  OCF;
   Byte_t  OGF;
   Byte_t  Length;
   Byte_t  Status;

      /* Verify that the parameters that were passed in appear valid.   */
   if(BluetoothStackID)
   {
      BTPS_MemInitialize(Buffer, 0, sizeof(Buffer));

      /* Initialization parameters taken from TI script.                */
      Buffer[0] = 0x01;
      Buffer[1] = 0x00;
      Buffer[2] = 0x01;
      Buffer[3] = 0x00;
      Buffer[4] = 0x00;

      Length    = sizeof(Buffer);
      OGF       = VS_COMMAND_OGF(VS_AVPR_DEBUG_COMMAND_OPCODE);
      OCF       = VS_COMMAND_OCF(VS_AVPR_DEBUG_COMMAND_OPCODE);
      ret_val   = HCI_Send_Raw_Command(BluetoothStackID, OGF, OCF, Length, Buffer, &Status, &Length, Buffer, TRUE);

      /* Map the Send Raw return results.                               */
      ret_val = MapSendRawResults(ret_val, Status, Length, Buffer);
   }
   else
      ret_val = BTPS_ERROR_INVALID_PARAMETER;

   return(ret_val);
}

int BTPSAPI VS_A3DP_Multi_Room_Mode(unsigned int BluetoothStackID, Byte_t Enable, Byte_t Connection_Handle)
{
   int    ret_val;
   Byte_t Buffer[10];
   Word_t OCF;
   Byte_t OGF;
   Byte_t Length;
   Byte_t Status;

   /* Verify that the parameters that were passed in appear valid.      */
   if(BluetoothStackID)
   {
      BTPS_MemInitialize(Buffer, 0, sizeof(Buffer));

      Buffer[0] = Enable;
      Buffer[1] = Connection_Handle;

      Length    = sizeof(Buffer);
      OGF       = VS_COMMAND_OGF(VS_A3DP_MULTI_ROOM_MODE_COMMAND_OPCODE);
      OCF       = VS_COMMAND_OCF(VS_A3DP_MULTI_ROOM_MODE_COMMAND_OPCODE);
      ret_val   = HCI_Send_Raw_Command(BluetoothStackID, OGF, OCF, Length, Buffer, &Status, &Length, Buffer, TRUE);

      /* Map the Send Raw return results.                               */
      ret_val = MapSendRawResults(ret_val, Status, Length, Buffer);
   }
   else
      ret_val = BTPS_ERROR_INVALID_PARAMETER;

   return(ret_val);
}
