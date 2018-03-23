/*****< hal.h >****************************************************************/
/*      Copyright 2010 - 2014 Stonestreet One.                                */
/*      All Rights Reserved.                                                  */
/*                                                                            */
/*      Copyright 2015 Texas Instruments Incorporated.                        */
/*      All Rights Reserved.                                                  */
/*                                                                            */
/*  HAL - Hardware Abstraction Layer constants and prototypes.                */
/*                                                                            */
/*  Author:  Tim Thomas                                                       */
/*                                                                            */
/*** MODIFICATION HISTORY *****************************************************/
/*                                                                            */
/*   mm/dd/yy  F. Lastname    Description of Modification                     */
/*   --------  -----------    ------------------------------------------------*/
/*   07/05/11  Tim Thomas     Initial creation.                               */
/******************************************************************************/
#ifndef __HALH__
#define __HALH__

#include <stdint.h>

   /*********************************************************************/
   /* Defines, Enumerations, & Type Definitions                         */
   /*********************************************************************/

   /* The following constant specifies the maximum HCI UART baud rate   */
   /* currently supported by this platform.                             */
#define HAL_HCI_UART_MAX_BAUD_RATE      2000000

   /* The following enumerated type defines the possible audio use      */
   /* cases.                                                            */
typedef enum
{
   aucA3DPSink,
   aucA3DPSource,
   aucHFP_HSP,
   aucLoopbackTest
} HAL_Audio_Use_Case_t;

   /* The following enumerated type defines the possible LED colors.    */
typedef enum
{
   hlcRed,
   hlcGreen,
   hlcBlue
} HAL_LED_Color_t;

   /*********************************************************************/
   /* Global/Non-Static Functions                                       */
   /*********************************************************************/

   /* The following function configures the microcontroller's           */
   /* peripherals and GPIOs to their default states.                    */
void HAL_ConfigureHardware(void);

   /* The following function enables a UART and configures it to the    */
   /* specified baud rate.  Note that this function enables the UART Rx */
   /* interrupt, but it is up to the caller to enable the UART Tx       */
   /* interrupt.                                                        */
void HAL_EnableUART(uint32_t ModuleInstance, uint32_t InterruptNumber, unsigned long BaudRate);

   /* The following function disables a UART.                           */
void HAL_DisableUART(uint32_t ModuleInstance, uint32_t InterruptNumber);

   /* The following function disables the controller.  Note that this   */
   /* function will also disable any peripherals and GPIOs that are used*/
   /* with the controller in order to conserve power.                   */
void HAL_DisableController(void);

   /* The following function enables the controller.  Note that this    */
   /* function will also enables any peripherals and GPIOs that are used*/
   /* with the controller in order to prepare them for use.             */
void HAL_EnableController(void);

   /* The following function is used to retrieve data from the UART     */
   /* input queue.  The function receives a pointer to a buffer that    */
   /* will receive the UART characters a the length of the buffer.  The */
   /* function will return the number of characters that were returned  */
   /* in Buffer.                                                        */
int HAL_ConsoleRead(unsigned int Length, char *Buffer);

   /* The following function is used to send data to the UART output    */
   /* queue.  The function receives a pointer to a buffer that will     */
   /* contains the data to send and the length of the data.             */
void HAL_ConsoleWrite(unsigned int Length, char *Buffer);

   /* This function is called to get the system tick count.             */
unsigned long HAL_GetTickCount(void);

   /* The following function is used to set the color of the LED.       */
void HAL_SetLEDColor(HAL_LED_Color_t LEDColor);

   /* The following function is used to toggle the state of the LED.    */
void HAL_ToggleLED(void);

#ifdef __SUPPORT_AUDIO_CODEC__

   /* The following function is used to initialize the audio codec on   */
   /* the controller.  Note that the controller's audio codec must be   */
   /* initialized before the first audio (SCO/eSCO) connection, if it is*/
   /* not then no audio will be heard during the first audio connection.*/
void HAL_IntializeControllerAudioCodec(unsigned int BluetoothStackID);

   /* This function enables the pins and peripherals used to communicate*/
   /* with the audio codec.                                             */
void HAL_EnableAudioCodec(unsigned int BluetoothStackID, HAL_Audio_Use_Case_t AudioUseCase, unsigned long SamplingFrequency, unsigned int NumChannels);

   /* This function disables the pins and peripherals used to           */
   /* communicate with the audio codec.                                 */
void HAL_DisableAudioCodec(void);

#endif

#endif
