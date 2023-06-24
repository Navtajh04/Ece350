/*
 ****************************************************************************
 *
 *                  UNIVERSITY OF WATERLOO ECE 350 RTOS LAB
 *
 *                     Copyright 2020-2022 Yiqing Huang
 *                          All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *  - Redistributions of source code must retain the above copyright
 *    notice and the following disclaimer.
 *
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL COPYRIGHT HOLDERS AND CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 ****************************************************************************
 */

/**************************************************************************//**
 * @file        k_mem.c
 * @brief       Kernel Memory Management API C Code
 *
 * @version     V1.2021.01.lab2
 * @authors     Yiqing Huang
 * @date        2021 JAN
 *
 * @note        skeleton code
 *
 *****************************************************************************/

#include "k_inc.h"
#include "k_mem.h"
#include "common.h"
#include "helper.h"

/*---------------------------------------------------------------------------
The memory map of the OS image may look like the following:
                   RAM1_END-->+---------------------------+ High Address
                              |                           |
                              |                           |
                              |       MPID_IRAM1          |
                              |   (for user space heap  ) |
                              |                           |
                 RAM1_START-->|---------------------------|
                              |                           |
                              |  unmanaged free space     |
                              |                           |
&Image$$RW_IRAM1$$ZI$$Limit-->|---------------------------|-----+-----
                              |         ......            |     ^
                              |---------------------------|     |
                              |                           |     |
                              |---------------------------|     |
                              |                           |     |
                              |      other data           |     |
                              |---------------------------|     |
                              |      PROC_STACK_SIZE      |  OS Image
              g_p_stacks[2]-->|---------------------------|     |
                              |      PROC_STACK_SIZE      |     |
              g_p_stacks[1]-->|---------------------------|     |
                              |      PROC_STACK_SIZE      |     |
              g_p_stacks[0]-->|---------------------------|     |
                              |   other  global vars      |     |
                              |                           |  OS Image
                              |---------------------------|     |
                              |      KERN_STACK_SIZE      |     |                
   g _k_stacks[MAX_TASKS-1]-->|---------------------------|     |
                              |                           |     |
                              |     other kernel stacks   |     |                              
                              |---------------------------|     |
                              |      KERN_STACK_SIZE      |  OS Image
              g_k_stacks[2]-->|---------------------------|     |
                              |      KERN_STACK_SIZE      |     |                      
              g_k_stacks[1]-->|---------------------------|     |
                              |      KERN_STACK_SIZE      |     |
              g_k_stacks[0]-->|---------------------------|     |
                              |   other  global vars      |     |
                              |---------------------------|     |
                              |        TCBs               |  OS Image
                      g_tcbs->|---------------------------|     |
                              |        global vars        |     |
                              |---------------------------|     |
                              |                           |     |          
                              |                           |     |
                              |        Code + RO          |     |
                              |                           |     V
                 IRAM1_BASE-->+---------------------------+ Low Address
    
---------------------------------------------------------------------------*/ 

/*
 *===========================================================================
 *                            GLOBAL VARIABLES
 *===========================================================================
 */
// kernel stack size, referred by startup_a9.s
// const U32 g_k_stack_size = KERN_STACK_SIZE;
// task proc space stack size in bytes, referred by system_a9.c
// const U32 g_p_stack_size = PROC_STACK_SIZE;

// task kernel stacks
// U32 g_k_stacks[MAX_TASKS][KERN_STACK_SIZE >> 2] __attribute__((aligned(8)));

// task process stack (i.e. user stack) for tasks in thread mode
// remove this bug array in your lab2 code
// the user stack should come from MPID_IRAM2 memory pool
//U32 g_p_stacks[MAX_TASKS][PROC_STACK_SIZE >> 2] __attribute__((aligned(8)));
//U32 g_p_stacks[NUM_TASKS][PROC_STACK_SIZE >> 2] __attribute__((aligned(8)));

static free_memory_block_t* iram1FreeList[IRAM1_MAX_BLK_SIZE_LOG2 - MIN_BLK_SIZE_LOG2];
static free_memory_block_t* iram2FreeList[IRAM2_MAX_BLK_SIZE_LOG2 - MIN_BLK_SIZE_LOG2];

/*
 *===========================================================================
 *                            FUNCTIONS
 *===========================================================================
 */

/* note list[n] is for blocks with order of n */
mpool_t k_mpool_create (int algo, U32 start, U32 end){
    mpool_t mpid = MPID_IRAM1;

#ifdef DEBUG_0
    printf("k_mpool_init: algo = %d\r\n", algo);
    printf("k_mpool_init: RAM range: [0x%x, 0x%x].\r\n", start, end);
#endif /* DEBUG_0 */    
    
    if (algo != BUDDY ) {
        errno = EINVAL;
        return RTX_ERR;
    }
    
    if ( start == RAM1_START) {
        mpid = MPID_IRAM1;
        // Create the initial memory block
        free_memory_block_t* block = (free_memory_block_t*) start;
        block->size = end - start - ALLOCATED_BLK_META_SIZE;
        block->freeFlag = 1;
        block->prev = NULL;
        block->next = NULL;

        iram1FreeList[IRAM1_MAX_BLK_SIZE_LOG2 - MIN_BLK_SIZE_LOG2 - 1] = block;
    } else if ( start == RAM2_START) {
        mpid = MPID_IRAM2;
        // add your own code
        free_memory_block_t* block = (free_memory_block_t*) start;
        block->size = start - end - ALLOCATED_BLK_META_SIZE;
        block->freeFlag = 1;
        block->prev = NULL;
        block->next = NULL;

        iram2FreeList[IRAM2_MAX_BLK_SIZE_LOG2 - MIN_BLK_SIZE_LOG2 - 1] = block;
    } else {
        errno = EINVAL;
        return RTX_ERR;
    }
    
    return mpid;
}

void *k_mpool_alloc (mpool_t mpid, size_t size)
{
#ifdef DEBUG_0
    printf("k_mpool_alloc: mpid = %d, size = %d, 0x%x\r\n", mpid, size, size);
#endif /* DEBUG_0 */
    free_memory_block_t** freeList;
    int maxBlkSizeLog2;

    if (mpid == MPID_IRAM1) {
        freeList = iram1FreeList;
        maxBlkSizeLog2 = IRAM1_MAX_BLK_SIZE_LOG2;
    } else if (mpid == MPID_IRAM2) {
        freeList = iram2FreeList;
        maxBlkSizeLog2 = IRAM2_MAX_BLK_SIZE_LOG2;
    } else {
        errno = EINVAL;
        return RTX_ERR;
    }

     // Calculate the required block size
    size_t blockSize = size + ALLOCATED_BLK_META_SIZE;  // Including size info and padding
    blockSize = (blockSize + 7) & ~0x07;  // Align to 8 bytes

    // Find the appropriate block size in the free list
    int blkSizeLog2 = log_two_ceil(blockSize);
    if (blkSizeLog2 < MIN_BLK_SIZE_LOG2) {
        blkSizeLog2 = MIN_BLK_SIZE_LOG2;
    } else if (blkSizeLog2 > maxBlkSizeLog2) {
        return NULL;  // Requested size exceeds maximum block size
    }

    free_memory_block_t* block;
    U8 i = blkSizeLog2;
    while((block == NULL) && (i <= maxBlkSizeLog2)){
        block = freeList[i];
        ++i;
    }

    if(i > maxBlkSizeLog2){
        // there was no memory block large enough that was free
        return NULL;
    }

    // Remove the block from it's curret linked list
    freeList[i] = block->next;

    while(i + MIN_BLK_SIZE_LOG2 > blkSizeLog2){
        i--;
        // keep splitting the current smallest suitable block into 2 buddies until it is blkSizeLog2 size
        free_memory_block_t* buddy = (free_memory_block_t*)((char*)block + (block->size >> 1));
        buddy->size = block->size >> 1;

        block->size >>= 1;
        // block->next = buddy;

        // Update the free list at the corresponding index
        buddy->next = freeList[i];
        buddy->prev = NULL;
        if (freeList[i] != NULL) {
            freeList[i]->prev = buddy;
        }
        freeList[i] = buddy;
    }
    block->freeFlag = 0;

    return (void *)((char *)block + ALLOCATED_BLK_META_SIZE);
}

int k_mpool_dealloc(mpool_t mpid, void *ptr)
{
#ifdef DEBUG_0
    printf("k_mpool_dealloc: mpid = %d, ptr = 0x%x\r\n", mpid, ptr);
#endif /* DEBUG_0 */
    free_memory_block_t** freeList;

    if (mpid == MPID_IRAM1) {
        if(ptr < RAM1_START || ptr > RAM1_END){
            errno = EFAULT;
            return RTX_ERR;
        }
        freeList = iram1FreeList;
    } else if (mpid == MPID_IRAM2) {
        if(ptr < RAM2_START || ptr > RAM2_END){
            errno = EFAULT;
            return RTX_ERR;
        }
        freeList = iram2FreeList;
    } else {
        errno = EINVAL;
        return RTX_ERR;  // Invalid memory pool ID
    }
    free_memory_block_t* freedBlock = (free_memory_block_t *)(ptr - ALLOCATED_BLK_META_SIZE);
    freedBlock->freeFlag = 1;
    // Get the address of the binary buddy of the block
    free_memory_block_t* buddy = (free_memory_block_t *)((char *)freedBlock ^ freedBlock->size);

    U8 index = log_two_ceil(freedBlock->size);

    // Keep coalescing until we reach a point where the binary buddy is not free
    while(buddy->freeFlag){
        // remove the buddy from the free block list it is currently in
        free_memory_block_t* prevBlock = buddy->prev;
        free_memory_block_t* nextBlock = buddy->next;
        if(prevBlock != NULL){
            prevBlock->next = nextBlock;
        } else {
            freeList[index] = nextBlock;
        }
        if(nextBlock != NULL){
            nextBlock->prev = prevBlock;
        }

        // coalesce the two blocks
        freedBlock->size <<= 1;
        buddy->size <<= 1;
        buddy = (free_memory_block_t *)((char *)freedBlock ^ freedBlock->size);
        ++index;
    }

    freedBlock->freeFlag = 1;
    // make sure freedblock starts at the address of the binary buddy that was at a lower part of memory
    freedBlock = (free_memory_block_t *)((char *)freedBlock & ~(freedBlock->size >> 1));
    freedBlock->next = freeList[index];
    freeList[index] = freedBlock;

    return RTX_OK; 
}

int k_mpool_dump (mpool_t mpid)
{
#ifdef DEBUG_0
    printf("k_mpool_dump: mpid = %d\r\n", mpid);
#endif /* DEBUG_0 */
    free_memory_block_t** freeList;
    U8 listLen;

    if (mpid == MPID_IRAM1) {
        if(ptr < RAM1_START || ptr > RAM1_END){
            errno = EFAULT;
            return RTX_ERR;
        }
        freeList = iram1FreeList;
        listLen = IRAM1_MAX_BLK_SIZE_LOG2 - MIN_BLK_SIZE_LOG2;
    } else if (mpid == MPID_IRAM2) {
        if(ptr < RAM2_START || ptr > RAM2_END){
            errno = EFAULT;
            return RTX_ERR;
        }
        freeList = iram2FreeList;
        listLen = IRAM2_MAX_BLK_SIZE_LOG2 - MIN_BLK_SIZE_LOG2;
    } else {
        errno = EINVAL;
        return RTX_ERR;  // Invalid memory pool ID
    }
    size_t freeBlockCount = 0;
    for(U8 i = 0; i < listLen; ++i){
        free_memory_block_t* currentBlk = freeList[i];
        while(currentBlk != NULL){
            printf("%x: %x\n", currentBlk, currentBlk->size);
            currentBlk = currentBlk->next;
            freeBlockCount++;
        }
    }
    printf("%zu free memory block(s) found\n", freeBlockCount);

    return 0;
}
 
int k_mem_init(int algo)
{
#ifdef DEBUG_0
    printf("k_mem_init: algo = %d\r\n", algo);
#endif /* DEBUG_0 */
        
    if ( k_mpool_create(algo, RAM1_START, RAM1_END) < 0 ) {
        return RTX_ERR;
    }
    
    if ( k_mpool_create(algo, RAM2_START, RAM2_END) < 0 ) {
        return RTX_ERR;
    }
    
    return RTX_OK;
}

/**
 * @brief allocate kernel stack statically
 */
U32* k_alloc_k_stack(task_t tid)
{
    
    if ( tid >= MAX_TASKS) {
        errno = EAGAIN;
        return NULL;
    }
    U32 *sp = g_k_stacks[tid+1];
    
    // 8B stack alignment adjustment
    if ((U32)sp & 0x04) {   // if sp not 8B aligned, then it must be 4B aligned
        sp--;               // adjust it to 8B aligned
    }
    return sp;
}

/**
 * @brief allocate user/process stack statically
 * @attention  you should not use this function in your lab
 */

U32* k_alloc_p_stack(task_t tid)
{
    if ( tid >= NUM_TASKS ) {
        errno = EAGAIN;
        return NULL;
    }
    
    U32 *sp = g_p_stacks[tid+1];
    
    
    // 8B stack alignment adjustment
    if ((U32)sp & 0x04) {   // if sp not 8B aligned, then it must be 4B aligned
        sp--;               // adjust it to 8B aligned
    }
    return sp;
}

/*
 *===========================================================================
 *                             END OF FILE
 *===========================================================================
 */

