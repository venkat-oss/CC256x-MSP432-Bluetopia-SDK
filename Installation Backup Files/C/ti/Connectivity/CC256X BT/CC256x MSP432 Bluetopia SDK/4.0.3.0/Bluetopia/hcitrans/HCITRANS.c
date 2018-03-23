/*****< hcitrans.c >***********************************************************/
/*      Copyright 2000 - 2014 Stonestreet One.                                */
/*      All Rights Reserved.                                                  */
/*                                                                            */
/*      Copyright 2015 Texas Instruments Incorporated.                        */
/*      All Rights Reserved.                                                  */
/*                                                                            */
/*  HCITRANS - HCI Transport Layer for use with Bluetopia.                    */
/*                                                                            */
/*  Author:  Rory Sledge                                                      */
/*                                                                            */
/*** MODIFICATION HISTORY *****************************************************/
/*                                                                            */
/*   mm/dd/yy  F. Lastname    Description of Modification                     */
/*   --------  -----------    ------------------------------------------------*/
/*   10/25/01  R. Sledge      Initial creation.                               */
/******************************************************************************/
#include <string.h>              /* Inluded for memcpy().                     */
#include "BTPSKRNL.h"            /* Bluetooth Kernel Protoypes/Constants.     */
#include "HCITRANS.h"            /* HCI Transport Prototypes/Constants.       */
#include "HAL.h"                 /* Hardware Abstraction Layer API.           */
#include "HRDWCFG.h"             /* Hardware Configuration Constants.         */
#include "driverlib.h"           /* MSPWare Driver Library Include.           */

   /*********************************************************************/
   /* Defines, Enumerations, & Type Definitions                         */
   /*********************************************************************/

   /* The following constants define the HCI UART buffer sizes and      */
   /* thresholds.  Note that the Rx and Tx buffer sizes must be powers  */
   /* of 2 so that the buffer indexes are wrapped properly.             */
#define RX_BUFFER_SIZE                 128
#define TX_BUFFER_SIZE                 64

   /* The following constants define the thresholds when Rx flow is     */
   /* disabled and enabled.  When only off-limit bytes are available in */
   /* the Rx buffer, Rx flow is turned off.  When Rx flow is off and    */
   /* when on-limit bytes are available in the Rx buffer, Rx flow is    */
   /* turned back on.                                                   */
#define RX_FLOW_OFF_LIMIT              8
#define RX_FLOW_ON_LIMIT               16

   /* The following constant defines the transport ID used by this      */
   /* module.                                                           */
#define TRANSPORT_ID                   1

   /* The following macros are used disable and enable interrupts.  They*/
   /* are defined here using compiler-intrinsic functions instead of    */
   /* using the MSPWare functions in order to prevent jumping into the  */
   /* functions and to save time, which will reduce the amount of time  */
   /* that ISRs are disabled allowing us to support a higher UART baud  */
   /* rate.                                                             */
#define _CPU_cpsid()                   __disable_irq();
#define _CPU_cpsie()                   __enable_irq()

   /* The following macros are used to turn Rx flow on or off by setting*/
   /* RTS low or high, respectively.  Note that the Bluetooth controller*/
   /* will not send us data while Rx flow is off, i.e. it will not send */
   /* us data while RTS is high.                                        */
#define RX_FLOW_OFF()                  (HRDWCFG_HCI_RTS_PORT_OUT |= HRDWCFG_HCI_RTS_PIN_NUM)
#define RX_FLOW_ON()                   (HRDWCFG_HCI_RTS_PORT_OUT &= ~HRDWCFG_HCI_RTS_PIN_NUM)

   /* The following macros return true or false depending on if Rx flow */
   /* is on or off.                                                     */
#define IS_RX_FLOW_OFF()               (HRDWCFG_HCI_RTS_PORT_OUT & HRDWCFG_HCI_RTS_PIN_NUM)
#define IS_RX_FLOW_ON()                (!(IS_RX_FLOW_OFF()))

   /* The following constants define status flags which are used to     */
   /* track the state of this module.                                   */
#define STATUS_FLAG_OPEN               (1 << 0)
#define STATUS_FLAG_UART_SUSPENDED     (1 << 1)
#define STATUS_FLAG_HCILL_CTS_WAKEUP   (1 << 2)

   /* The following structure contains variables used by this module.   */
   /* Variables that can modified by ISRs, and are also accessed outside*/
   /* of the ISRs in which they are modified, are declared as volatile. */
typedef struct _tagUartContext_t
{
   volatile unsigned char RxBuffer[RX_BUFFER_SIZE];
   volatile unsigned int  RxBytesFree;
   unsigned int           RxInIndex;
   unsigned int           RxOutIndex;
   volatile unsigned char TxBuffer[TX_BUFFER_SIZE];
   volatile unsigned int  TxBytesFree;
   unsigned int           TxInIndex;
   unsigned int           TxOutIndex;
   volatile unsigned int  EUSCIStatus;
   volatile unsigned int  StatusFlags;
   volatile unsigned int  ErrorFlags;
} UartContext_t;

   /*********************************************************************/
   /* Local/Static Variables                                            */
   /*********************************************************************/

   /* Note that all variables declared static are initialized to 0      */
   /* automatically by the compiler as part of standard C/C++.          */

   /* UART context variable.                                            */
static UartContext_t UartContext;

   /* COM Data Callback Function and Callback Parameter information.    */
static HCITR_COMDataCallback_t _COMDataCallback;
static unsigned long _COMCallbackParameter;

   /*********************************************************************/
   /* Global/Non-Static Functions                                       */
   /*********************************************************************/

   /* The following function is responsible for opening the HCI         */
   /* Transport layer that will be used by Bluetopia to send and receive*/
   /* COM (Serial) data.  This function must be successfully issued in  */
   /* order for Bluetopia to function.  This function accepts as its    */
   /* parameter the HCI COM Transport COM Information that is to be used*/
   /* to open the port.  The final two parameters specify the HCI       */
   /* Transport Data Callback and Callback Parameter (respectively) that*/
   /* is to be called when data is received from the UART.  A successful*/
   /* call to this function will return a non-zero, positive value which*/
   /* specifies the HCITransportID that is used with the remaining      */
   /* transport functions in this module.  This function returns a      */
   /* negative return value to signify an error.                        */
int BTPSAPI HCITR_COMOpen(HCI_COMMDriverInformation_t *COMMDriverInformation, HCITR_COMDataCallback_t COMDataCallback, unsigned long CallbackParameter)
{
   int ret_val;

   /* First, make sure that the port is not already open and make sure  */
   /* that valid COMM Driver Information was specified.                 */
   if((!(UartContext.StatusFlags & STATUS_FLAG_OPEN)) && (COMMDriverInformation) && (COMDataCallback))
   {
      /* Initialize the return value for success.                       */
      ret_val = TRANSPORT_ID;

      /* Note the COM Callback information.                             */
      _COMDataCallback      = COMDataCallback;
      _COMCallbackParameter = CallbackParameter;

      /* Try to Open the port for Reading/Writing.                      */
      BTPS_MemInitialize(&UartContext, 0, sizeof(UartContext_t));

      UartContext.RxBytesFree = RX_BUFFER_SIZE;
      UartContext.TxBytesFree = TX_BUFFER_SIZE;

      /* The controller should be disabled before this function was     */
      /* called and it needs to be enabled, enable it now.              */
      HAL_EnableController();

      /* CTS is high while the controller is in reset and for several   */
      /* milliseconds after its pulled out of reset.  We cannot send    */
      /* data at this time.  We will configure the CTS interrupt to be  */
      /* high to low active to detect when the pin goes low.            */
      GPIO_interruptEdgeSelect(HRDWCFG_HCI_CTS_PORT_NUM, HRDWCFG_HCI_CTS_PIN_NUM, GPIO_HIGH_TO_LOW_TRANSITION);

      /* Clear the interrupt flag in case that it was set when we       */
      /* enabled the high-to-low transition interrupt.                  */
      GPIO_clearInterruptFlag(HRDWCFG_HCI_CTS_PORT_NUM, HRDWCFG_HCI_CTS_PIN_NUM);

      /* We have finished configuring the CTS interrupt, enable the     */
      /* interrupt.                                                     */
      GPIO_enableInterrupt(HRDWCFG_HCI_CTS_PORT_NUM, HRDWCFG_HCI_CTS_PIN_NUM);

      /* Initialize our status flags.  Note that this variable is also  */
      /* modified by the CTS interrupt, but we will have plenty of time */
      /* to set the variable before the interrupt gets triggered, so we */
      /* don't disable the interrupt here.                              */
      UartContext.StatusFlags = STATUS_FLAG_OPEN;

      /* Bring RTS low to indicate that we are ready to receive.        */
      RX_FLOW_ON();
   }
   else
   {
      ret_val = HCITR_ERROR_UNABLE_TO_OPEN_TRANSPORT;
   }

   return(ret_val);
}

   /* The following function is responsible for closing the the specific*/
   /* HCI Transport layer that was opened via a successful call to the  */
   /* HCITR_COMOpen() function (specified by the first parameter).      */
   /* Bluetopia makes a call to this function whenever an either        */
   /* Bluetopia is closed, or an error occurs during initialization and */
   /* the driver has been opened (and ONLY in this case).  Once this    */
   /* function completes, the transport layer that was closed will no   */
   /* longer process received data until the transport layer is         */
   /* Re-Opened by calling the HCITR_COMOpen() function.                */
   /* * NOTE * This function *MUST* close the specified COM Port.       */
   /*          This module will then call the registered COM Data       */
   /*          Callback function with zero as the data length and NULL  */
   /*          as the data pointer.  This will signify to the HCI       */
   /*          Driver that this module is completely finished with the  */
   /*          port and information and (more importantly) that NO      */
   /*          further data callbacks will be issued.  In other words   */
   /*          the very last data callback that is issued from this     */
   /*          module *MUST* be a data callback specifying zero and NULL*/
   /*          for the data length and data buffer (respectively).      */
void BTPSAPI HCITR_COMClose(unsigned int HCITransportID)
{
   HCITR_COMDataCallback_t COMDataCallback;
   unsigned long           CallbackParameter;

   /* Check to make sure that the specified Transport ID is valid.      */
   if((HCITransportID == TRANSPORT_ID) && (UartContext.StatusFlags & STATUS_FLAG_OPEN))
   {
      /* Input parameters appear to be valid, disable the controller.   */
      HAL_DisableController();

      /* Clear the UART's context flags.                                */
      UartContext.StatusFlags = 0;

      /* Note the Callback information.                                 */
      COMDataCallback   = _COMDataCallback;
      CallbackParameter = _COMCallbackParameter;

      /* Flag that there is no callback information present.            */
      _COMDataCallback      = NULL;
      _COMCallbackParameter = 0;

      /* All finished, perform the callback to let the upper layer know */
      /* that this module will no longer issue data callbacks and is    */
      /* completely cleaned up.                                         */
      if(COMDataCallback)
         (*COMDataCallback)(HCITransportID, 0, NULL, CallbackParameter);
   }
}

   /* The following function is responsible for instructing the         */
   /* specified HCI Transport layer (first parameter) that was opened   */
   /* via a successful call to the HCITR_COMOpen() function to          */
   /* reconfigure itself with the specified information.  This          */
   /* information is completely opaque to the upper layers and is passed*/
   /* through the HCI Driver layer to the transport untouched.  It is   */
   /* the responsibility of the HCI Transport driver writer to define   */
   /* the contents of this member (or completely ignore it).            */
   /* * NOTE * This function does not close the HCI Transport specified */
   /*          by HCI Transport ID, it merely reconfigures the          */
   /*          transport.  This means that the HCI Transport specified  */
   /*          by HCI Transport ID is still valid until it is closed    */
   /*          via the HCI_COMClose() function.                         */
void BTPSAPI HCITR_COMReconfigure(unsigned int HCITransportID, HCI_Driver_Reconfigure_Data_t *DriverReconfigureData)
{
   HCI_COMMReconfigureInformation_t *ReconfigureInformation;

   /* Check to make sure that the specified Transport ID is valid.      */
   if((HCITransportID == TRANSPORT_ID) && (UartContext.StatusFlags & STATUS_FLAG_OPEN) && (DriverReconfigureData))
   {
      switch(DriverReconfigureData->ReconfigureCommand)
      {
         case HCI_COMM_DRIVER_RECONFIGURE_DATA_COMMAND_CHANGE_COMM_PARAMETERS:
            if(DriverReconfigureData->ReconfigureData)
            {
               ReconfigureInformation = (HCI_COMMReconfigureInformation_t *)(DriverReconfigureData->ReconfigureData);

               if(ReconfigureInformation->ReconfigureFlags & HCI_COMM_RECONFIGURE_INFORMATION_RECONFIGURE_FLAGS_CHANGE_BAUDRATE)
               {
                  /* Turn Rx flow off so that the controller doesn't    */
                  /* send us any data while we are changing the baud    */
                  /* rate.                                              */
                  RX_FLOW_OFF();

                  /* Wait until we have finished transmitting any bytes */
                  /* in this module's transmit buffer.                  */
                  while(UART_queryStatusFlags(HRDWCFG_HCI_UART_MODULE, EUSCI_A_UART_BUSY))
                     ;

                  /* Disable interrupts.                                */
                  _CPU_cpsid();

                  /* Reconfigure the UART with the new baud rate.       */
                  HAL_EnableUART(HRDWCFG_HCI_UART_MODULE, HRDWCFG_HCI_UART_INT_NUM, ReconfigureInformation->BaudRate);

                  /* Re-enable interrupts.                              */
                  _CPU_cpsie();

                  /* Turn Rx flow on.                                   */
                  RX_FLOW_ON();
               }
            }
            break;
      }
   }
}

   /* The following function is provided to allow a mechanism for       */
   /* modules to force the processing of incoming COM Data.             */
   /* * NOTE * This function is only applicable in device stacks that   */
   /*          are non-threaded.  This function has no effect for device*/
   /*          stacks that are operating in threaded environments.      */
void BTPSAPI HCITR_COMProcess(unsigned int HCITransportID)
{
   unsigned int MaxAvailable;
   unsigned int Count;

   /* Check to make sure that the specified Transport ID is valid.      */
   if((HCITransportID == TRANSPORT_ID) && (UartContext.StatusFlags & STATUS_FLAG_OPEN))
   {
      /* Determine the number of characters that can be delivered.      */
      Count = RX_BUFFER_SIZE - UartContext.RxBytesFree;

      if(Count)
      {
         /* Determine the maximum number of bytes we can access before  */
         /* we reach the end of the buffer.                             */
         MaxAvailable = RX_BUFFER_SIZE - UartContext.RxOutIndex;

         /* Make sure we aren't going to dispatch more bytes than what  */
         /* we have available before the end of the buffer is reached.  */
         if(MaxAvailable < Count)
            Count = MaxAvailable;

         /* Call the upper layer back with the data.                    */
         if((Count) && (_COMDataCallback))
            (*_COMDataCallback)(TRANSPORT_ID, Count, (unsigned char *)(&UartContext.RxBuffer[UartContext.RxOutIndex]), _COMCallbackParameter);

         /* Adjust the Out Index and handle any looping.                */
         UartContext.RxOutIndex  = (UartContext.RxOutIndex + Count) & (~RX_BUFFER_SIZE);

         /* Disable interrupts, credit the amount that was sent to the  */
         /* upper layer, and re-enable the receive interrupt.  Note that*/
         /* we actually see slightly less time that the Rx interrupt is */
         /* disabled when using cpsid/cpsie versus disabling only the Rx*/
         /* interrupt.                                                  */
         _CPU_cpsid();
         UartContext.RxBytesFree += Count;
         _CPU_cpsie();

         /* Check if Rx flow is off and if we have reached the          */
         /* threshold when we should re-enable it.                      */
         if((IS_RX_FLOW_OFF()) && (UartContext.RxBytesFree >= RX_FLOW_ON_LIMIT) && (!(UartContext.StatusFlags & STATUS_FLAG_UART_SUSPENDED)))
         {
            /* Rx flow is off and we have reached the threshold when we */
            /* should re-enable it, we do so now.                       */
            RX_FLOW_ON();
         }
      }
   }
}

   /* The following function is responsible for actually sending data   */
   /* through the opened HCI Transport layer (specified by the first    */
   /* parameter). Bluetopia uses this function to send formatted HCI    */
   /* packets to the attached Bluetooth Device.  The second parameter to*/
   /* this function specifies the number of bytes pointed to by the     */
   /* third parameter that are to be sent to the Bluetooth Device.  This*/
   /* function returns a zero if the all data was transfered            */
   /* successfully or a negative value if an error occurred.  This      */
   /* function MUST NOT return until all of the data is sent (or an     */
   /* error condition occurs). Bluetopia WILL NOT attempt to call this  */
   /* function repeatedly if data fails to be delivered.  This function */
   /* will block until it has either buffered the specified data or sent*/
   /* all of the specified data to the Bluetooth Device.                */
   /* * NOTE * The type of data (Command, ACL, SCO, etc.) is NOT passed */
   /*          to this function because it is assumed that this         */
   /*          information is contained in the Data Stream being passed */
   /*          to this function.                                        */
int BTPSAPI HCITR_COMWrite(unsigned int HCITransportID, unsigned int Length, unsigned char *Buffer)
{
   int          ret_val = 0;
   unsigned int TempTxBytesFree;
   unsigned int Count;

   /* Check to make sure that the specified Transport ID is valid and   */
   /* the output buffer appears to be valid as well.                    */
   if((HCITransportID == TRANSPORT_ID) && (UartContext.StatusFlags & STATUS_FLAG_OPEN) && (Length) && (Buffer))
   {
      /* If the UART is suspended, resume it.                           */
      if(UartContext.StatusFlags & STATUS_FLAG_UART_SUSPENDED)
      {
         /* Flag that the UART is no longer suspended.  Note that we    */
         /* disable the CTS interrupt while we modify the status flags  */
         /* because the CTS interrupt can also modify the flags.        */
         GPIO_disableInterrupt(HRDWCFG_HCI_CTS_PORT_NUM, HRDWCFG_HCI_CTS_PIN_NUM);
         UartContext.StatusFlags &= ~STATUS_FLAG_UART_SUSPENDED;
         GPIO_enableInterrupt(HRDWCFG_HCI_CTS_PORT_NUM, HRDWCFG_HCI_CTS_PIN_NUM);

         /* Turn Rx flow on.                                            */
         RX_FLOW_ON();
      }

      /* Process all of the data.                                       */
      while(Length)
      {
         /* Loop until space becomes available in the Tx Buffer         */
         while(UartContext.TxBytesFree == 0)
            ;

         /* The data may have to be copied in 2 phases.  Calculate the  */
         /* number of character that can be placed in the buffer before */
         /* the buffer must be wrapped.                                 */
         Count = TX_BUFFER_SIZE - UartContext.TxInIndex;

         /* Save a copy of the number of free bytes in the Tx buffer.   */
         /* Note that the number of free bytes in the Tx buffer variable*/
         /* can be modified by the HCI UART ISR, and it is accessed     */
         /* twice in the statements below.  Doing this prevents each    */
         /* access below from returning a different value.              */
         TempTxBytesFree = UartContext.TxBytesFree;

         /* Make sure we don't copy over data waiting to be sent.       */
         if(TempTxBytesFree < Count)
            Count = TempTxBytesFree;

         /* Next make sure we aren't trying to copy greater than what we*/
         /* are given.                                                  */
         if(Count > Length)
            Count = Length;

         /* Copy the data.                                              */
         memcpy((void *)&(UartContext.TxBuffer[UartContext.TxInIndex]), Buffer, Count);

         /* Update the number of free bytes in the buffer.  This        */
         /* variable can also be updated in the interrupt service       */
         /* routine so we disable interrupts before accessing it.       */
         _CPU_cpsid();
         UartContext.TxBytesFree -= Count;
         _CPU_cpsie();

         /* Enable the transmit interrupt.  Note that this will have no */
         /* effect if the interrupt is already enabled, nor will it     */
         /* cause problems if the ISR has already sent all of the data  */
         /* we just added to the buffer.  In the latter case the ISR    */
         /* will simply disable the transmit interrupt when it sees     */
         /* there is no more data to send.                              */
         EUSCI_A_CMSIS(HRDWCFG_HCI_UART_MODULE)->IE |= EUSCI_A_UART_TRANSMIT_INTERRUPT;

         /* Adjust the count and index values.                          */
         Buffer                += Count;
         Length                -= Count;
         UartContext.TxInIndex  = (UartContext.TxInIndex + Count) & (~TX_BUFFER_SIZE);
      }

      /* Return success to the caller.                                  */
      ret_val = 0;
   }
   else
   {
      ret_val = HCITR_ERROR_INVALID_PARAMETER;
   }

   return(ret_val);
}

   /* The following function is responsible for suspending the HCI COM  */
   /* transport.  It will block until the transmit buffers are empty and*/
   /* all data has been sent then put the transport in a suspended      */
   /* state.  This function will return a value of 0 if the suspend was */
   /* successful or a negative value if there is an error.              */
   /* * NOTE * An error will occur if the suspend operation was         */
   /*          interrupted by another event, such as data being received*/
   /*          before the transmit buffer was empty.                    */
   /* * NOTE * The calling function must lock the Bluetooth stack with a*/
   /*          call to BSC_LockBluetoothStack() before this function is */
   /*          called.                                                  */
   /* * NOTE * This function should only be called when the baseband    */
   /*          low-power protocol in use has indicated that it is safe  */
   /*          to sleep.  Also, this function must be called            */
   /*          successfully before any clocks necessary for the         */
   /*          transport to operate are disabled.                       */
int BTPSAPI HCITR_COMSuspend(void)
{
   int          ret_val;
   unsigned int TempRxBytesFree;

   /* Verify that the UART is not currently suspended.                  */
   if(!(UartContext.StatusFlags & STATUS_FLAG_UART_SUSPENDED))
   {
      /* Save the number of free bytes in the Rx buffer.                */
      TempRxBytesFree = UartContext.RxBytesFree;

      /* Disable Rx flow.                                               */
      RX_FLOW_OFF();

      /* Flag that the RTS is asserted due to the fact that the UART is */
      /* suspended.  Note that we disable the CTS interrupt while we    */
      /* modify the status flags because the CTS interrupt can also     */
      /* modify the flags.                                              */
      GPIO_disableInterrupt(HRDWCFG_HCI_CTS_PORT_NUM, HRDWCFG_HCI_CTS_PIN_NUM);
      UartContext.StatusFlags |= STATUS_FLAG_UART_SUSPENDED;
      GPIO_enableInterrupt(HRDWCFG_HCI_CTS_PORT_NUM, HRDWCFG_HCI_CTS_PIN_NUM);

      /* Add a small delay to allow controller to process the RTS       */
      /* change.                                                        */
      BTPS_Delay(1);

      /* Wait for the UART transmit buffer and FIFO to be empty.        */
      while(((UartContext.TxBytesFree != TX_BUFFER_SIZE) || (UART_queryStatusFlags(HRDWCFG_HCI_UART_MODULE, EUSCI_A_UART_BUSY))) && (UartContext.StatusFlags & STATUS_FLAG_UART_SUSPENDED))
         ;

      /* Check if the UART is still suspended and that no UART data was */
      /* received.                                                      */
      if((UartContext.StatusFlags & STATUS_FLAG_UART_SUSPENDED) && (UartContext.RxBytesFree == TempRxBytesFree))
      {
         /* Return success to the caller.                               */
         ret_val = 0;
      }
      else
      {
         /* Re-enable Rx flow.                                          */
         RX_FLOW_ON();

         /* Data was received, abort suspending the UART.               */
         ret_val = HCITR_ERROR_SUSPEND_ABORTED;
      }
   }
   else
   {
      /* We are already suspended, check to see if we are in the process*/
      /* of being woke up by the controller.                            */
      if(!(UartContext.StatusFlags & STATUS_FLAG_HCILL_CTS_WAKEUP))
      {
         /* We are not in the process of being woke up by the           */
         /* controller, return success to the caller.                   */
         ret_val = 0;
      }
      else
      {
         /* We are in the process of being woke up by the controller,   */
         /* return an error to the caller.                              */
         ret_val = HCITR_ERROR_SUSPEND_ABORTED;
      }
   }

   /* Finally return the result to the caller.                          */
   return(ret_val);
}

   /* The following function is used to determine if the COM port is    */
   /* currently suspended.  This function returns TRUE if the COM port  */
   /* is suspended or FALSE otherwise.                                  */
Boolean_t BTPSAPI HCITR_COMSuspended(void)
{
   return((Boolean_t)((UartContext.StatusFlags & STATUS_FLAG_UART_SUSPENDED) ? TRUE : FALSE));
}

   /* The following function is used to query any errors that may have  */
   /* occurred within the HCITRANS module.  This function returns a bit */
   /* mask of the error flags.  Note that the error flags are cleared   */
   /* when this function is called.                                     */
unsigned int BTPSAPI HCITR_COMQueryErrorFlags(void)
{
   unsigned int ret_val;
   unsigned int ErrorFlags;

   /* Save a copy of the volatile error flags variable in order to      */
   /* prevent accessing a volatile variable more frequently than we need*/
   /* to, which would result in excessive memory accesses.              */
   ErrorFlags = UartContext.ErrorFlags;

   /* Check if an overrun occurred.                                     */
   if(UartContext.EUSCIStatus & EUSCI_A_UART_OVERRUN_ERROR)
   {
      /* An overrun has occurred, flag the error.                       */
      ErrorFlags |= HCITR_ERROR_FLAG_EUSCI_UART_RXBUF_OVERRUN;

      /* Clear the EUSCI status flags.                                  */
      UartContext.EUSCIStatus = 0;
   }

   /* Check to see if we have overflowed the Rx buffer.  Note that we   */
   /* don't check to see if there is room in the Rx buffer before adding*/
   /* data in the UART ISR in order to save time in the ISR. This is a  */
   /* viable solution because the Rx buffer should never overflow       */
   /* because of RTS/CTS hardware flow control.  If it does overflow    */
   /* then there could be an issue with the hardware flow control.      */
   if(UartContext.RxBytesFree > RX_BUFFER_SIZE)
   {
      /* The unsigned Rx bytes free variable has been decremented when  */
      /* it was already 0, flag the error.                              */
      ErrorFlags |= HCITR_ERROR_FLAG_UART_RX_BUFFER_OVERRUN;
   }

   /* Set the return value.                                             */
   ret_val = ErrorFlags;

   /* Check if there are any error flags set.                           */
   if(ErrorFlags != 0)
   {
      /* One or more error flags is set, reset the flags to 0.          */
      UartContext.ErrorFlags = 0;
   }

   return(ret_val);
}

   /*********************************************************************/
   /* Interrupt Service Routines                                        */
   /*********************************************************************/

   /* The following function is the Interrupt Service Routine for the   */
   /* UART RX interrupt.                                                */
   /* * NOTE * All MSPWare functions in the Rx/Tx path that could affect*/
   /*          the UART bandwidth have been replaced with the code from */
   /*          the respective function call.                            */
void HCI_UART_eUSCI_A_ISR(void)
{
   unsigned int RxBytesFree;
   uint16_t     StatusRegister;

   /* Check if the Rx interrupt flag requested the interrupt.           */
   if(EUSCI_A_CMSIS(HRDWCFG_HCI_UART_MODULE)->IFG & EUSCI_A_UART_RECEIVE_INTERRUPT_FLAG)
   {
      /* The Rx interrupt flag requested the interrupt, next read the   */
      /* status register immediately before reading the RXBUF register  */
      /* in order to check for overrun errors.                          */
      StatusRegister = EUSCI_A_CMSIS(HRDWCFG_HCI_UART_MODULE)->STATW;

      /* Read the received byte and store it in our buffer.  This code  */
      /* snippet is from UART_receiveData() is used here instead of     */
      /* calling the actual function.                                   */
      UartContext.RxBuffer[UartContext.RxInIndex] = (unsigned char)EUSCI_A_CMSIS(HRDWCFG_HCI_UART_MODULE)->RXBUF;

      /* Save the UART status for error-checking purposes.              */
      UartContext.EUSCIStatus |= (unsigned int)StatusRegister;

      /* Increment the in index, and perform an and operation to wrap it*/
      /* back to zero when it reaches the end of the buffer.            */
      UartContext.RxInIndex = (UartContext.RxInIndex + 1) & (~RX_BUFFER_SIZE);

      /* Save a copy of the volatile Rx bytes free variable so that it  */
      /* is not fetched from memory every time it is read which will    */
      /* reduce the time spent in this ISR.                             */
      RxBytesFree = UartContext.RxBytesFree;

      /* Decrement the number of free bytes in our Rx buffer.           */
      RxBytesFree -= 1;

      /* Check to see if our buffer is near full and we need to disable */
      /* Rx flow.  Note that we don't check if Rx flow is already off   */
      /* here, this is not done in order to save time in this ISR.      */
      if(RxBytesFree <= RX_FLOW_OFF_LIMIT)
      {
         /* Our Rx buffer is near full and we need to disable Rx flow in*/
         /* order to prevent buffer overrun, disable Rx flow now.       */
         RX_FLOW_OFF();
      }

      /* Save the volatile variable back to memory.                     */
      UartContext.RxBytesFree = RxBytesFree;
   }
   else
   {
      /* The transmit buffer empty interrupt has occurred.  Check if Tx */
      /* flow is enabled (CTS is low) and the Tx Buffer is not empty.   */
      if(((HRDWCFG_HCI_CTS_PORT_IN & HRDWCFG_HCI_CTS_PIN_NUM) == 0) && (UartContext.TxBytesFree != TX_BUFFER_SIZE))
      {
         /* Load the Tx buffer using the code snippet from              */
         /* UART_transmitData() instead of calling the function, in     */
         /* order to reduce the amount of time spent in this ISR.  Note */
         /* that the UCTXIFG bit is cleared automatically when we write */
         /* new data to the UART's Tx buffer and we therefore don't need*/
         /* to manually clear the flag.                                 */
         EUSCI_A_CMSIS(HRDWCFG_HCI_UART_MODULE)->TXBUF = UartContext.TxBuffer[UartContext.TxOutIndex];

         /* Increment the out index, and perform an and operation to    */
         /* wrap it back to zero when it reaches the end of the buffer. */
         UartContext.TxOutIndex = (UartContext.TxOutIndex + 1) & (~TX_BUFFER_SIZE);

         /* Increment the number of free bytes.                         */
         UartContext.TxBytesFree += 1;
      }
      else
      {
         /* Disable the transmit interrupt, note that this code snippet */
         /* is from the MSPWare UART_disableInterrupt() function.       */
         EUSCI_A_CMSIS(HRDWCFG_HCI_UART_MODULE)->IE &= ~EUSCI_A_UART_TRANSMIT_INTERRUPT;
      }
   }
}

   /* The following function is the CTS port interrupt handler.  This   */
   /* handler is called on CTS high to low and low to high transitions. */
   /* This ISR must change the interrupt polarity and flag what state   */
   /* the CTS line is in.                                               */
void CTS_Port_ISR(void)
{
   /* Check if this interrupt was caused by the CTS interrupt.          */
   if(HRDWCFG_HCI_CTS_IV == HRDWCFG_HCI_CTS_IV_NUM)
   {
      /* Disable the CTS pin interrupt before we toggle the state of the*/
      /* interrupt-edge-select bit to ensure that this ISR does not run */
      /* twice.                                                         */
      HRDWCFG_HCI_CTS_PORT_IE &= ~HRDWCFG_HCI_CTS_PIN_NUM;

      /* This interrupt was caused by the CTS interrupt, check if this  */
      /* pin is being driven low.                                       */
      if((HRDWCFG_HCI_CTS_PORT_IN & HRDWCFG_HCI_CTS_PIN_NUM) == 0)
      {
         /* This interrupt was caused by a high to low transition, next */
         /* set the CTS interrupt to positive edge (low to high).       */
         HRDWCFG_HCI_CTS_PORT_IES &= ~HRDWCFG_HCI_CTS_PIN_NUM;

         /* Check if there is any Tx data to send.                      */
         if(UartContext.TxBytesFree != TX_BUFFER_SIZE)
         {
            /* There is Tx data to send, enable the Tx interrupt.  Note */
            /* that this code snippet is based off of the MSPWare       */
            /* UART_enableInterrupt() function.                         */
            EUSCI_A_CMSIS(HRDWCFG_HCI_UART_MODULE)->IE |= EUSCI_A_UART_TRANSMIT_INTERRUPT;
         }

         /* Check to see if this is a CTS Wakeup.  If so we need to     */
         /* de-assert RTS so the controller can send a wakeup byte.     */
         if(UartContext.StatusFlags & STATUS_FLAG_HCILL_CTS_WAKEUP)
         {
            /* Clear the CTS-wakeup and UART-suspended flags.           */
            UartContext.StatusFlags &= ~(STATUS_FLAG_HCILL_CTS_WAKEUP | STATUS_FLAG_UART_SUSPENDED);

            /* Turn Rx flow on.                                         */
            RX_FLOW_ON();
         }
      }
      else
      {
         /* This interrupt was caused by a low to high transition, next */
         /* set the CTS interrupt to negative edge (high to low).       */
         HRDWCFG_HCI_CTS_PORT_IES |= HRDWCFG_HCI_CTS_PIN_NUM;

         /* Check to see what the current UART state is.  If the UART is*/
         /* currently suspended we will re-start the UART here as this  */
         /* must mean that we are in HCILL sleep mode and the Bluetooth */
         /* controller is attempting to wake us.                        */
         if(UartContext.StatusFlags & STATUS_FLAG_UART_SUSPENDED)
         {
            /* Flag that we are waiting for the CTS high-to-low         */
            /* transition.                                              */
            UartContext.StatusFlags |= STATUS_FLAG_HCILL_CTS_WAKEUP;
         }
      }

      /* Clear the interrupt flag which was cleared when we read the    */
      /* PxIV register, but was possibly set again when we modified the */
      /* interrupt edge select bit.  This is done so that this interrupt*/
      /* does not run twice on each transition of CTS.                  */
      HRDWCFG_HCI_CTS_PORT_IFG &= ~HRDWCFG_HCI_CTS_PIN_NUM;

      /* Re-enable the CTS pin interrupt.                               */
      HRDWCFG_HCI_CTS_PORT_IE |= HRDWCFG_HCI_CTS_PIN_NUM;
   }
}
