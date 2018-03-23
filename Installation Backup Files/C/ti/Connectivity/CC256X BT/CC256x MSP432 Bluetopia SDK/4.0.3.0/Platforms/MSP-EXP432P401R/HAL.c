/*****< hal.c >****************************************************************/
/*      Copyright 2010 - 2014 Stonestreet One.                                */
/*      All Rights Reserved.                                                  */
/*                                                                            */
/*      Copyright 2015 Texas Instruments Incorporated.                        */
/*      All Rights Reserved.                                                  */
/*                                                                            */
/*  HAL - Hardware Abstraction Layer implementation.                          */
/*                                                                            */
/*  Author:  Tim Cook                                                         */
/*                                                                            */
/*** MODIFICATION HISTORY *****************************************************/
/*                                                                            */
/*   mm/dd/yy  F. Lastname    Description of Modification                     */
/*   --------  -----------    ------------------------------------------------*/
/*   07/07/10  Tim Cook       Initial creation.                               */
/******************************************************************************/
#include "HAL.h"                 /* Hardware Abstraction Layer API.           */
#include "HRDWCFG.h"             /* Hardware Configuration Header.            */
#include "BTPSKRNL.h"            /* Bluetooth Protocol Stack Kernel APIs.     */
#include "SS1BTVS.h"             /* Vendor Specific Prototypes/Constants.     */
#include "driverlib.h"           /* MSPWare Driver Library Include.           */

#ifdef __SUPPORT_AUDIO_CODEC__

   #include "CC3200AUDBOOST.h"   /* CC3200 Audio Booser Pack APIs.            */

#endif

   /*********************************************************************/
   /* Defines, Enumerations, & Type Definitions                         */
   /*********************************************************************/

   /* Maximum numbered of buffered characters on the debug UART         */
   /* receiver.                                                         */
#define DEBUG_UART_RX_BUFFER_SIZE   128

   /* Maximum numbered of buffered characters on the debug UART         */
   /* transmitter.                                                      */
#define DEBUG_UART_TX_BUFFER_SIZE   128

   /* The following constants specifies the debug UART's baud rate.     */
#define DEBUG_UART_BAUD_RATE        115200

   /* The following constant specifies the high-frequency external      */
   /* oscillator's clock frequency.                                     */
#define HFXTCLK_FREQUENCY           48000000

   /* The following constants specify information about MCLK.           */
#define MCLK_SOURCE                 CS_HFXTCLK_SELECT
#define MCLK_DIVIDER                1
#define MCLK_FREQUENCY              (HFXTCLK_FREQUENCY / MCLK_DIVIDER)

   /* The following constants specify information about HSMCLK.         */
#define HSMCLK_SOURCE               CS_HFXTCLK_SELECT
#define HSMCLK_DIVIDER              2
#define HSMCLK_FREQUENCY            (HFXTCLK_FREQUENCY / HSMCLK_DIVIDER)

   /* The following constants specify information about SMCLK. Note     */
   /* that: 1) SMCLK can only be sourced by HSMCLK, 2) the MSPWare APIs */
   /* require you specify HSMCLK's clock source when configuring SMCLK, */
   /* and 3) the divider is relevant to HSMCLK's source.  Also note that*/
   /* in order to support a 2 Mbps UART baud rate SMCLK must run at 24  */
   /* MHz or faster.                                                    */
#define SMCLK_SOURCE                HSMCLK_SOURCE
#define SMCLK_DIVIDER               2
#define SMCLK_FREQUENCY             ((HSMCLK_FREQUENCY * HSMCLK_DIVIDER) / SMCLK_DIVIDER)

   /* The following macros are used to concatenate macro values.        */
#define CONCAT_(A,B)                A##B
#define CONCAT(A,B)                 CONCAT_(A,B)

   /* The following constants define interrupt priorities used by this  */
   /* module. Note that the there are 8 bits used to set the priority   */
   /* level, but only the upper 3 bits are currently used by MSP432     */
   /* MCUs, please see the MSP432 Technical Reference Manual and the    */
   /* MSP432 Driver Library's User's Guide for more information.        */
#define PRIORITY_HIGH               (0)
#define PRIORITY_NORMAL             (1 << 5)

   /*********************************************************************/
   /* Local/Static Variables                                            */
   /*********************************************************************/

   /* Note that all variables declared static are initialized to 0      */
   /* automatically by the compiler as part of standard C/C++.          */

   /* The following variable is used to hold a system tick count for the*/
   /* Bluetopia No-OS stack.                                            */
static volatile unsigned long TickCount;

   /* The following is used to store characters read from the debug UART*/
   /* into a circular buffer.                                           */
static volatile unsigned char DebugUARTRxBuffer[DEBUG_UART_RX_BUFFER_SIZE];
static unsigned int DebugUARTRxInIndex;
static unsigned int DebugUARTRxOutIndex;
static volatile unsigned int DebugUARTRxBytesFree = DEBUG_UART_RX_BUFFER_SIZE;

   /* The following is used to store characters written to the debug    */
   /* UART into a circular buffer.                                      */
static volatile unsigned char DebugUARTTxBuffer[DEBUG_UART_TX_BUFFER_SIZE];
static unsigned int DebugUARTTxInIndex;
static unsigned int DebugUARTTxOutIndex;
static volatile unsigned int DebugUARTTxBytesFree = DEBUG_UART_TX_BUFFER_SIZE;

   /* The following variables stores the current LED color.             */
static HAL_LED_Color_t CurrentLEDColor;
static Boolean_t LEDOn;

   /*  The following variable flags if the system is being initialized. */
static Boolean_t SysInit;

#if __SUPPORT_AUDIO_CODEC__

   /* The following variable is used to determine if the audio codec is */
   /* currently enabled.                                                */
static volatile Boolean_t AudioCodecEnabled;

#endif

   /*********************************************************************/
   /* Local/Static Functions                                            */
   /*********************************************************************/

#ifdef __SUPPORT_AUDIO_CODEC__

static void ConfigureControllerAudioCodec(unsigned int BluetoothStackID, unsigned long SamplingFrequency, unsigned int NumChannels)
{
   Word_t Channel1Offset;
   Word_t Channel2Offset;

   union
   {
      VS_Write_Codec_Config_Params_t          WriteCodecConfigParams;
      VS_Write_Codec_Config_Enhanced_Params_t WriteCodecConfigEnhancedParams;
   } u;

   /* Set the codec config parameters.  The PCM clock rate is set to 80 */
   /* times faster than the frame sync clock frequency in order to match*/
   /* the BCLK/WCLK ratio expected by the CC3200AUDBOOST's audio codec  */
   /* (the TLV320AIC3254).                                              */
   Channel1Offset = (NumChannels == 1) ? 17 : 1;
   Channel2Offset = Channel1Offset + 16;
   BTPS_MemInitialize(&u.WriteCodecConfigParams, 0, sizeof(u.WriteCodecConfigParams));
   u.WriteCodecConfigParams.PCMClockRate_KHz      = (SamplingFrequency / 100) * 8;
   u.WriteCodecConfigParams.FrameSyncFrequency_Hz = (DWord_t)SamplingFrequency;
   u.WriteCodecConfigParams.FrameSyncDutyCycle    = 0x0001;
   u.WriteCodecConfigParams.CH1DataOutSize        = 16;
   u.WriteCodecConfigParams.CH1DataOutOffset      = Channel1Offset;
   u.WriteCodecConfigParams.CH1DataInSize         = 16;
   u.WriteCodecConfigParams.CH1DataInOffset       = Channel1Offset;
   u.WriteCodecConfigParams.CH1InEdge             = 1;
   u.WriteCodecConfigParams.CH2DataOutSize        = 16;
   u.WriteCodecConfigParams.CH2DataOutOffset      = Channel2Offset;
   u.WriteCodecConfigParams.CH2DataInSize         = 16;
   u.WriteCodecConfigParams.CH2DataInOffset       = Channel2Offset;
   u.WriteCodecConfigParams.CH2InEdge             = 1;
   VS_Write_Codec_Config(BluetoothStackID, &u.WriteCodecConfigParams);

   /* Set the codec config enhanced parameters.                         */
   BTPS_MemInitialize(&u.WriteCodecConfigEnhancedParams, 0, sizeof(u.WriteCodecConfigEnhancedParams));
   u.WriteCodecConfigEnhancedParams.CH1DataOutMode = 0x01;
   u.WriteCodecConfigEnhancedParams.CH2DataOutMode = 0x01;
   VS_Write_Codec_Config_Enhanced(BluetoothStackID, &u.WriteCodecConfigEnhancedParams);
}

#endif

   /* The following function configures the system clocks.              */
static void ConfigureClocks(void)
{
   /* Configure the HFXT external oscillator pins.                      */
   GPIO_setAsPeripheralModuleFunctionOutputPin(GPIO_PORT_PJ, GPIO_PIN2 | GPIO_PIN3, GPIO_PRIMARY_MODULE_FUNCTION);

   /* Set the external clock source frequencies for LFXTCLK and HFXTCLK.*/
   /* Note that we haven't configured LFXTCLK so we set it to 0 here.   */
   CS_setExternalClockSourceFrequency(0, HFXTCLK_FREQUENCY);

   /* Starting HFXT in non-bypass mode without a timeout.  Before we    */
   /* start we have to change VCORE to 1 to support the 48 MHz          */
   /* frequency.                                                        */
   PCM_setCoreVoltageLevel(PCM_VCORE1);
   FlashCtl_setWaitState(FLASH_BANK0, 2);
   FlashCtl_setWaitState(FLASH_BANK1, 2);
   CS_startHFXT(false);

   /* Initialize the clock sources.                                     */
   CS_initClockSignal(CS_MCLK, MCLK_SOURCE, CONCAT(CS_CLOCK_DIVIDER_,MCLK_DIVIDER));
   CS_initClockSignal(CS_HSMCLK, HSMCLK_SOURCE, CONCAT(CS_CLOCK_DIVIDER_,HSMCLK_DIVIDER));
   CS_initClockSignal(CS_SMCLK, SMCLK_SOURCE, CONCAT(CS_CLOCK_DIVIDER_,SMCLK_DIVIDER));
}

   /*********************************************************************/
   /* Global/Non-Static Functions                                       */
   /*********************************************************************/

   /* The following function configures the microcontroller's           */
   /* peripherals and GPIOs to their default states.                    */
void HAL_ConfigureHardware(void)
{
   /* Hold the Watchdog Timer.                                          */
   WDT_A_holdTimer();

   /* Disable interrupts.                                               */
   Interrupt_disableMaster();

   /* Configure the clocks.                                             */
   ConfigureClocks();

   /* Configure the SysTick timer.                                      */
   SysTick_setPeriod((BTPS_TICK_COUNT_INTERVAL * MCLK_FREQUENCY) / 1000);
   SysTick_enableModule();
   SysTick_enableInterrupt();
   Interrupt_setPriority(FAULT_SYSTICK, PRIORITY_NORMAL);

   /* Configure the nSHUT pin, drive it low to put the radio into reset.*/
   GPIO_setOutputLowOnPin(HRDWCFG_NSHUTD_PORT_NUM, HRDWCFG_NSHUTD_PIN_NUM);
   GPIO_setAsOutputPin(HRDWCFG_NSHUTD_PORT_NUM, HRDWCFG_NSHUTD_PIN_NUM);

   /* Configure the debug UART.                                         */
   GPIO_setAsPeripheralModuleFunctionOutputPin(HRDWCFG_DEBUG_UART_TX_PORT_NUM, HRDWCFG_DEBUG_UART_TX_PIN_NUM, GPIO_PRIMARY_MODULE_FUNCTION);
   GPIO_setAsPeripheralModuleFunctionInputPin(HRDWCFG_DEBUG_UART_RX_PORT_NUM, HRDWCFG_DEBUG_UART_RX_PIN_NUM, GPIO_PRIMARY_MODULE_FUNCTION);
   HAL_EnableUART(HRDWCFG_DEBUG_UART_MODULE, HRDWCFG_DEBUG_UART_INT_NUM, DEBUG_UART_BAUD_RATE);
   Interrupt_setPriority(HRDWCFG_DEBUG_UART_INT_NUM, PRIORITY_NORMAL);

   /* Set the priority of the HCI UART.  This interrupt needs to be     */
   /* highest priority in order to prevent UART Rx overrun, this is     */
   /* especially important at faster baud rates.                        */
   Interrupt_setPriority(HRDWCFG_HCI_UART_INT_NUM, PRIORITY_HIGH);

   /* Set the priority of the CTS interrupt.                            */
   Interrupt_setPriority(HRDWCFG_HCI_CTS_INT_NUM, PRIORITY_NORMAL);

   /* Configure the audio pins as inputs to avoid contention with the   */
   /* CC3200AUDBOOST.                                                   */
   GPIO_setAsInputPinWithPullDownResistor(HRDWCFG_AUD_FSYNC_PORT_NUM, HRDWCFG_AUD_FSYNC_PIN_NUM);
   GPIO_setAsInputPinWithPullDownResistor(HRDWCFG_AUD_CLK_PORT_NUM, HRDWCFG_AUD_CLK_PIN_NUM);
   GPIO_setAsInputPinWithPullDownResistor(HRDWCFG_AUD_IN_PORT_NUM, HRDWCFG_AUD_IN_PIN_NUM);
   GPIO_setAsInputPinWithPullDownResistor(HRDWCFG_AUD_OUT_PORT_NUM, HRDWCFG_AUD_OUT_PIN_NUM);

   /* Configure the RGB LED pins.                                       */
   GPIO_setOutputLowOnPin(HRDWCFG_LED_RED_PORT_NUM, HRDWCFG_LED_RED_PIN_NUM);
   GPIO_setOutputLowOnPin(HRDWCFG_LED_GREEN_PORT_NUM, HRDWCFG_LED_GREEN_PIN_NUM);
   GPIO_setOutputLowOnPin(HRDWCFG_LED_BLUE_PORT_NUM, HRDWCFG_LED_BLUE_PIN_NUM);
   GPIO_setAsOutputPin(HRDWCFG_LED_RED_PORT_NUM, HRDWCFG_LED_RED_PIN_NUM);
   GPIO_setAsOutputPin(HRDWCFG_LED_GREEN_PORT_NUM, HRDWCFG_LED_GREEN_PIN_NUM);
   GPIO_setAsOutputPin(HRDWCFG_LED_BLUE_PORT_NUM, HRDWCFG_LED_BLUE_PIN_NUM);

   /* Enable interrupts.                                                */
   Interrupt_enableMaster();

   /* Now that interrupts are enabled, disable the controller, flagging */
   /* that the system is initializing so that the function knows how to */
   /* delay.  Note that we need interrupts enabled so that the SysTick  */
   /* timer runs, which is used for delaying purposes.                  */
   SysInit = TRUE;
   HAL_DisableController();
   SysInit = FALSE;
}

   /* The following function enables a UART and configures it to the    */
   /* specified baud rate.  Note that this function enables the UART Rx */
   /* interrupt, but it is up to the caller to enable the UART Tx       */
   /* interrupt.                                                        */
void HAL_EnableUART(uint32_t ModuleInstance, uint32_t InterruptNumber, unsigned long BaudRate)
{
   /* Set the UART configuration parameters.                            */
   eUSCI_UART_Config UARTConfig =
   {
      EUSCI_A_UART_CLOCKSOURCE_SMCLK,                /* SMCLK Source    */
      0,                                             /* BRDIV = 0       */
      0,                                             /* UCxBRF = 0      */
      0,                                             /* UCxBRS = 0      */
      EUSCI_A_UART_NO_PARITY,                        /* No Parity       */
      EUSCI_A_UART_LSB_FIRST,                        /* MSB First       */
      EUSCI_A_UART_ONE_STOP_BIT,                     /* One stop bit    */
      EUSCI_A_UART_MODE,                             /* UART mode       */
      EUSCI_A_UART_LOW_FREQUENCY_BAUDRATE_GENERATION /* Low Frequency   */
   };

   /* Configure the UART using the procedure described in "22.3.1       */
   /* eUSCI_A Initialization and Reset" of the MSP432P4xx Family        */
   /* Technical Reference Manual.                                       */
   UART_disableModule(ModuleInstance);

   /* Calculate the clock prescaler based on SMCLK's frequency and the  */
   /* target baud rate.                                                 */
   UARTConfig.clockPrescalar = (uint_fast16_t)(SMCLK_FREQUENCY / BaudRate);

   /* Initialize the UART.                                              */
   UART_initModule(ModuleInstance, &UARTConfig);

   /* Enable the UART.                                                  */
   UART_enableModule(ModuleInstance);

   /* Enable the receive interrupt.                                     */
   UART_enableInterrupt(ModuleInstance, EUSCI_A_UART_RECEIVE_INTERRUPT);

   /* Enable the eUSCI interrupt.                                       */
   Interrupt_enableInterrupt(InterruptNumber);
}

   /* The following function disables a UART.                           */
void HAL_DisableUART(uint32_t ModuleInstance, uint32_t InterruptNumber)
{
   Interrupt_disableInterrupt(InterruptNumber);
   UART_disableModule(ModuleInstance);
}

   /* The following function disables the controller.  Note that this   */
   /* function will also disable any peripherals and GPIOs that are used*/
   /* with the controller in order to conserve power.                   */
void HAL_DisableController(void)
{
   unsigned long TargetTickCount;

   HAL_DisableUART(HRDWCFG_HCI_UART_MODULE, HRDWCFG_HCI_UART_INT_NUM);

   /* Disable the CTS interrupt.                                        */
   GPIO_disableInterrupt(HRDWCFG_HCI_CTS_PORT_NUM, HRDWCFG_HCI_CTS_PIN_NUM);
   Interrupt_disableInterrupt(HRDWCFG_HCI_CTS_INT_NUM);

   /* Configure all pins as inputs match the pull-up value used by the  */
   /* radio while it is in reset.                                       */
   GPIO_setAsInputPinWithPullUpResistor(HRDWCFG_HCI_UART_TX_PORT_NUM, HRDWCFG_HCI_UART_TX_PIN_NUM);
   GPIO_setAsInputPinWithPullUpResistor(HRDWCFG_HCI_UART_RX_PORT_NUM, HRDWCFG_HCI_UART_RX_PIN_NUM);
   GPIO_setAsInputPinWithPullUpResistor(HRDWCFG_HCI_RTS_PORT_NUM, HRDWCFG_HCI_RTS_PIN_NUM);
   GPIO_setAsInputPinWithPullUpResistor(HRDWCFG_HCI_CTS_PORT_NUM, HRDWCFG_HCI_CTS_PIN_NUM);

#if __SUPPORT_AUDIO_CODEC__

   /* Disable the audio codec.                                          */
   HAL_DisableAudioCodec();

#endif

   /* Put the controller into reset and wait the minimum reset time.    */
   GPIO_setOutputLowOnPin(HRDWCFG_NSHUTD_PORT_NUM, HRDWCFG_NSHUTD_PIN_NUM);

   if(!SysInit)
   {
      /* The system is not initializing, we can use BTPS_Delay().       */
      BTPS_Delay(CONTROLLER_MINIMUM_NSHUTD_LOW_TIME);
   }
   else
   {
      /* The system is initializing, use the SysTick ISR and the timer  */
      /* tick variable in order to delay.  We don't need to account for */
      /* timer tick roll over because the variable will always be zero  */
      /* on reset and it will take several days to roll over at a 1     */
      /* millisecond tick rate.                                         */
      TargetTickCount = TickCount + CONTROLLER_MINIMUM_NSHUTD_LOW_TIME + 1;
      while(TickCount < TargetTickCount)
         ;
   }
}

   /* The following function enables the controller.  Note that this    */
   /* function will also enables any peripherals and GPIOs that are used*/
   /* with the controller in order to prepare them for use.             */
void HAL_EnableController(void)
{
   /* Selecting P3.2 and P3.3 in UART mode for use as the HCI UART.     */
   GPIO_setAsPeripheralModuleFunctionOutputPin(HRDWCFG_HCI_UART_TX_PORT_NUM, HRDWCFG_HCI_UART_TX_PIN_NUM, GPIO_PRIMARY_MODULE_FUNCTION);
   GPIO_setAsPeripheralModuleFunctionInputPin(HRDWCFG_HCI_UART_RX_PORT_NUM, HRDWCFG_HCI_UART_RX_PIN_NUM, GPIO_PRIMARY_MODULE_FUNCTION);

   /* Enable the HCI UART.                                              */
   HAL_EnableUART(HRDWCFG_HCI_UART_MODULE, HRDWCFG_HCI_UART_INT_NUM, CONTROLLER_STARTUP_HCI_BAUD_RATE);

   /* Configure the RTS pin as an output.                               */
   GPIO_setOutputHighOnPin(HRDWCFG_HCI_RTS_PORT_NUM, HRDWCFG_HCI_RTS_PIN_NUM);
   GPIO_setAsOutputPin(HRDWCFG_HCI_RTS_PORT_NUM, HRDWCFG_HCI_RTS_PIN_NUM);

   /* Configure the CTS pin as an input.  Default to using a pull-down  */
   /* resistor because the pin is typically low while the controller is */
   /* both active and in sleep mode.  Note that we enable the CTS port  */
   /* interrupt here, but do not enable the CTS pin interrupt.  We will */
   /* leave it up to the caller to enable the CTS pin interrupt after   */
   /* they have configued the interrupt edge select bit.                */
   GPIO_disableInterrupt(HRDWCFG_HCI_CTS_PORT_NUM, HRDWCFG_HCI_CTS_PIN_NUM);
   GPIO_setAsInputPinWithPullDownResistor(HRDWCFG_HCI_CTS_PORT_NUM, HRDWCFG_HCI_CTS_PIN_NUM);
   GPIO_clearInterruptFlag(HRDWCFG_HCI_CTS_PORT_NUM, HRDWCFG_HCI_CTS_PIN_NUM);
   Interrupt_enableInterrupt(HRDWCFG_HCI_CTS_INT_NUM);

   /* Pull the controller out reset by setting the nSHUTD line high.    */
   GPIO_setOutputHighOnPin(HRDWCFG_NSHUTD_PORT_NUM, HRDWCFG_NSHUTD_PIN_NUM);
}

   /* The following function is used to retrieve data from the UART     */
   /* input queue.  The function receives a pointer to a buffer that    */
   /* will receive the UART characters a the length of the buffer.  The */
   /* function will return the number of characters that were returned  */
   /* in Buffer.                                                        */
int HAL_ConsoleRead(unsigned int Length, char *Buffer)
{
   int Processed = 0;
   int CopyLength;
   int MaxRead;
   int Count;

   /* Make sure the passed in parameters seem valid.                    */
   if((Buffer) && (Length))
   {
      /* Read the characters if necessary.                              */
      Processed = 0;
      while((Length) && (DebugUARTRxBytesFree != DEBUG_UART_RX_BUFFER_SIZE))
      {
         /* Determine the number of characters until the buffer wraps.  */
         Count      = DEBUG_UART_RX_BUFFER_SIZE - DebugUARTRxBytesFree;
         MaxRead    = DEBUG_UART_RX_BUFFER_SIZE - DebugUARTRxOutIndex;
         CopyLength = (MaxRead < Count) ? MaxRead : Count;

         /* Process the number of characters till the buffer wraps or   */
         /* maximum number that we can store in the passed in buffer.   */
         if(Length < CopyLength)
            CopyLength = Length;

         /* Copy the characters over.                                   */
         BTPS_MemCopy(&Buffer[Processed], (void const *)&DebugUARTRxBuffer[DebugUARTRxOutIndex], CopyLength);

         /* Update the counts.                                          */
         Processed           += CopyLength;
         Length              -= CopyLength;
         DebugUARTRxOutIndex += CopyLength;

         /* Handle the case where the out index wraps.                  */
         if(DebugUARTRxOutIndex >= DEBUG_UART_RX_BUFFER_SIZE)
            DebugUARTRxOutIndex = 0;

         /* The Rx bytes free variable is changed in an interrupt so we */
         /* must disable the UART interrupt before reading and writing  */
         /* to it.                                                      */
         Interrupt_disableInterrupt(HRDWCFG_DEBUG_UART_INT_NUM);
         DebugUARTRxBytesFree += CopyLength;
         Interrupt_enableInterrupt(HRDWCFG_DEBUG_UART_INT_NUM);
      }
   }

   return(Processed);
}

   /* The following function is used to send data to the UART output    */
   /* queue.  The function receives a pointer to a buffer that will     */
   /* contains the data to send and the length of the data.             */
void HAL_ConsoleWrite(unsigned int Length, char *String)
{
   unsigned int Count;
   unsigned int TempDebugUARTTxBytesFree;

   /* First make sure the parameters seem semi valid.                   */
   if((Length) && (String))
   {
      /* Loop and transmit all characters to the Debug UART.            */
      while(Length)
      {
         /* Block until there is room in the buffer.                    */
         while(DebugUARTTxBytesFree == 0)
            BTPS_Delay(1);

         /* Get the number of bytes till we reach the end of the buffer.*/
         Count = DEBUG_UART_TX_BUFFER_SIZE - DebugUARTTxInIndex;

         /* Save a copy of the number of free bytes in the Tx buffer.   */
         /* Note that the number of free bytes in the Tx buffer variable*/
         /* can be modified by the Debug UART ISR, and it is accessed   */
         /* twice in the statements below.  Doing this prevents each    */
         /* access below from returning a different value.              */
         TempDebugUARTTxBytesFree = DebugUARTTxBytesFree;

         /* Limit this by the number of bytes that are available.       */
         if(Count > TempDebugUARTTxBytesFree)
            Count = TempDebugUARTTxBytesFree;

         if(Count > Length)
            Count = Length;

         /* Copy as much data as we can.                                */
         BTPS_MemCopy((void *)&DebugUARTTxBuffer[DebugUARTTxInIndex], String, Count);

         /* Adjust the number of free bytes, disable the UART interrupt */
         /* because the variable is also modified in the UART ISR.      */
         Interrupt_disableInterrupt(HRDWCFG_DEBUG_UART_INT_NUM);
         DebugUARTTxBytesFree -= Count;
         Interrupt_enableInterrupt(HRDWCFG_DEBUG_UART_INT_NUM);

         /* Enable the transmit interrupt.  Note that this will have no */
         /* effect if the interrupt is already enabled, nor will it     */
         /* cause problems if the ISR has already sent all of the data  */
         /* we just added to the buffer.  In the latter case the ISR    */
         /* will simply disable the transmit interrupt when it sees     */
         /* there is no more data to send.                              */
         UART_enableInterrupt(HRDWCFG_DEBUG_UART_MODULE, EUSCI_A_UART_TRANSMIT_INTERRUPT);

         /* Adjust the Index and Counts.                                */
         DebugUARTTxInIndex += Count;
         String             += Count;
         Length             -= Count;
         if(DebugUARTTxInIndex == DEBUG_UART_TX_BUFFER_SIZE)
            DebugUARTTxInIndex = 0;
      }
   }
}

   /* This function is called to get the system tick count.             */
unsigned long HAL_GetTickCount(void)
{
   return(TickCount);
}

   /* The following function is used to set the color of the LED.       */
void HAL_SetLEDColor(HAL_LED_Color_t LEDColor)
{
   Boolean_t PreviousLEDOn;

   PreviousLEDOn = LEDOn;

   /* Check if the LED is currently lit.                                */
   if(LEDOn)
   {
      /* The LED is lit, turn off the current color before switching the*/
      /* color.                                                         */
      HAL_ToggleLED();
   }

   CurrentLEDColor = LEDColor;

   /* Check if the LED was previously on.                               */
   if(PreviousLEDOn)
   {
      /* The LED was previously on, turn it back on with the new color. */
      HAL_ToggleLED();
   }
}

   /* The following function is used to toggle the state of the LED.    */
void HAL_ToggleLED(void)
{
   switch(CurrentLEDColor)
   {
      case hlcRed:
         GPIO_toggleOutputOnPin(HRDWCFG_LED_RED_PORT_NUM, HRDWCFG_LED_RED_PIN_NUM);
         break;
      case hlcGreen:
         GPIO_toggleOutputOnPin(HRDWCFG_LED_GREEN_PORT_NUM, HRDWCFG_LED_GREEN_PIN_NUM);
         break;
      case hlcBlue:
         GPIO_toggleOutputOnPin(HRDWCFG_LED_BLUE_PORT_NUM, HRDWCFG_LED_BLUE_PIN_NUM);
         break;
   }

   LEDOn = !LEDOn;
}

#ifdef __SUPPORT_AUDIO_CODEC__

   /* The following function is used to initialize the audio codec on   */
   /* the controller.  Note that the controller's audio codec must be   */
   /* initialized before the first audio (SCO/eSCO) connection, if it is*/
   /* not then no audio will be heard during the first audio connection.*/
void HAL_IntializeControllerAudioCodec(unsigned int BluetoothStackID)
{
   /* Configure the controller's audio codec.  Note that the sample rate*/
   /* and number of channels passed to this function are not important  */
   /* during initialization.  It is important, however, that the        */
   /* direction of the clock and frame sync pins are set before an audio*/
   /* connection is created, and this is done by the function.          */
   ConfigureControllerAudioCodec(BluetoothStackID, 8000, 1);
}

   /* This function enables the audio-specific pins.  The possible audio*/
   /* flags are defined above.                                          */
void HAL_EnableAudioCodec(unsigned int BluetoothStackID, HAL_Audio_Use_Case_t AudioUseCase, unsigned long SamplingFrequency, unsigned int NumChannels)
{
   unsigned char InputLine;
   unsigned char OutputLine;

   const eUSCI_I2C_MasterConfig I2CConfig =
   {
      EUSCI_B_I2C_CLOCKSOURCE_SMCLK,     /* SMCLK Clock Source          */
      SMCLK_FREQUENCY,                   /* SMCLK Frequency             */
      EUSCI_B_I2C_SET_DATA_RATE_400KBPS, /* I2C Clock Rate              */
      0,                                 /* No byte counter threshold   */
      EUSCI_B_I2C_NO_AUTO_STOP           /* No Autostop                 */
   };

   /* Configure the I2C SDA and SCL pins.                               */
   GPIO_setAsPeripheralModuleFunctionInputPin(HRDWCFG_I2C_SDA_PORT_NUM, HRDWCFG_I2C_SDA_PIN_NUM, GPIO_PRIMARY_MODULE_FUNCTION);
   GPIO_setAsPeripheralModuleFunctionInputPin(HRDWCFG_I2C_SCL_PORT_NUM, HRDWCFG_I2C_SCL_PIN_NUM, GPIO_PRIMARY_MODULE_FUNCTION);

   /* Initialize I2C as the master.                                     */
   I2C_initMaster(HRDWCFG_I2C_MODULE, &I2CConfig);
   I2C_setSlaveAddress(HRDWCFG_I2C_MODULE, SLAVE_ADDRESS);
   I2C_setMode(HRDWCFG_I2C_MODULE, EUSCI_B_I2C_TRANSMIT_MODE);
   I2C_enableModule(HRDWCFG_I2C_MODULE);

   switch(AudioUseCase)
   {
      case aucA3DPSink:
         InputLine  = NO_INPUT;
         OutputLine = CODEC_LINE_OUT;
         break;
      case aucA3DPSource:
         InputLine  = CODEC_LINE_IN;
         OutputLine = NO_OUTPUT;
         break;
      case aucHFP_HSP:
         InputLine  = CODEC_ONBOARD_MIC;
         OutputLine = CODEC_LINE_OUT;
         break;
      case aucLoopbackTest:
         InputLine  = CODEC_LINE_IN;
         OutputLine = CODEC_LINE_OUT;
         break;
   }

   /* Initialize the local audio codec.                                 */
   CodecInit(InputLine, OutputLine);

   /* Flag that the local audio codec is enabled.                       */
   AudioCodecEnabled = TRUE;

   /* Configure the controller's audio codec.                           */
   ConfigureControllerAudioCodec(BluetoothStackID, SamplingFrequency, NumChannels);
}

   /* This function disables the audio-specific pins.                   */
void HAL_DisableAudioCodec(void)
{
   /* Reset the audio codec if it is enabled.                           */
   if(AudioCodecEnabled)
   {
      CodecReset();

      BTPS_Delay(100);

      AudioCodecEnabled = FALSE;
   }

   /* Disable the I2C module.                                           */
   I2C_disableModule(HRDWCFG_I2C_MODULE);

   /* Configure the I2C pins as inputs.                                 */
   GPIO_setAsInputPinWithPullUpResistor(HRDWCFG_I2C_SDA_PORT_NUM, HRDWCFG_I2C_SDA_PIN_NUM);
   GPIO_setAsInputPinWithPullUpResistor(HRDWCFG_I2C_SCL_PORT_NUM, HRDWCFG_I2C_SCL_PIN_NUM);
}

#endif

   /*********************************************************************/
   /* Interrupt Service Routines                                        */
   /*********************************************************************/

   /* SysTick interrupt handler.  This handler toggles RGB LED on/off.  */
void SysTick_ISR(void)
{
   TickCount += 1;
}

   /* Debug UART Interrupt Handler.                                     */
void Debug_UART_eUSCI_A_ISR(void)
{
   unsigned char ReceivedByte;

   /* Check if the Rx interrupt flag requested the interrupt.           */
   if(EUSCI_A_CMSIS(HRDWCFG_DEBUG_UART_MODULE)->IFG & EUSCI_A_UART_RECEIVE_INTERRUPT_FLAG)
   {
      /* Read the received character, note that the interrupt flag is   */
      /* cleared automatically when this register is read and we don't  */
      /* need to do it explicitly.  The code snippet from               */
      /* UART_receiveData() is used instead of calling the actual       */
      /* function.                                                      */
      ReceivedByte = (unsigned char)EUSCI_A_CMSIS(HRDWCFG_DEBUG_UART_MODULE)->RXBUF;

      /* Place characters in receive buffer if there is any space.      */
      if(DebugUARTRxBytesFree)
      {
         /* Save the character in the Receive Buffer.                   */
         DebugUARTRxBuffer[DebugUARTRxInIndex++] = ReceivedByte;
         DebugUARTRxBytesFree--;

         /* Wrap the buffer if necessary.                               */
         if(DebugUARTRxInIndex >= DEBUG_UART_RX_BUFFER_SIZE)
            DebugUARTRxInIndex = 0;
      }
   }
   else
   {
      if(DebugUARTTxBytesFree != DEBUG_UART_TX_BUFFER_SIZE)
      {
         /* Load the Tx buffer using the code snippet from              */
         /* UART_transmitData() instead of calling the function, in     */
         /* order to reduce the amount of time spent in this ISR.  Note */
         /* that the UCTXIFG bit is cleared automatically when we write */
         /* new data to the UART's Tx buffer and we therefore don't need*/
         /* to manually clear the flag.                                 */
         EUSCI_A_CMSIS(HRDWCFG_DEBUG_UART_MODULE)->TXBUF = DebugUARTTxBuffer[DebugUARTTxOutIndex++];

         /* Decrement the number of characters that are in the transmit */
         /* buffer and adjust the out index.                            */
         DebugUARTTxBytesFree++;
         if(DebugUARTTxOutIndex >= DEBUG_UART_TX_BUFFER_SIZE)
            DebugUARTTxOutIndex = 0;
      }
      else
      {
         /* Disable the transmit interrupt, note that this code snippet */
         /* is from the MSPWare UART_disableInterrupt() function.       */
         EUSCI_A_CMSIS(HRDWCFG_DEBUG_UART_MODULE)->IE &= ~EUSCI_A_UART_TRANSMIT_INTERRUPT;
      }
   }
}
