/*****< btpskrnl.c >***********************************************************/
/*      Copyright 2012 - 2014 Stonestreet One.                                */
/*      All Rights Reserved.                                                  */
/*                                                                            */
/*      Copyright 2015 Texas Instruments Incorporated.                        */
/*      All Rights Reserved.                                                  */
/*                                                                            */
/*  BTPSKRNL - Stonestreet One Bluetooth Stack Kernel Implementation.         */
/*                                                                            */
/*  Author:  Marcus Funk                                                      */
/*                                                                            */
/*** MODIFICATION HISTORY *****************************************************/
/*                                                                            */
/*   mm/dd/yy  F. Lastname    Description of Modification                     */
/*   --------  -----------    ------------------------------------------------*/
/*   11/08/12  M. Funk        Initial creation.                               */
/******************************************************************************/
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#include "BTPSKRNL.h"         /* BTPS Kernel Prototypes/Constants.            */
#include "BTTypes.h"          /* BTPS internal data types.                    */

   /* The following constant represents the maximum number of functions */
   /* that can be added to the scheduler.                               */
#define MAX_NUMBER_SCHEDULE_FUNCTIONS                (8)

   /* The following type declaration represents an individual Scheduler */
   /* Function Entry.  This Entry contains all information needed to    */
   /* Schedule and Execute a Function that has been added to the        */
   /* Scheduler.                                                        */
typedef struct _tagSchedulerInformation_t
{
   unsigned long             ScheduleCount;
   unsigned long             ScheduleExpireCount;
   BTPS_SchedulerFunction_t  ScheduleFunction;
   void                     *ScheduleParameter;
} SchedulerInformation_t;

   /* The following type declaration represents the entire state        */
   /* information for a Mailbox.  This structure is used with all of the*/
   /* Mailbox functions contained in this module.                       */
typedef struct _tagMailboxHeader_t
{
   unsigned int  HeadSlot;
   unsigned int  TailSlot;
   unsigned int  OccupiedSlots;
   unsigned int  NumberSlots;
   unsigned int  SlotSize;
   void         *Slots;
} MailboxHeader_t;

   /*********************************************************************/
   /* Heap Manager Definitions                                          */
   /*********************************************************************/

   /* Defines the maximum number of bytes that will be allocated by the */
   /* kernel abstraction module to support allocations.                 */
   /* * NOTE * This module declares a memory array of this size (in     */
   /*          bytes) that will be used by this module for memory       */
   /*          allocation.                                              */
#define BTPS_MEMORY_BUFFER_SIZE        (20 * 1024)

   /* The following defines a type that is the size in bytes of the     */
   /* desired alignment of each data fragment.                          */
typedef unsigned int Alignment_t;

   /* The following defines the size that memory needs to be aligned to.*/
   /* For correct operation, this value must be a power of two.         */
#define ALIGNMENT_SIZE                 sizeof(Alignment_t)

   /* The following structure contains the information for a single     */
   /* memory block.  Its members include the size of the previous block,*/
   /* and the size of the block, and the start of the data region of the*/
   /* The size of the block also includes a flag to indicate if the     */
   /* block is allocated.                                               */
typedef struct _tagBlockInfo_t
{
   Word_t      PrevSize;
   Word_t      Size;
   Alignment_t Data[1];
} BlockInfo_t;

#define BLOCK_INFO_SIZE(_x)            ((BTPS_STRUCTURE_OFFSET(BlockInfo_t, Data) / ALIGNMENT_SIZE) + (_x))

   /* The following constants are used with the Size member of the      */
   /* BlockInfo_t to denote various information about the memory block. */
#define SEGMENT_SIZE_BITMASK           ((Word_t)(((Word_t)-1) >> 1))
#define SEGMENT_ALLOCATED_BITMASK      (Word_t)(~SEGMENT_SIZE_BITMASK)

   /* The following defines the size in bytes of a data fragment that is*/
   /* considered a large value.  Allocations that are equal to and      */
   /* larger than this value will be allocated from the end of the heap.*/
#define LARGE_SIZE                     (256 / ALIGNMENT_SIZE)

   /* The following defines the minimum and maximum sizes of a block (in*/
   /* Alignment_t units) that can be allocated.                         */
#define MINIMUM_MEMORY_SIZE            (BLOCK_INFO_SIZE(1))
#define MAXIMUM_MEMORY_SIZE            (SEGMENT_SIZE_BITMASK)

   /* The following structure provides the information for a heap. Its  */
   /* members include a flag to indicate if it has been initializes, the*/
   /* current and maximum amount of the heap used (in Alignment_t       */
   /* units), a pointer to the end of the heap and the start of the     */
   /* heap.                                                             */
typedef struct _tagHeapInfo_t
{
   Boolean_t     Initialized;
   unsigned int  CurrentHeapUsed;
   unsigned int  MaximumHeapUsed;
   BlockInfo_t  *HeapTail;
   BlockInfo_t   HeapHead[1];
} HeapInfo_t;

#define HEAP_INFO_SIZE(_x)             (BTPS_STRUCTURE_OFFSET(HeapInfo_t, HeapHead) + (_x))

   /* Internal Variables to this Module (Remember that all variables    */
   /* declared static are initialized to 0 automatically by the         */
   /* compiler as part of standard C/C++).                              */

   /* Declare a buffer to use for the Heap.  Note that we declare this  */
   /* as an Alignment_t so that we can force alignment to be correct.   */
static Alignment_t               MemoryBuffer[(BTPS_MEMORY_BUFFER_SIZE / ALIGNMENT_SIZE)];

   /*********************************************************************/
   /* Miscellaneous Definitions                                         */
   /*********************************************************************/

   /* Flag to indicate if the scheduler has been successfully           */
   /* Initialized.                                                      */
static Boolean_t               SchedulerInitialized;

   /* Variable which holds the total number of Functions that have been */
   /* added to the Scheduler.                                           */
static unsigned int            NumberScheduledFunctions;

   /* Variable which holds ALL Information regarding ALL Scheduled      */
   /* Functions.                                                        */
static SchedulerInformation_t  SchedulerInformation[MAX_NUMBER_SCHEDULE_FUNCTIONS];

   /* Variable which holds the current Debug Zone Mask.                 */
static unsigned long           DebugZoneMask;

   /* Variable which holds the previous Tick Count that was present     */
   /* through the last pass through the scheduler.                      */
static unsigned long           PreviousTickCount;

   /* Variable which holds the currently registered function that is to */
   /* be called when this module needs to know the current value of the */
   /* millisecond Tick Count of the system.                             */
static BTPS_GetTickCountCallback_t  GetTickCountCallback;

   /* The following buffer is used when writing Debug Messages to Debug */
   /* UART.                                                             */
static BTPS_MessageOutputCallback_t MessageOutputCallback;

   /* Internal Function Prototypes.                                     */
static void HeapInit(void *Heap, unsigned long Size);
static void *MemAlloc(void *Heap, unsigned long Size);
static void MemFree(void *Heap, void *MemoryPtr);
static int GetHeapStatistics(void *Heap, BTPS_MemoryStatistics_t *MemoryStatistics, Boolean_t AdvancedStatitics);

   /* The following function is used to initialize the heap structure.  */
   /* The function takes no parameters and returns no status.           */
static void HeapInit(void *Heap, unsigned long Size)
{
   HeapInfo_t *HeapInfo;

   HeapInfo = (HeapInfo_t *)Heap;

   /* Confirm that the parameters are valid and that the heap has not   */
   /* already been initialized.                                         */
   if((HeapInfo) && (!(HeapInfo->Initialized)) && (Size > HEAP_INFO_SIZE(MINIMUM_MEMORY_SIZE)))
   {
      Size = (Size - HEAP_INFO_SIZE(0)) / ALIGNMENT_SIZE;

      /* Confirm that the size is valid.                                */
      if((Size >= MINIMUM_MEMORY_SIZE) && (Size <= MAXIMUM_MEMORY_SIZE))
      {
         /* Initialize the Heap information.                            */
         BTPS_MemInitialize(HeapInfo, 0, sizeof(HeapInfo_t));
         HeapInfo->HeapHead->PrevSize = Size;
         HeapInfo->HeapHead->Size     = Size;
         HeapInfo->HeapTail           = (BlockInfo_t *)(((Alignment_t *)HeapInfo->HeapHead) + Size);

         /* Indicate the heap has been initialized.                     */
         HeapInfo->Initialized        = TRUE;
      }
   }
}

   /* The following function is used to allocate a fragment of memory   */
   /* from a large buffer.  The function takes as its parameter the size*/
   /* in bytes of the fragment to be allocated.  The function tries to  */
   /* avoid fragmentation by obtaining memory requests larger than      */
   /* LARGE_SIZE from the end of the buffer, while small fragments are  */
   /* taken from the start of the buffer.                               */
static void *MemAlloc(void *Heap, unsigned long Size)
{
   void        *ret_val;
   HeapInfo_t  *HeapInfo;
   BlockInfo_t *BlockInfo;
   BlockInfo_t *TempBlockInfo;
   Word_t       RemainingSize;

   HeapInfo = (HeapInfo_t *)Heap;

   /* Convert the requested memory allocation in bytes to alignment     */
   /* size, rounding up, and add the block info header size to it.      */
   Size = BLOCK_INFO_SIZE((Size + (ALIGNMENT_SIZE - 1)) / ALIGNMENT_SIZE);

   /* Verify that the parameters are valid.                             */
   if((HeapInfo) && (HeapInfo->Initialized) && (Size >= MINIMUM_MEMORY_SIZE) && (Size <= MAXIMUM_MEMORY_SIZE))
   {
      /* Start at the beginning of the heap for small segments and the  */
      /* end for large segments.                                        */
      if(Size < LARGE_SIZE)
         BlockInfo = HeapInfo->HeapHead;
      else
         BlockInfo = (BlockInfo_t *)(((Alignment_t *)(HeapInfo->HeapTail)) - HeapInfo->HeapHead->PrevSize);

      /* Loop until we have walked the entire list.                     */
      while(((Size < LARGE_SIZE) || (BlockInfo != HeapInfo->HeapHead)) && (BlockInfo != HeapInfo->HeapTail))
      {
         /* Check to see if the current entry is free and is large      */
         /* enough to hold the data being requested.                    */
         if((BlockInfo->Size & SEGMENT_ALLOCATED_BITMASK) || (BlockInfo->Size < Size))
         {
            /* If the requested size is larger than the limit then      */
            /* search backwards for an available buffer, else go        */
            /* forward.  This will hopefully help to reduce             */
            /* fragmentation problems.                                  */
            if(Size >= LARGE_SIZE)
               BlockInfo = (BlockInfo_t *)(((Alignment_t *)BlockInfo) - (BlockInfo->PrevSize));
            else
               BlockInfo = (BlockInfo_t *)(((Alignment_t *)BlockInfo) + (BlockInfo->Size & SEGMENT_SIZE_BITMASK));
         }
         else
         {
            /* Suitable memory block found.                             */
            break;
         }
      }

      /* Check to see if we found a segment large enough for the        */
      /* request.                                                       */
      if((BlockInfo != HeapInfo->HeapTail) && (BlockInfo->Size >= Size) && (!(BlockInfo->Size & SEGMENT_ALLOCATED_BITMASK)))
      {
         /* Check to see if we need to split this into two entries.     */
         /* * NOTE * If there is not enough room to make another entry  */
         /*          then we will not adjust the size of this entry to  */
         /*          match the amount requested.                        */
         if((RemainingSize = BlockInfo->Size - Size) >= MINIMUM_MEMORY_SIZE)
         {
            /* If this is a large segment allocation, then split the    */
            /* segment so that the free segment is at the beginning.    */
            if(Size >= LARGE_SIZE)
            {
               /* Re-size the current block for the remaining space.    */
               BlockInfo->Size = RemainingSize;

               /* Initialize the new block, setting it to allocated.    */
               BlockInfo = (BlockInfo_t *)(((Alignment_t *)BlockInfo) + RemainingSize);
               BlockInfo->PrevSize = RemainingSize;
               BlockInfo->Size     = Size | SEGMENT_ALLOCATED_BITMASK;

               /* Set the TmpBlock pointer to the current block and the */
               /* RemainingSize to its size for setting the previous    */
               /* size of the next block.                               */
               TempBlockInfo = BlockInfo;
               RemainingSize = Size;
            }
            else
            {
               /* Re-size the current block and set it as allocated.    */
               BlockInfo->Size = Size | SEGMENT_ALLOCATED_BITMASK;

               /* Initialize the new block.                             */
               TempBlockInfo = (BlockInfo_t *)(((Alignment_t *)BlockInfo) + Size);
               TempBlockInfo->PrevSize = Size;
               TempBlockInfo->Size     = RemainingSize;
            }

            /* Calculate the pointer to the next segment, checking for a*/
            /* wrap condition, and update the next segment's PrevSize   */
            /* field.                                                   */
            if((TempBlockInfo = (BlockInfo_t *)(((Alignment_t *)TempBlockInfo) + RemainingSize)) != HeapInfo->HeapTail)
               TempBlockInfo->PrevSize = RemainingSize;
            else
            {
               /* This block is the last in the Heap so set the previous*/
               /* size of the heap head instead.                        */
               HeapInfo->HeapHead->PrevSize = RemainingSize;
            }
         }
         else
         {
            /* Update the allocated size to be that of the block that   */
            /* was found and set the block to allocated.                */
            Size             = BlockInfo->Size;
            BlockInfo->Size |= SEGMENT_ALLOCATED_BITMASK;
         }

         /* Get the address of the start of the allocated memory.       */
         ret_val = (void *)(BlockInfo->Data);

         /* Adjust the memory statistics.                               */
         HeapInfo->CurrentHeapUsed += Size;
         if(HeapInfo->MaximumHeapUsed < HeapInfo->CurrentHeapUsed)
            HeapInfo->MaximumHeapUsed = HeapInfo->CurrentHeapUsed;
      }
      else
      {
         /* No suitable memory block found.                             */
         ret_val = NULL;
      }
   }
   else
      ret_val = NULL;

   return(ret_val);
}

   /* The following function is used to free memory that was previously */
   /* allocated with MemAlloc.  The function takes as its parameter a   */
   /* pointer to the memory that was allocated.  The pointer is used to */
   /* locate the structure of information that describes the allocated  */
   /* fragment.  The function tries to a verify that the structure is a */
   /* valid fragment structure before the memory is freed.  When a      */
   /* fragment is freed, it may be combined with adjacent fragments to  */
   /* produce a larger free fragment.                                   */
static void MemFree(void *Heap, void *MemoryPtr)
{
   HeapInfo_t  *HeapInfo;
   BlockInfo_t *BlockInfo;
   BlockInfo_t *TempBlockInfo;

   HeapInfo = (HeapInfo_t *)Heap;

   /* Verify that the parameter passed in appears valid.                */
   if((HeapInfo) && (HeapInfo->Initialized) && (MemoryPtr) && (MemoryPtr >= (void *)(HeapInfo->HeapHead->Data)) && (MemoryPtr < (void *)(HeapInfo->HeapTail)))
   {
      /* Get a pointer to the Block Info.                               */
      BlockInfo = (BlockInfo_t *)(((Alignment_t *)MemoryPtr) - BLOCK_INFO_SIZE(0));

      /* Verify that this segment is allocated.                         */
      if(BlockInfo->Size & SEGMENT_ALLOCATED_BITMASK)
      {
         /* Set the current block as unallocated.                       */
         BlockInfo->Size &= ~SEGMENT_ALLOCATED_BITMASK;

         /* Update the Heap Statistics.                                 */
         HeapInfo->CurrentHeapUsed -= BlockInfo->Size;

         /* Try to combine this segment with the previous segment.      */
         if(BlockInfo != HeapInfo->HeapHead)
         {
            TempBlockInfo = (BlockInfo_t *)(((Alignment_t *)BlockInfo) - BlockInfo->PrevSize);

            if(!(TempBlockInfo->Size & SEGMENT_ALLOCATED_BITMASK))
            {
               /* Combine this segment with the newly freed segment.    */
               TempBlockInfo->Size += BlockInfo->Size;
               BlockInfo = TempBlockInfo;
            }
         }

         /* Try to combine this segment with the following segment.     */
         if((TempBlockInfo = (BlockInfo_t *)(((Alignment_t *)BlockInfo) + BlockInfo->Size)) < HeapInfo->HeapTail)
         {
            if(!(TempBlockInfo->Size & SEGMENT_ALLOCATED_BITMASK))
            {
               BlockInfo->Size += TempBlockInfo->Size;

            }
         }

         /* Update the previous size of the next block.                 */
         if((TempBlockInfo = (BlockInfo_t *)(((Alignment_t *)BlockInfo) + BlockInfo->Size)) != HeapInfo->HeapTail)
            TempBlockInfo->PrevSize = BlockInfo->Size;
         else
         {
            /* This is the last block so adjust the previous size of the*/
            /* first block instead.                                     */
            HeapInfo->HeapHead->PrevSize = BlockInfo->Size;
         }
      }
   }
}

   /* The following function will find statistics for current heap      */
   /* usage.  This function accepts as its parameter a pointer to a     */
   /* memory statistics structure and a flag to indicate if fragment    */
   /* information will be determined.  The function will return zero if */
   /* successful or a negative value if there is an error.              */
   /* * NOTE * If the advanced statistics flag is set to FALSE, then the*/
   /*          largest free fragment and free fragment count will be    */
   /*          set to zero.                                             */
static int GetHeapStatistics(void *Heap, BTPS_MemoryStatistics_t *MemoryStatistics, Boolean_t AdvancedStatitics)
{
   int          ret_val;
   HeapInfo_t  *HeapInfo;
   BlockInfo_t *BlockInfo;

   HeapInfo = (HeapInfo_t *)Heap;

   /* Verify that the parameters that were passed in appear valid.      */
   if((HeapInfo) && (HeapInfo->Initialized) && (MemoryStatistics))
   {
      BTPS_MemInitialize(MemoryStatistics, 0, sizeof(BTPS_MemoryStatistics_t));

      /* Assign the basic heap statistics.                              */
      MemoryStatistics->HeapSize        = (unsigned int)(((unsigned char *)(HeapInfo->HeapTail)) - ((unsigned char *)(HeapInfo->HeapHead)));
      MemoryStatistics->CurrentHeapUsed = HeapInfo->CurrentHeapUsed * ALIGNMENT_SIZE;
      MemoryStatistics->MaximumHeapUsed = HeapInfo->MaximumHeapUsed * ALIGNMENT_SIZE;

      if(AdvancedStatitics)
      {
         /* Walk the heap and calculate the advanced statistics.        */
         BlockInfo = HeapInfo->HeapHead;

         while(BlockInfo < HeapInfo->HeapTail)
         {
            if(!(BlockInfo->Size & SEGMENT_ALLOCATED_BITMASK))
            {
               /* Block is unallocated.                                 */
               MemoryStatistics->FreeFragmentCount ++;

               if(MemoryStatistics->LargestFreeFragment < BlockInfo->Size)
                  MemoryStatistics->LargestFreeFragment = BlockInfo->Size;
            }

            /* Adjust the pointer to the next entry.                    */
            BlockInfo = (BlockInfo_t *)(((Alignment_t *)BlockInfo) + (BlockInfo->Size & SEGMENT_SIZE_BITMASK));
         }

         /* Convert the size of the largest free fragment to bytes.     */
         MemoryStatistics->LargestFreeFragment *= ALIGNMENT_SIZE;
      }


      ret_val = 0;
   }
   else
      ret_val = -1;

   return(ret_val);
}

   /* The following function is responsible for the Memory Usage        */
   /* Information.  This function accepts as its parameter a pointer to */
   /* a memory statistics structure and a flag to indicate if fragment  */
   /* information will be determined.  The function will return zero if */
   /* successful or a negative value if there is an error.              */
   /* * NOTE * If the advanced statistics flag is set to FALSE, then the*/
   /*          largest free fragment and free fragment count will be    */
   /*          set to zero.                                             */
int BTPSAPI BTPS_QueryMemoryUsage(BTPS_MemoryStatistics_t *MemoryStatistics, Boolean_t AdvancedStatitics)
{
   return(GetHeapStatistics(MemoryBuffer, MemoryStatistics, AdvancedStatitics));
}

   /* The following function is responsible for delaying the current    */
   /* task for the specified duration (specified in Milliseconds).      */
   /* * NOTE * Very small timeouts might be smaller in granularity than */
   /*          the system can support !!!!                              */
void BTPSAPI BTPS_Delay(unsigned long MilliSeconds)
{
   unsigned long StartTickCount;
   unsigned long ElapsedTicks;
   unsigned long CurrentTicks;

   /* Increment the delay time by one millisecond to ensure that the    */
   /* caller gets at least as much time as they requested.              */
   MilliSeconds += 1;

   ElapsedTicks  = 0;
   StartTickCount = BTPS_GetTickCount();

   while(ElapsedTicks < MilliSeconds)
   {
      CurrentTicks = BTPS_GetTickCount();

      if(CurrentTicks >= StartTickCount)
      {
         ElapsedTicks = CurrentTicks - StartTickCount;
      }
      else
      {
         ElapsedTicks = ((0xFFFFFFFFUL - StartTickCount) + 1) + CurrentTicks;
      }
   }
}

   /* The following function is responsible for retrieving the current  */
   /* Tick Count of system.  This function returns the System Tick Count*/
   /* in Milliseconds resolution.                                       */
unsigned long BTPSAPI BTPS_GetTickCount(void)
{
   /* Simply wrap the application-supplied get tick count function.     */
   if(GetTickCountCallback)
      return((*GetTickCountCallback)());
   else
      return(0);
}

   /* The following function is provided to allow a mechanism for adding*/
   /* Scheduler Functions to the Scheduler.  These functions are called */
   /* periodically by the Scheduler (based upon the requested Schedule  */
   /* Period).  This function accepts as input the Scheduler Function to*/
   /* add to the Scheduler, the Scheduler parameter that is passed to   */
   /* the Scheduled function, and the Scheduler Period.  The Scheduler  */
   /* Period is specified in Milliseconds.  This function returns TRUE  */
   /* if the function was added successfully or FALSE if there was an   */
   /* error.                                                            */
   /* * NOTE * Once a function is added to the Scheduler, it can only be*/
   /*          removed by calling the BTPS_DeleteFunctionFromScheduler()*/
   /*          function.                                                */
   /* * NOTE * The BTPS_ExecuteScheduler() function *MUST* be called    */
   /*          ONCE (AND ONLY ONCE) to begin the Scheduler Executing    */
   /*          periodic Scheduled functions (or calling the             */
   /*          BTPS_ProcessScheduler() function repeatedly.             */
Boolean_t BTPSAPI BTPS_AddFunctionToScheduler(BTPS_SchedulerFunction_t SchedulerFunction, void *SchedulerParameter, unsigned int Period)
{
   Boolean_t ret_val;

   /* First, let's make sure that the Scheduler has been initialized    */
   /* successfully AND that the Scheduler is NOT full.                  */
   if((SchedulerInitialized) && (NumberScheduledFunctions != MAX_NUMBER_SCHEDULE_FUNCTIONS))
   {
      /* Next, let's make sure that the Scheduled Function specified    */
      /* appears to be semi-valid.                                      */
      if(SchedulerFunction)
      {
         /* Simply add the Scheduled Function into the Next available   */
         /* Schedule Slot.                                              */
         SchedulerInformation[NumberScheduledFunctions].ScheduleCount     = 0;
         SchedulerInformation[NumberScheduledFunctions].ScheduleFunction  = SchedulerFunction;
         SchedulerInformation[NumberScheduledFunctions].ScheduleParameter = SchedulerParameter;

#if BTPS_MINIMUM_SCHEDULER_RESOLUTION

         if(Period < BTPS_MINIMUM_SCHEDULER_RESOLUTION)
            Period = BTPS_MINIMUM_SCHEDULER_RESOLUTION;

#endif

         SchedulerInformation[NumberScheduledFunctions].ScheduleExpireCount = Period;

         /* Update the total number of Functions that have been added to*/
         /* the Scheduler.                                              */
         NumberScheduledFunctions++;

         /* Finally return success to the caller.                       */
         ret_val = TRUE;
      }
      else
         ret_val = FALSE;
   }
   else
      ret_val = FALSE;

   /* Finally return the result to the caller.                          */
   return(ret_val);
}

   /* The following function is provided to allow a mechanism for       */
   /* deleting a Function that has previously been registered with the  */
   /* Scheduler via a successful call to the                            */
   /* BTPS_AddFunctionToScheduler() function.  This function accepts as */
   /* input the Scheduler Function to that was added to the Scheduler,  */
   /* as well as the Scheduler Parameter that was registered.  Both of  */
   /* these values *must* match to remove a specific Scheduler Entry.   */
void BTPSAPI BTPS_DeleteFunctionFromScheduler(BTPS_SchedulerFunction_t SchedulerFunction, void *SchedulerParameter)
{
   unsigned int Index;

   /* First, let's make sure that the Scheduler has been initialized    */
   /* successfully AND that the Scheduler is NOT full.                  */
   if(SchedulerInitialized)
   {
      /* Next, let's make sure that the Scheduled Function specified    */
      /* appears to be semi-valid.                                      */
      if(SchedulerFunction)
      {
         /* Loop through the scheduler and remove the function (if we   */
         /* find it).                                                   */
         for(Index = 0; Index < NumberScheduledFunctions; Index++)
         {
            if((SchedulerInformation[Index].ScheduleFunction == SchedulerFunction) && (SchedulerInformation[Index].ScheduleParameter == SchedulerParameter))
               break;
         }

         /* Check to see if we have found the scheduled function.  Also,*/
         /* if we have we need to copy all the functions in the         */
         /* scheduler that exist AFTER the current index into the       */
         /* Scheduler array starting at the index that was found above. */
         if(Index < NumberScheduledFunctions)
         {
            /* Simply start copying from the next location back one in  */
            /* the array.                                               */
            for(Index++;Index<NumberScheduledFunctions;Index++)
            {
               SchedulerInformation[Index - 1].ScheduleCount       = SchedulerInformation[Index].ScheduleCount;
               SchedulerInformation[Index - 1].ScheduleFunction    = SchedulerInformation[Index].ScheduleFunction;
               SchedulerInformation[Index - 1].ScheduleParameter   = SchedulerInformation[Index].ScheduleParameter;
               SchedulerInformation[Index - 1].ScheduleExpireCount = SchedulerInformation[Index].ScheduleExpireCount;
            }

            /* Update the total number of Functions that have been added*/
            /* to the Scheduler.                                        */
            NumberScheduledFunctions--;
         }
      }
   }
}

   /* The following function begins execution of the actual Scheduler.  */
   /* Once this function is called, it NEVER returns.  This function is */
   /* responsible for executing all functions that have been added to   */
   /* the Scheduler with the BTPS_AddFunctionToScheduler() function.    */
void BTPSAPI BTPS_ExecuteScheduler(void)
{
   Boolean_t Done;

   /* Initialize the Scheduler state information.                       */
   Done = FALSE;

   /* Simply loop until Done (Forever), calling each scheduled function */
   /* when it expires.                                                  */
   while(!Done)
   {
      /* Simply process the scheduler.                                  */
      BTPS_ProcessScheduler();
   }
}

   /* The following function is provided to allow a mechanism to process*/
   /* the scheduled functions in the scheduler.  This function performs */
   /* the same functionality as the BTPS_ExecuteScheduler() function    */
   /* except that it returns as soon as it has made an iteration through*/
   /* the scheduled functions.  This function is provided for platforms */
   /* that would like to implement their own processing loop and/or     */
   /* scheduler and not rely on the Bluetopia implementation.           */
   /* * NOTE * This function should NEVER be called if the              */
   /*          BTPS_ExecuteScheduler() schema is used.                  */
   /* * NOTE * This function *MUST* not be called from any scheduled    */
   /*          function that was added to the scheduler via the         */
   /*          BTPS_AddFunctionToScheduler() function or an infinite    */
   /*          loop will occur.                                         */
void BTPSAPI BTPS_ProcessScheduler(void)
{
   unsigned int  SchedulerPointer;
   unsigned long ElapsedTicks;
   unsigned long CurrentTickCount;

   /* First, let's calculate the Elapsed Number of Ticks that have      */
   /* occurred since the last time through the scheduler (taking into   */
   /* account the possibility that the Tick Counter could have wrapped).*/
   CurrentTickCount = BTPS_GetTickCount();
   ElapsedTicks     = CurrentTickCount - PreviousTickCount;

   if(ElapsedTicks)
   {
      /* Now that we have calculated the Elapsed Time, let's increment  */
      /* the Scheduler Count for each Scheduled Function by the Elapsed */
      /* Tick Amount.                                                   */
      SchedulerPointer = 0;
      while(SchedulerPointer < NumberScheduledFunctions)
      {
         SchedulerInformation[SchedulerPointer].ScheduleCount += ElapsedTicks;

         if(SchedulerInformation[SchedulerPointer].ScheduleCount >= SchedulerInformation[SchedulerPointer].ScheduleExpireCount)
         {
            /* Simply call the Scheduled function.                      */
            (*(BTPS_SchedulerFunction_t)(SchedulerInformation[SchedulerPointer].ScheduleFunction))(SchedulerInformation[SchedulerPointer].ScheduleParameter);

            /* Reset the current Schedule Count back to zero.           */
            SchedulerInformation[SchedulerPointer].ScheduleCount = 0;
         }

         SchedulerPointer++;
      }

      /* Note the last time that we processed the scheduler.            */
      PreviousTickCount = CurrentTickCount;
   }
}

   /* The following function is provided to allow a mechanism to        */
   /* actually allocate a Block of Memory (of at least the specified    */
   /* size).  This function accepts as input the size (in Bytes) of the */
   /* Block of Memory to be allocated.  This function returns a NON-NULL*/
   /* pointer to this Memory Buffer if the Memory was successfully      */
   /* allocated, or a NULL value if the memory could not be allocated.  */
void *BTPSAPI BTPS_AllocateMemory(unsigned long MemorySize)
{
   void *ret_val;

   ret_val = MemAlloc(MemoryBuffer, MemorySize);

   if(!ret_val)
      DBG_MSG(DBG_ZONE_BTPSKRNL, ("Malloc Failed: %d.\r\n", MemorySize));

   /* Finally return the result to the caller.                          */
   return(ret_val);
}

   /* The following function is responsible for de-allocating a Block of*/
   /* Memory that was successfully allocated with the                   */
   /* BTPS_AllocateMemory() function.  This function accepts a NON-NULL */
   /* Memory Pointer which was returned from the BTPS_AllocateMemory()  */
   /* function.  After this function completes the caller CANNOT use ANY*/
   /* of the Memory pointed to by the Memory Pointer.                   */
void BTPSAPI BTPS_FreeMemory(void *MemoryPointer)
{
   MemFree(MemoryBuffer, MemoryPointer);
}

   /* The following function is responsible for copying a block of      */
   /* memory of the specified size from the specified source pointer to */
   /* the specified destination memory pointer.  This function accepts  */
   /* as input a pointer to the memory block that is to be Destination  */
   /* Buffer (first parameter), a pointer to memory block that points to*/
   /* the data to be copied into the destination buffer, and the size   */
   /* (in bytes) of the Data to copy.  The Source and Destination Memory*/
   /* Buffers must contain AT LEAST as many bytes as specified by the   */
   /* Size parameter.                                                   */
   /* * NOTE * This function does not allow the overlapping of the      */
   /*          Source and Destination Buffers !!!!                      */
void BTPSAPI BTPS_MemCopy(void *Destination, BTPSCONST void *Source, unsigned long Size)
{
   /* Simply wrap the C Run-Time memcpy() function.                     */
   memcpy(Destination, Source, Size);
}

   /* The following function is responsible for moving a block of memory*/
   /* of the specified size from the specified source pointer to the    */
   /* specified destination memory pointer.  This function accepts as   */
   /* input a pointer to the memory block that is to be Destination     */
   /* Buffer (first parameter), a pointer to memory block that points to*/
   /* the data to be copied into the destination buffer, and the size   */
   /* (in bytes) of the Data to copy.  The Source and Destination Memory*/
   /* Buffers must contain AT LEAST as many bytes as specified by the   */
   /* Size parameter.                                                   */
   /* * NOTE * This function DOES allow the overlapping of the Source   */
   /*          and Destination Buffers.                                 */
void BTPSAPI BTPS_MemMove(void *Destination, BTPSCONST void *Source, unsigned long Size)
{
   /* Simply wrap the C Run-Time memmove() function.                    */
   memmove(Destination, Source, Size);
}

   /* The following function is provided to allow a mechanism to fill a */
   /* block of memory with the specified value.  This function accepts  */
   /* as input a pointer to the Data Buffer (first parameter) that is to*/
   /* filled with the specified value (second parameter).  The final    */
   /* parameter to this function specifies the number of bytes that are */
   /* to be filled in the Data Buffer.  The Destination Buffer must     */
   /* point to a Buffer that is AT LEAST the size of the Size parameter.*/
void BTPSAPI BTPS_MemInitialize(void *Destination, unsigned char Value, unsigned long Size)
{
   /* Simply wrap the C Run-Time memset() function.                     */
   memset(Destination, Value, Size);
}

   /* The following function is provided to allow a mechanism to Compare*/
   /* two blocks of memory to see if the two memory blocks (each of size*/
   /* Size (in bytes)) are equal (each and every byte up to Size bytes).*/
   /* This function returns a negative number if Source1 is less than   */
   /* Source2, zero if Source1 equals Source2, and a positive value if  */
   /* Source1 is greater than Source2.                                  */
int BTPSAPI BTPS_MemCompare(BTPSCONST void *Source1, BTPSCONST void *Source2, unsigned long Size)
{
   /* Simply wrap the C Run-Time memcmp() function.                     */
   return(memcmp(Source1, Source2, Size));
}

   /* The following function is provided to allow a mechanism to Compare*/
   /* two blocks of memory to see if the two memory blocks (each of size*/
   /* Size (in bytes)) are equal (each and every byte up to Size bytes) */
   /* using a Case-Insensitive Compare.  This function returns a        */
   /* negative number if Source1 is less than Source2, zero if Source1  */
   /* equals Source2, and a positive value if Source1 is greater than   */
   /* Source2.                                                          */
int BTPSAPI BTPS_MemCompareI(BTPSCONST void *Source1, BTPSCONST void *Source2, unsigned long Size)
{
   int           ret_val = 0;
   unsigned char Byte1;
   unsigned char Byte2;
   unsigned int  Index;

   /* Simply loop through each byte pointed to by each pointer and check*/
   /* to see if they are equal.                                         */
   for(Index = 0; ((Index<Size) && (!ret_val)); Index++)
   {
      /* Note each Byte that we are going to compare.                   */
      Byte1 = ((unsigned char *)Source1)[Index];
      Byte2 = ((unsigned char *)Source2)[Index];

      /* If the Byte in the first array is lower case, go ahead and make*/
      /* it upper case (for comparisons below).                         */
      if((Byte1 >= 'a') && (Byte1 <= 'z'))
         Byte1 = Byte1 - ('a' - 'A');

      /* If the Byte in the second array is lower case, go ahead and    */
      /* make it upper case (for comparisons below).                    */
      if((Byte2 >= 'a') && (Byte2 <= 'z'))
         Byte2 = Byte2 - ('a' - 'A');

      /* If the two Bytes are equal then there is nothing to do.        */
      if(Byte1 != Byte2)
      {
         /* Bytes are not equal, so set the return value accordingly.   */
         if(Byte1 < Byte2)
            ret_val = -1;
         else
            ret_val = 1;
      }
   }

   /* Simply return the result of the above comparison(s).              */
   return(ret_val);
}

   /* The following function is provided to allow a mechanism to copy a */
   /* source NULL Terminated ASCII (character) String to the specified  */
   /* Destination String Buffer.  This function accepts as input a      */
   /* pointer to a buffer (Destination) that is to receive the NULL     */
   /* Terminated ASCII String pointed to by the Source parameter.  This */
   /* function copies the string byte by byte from the Source to the    */
   /* Destination (including the NULL terminator).                      */
void BTPSAPI BTPS_StringCopy(char *Destination, BTPSCONST char *Source)
{
   /* Simply wrap the C Run-Time strcpy() function.                     */
   strcpy(Destination, Source);
}

   /* The following function is provided to allow a mechanism to        */
   /* determine the Length (in characters) of the specified NULL        */
   /* Terminated ASCII (character) String.  This function accepts as    */
   /* input a pointer to a NULL Terminated ASCII String and returns the */
   /* number of characters present in the string (NOT including the     */
   /* terminating NULL character).                                      */
unsigned int BTPSAPI BTPS_StringLength(BTPSCONST char *Source)
{
   /* Simply wrap the C Run-Time strlen() function.                     */
   return(strlen(Source));
}

   /* The following function is provided to allow a mechanism for a C   */
   /* Run-Time Library sprintf() function implementation.  This function*/
   /* accepts as its input the output buffer, a format string and a     */
   /* variable number of arguments determined by the format string.     */
int BTPSAPI BTPS_SprintF(char *Buffer, BTPSCONST char *Format, ...)
{
   int     ret_val;
   va_list args;

   va_start(args, Format);
   ret_val = vsprintf(Buffer, Format, args);
   va_end(args);

   return(ret_val);
}

   /* The following function is provided to allow a mechanism to create */
   /* a Mailbox.  A Mailbox is a Data Store that contains slots (all of */
   /* the same size) that can have data placed into (and retrieved      */
   /* from).  Once Data is placed into a Mailbox (via the               */
   /* BTPS_AddMailbox() function, it can be retrieved by using the      */
   /* BTPS_WaitMailbox() function.  Data placed into the Mailbox is     */
   /* retrieved in a FIFO method.  This function accepts as input the   */
   /* Maximum Number of Slots that will be present in the Mailbox and   */
   /* the Size of each of the Slots.  This function returns a NON-NULL  */
   /* Mailbox Handle if the Mailbox is successfully created, or a NULL  */
   /* Mailbox Handle if the Mailbox was unable to be created.           */
Mailbox_t BTPSAPI BTPS_CreateMailbox(unsigned int NumberSlots, unsigned int SlotSize)
{
   Mailbox_t        ret_val;
   MailboxHeader_t *MailboxHeader;

   /* Before proceeding any further we need to make sure that the       */
   /* parameters that were passed to us appear semi-valid.              */
   if((NumberSlots) && (SlotSize))
   {
      /* Parameters appear semi-valid, so now let's allocate enough     */
      /* Memory to hold the Mailbox Header AND enough space to hold all */
      /* requested Mailbox Slots.                                       */
      if((MailboxHeader = (MailboxHeader_t *)BTPS_AllocateMemory(sizeof(MailboxHeader_t)+(NumberSlots*SlotSize))) != NULL)
      {
         /* Memory allocated, now let's initialize the state of the     */
         /* Mailbox such that it contains NO Data.                      */
         MailboxHeader->NumberSlots   = NumberSlots;
         MailboxHeader->SlotSize      = SlotSize;
         MailboxHeader->HeadSlot      = 0;
         MailboxHeader->TailSlot      = 0;
         MailboxHeader->OccupiedSlots = 0;
         MailboxHeader->Slots         = ((unsigned char *)MailboxHeader) + sizeof(MailboxHeader_t);

         /* All finished, return success to the caller (the Mailbox     */
         /* Header).                                                    */
         ret_val                      = (Mailbox_t)MailboxHeader;
      }
      else
         ret_val = NULL;
   }
   else
      ret_val = NULL;

   /* Return the result to the caller.                                  */
   return(ret_val);
}

   /* The following function is provided to allow a means to Add data to*/
   /* the Mailbox (where it can be retrieved via the BTPS_WaitMailbox() */
   /* function.  This function accepts as input the Mailbox Handle of   */
   /* the Mailbox to place the data into and a pointer to a buffer that */
   /* contains the data to be added.  This pointer *MUST* point to a    */
   /* data buffer that is AT LEAST the Size of the Slots in the Mailbox */
   /* (specified when the Mailbox was created) and this pointer CANNOT  */
   /* be NULL.  The data that the MailboxData pointer points to is      */
   /* placed into the Mailbox where it can be retrieved via the         */
   /* BTPS_WaitMailbox() function.                                      */
   /* * NOTE * This function copies from the MailboxData Pointer the    */
   /*          first SlotSize Bytes.  The SlotSize was specified when   */
   /*          the Mailbox was created via a successful call to the     */
   /*          BTPS_CreateMailbox() function.                           */
Boolean_t BTPSAPI BTPS_AddMailbox(Mailbox_t Mailbox, void *MailboxData)
{
   Boolean_t ret_val;

   /* Before proceeding any further make sure that the Mailbox Handle   */
   /* and the MailboxData pointer that was specified appears semi-valid.*/
   if((Mailbox) && (MailboxData))
   {
      /* Before adding the data to the Mailbox, make sure that the      */
      /* Mailbox is not already full.                                   */
      if(((MailboxHeader_t *)Mailbox)->OccupiedSlots < ((MailboxHeader_t *)Mailbox)->NumberSlots)
      {
         /* Mailbox is NOT full, so add the specified User Data to the  */
         /* next available free Mailbox Slot.                           */
         BTPS_MemCopy(&(((unsigned char *)(((MailboxHeader_t *)Mailbox)->Slots))[((MailboxHeader_t *)Mailbox)->HeadSlot*((MailboxHeader_t *)Mailbox)->SlotSize]), MailboxData, ((MailboxHeader_t *)Mailbox)->SlotSize);

         /* Update the Next available Free Mailbox Slot (taking into    */
         /* account wrapping the pointer).                              */
         if(++(((MailboxHeader_t *)Mailbox)->HeadSlot) == ((MailboxHeader_t *)Mailbox)->NumberSlots)
            ((MailboxHeader_t *)Mailbox)->HeadSlot = 0;

         /* Update the Number of occupied slots to signify that there   */
         /* was additional Mailbox Data added to the Mailbox.           */
         ((MailboxHeader_t *)Mailbox)->OccupiedSlots++;

         /* Finally, return success to the caller.                      */
         ret_val = TRUE;
      }
      else
         ret_val = FALSE;
   }
   else
      ret_val = FALSE;

   /* Return the result to the caller.                                  */
   return(ret_val);
}

   /* The following function is provided to allow a means to retrieve   */
   /* data from the specified Mailbox.  This function will block until  */
   /* either Data is placed in the Mailbox or an error with the Mailbox */
   /* was detected.  This function accepts as its first parameter a     */
   /* Mailbox Handle that represents the Mailbox to wait for the data   */
   /* with.  This function accepts as its second parameter, a pointer to*/
   /* a data buffer that is AT LEAST the size of a single Slot of the   */
   /* Mailbox (specified when the BTPS_CreateMailbox() function was     */
   /* called).  The MailboxData parameter CANNOT be NULL.  This function*/
   /* will return TRUE if data was successfully retrieved from the      */
   /* Mailbox or FALSE if there was an error retrieving data from the   */
   /* Mailbox.  If this function returns TRUE then the first SlotSize   */
   /* bytes of the MailboxData pointer will contain the data that was   */
   /* retrieved from the Mailbox.                                       */
   /* * NOTE * This function copies to the MailboxData Pointer the data */
   /*          that is present in the Mailbox Slot (of size SlotSize).  */
   /*          The SlotSize was specified when the Mailbox was created  */
   /*          via a successful call to the BTPS_CreateMailbox()        */
   /*          function.                                                */
Boolean_t BTPSAPI BTPS_WaitMailbox(Mailbox_t Mailbox, void *MailboxData)
{
   Boolean_t ret_val;

   /* Before proceeding any further make sure that the Mailbox Handle   */
   /* and the MailboxData pointer that was specified appears semi-valid.*/
   if((Mailbox) && (MailboxData))
   {
      /* Let's check to see if there exists at least one slot with      */
      /* Mailbox Data present in it.                                    */
      if(((MailboxHeader_t *)Mailbox)->OccupiedSlots)
      {
         /* Flag success to the caller.                                 */
         ret_val = TRUE;

         /* Now copy the Data into the Memory Buffer specified by the   */
         /* caller.                                                     */
         BTPS_MemCopy(MailboxData, &((((unsigned char *)((MailboxHeader_t *)Mailbox)->Slots))[((MailboxHeader_t *)Mailbox)->TailSlot*((MailboxHeader_t *)Mailbox)->SlotSize]), ((MailboxHeader_t *)Mailbox)->SlotSize);

         /* Now that we've copied the data into the Memory Buffer       */
         /* specified by the caller we need to mark the Mailbox Slot as */
         /* free.                                                       */
         if(++(((MailboxHeader_t *)Mailbox)->TailSlot) == ((MailboxHeader_t *)Mailbox)->NumberSlots)
            ((MailboxHeader_t *)Mailbox)->TailSlot = 0;

         ((MailboxHeader_t *)Mailbox)->OccupiedSlots--;
      }
      else
         ret_val = FALSE;
   }
   else
      ret_val = FALSE;

   /* Return the result to the caller.                                  */
   return(ret_val);
}

   /* The following function is a utility function that exists to       */
   /* determine if there is anything queued in the specified Mailbox.   */
   /* This function returns TRUE if there is something queued in the    */
   /* Mailbox, or FALSE if there is nothing queued in the specified     */
   /* Mailbox.                                                          */
Boolean_t BTPSAPI BTPS_QueryMailbox(Mailbox_t Mailbox)
{
   Boolean_t ret_val;

   /* Before proceeding any further make sure that the Mailbox Handle   */
   /* and the MailboxData pointer that was specified appears semi-valid.*/
   if(Mailbox)
   {
      /* Let's check to see if there exists at least one slot with      */
      /* Mailbox Data present in it.                                    */
      if(((MailboxHeader_t *)Mailbox)->OccupiedSlots)
      {
         /* Flag success to the caller.                                 */
         ret_val = TRUE;
      }
      else
         ret_val = FALSE;
   }
   else
      ret_val = FALSE;

   /* Return the result to the caller.                                  */
   return(ret_val);
}

   /* The following function is responsible for destroying a Mailbox    */
   /* that was created successfully via a successful call to the        */
   /* BTPS_CreateMailbox() function.  This function accepts as input the*/
   /* Mailbox Handle of the Mailbox to destroy.  Once this function is  */
   /* completed the Mailbox Handle is NO longer valid and CANNOT be     */
   /* used.  Calling this function will cause all outstanding           */
   /* BTPS_WaitMailbox() functions to fail with an error.  The final    */
   /* parameter specifies an (optional) callback function that is called*/
   /* for each queued Mailbox entry.  This allows a mechanism to free   */
   /* any resources that might be associated with each individual       */
   /* Mailbox item.                                                     */
void BTPSAPI BTPS_DeleteMailbox(Mailbox_t Mailbox, BTPS_MailboxDeleteCallback_t MailboxDeleteCallback)
{
   /* Before proceeding any further make sure that the Mailbox Handle   */
   /* that was specified appears semi-valid.                            */
   if(Mailbox)
   {
      /* Check to see if a Mailbox Delete Item Callback was specified.  */
      if(MailboxDeleteCallback)
      {
         /* Now loop though all of the occupied slots and call the      */
         /* callback with the slot data.                                */
         while(((MailboxHeader_t *)Mailbox)->OccupiedSlots)
         {
            __BTPSTRY
            {
               (*MailboxDeleteCallback)(&((((unsigned char *)((MailboxHeader_t *)Mailbox)->Slots))[((MailboxHeader_t *)Mailbox)->TailSlot*((MailboxHeader_t *)Mailbox)->SlotSize]));
            }
            __BTPSEXCEPT(1)
            {
               /* Do Nothing.                                           */
            }

            /* Now that we've called back with the data, we need to     */
            /* advance to the next slot.                                */
            if(++(((MailboxHeader_t *)Mailbox)->TailSlot) == ((MailboxHeader_t *)Mailbox)->NumberSlots)
               ((MailboxHeader_t *)Mailbox)->TailSlot = 0;

            /* Flag that there is one less occupied slot.               */
            ((MailboxHeader_t *)Mailbox)->OccupiedSlots--;
         }
      }

      /* Finally free all memory that was allocated for the Mailbox.    */
      BTPS_FreeMemory(Mailbox);
   }
}

   /* The following function is used to initialize the Platform module. */
   /* The Platform module relies on some static variables that are used */
   /* to coordinate the abstraction.  When the module is initially      */
   /* started from a cold boot, all variables are set to the proper     */
   /* state.  If the Warm Boot is required, then these variables need to*/
   /* be reset to their default values.  This function sets all static  */
   /* parameters to their default values.                               */
   /* * NOTE * The implementation is free to pass whatever information  */
   /*          required in this parameter.                              */
void BTPSAPI BTPS_Init(void *UserParam)
{
   /* Input parameter represents the Debug Message Output Callback      */
   /* function.                                                         */
   if(UserParam)
   {
      if(((BTPS_Initialization_t *)UserParam)->GetTickCountCallback)
         GetTickCountCallback  = ((BTPS_Initialization_t *)UserParam)->GetTickCountCallback;

      if(((BTPS_Initialization_t *)UserParam)->MessageOutputCallback)
         MessageOutputCallback = ((BTPS_Initialization_t *)UserParam)->MessageOutputCallback;
   }
   else
      MessageOutputCallback = NULL;

   /* Initialize the Heap.                                              */
   HeapInit(MemoryBuffer, sizeof(MemoryBuffer));

   /* Initialize the static variables for this module.                  */
   DebugZoneMask              = DEBUG_ZONES;
   NumberScheduledFunctions   = 0;
   PreviousTickCount          = 0;

   /* Finally flag that the Scheduler has been initialized successfully.*/
   SchedulerInitialized       = TRUE;
}

   /* The following function is used to cleanup the Platform module.    */
void BTPSAPI BTPS_DeInit(void)
{
   MessageOutputCallback = NULL;
   SchedulerInitialized  = FALSE;
}

   /* Write out the specified NULL terminated Debugging String to the   */
   /* Debug output.                                                     */
void BTPSAPI BTPS_OutputMessage(BTPSCONST char *DebugString, ...)
{
   va_list args;
   int     Length;
   char    MsgBuffer[256];

   if(MessageOutputCallback)
   {
      /* Write out the Data.                                            */
      va_start(args, DebugString);

      Length = vsnprintf(MsgBuffer, (sizeof(MsgBuffer) - 1), DebugString, args);

      va_end(args);

      MessageOutputCallback((unsigned int)Length, MsgBuffer);
   }
}

   /* The following function is used to set the Debug Mask that controls*/
   /* which debug zone messages get displayed.  The function takes as   */
   /* its only parameter the Debug Mask value that is to be used.  Each */
   /* bit in the mask corresponds to a debug zone.  When a bit is set,  */
   /* the printing of that debug zone is enabled.                       */
void BTPSAPI BTPS_SetDebugMask(unsigned long DebugMask)
{
   DebugZoneMask = DebugMask;
}

   /* The following function is a utility function that can be used to  */
   /* determine if a specified Zone is currently enabled for debugging. */
int BTPSAPI BTPS_TestDebugZone(unsigned long Zone)
{
   return(DebugZoneMask & Zone);
}

   /* The following function is responsible for displaying binary debug */
   /* data.  The first parameter to this function is the length of data */
   /* pointed to by the next parameter.  The final parameter is a       */
   /* pointer to the binary data to be displayed.                       */
int BTPSAPI BTPS_DumpData(unsigned int DataLength, BTPSCONST unsigned char *DataPtr)
{
   int          ret_val;
   unsigned int Index;
   unsigned int Offset;

   /* Before proceeding any further, lets make sure that the parameters */
   /* passed to us appear semi-valid.                                   */
   if((DataLength > 0) && (DataPtr != NULL))
   {
      /* Print out the header.                                          */
      BTPS_OutputMessage("\r\n");
      BTPS_OutputMessage("  -OFFSET- | 00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F\r\n");
      BTPS_OutputMessage(" ----------+-------------------------------------------------\r\n");

      Offset = 0;

      /* Output the Debug Data, 16 bytes per line.                      */
      while(DataLength)
      {
         /* Print the line header.                                      */
         BTPS_OutputMessage("  %08X |", Offset);
         Offset += 16;

         /* Print the row of data.                                      */
         for(Index = 0; (Index < 16) && (DataLength); Index ++)
         {
            BTPS_OutputMessage(" %02X", *DataPtr);

            DataLength --;
            DataPtr    ++;
         }

         BTPS_OutputMessage("\r\n");
      }

      BTPS_OutputMessage("\r\n");

      /* Finally, set the return value to indicate success.             */
      ret_val = 0;
   }
   else
      ret_val = -1;

   return(ret_val);
}
