#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "debug.h"
#include "sfmm.h"

/* Headers, Footers, and Block pointers occupy a single memory row (4 Words = 8-bytes) */
#define HEADER_SIZE    8   
#define FOOTER_SIZE    8    
#define BLOCK_PTR_SIZE 8
#define MIN_BLOCK_SIZE (HEADER_SIZE + FOOTER_SIZE + (2*BLOCK_PTR_SIZE))

/* Allocated blocks aligned to Double Memory Row (2 Rows = 16-bytes) */
#define ROW_ALIGNMENT  16 

/* 
 * Setup the header and footer of a 
 * Free Memory Block.
 * 
 * @param bsize : Size of the block : bytes
 * @param fb : Pointer to the Free Block 
 * @param prev_alloc : previous block allocated?
 *      0 : No
 *      1 : Yes
*/
static void 
init_free_block(int bsize, sf_block *fb, unsigned prev_alloc) {
    sf_header *footer; // Free Block Footer

    /* Setup Header */
    fb->header.block_size = bsize;
    fb->header.block_size &= ~(1UL); // Not Allocated
    fb->header.block_size |= (prev_alloc) ? PREV_BLOCK_ALLOCATED : 0;
    /* Setup Footer - Identical to Header */
    footer = (sf_header *)((void *)fb+bsize-FOOTER_SIZE); // Footer @ End of block
    footer->block_size = fb->header.block_size; 
    footer->requested_size = fb->header.requested_size; 
}

/*
 * Insert a Free Block into the Main Free List. 
 * 
 * @param free_block : Pointer to the Free Block
*/
static void 
insert_main(sf_block *free_block) {
    /* If list is empty */
    if(sf_free_list_head.body.links.next == &sf_free_list_head &&
       sf_free_list_head.body.links.prev == &sf_free_list_head) {
           sf_free_list_head.body.links.next = free_block;
           sf_free_list_head.body.links.prev = free_block;
           free_block->body.links.next = &sf_free_list_head;
           free_block->body.links.prev = &sf_free_list_head;
    } else {
        /* Always insert at beginning of list (LIFO) */
        free_block->body.links.next = sf_free_list_head.body.links.next;
        free_block->body.links.prev = &sf_free_list_head;
        (sf_free_list_head.body.links.next)->body.links.prev = free_block;
        sf_free_list_head.body.links.next = free_block;
    }
}

/*
 * Removes the first block of the passed in
 * Quick List.
 * 
 * @param : Index of Quick List
*/
static void 
remove_quick(int quick_i) {
    sf_block *rm; // Pointer to block being removed

    /* Update Quick List Length */
    sf_quick_lists[quick_i].length--; 

    /* Remove block */
    rm = sf_quick_lists[quick_i].first;
    sf_quick_lists[quick_i].first = sf_quick_lists[quick_i].first->body.links.next;
    rm->body.links.next = NULL;
}

/*
 * Remove a Free Block from the Main Free List. 
 * 
 * @param free_block : Pointer to the Free Block to remove
*/
static void
remove_main(sf_block *free_block) {
    free_block->body.links.next->body.links.prev = free_block->body.links.prev;
    free_block->body.links.prev->body.links.next = free_block->body.links.next;
    free_block->body.links.next = NULL;
    free_block->body.links.prev = NULL;
}

/*
 * Attempt to Split the passed in Block
 * without causing a splinter. 
 * 
 * @param block_ptr : Pointer to the Free Block being Split
 * @param bsize : Size of allocation request block
 * @param requested_size : Size of Allocation Request
 * 
 * @returns : NULL : If block cannot be split due to splinter
 * @returns : Address to remainder of block after splitting
*/
static void *
split_block(sf_block *block_ptr, int bsize, int requested_size) {
    int r_flag = 0; // Flag signaling a New Free Block (splitting successful)

    /* Make sure Splitting wont cause a Splinter */
    if(!(((block_ptr->header.block_size & BLOCK_SIZE_MASK) - bsize) < MIN_BLOCK_SIZE)) {
        r_flag = 1;
    } else {
        /* Cannot Split - Use Full Block */
        bsize = block_ptr->header.block_size & BLOCK_SIZE_MASK;
    }

    /* Setup Allocated Block Header */
    block_ptr->header.block_size = bsize; // New block size
    block_ptr->header.block_size |= THIS_BLOCK_ALLOCATED | PREV_BLOCK_ALLOCATED; // Block is now allcoated
    block_ptr->header.requested_size = requested_size;    // Size of original Allocation Request

    return (r_flag) ? ((void *)block_ptr + bsize) : NULL;
}

/*
 * Searches the Main Free List for the first 
 * block big enough to satisfy the request.
 * 
 * @param bsize : Size of Allocation Request Block
 * @param requested_size : Size of Allocation Request
 * 
 * @returns : Split Free block capable of satis[pfying
 * allocation request.
 * @returns : NULL : If no block big enough t
*/
static sf_block*
search_main(int bsize, int requested_size) {
    /* Pointer for traversing Main Free List */
    /* Begin at first node */
    sf_block *block_ptr = sf_free_list_head.body.links.next; 
    int bsize_cpy = block_ptr->header.block_size & BLOCK_SIZE_MASK; // Copy of Block Size before splitting

    /* Traverse Free List for big enough Free Block */
    while(block_ptr != &sf_free_list_head &&
         (block_ptr->header.block_size & BLOCK_SIZE_MASK) < bsize) {
        block_ptr = block_ptr->body.links.next;
    }

    /* Make sure sentinel will not get removed from List */
    if(block_ptr != &sf_free_list_head) {
        sf_block *new_free_block; // Resulting block from splitting

        /* Remove Block from Main Free List */
        remove_main(block_ptr);

        /* Split the Block if possible */
        if(!(new_free_block = (sf_block *)split_block(block_ptr, bsize, requested_size))) {
            return block_ptr;
        } else {
            /* Initialze the remainder as a Free Block */
            init_free_block(bsize_cpy-bsize, new_free_block, 1);

            /* Insert remainder back into the Main Free List */
            insert_main(new_free_block);

            return block_ptr;
        }
    }

    return NULL;
}

/*
 * Coalesces the newly allocateed block (after calling sf_mem_grow()) 
 * with the free block immediately preceding it.
 * 
 * @param block_ptr : Pointer to block to be coalesced
 * @param cflag : 
 *     0 : Coalesce with preceding Free Block ONLY
 *     1 : Coalesce with both the preceding & following Free Blocks
*/
static void 
coalesce(sf_block *block_ptr, unsigned cflag) {
    int old_size = block_ptr->header.block_size & BLOCK_SIZE_MASK; // Size of un-coalesced block
    int size = 0; // Size of following or preceding block
    sf_block *new_free_block;  // Pointer to Coalesced Block

    /* Coalesce Preceding Block */
    /* Is previous block allcoated? - Only coalesce if block is free*/
    if(!((block_ptr->header.block_size & ~(BLOCK_SIZE_MASK)) >> 1)) {
        /* Get size of preceding block from footer */
        size = ((sf_header *)((void *)block_ptr - FOOTER_SIZE))->block_size & BLOCK_SIZE_MASK;
        new_free_block = (sf_block *)((void *)block_ptr - size);

        /* Initialize the Coalesced Block */
        init_free_block((size+old_size), new_free_block, 
                        ((new_free_block->header.block_size & ~(BLOCK_SIZE_MASK)) >> 1));

        
    }

    /* Coalesce Following Block */
    if(cflag) {

    }

    /* Remove Coalesced Block from Main Free List */
    remove_main(new_free_block);
    /* Insert Coalesesced Block at beginning Main Free List */
    insert_main(new_free_block);
}

void *
sf_malloc(size_t size) {
    sf_block *a_block = NULL;  // Allocated block
    int bsize = 0;      // Size of the current block being allocated
    int padding = 0;    // Block Size alignment padding
    int quick_i = 0;    // Quicklist Index

    /* ==================== PARAMETER VALIDATION ==================== */
    if(!size) {  // size == 0
        return NULL;
    } else if((padding = ((bsize = size + HEADER_SIZE) % ROW_ALIGNMENT))) { 
        /* Align block size : double word aligned (16-bytes) */
        bsize += ROW_ALIGNMENT - padding;
    }

    /* Block must be able to store at least: */
    /* 1 header, 1 footer, and 2 pointers when free (32 Bytes) */
    if(bsize < MIN_BLOCK_SIZE) {
        bsize += ROW_ALIGNMENT;
    }

    /* ==================== INITALIZE HEAP ==================== */
    /* Empty Heap */
    if(sf_mem_start() == sf_mem_end()) {
        sf_prologue *prologue;   // @ Start of heap 
        sf_epilogue *epilogue;   // @ End of the heap 
        sf_block *free_block;    // Free Block of memory to be inserted to Main Free List
        int f_bsize;             // Block Size of Free Block
        
        /* Extend Heap (+1 page) */
        if(!(prologue = (sf_prologue *)sf_mem_grow())) {
            sf_errno = ENOMEM;
            return NULL;
        }
        
        /* Setup Prologue Header and Footer (Allocated : block size = 32) */
        prologue->block.header.block_size = MIN_BLOCK_SIZE;
        prologue->block.header.block_size |= THIS_BLOCK_ALLOCATED;
        /* Prologue Footer must be identical to Header */
        prologue->footer = prologue->block.header; 

        /* Setup Epilogue (Allocated : block size = 0) */
        epilogue = sf_mem_end()-sizeof(sf_epilogue);
        epilogue->header.block_size = 0;
        epilogue->header.block_size |= THIS_BLOCK_ALLOCATED;

        /* Initialze the rest of the page as a Free Block*/
        f_bsize = (sf_mem_end()-sizeof(sf_epilogue))-(sf_mem_start()+sizeof(sf_prologue));
        free_block = sf_mem_start()+sizeof(sf_prologue);
        init_free_block(f_bsize, free_block, 1);

        /* Insert Free Block to Main Free List */
        /* Initialize Main Free List */
        sf_free_list_head.body.links.next = &sf_free_list_head;
        sf_free_list_head.body.links.prev = &sf_free_list_head;
        insert_main(free_block);
    }

    /* ==================== QUICK FREE LISTS ==================== */
    /* Find Quick List of exact size to satisfy current Allocation Request */
    if((quick_i = ((bsize/ROW_ALIGNMENT)-2)) < NUM_QUICK_LISTS &&
        /* Any Free Blocks available in current Quicklist? */
        sf_quick_lists[quick_i].length) {
            /* Remove first free block and return it */
            a_block = sf_quick_lists[quick_i].first; 
            remove_quick(quick_i);
    } else {
    /* ==================== MAIN FREE LIST ==================== */
    /* Find first fit in the Main Free List */
        while(!(a_block = search_main(bsize, size))) {
            /* Big enough block not found - Get more memory */
            sf_block *new_free_block;   // New Free Block to be inserted to Main Free List
            int f_bsize;                // Block Size of New Free Block
            sf_epilogue *new_epilogue;  // @ End of Heap
            
            /* Extend Heap (+1 page) */
            if(!(new_free_block = ((sf_block *)(sf_mem_grow() - HEADER_SIZE)))) {
                sf_errno = ENOMEM;
                return NULL;
            }

            /* Update Epilogue (Allocated : block size = 0) */
            new_epilogue = sf_mem_end()-sizeof(sf_epilogue);
            new_epilogue->header.block_size = 0;
            new_epilogue->header.block_size |= THIS_BLOCK_ALLOCATED;

            /* Initialze the rest of the page as a Free Block */
            f_bsize = ((sf_mem_end()-sizeof(sf_epilogue)) - (void*)new_free_block) ;
            init_free_block(f_bsize, new_free_block, 0);

            /* Coalesce the newly created Free Block */
            coalesce(new_free_block, 0);
        }
    }
    
    return &a_block->body.payload;
}

void 
sf_free(void *pp) {

    return;
}

void *
sf_realloc(void *pp, size_t rsize) {
    return NULL;
}
