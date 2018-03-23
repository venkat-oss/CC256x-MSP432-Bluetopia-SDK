/*****< hrdwcfg.h >************************************************************/
/*      Copyright 2010 - 2014 Stonestreet One.                                */
/*      All Rights Reserved.                                                  */
/*                                                                            */
/*      Copyright 2015 Texas Instruments Incorporated.                        */
/*      All Rights Reserved.                                                  */
/*                                                                            */
/*  HRDWCFG - Hardware configuration header.                                  */
/*                                                                            */
/*  Author:  Tim Cook                                                         */
/*                                                                            */
/*** MODIFICATION HISTORY *****************************************************/
/*                                                                            */
/*   mm/dd/yy  F. Lastname    Description of Modification                     */
/*   --------  -----------    ------------------------------------------------*/
/*   08/31/10  Tim Cook       Initial creation.                               */
/******************************************************************************/
#ifndef __HRDWCFGH__
#define __HRDWCFGH__

#include "driverlib.h"

   /*********************************************************************/
   /* HCI UART Pin Definitions                                          */
   /*********************************************************************/

#define HRDWCFG_HCI_UART_MODULE           EUSCI_A2_BASE
#define HRDWCFG_HCI_UART_INT_NUM          INT_EUSCIA2

#define HRDWCFG_HCI_UART_TX_PORT_NUM      GPIO_PORT_P3
#define HRDWCFG_HCI_UART_TX_PIN_NUM       GPIO_PIN3

#define HRDWCFG_HCI_UART_RX_PORT_NUM      GPIO_PORT_P3
#define HRDWCFG_HCI_UART_RX_PIN_NUM       GPIO_PIN2

   /*********************************************************************/
   /* HCI Flow Control Pin Definitions                                  */
   /*********************************************************************/

#define HRDWCFG_HCI_RTS_PORT_NUM          GPIO_PORT_P5
#define HRDWCFG_HCI_RTS_PIN_NUM           GPIO_PIN6
#define HRDWCFG_HCI_RTS_PORT_OUT          P5OUT

#define HRDWCFG_HCI_CTS_PORT_NUM          GPIO_PORT_P6
#define HRDWCFG_HCI_CTS_PIN_NUM           GPIO_PIN6
#define HRDWCFG_HCI_CTS_INT_NUM           INT_PORT6
#define HRDWCFG_HCI_CTS_PORT_IN           P6IN
#define HRDWCFG_HCI_CTS_PORT_IE           P6IE
#define HRDWCFG_HCI_CTS_PORT_IES          P6IES
#define HRDWCFG_HCI_CTS_PORT_IFG          P6IFG
#define HRDWCFG_HCI_CTS_IV                P6IV
#define HRDWCFG_HCI_CTS_IV_NUM            0x0E

   /*********************************************************************/
   /* nSHUTD Pin Definitions                                            */
   /*********************************************************************/

#define HRDWCFG_NSHUTD_PORT_NUM           GPIO_PORT_P2
#define HRDWCFG_NSHUTD_PIN_NUM            GPIO_PIN5

   /*********************************************************************/
   /* PCM Pin Definitions                                               */
   /*********************************************************************/

#define HRDWCFG_AUD_FSYNC_PORT_NUM        GPIO_PORT_P4
#define HRDWCFG_AUD_FSYNC_PIN_NUM         GPIO_PIN5

#define HRDWCFG_AUD_CLK_PORT_NUM          GPIO_PORT_P4
#define HRDWCFG_AUD_CLK_PIN_NUM           GPIO_PIN7

#define HRDWCFG_AUD_IN_PORT_NUM           GPIO_PORT_P5
#define HRDWCFG_AUD_IN_PIN_NUM            GPIO_PIN4

#define HRDWCFG_AUD_OUT_PORT_NUM          GPIO_PORT_P5
#define HRDWCFG_AUD_OUT_PIN_NUM           GPIO_PIN5

#define HRDWCFG_I2C_MODULE                EUSCI_B1_BASE

#define HRDWCFG_I2C_SDA_PORT_NUM          GPIO_PORT_P6
#define HRDWCFG_I2C_SDA_PIN_NUM           GPIO_PIN4

#define HRDWCFG_I2C_SCL_PORT_NUM          GPIO_PORT_P6
#define HRDWCFG_I2C_SCL_PIN_NUM           GPIO_PIN5

   /*********************************************************************/
   /* Debug UART Pin Definitions                                        */
   /*********************************************************************/

#define HRDWCFG_DEBUG_UART_MODULE         EUSCI_A0_BASE
#define HRDWCFG_DEBUG_UART_INT_NUM        INT_EUSCIA0

#define HRDWCFG_DEBUG_UART_TX_PORT_NUM    GPIO_PORT_P1
#define HRDWCFG_DEBUG_UART_TX_PIN_NUM     GPIO_PIN3

#define HRDWCFG_DEBUG_UART_RX_PORT_NUM    GPIO_PORT_P1
#define HRDWCFG_DEBUG_UART_RX_PIN_NUM     GPIO_PIN2

   /*********************************************************************/
   /* LED Pin Definitions                                               */
   /*********************************************************************/

#define HRDWCFG_LED_RED_PORT_NUM          GPIO_PORT_P2
#define HRDWCFG_LED_RED_PIN_NUM           GPIO_PIN0

#define HRDWCFG_LED_GREEN_PORT_NUM        GPIO_PORT_P2
#define HRDWCFG_LED_GREEN_PIN_NUM         GPIO_PIN1

#define HRDWCFG_LED_BLUE_PORT_NUM         GPIO_PORT_P2
#define HRDWCFG_LED_BLUE_PIN_NUM          GPIO_PIN2

#endif
