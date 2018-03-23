/*****< btpskrnl.h >***********************************************************/
/*      Copyright 2000 - 2014 Stonestreet One.                                */
/*      All Rights Reserved.                                                  */
/*                                                                            */
/*  BTPSKRNL - Stonestreet One Bluetooth Stack Kernel Implementation Type     */
/*             Definitions, Constants, and Prototypes.                        */
/*                                                                            */
/*  Author:  Damon Lange                                                      */
/*                                                                            */
/*** MODIFICATION HISTORY *****************************************************/
/*                                                                            */
/*   mm/dd/yy  F. Lastname    Description of Modification                     */
/*   --------  -----------    ------------------------------------------------*/
/*   05/30/01  D. Lange       Initial creation.                               */
/******************************************************************************/
#ifndef __BTPSKRNLH__
#define __BTPSKRNLH__

   /* The following constant represents the Minimum Amount of Time      */
   /* that can be scheduled with the BTPS Scheduler.  Attempting to     */
   /* Schedule a Scheduled Function less than this time will result in  */
   /* the function being scheduled for the specified Amount.  This      */
   /* value is in Milliseconds.                                         */
#define BTPS_MINIMUM_SCHEDULER_RESOLUTION                (0)

#include "BKRNLAPI.h"           /* Bluetooth Kernel Prototypes/Constants.     */

#endif
