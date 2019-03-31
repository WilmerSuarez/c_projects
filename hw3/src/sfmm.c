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
 * new Free Memory Block.
 * 
 * @param : bsize       : Size of the block : Bytes
 * @param : free_block  : Pointer to the Free Block 
 * @param : prev        : previous block allocated? :
 *     0 : prev_alloc = 0
 *     1 : prev_alloc = prev
 * @param : quick_alloc : Quick List Free Block?
 *     0 : alloc bit = 0
 *     1 : alloc bit = quick_alloc
*/
static void 
init_free_block(unsigned bsize, sf_block *free_block) {
    sf_header *footer; // Free Block Footer

    /* Setup Header */
    if(free_block->header.block_size & PREV_BLOCK_ALLOCATED) {
        free_block->header.block_size = bsize; 
        free_block->header.block_size |= PREV_BLOCK_ALLOCATED;
    } else {    
        free_block->header.block_size = bsize; 
    }
    free_block->header.requested_size = 0;
    /* Clear alloc Flag */
    free_block->header.block_size &= ~(1UL);

    /* Setup Footer - Identical to Header */
    footer = (sf_header *)((void *)free_block+bsize-FOOTER_SIZE); 
    footer->block_size = free_block->header.block_size; 
    footer->requested_size = free_block->header.requested_size; 
}

/*
 * Clear or Set the prev_alloc flag of the block
 * after the one pointed to by block_ptr.
 * 
 * @param block_ptr : pointer of previous block
 * @param cs        : 
 *     0 : clear prev_alloc flag
 *     1 : set prev_alloc flag
*/
static void
set_clear_prev_alloc(sf_header *block_ptr, unsigned bsize, unsigned cs) {
    /* Pointer to header of the block being updated */
    sf_header *h = ((sf_header *)((void*)block_ptr + bsize)); 
    /* Pointer to footer of the block being updated */
    sf_header *f = (sf_header *)((void *)h + (h->block_size & BLOCK_SIZE_MASK) - FOOTER_SIZE);

    if(cs) {
        /* Header Update */
        h->block_size |= PREV_BLOCK_ALLOCATED;
        
        /* Is block Free? */
        if(!(h->block_size & 1UL)) {
            /* Footer Update */
            f->block_size |= PREV_BLOCK_ALLOCATED;
        }
    } else {
        /* Header Update */
        h->block_size &= ~(PREV_BLOCK_ALLOCATED);

        /* Is block Free? */
        if(!(h->block_size & 1UL)) {
            /* Footer Update */
            f->block_size &= ~(PREV_BLOCK_ALLOCATED);
        }
    }
}

/*
 * Insert a Free Block into the Main Free List. 
 * 
 * @param : free_block : Pointer to the Free Block
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
 * Removes the first block in the Quicklist at 
 * position quick_i
 * 
 * @param : quick_i : Index of sf_quick_lists
 * 
 * @returns : Pointer to removed block
*/
static sf_block *
remove_quick(unsigned quick_i) {
    sf_block *q_block; // Temporary pointer to block being removed

    /* Remove block */
    q_block = sf_quick_lists[quick_i].first;
    sf_quick_lists[quick_i].first = sf_quick_lists[quick_i].first->body.links.next;
    q_block->body.links.next = NULL;

    /* Update Quick List Length */
    sf_quick_lists[quick_i].length--; 

    return q_block;
}

/*
 * Remove a Free Block from the Main Free List. 
 * 
 * @param : free_block : Pointer to the Free Block being removed
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
 * @param : block_ptr      : Pointer to the Block being Split
 * @param : bsize          : Size of Allocation Request (with padding)
 * @param : requested_size : Size of orignal Allocation Request
 * @param : alloc          : Block to be split is :
 *     0 : free
 *     1 : allocated
 * 
 * @returns : NULL : If block cannot be split due to splinter
 * @returns : Address to remainder of block after splitting
*/
static void *
split_block(sf_block *block_ptr, unsigned bsize, unsigned requested_size, unsigned alloc) {
    unsigned r_flag = 0; // Flag signaling a New Free Block (splitting successful)

    /* Make sure Splitting wont cause a Splinter */
    if(!(((block_ptr->header.block_size & BLOCK_SIZE_MASK) - bsize) < MIN_BLOCK_SIZE)) {
        r_flag = 1;
    } else {
        /* Cannot Split - Use Full Block */
        bsize = block_ptr->header.block_size & BLOCK_SIZE_MASK;
    }

    /* Setup Allocated Block Header */
    /* Preserve prev_alloc flag */
    if(block_ptr->header.block_size & PREV_BLOCK_ALLOCATED) {
        block_ptr->header.block_size = bsize; 
        block_ptr->header.block_size |= PREV_BLOCK_ALLOCATED;
    } else {    
        block_ptr->header.block_size = bsize; 
    }
    block_ptr->header.requested_size = requested_size; 
    /* Block is now allcoated */
    block_ptr->header.block_size |= THIS_BLOCK_ALLOCATED;

    /* Update next block prev_alloc flag (set) */
    set_clear_prev_alloc((sf_header *)block_ptr, bsize, 1);
    
    return (r_flag) ? ((void *)block_ptr + bsize) : NULL;
}

/*
 * Searches the Main Free List for the first 
 * block big enough to satisfy the request.
 * 
 * @param : bsize          : Size of Allocation Request (with padding)
 * @param : requested_size : Size of orignal Allocation Request
 * 
 * @returns : Split Free block capable of satisfying allocation request.
 * @returns : NULL : If no block big enough
*/
static sf_block*
search_main(unsigned bsize, unsigned requested_size) {
    /* Pointer for traversing Main Free List */
    /* Begin at first node */
    sf_block *block_ptr = sf_free_list_head.body.links.next; 
    unsigned bsize_cpy = block_ptr->header.block_size & BLOCK_SIZE_MASK; // Copy of Block Size before splitting

    /* Traverse Free List for big enough Free Block */
    while(block_ptr != &sf_free_list_head &&
         (block_ptr->header.block_size & BLOCK_SIZE_MASK) < bsize) {
        block_ptr = block_ptr->body.links.next;
    }

    /* Make sure sentinel will not get removed from List */
    if(block_ptr != &sf_free_list_head) {
        sf_block *new_free_block; // Pointer to remainder of split block

        /* Remove Block from Main Free List */
        remove_main(block_ptr);

        /* Split the Block if possible */
        if(!(new_free_block = (sf_block *)split_block(block_ptr, bsize, requested_size, 0))) {
            /* Spliting not possible - return Full Block */
            return block_ptr;
        } else {
            /* Initialze the remainder as a Free Block */
            init_free_block(bsize_cpy-bsize, new_free_block);

            /* Update next block prev_alloc flag (clear) */
            set_clear_prev_alloc((sf_header *)new_free_block, bsize_cpy-bsize, 0);

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
 * If clag set, coalesce with free block following it as well.
 * 
 * @param : block_ptr : Pointer to block to be coalesced
 * 
 * @returns : Pointer to coalesced block
*/
static sf_block * 
coalesce(sf_block *block_ptr) {
    unsigned old_size = block_ptr->header.block_size & BLOCK_SIZE_MASK; // Size of un-coalesced block
    unsigned size_preceding = 0; // Size of preceding block
    unsigned size_following = 0; // Size of following block
    sf_block *new_free_block = NULL;  // Pointer to Coalesced Block

    /* Coalesce the Preceding Block */
    /* Is preceding block allcoated? - Only coalesce if free */
    if(!(block_ptr->header.block_size & 2UL)) {
        /* Get size of preceding block from footer */
        size_preceding = ((sf_header *)((void *)block_ptr - FOOTER_SIZE))->block_size & BLOCK_SIZE_MASK;
        new_free_block = (sf_block *)((void *)block_ptr - size_preceding);
        /* Remove old block from Main Free List */
        remove_main((sf_block *)((void *)block_ptr - size_preceding));
    } else {
        new_free_block = block_ptr;
    }

    /* Coalesce the Following Block */
    if(!((((sf_header *)((void *)block_ptr + old_size)))->block_size & 1UL)) {
        size_following = (((sf_header *)((void *)block_ptr + old_size)))->block_size & BLOCK_SIZE_MASK;
        /* Remove old block from Main Free List */
        remove_main((sf_block *)((void *)block_ptr + old_size));
    }

    /* Initialize the Coalesced Block to a free block*/
    init_free_block((old_size+size_preceding+size_following), new_free_block);

    /* Update next block prev_alloc flag (clear) */
    set_clear_prev_alloc((sf_header *)new_free_block, old_size+size_preceding+size_following, 0);

    return new_free_block;
}

void *
sf_malloc(size_t size) {
    sf_block *a_block = NULL; // Allocated block
    unsigned bsize = 0;       // Size of the current block being allocated (with padding)
    unsigned padding = 0;     // Block Size alignment padding
    unsigned quick_i = 0;     // Quicklist Index

    /* ==================== ARGUMENT VALIDATION ==================== */
    if(!size) {  
        return NULL;
    } else if((padding = ((bsize = (size + HEADER_SIZE)) % ROW_ALIGNMENT))) { 
        /* Align block size : double word aligned (16-Bytes) */
        bsize += ROW_ALIGNMENT - padding;
    }

    /* Block must be able to store at least: */
    /* 1 header, 1 footer, and 2 pointers when free (32-Bytes) */
    if(bsize < MIN_BLOCK_SIZE) {
        bsize += ROW_ALIGNMENT;
    }

    /* ==================== INITALIZE HEAP ==================== */
    /* Empty Heap */
    if(sf_mem_start() == sf_mem_end()) {
        sf_prologue *prologue;   // @ Start of heap 
        sf_epilogue *epilogue;   // @ End of the heap 
        sf_block *free_block;    // Free Block of memory to be inserted to Main Free List
        unsigned f_bsize;        // Block Size of Free Block
        
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
        init_free_block(f_bsize, free_block);

        /* Update next block prev_alloc flag (set) */
        set_clear_prev_alloc((sf_header *)prologue, sizeof(sf_prologue), 1);

        /* Update next block prev_alloc flag (clear) */
        set_clear_prev_alloc((sf_header *)free_block, f_bsize, 0);
        
        /* Initialize Main Free List */
        sf_free_list_head.body.links.next = &sf_free_list_head;
        sf_free_list_head.body.links.prev = &sf_free_list_head;

        /* Insert Free Block to Main Free List */
        insert_main(free_block);
    }

    /* ==================== QUICK FREE LISTS ==================== */
    /* Find Quick List of exact size to satisfy current Allocation Request */
    if(((quick_i = ((bsize/ROW_ALIGNMENT)-2)) < NUM_QUICK_LISTS) &&
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
            sf_block *new_free_block = NULL;   // New Free Block to be inserted to Main Free List
            unsigned f_bsize = 0;              // Block Size of New Free Block
            sf_epilogue *new_epilogue = NULL;  // @ End of Heap
            
            /* Extend Heap (+1 page) */
            /* new_free_block uses old epilogue as header */
            if(!(new_free_block = sf_mem_grow())) {
                sf_errno = ENOMEM;
                return NULL;
            }

            /* new_free_block uses old epilogue as header */
            new_free_block = (sf_block *)((void *)new_free_block - HEADER_SIZE);

            /* Update Epilogue (Allocated : block size = 0) */
            new_epilogue = sf_mem_end()-sizeof(sf_epilogue);
            new_epilogue->header.block_size = 0;
            new_epilogue->header.block_size |= THIS_BLOCK_ALLOCATED;

            /* Initialze the rest of the page as a Free Block */
            f_bsize = ((sf_mem_end()-sizeof(sf_epilogue)) - (void*)new_free_block) ;
            init_free_block(f_bsize, new_free_block);

            /* Update next block prev_alloc flag (clear) */
            set_clear_prev_alloc((sf_header *)new_free_block, f_bsize, 0);

            /* Coalesce the newly created Free Block with the Preceding Free Block */
            sf_block *c_block = coalesce(new_free_block);
            
            /* Insert Coalesesced Block at beginning Main Free List */
            insert_main(c_block);
        }
    }

    return &a_block->body.payload;
}

/*
 * Validates the pointer passed in to sf_free();
 * 
 * @param : pp : Pointer to Allocated Block to free
 * 
 * @returns : 0 : Invalid Pointer
 * @returns : 1 : Valid Pointer
*/
static unsigned
validate_arg(sf_block *pp) {
    unsigned prev_block_size = ((sf_header *)((void *)pp - FOOTER_SIZE))->block_size;
    if(
            /* NULL pointer */
            (pp == NULL) || 
            /* Block is Unallocated */
            (!(pp->header.block_size & 1UL)) ||   
            /* Block Header is Before End of Prologue */       
            ((void *)pp <= sf_mem_start()) ||
            /* Block Header is After Beginning of Epilogue */  
            ((void *)pp >= sf_mem_end()) ||
            /* Not aligned to to a double-row boundary */       
            ((pp->header.block_size & BLOCK_SIZE_MASK) % ROW_ALIGNMENT) ||     
            /* Block size < 32-Bytes */    
            ((pp->header.block_size & BLOCK_SIZE_MASK) < MIN_BLOCK_SIZE) ||       
            /* Requested Size + Block Header Size > Block Size Field */
            (((pp->header.requested_size)
                + HEADER_SIZE) > (pp->header.block_size & BLOCK_SIZE_MASK)) ||       
            /* Preceding Footer & Header alloc flags are set when *pp prev_alloc flag is not */ 
            (!(pp->header.block_size & 2UL) &&
            (prev_block_size & THIS_BLOCK_ALLOCATED) && 
            (((sf_header *)((void *)pp - prev_block_size))->block_size & THIS_BLOCK_ALLOCATED))
        ) {       
        return 0;
    }

    return 1;
}

/*
 * Inserts a newly freed block at the head of the
 * Quicklist at position quick_i
 * 
 * @param : quick_i   : Index of sf_quick_lists
 * @param : new_block : Pointer to block being inserted into Quick List
*/
static void 
insert_quick(unsigned quick_i, sf_block *new_block) {
    sf_block *old_first; // Pointer to old head of Quick List

    /* If list is empty */
    if(!(sf_quick_lists[quick_i].length)) {
        sf_quick_lists[quick_i].first = new_block;
        new_block->body.links.next = NULL;
        new_block->body.links.prev = NULL;
    } else {
        /* Always insert at beginning of list (LIFO) */
        old_first = sf_quick_lists[quick_i].first;
        sf_quick_lists[quick_i].first = new_block;
        new_block->body.links.next = old_first;
        new_block->body.links.prev = NULL;
    }

    /* Update Quick List Length */
    sf_quick_lists[quick_i].length++; 
}

void 
sf_free(void *pp) {
    /* ==================== ARGUMENT VALIDATION ==================== */
    /* Abort if Block Pointer is Invalid */
    if(!(validate_arg((sf_block *)(pp - HEADER_SIZE)))) {
        abort();
    }

    /* Pointer to Allocated Block */
    sf_block *block_ptr = (sf_block *)(pp - HEADER_SIZE);

    /* ==================== FREE THE BLOCK ==================== */
    /* ==================== QUICK FREE LISTS ==================== */
    /* Block size of Block to be Freed */
    unsigned bsize = block_ptr->header.block_size & BLOCK_SIZE_MASK; 
    unsigned quick_i = 0;     // Quicklist Index
    sf_block *new_free_block; // Resulting block from Coalescing 

    /* Find Quick List of exact size to fit the Block */
    if(((quick_i = ((bsize/ROW_ALIGNMENT)-2)) < NUM_QUICK_LISTS)) {
        /* Is current Quicklist Full? */
        if(sf_quick_lists[quick_i].length == QUICK_LIST_MAX) {
            /* Quick List Block Pointer - for Quick List traversal */
            sf_block *qb_ptr = sf_quick_lists[quick_i].first; 
            
            /* ==================== FLUSH ==================== */
            /* Flush the current Quicklist */
            for(int i = 0; i < QUICK_LIST_MAX; ++i) {
                sf_block *rm_q; // Pointer to removed Quick List Block

                /* Go to next block in current Qucik List */
                qb_ptr = qb_ptr->body.links.next;

                /* Remove from Quicklist */
                rm_q = remove_quick(quick_i);

                /* Initialize as a Free Block */
                init_free_block(bsize, rm_q);

                /* Coalesce with adjacent Blocks & */
                /* Insert into Main Free List */
                if((new_free_block = coalesce(rm_q))) {
                    insert_main(new_free_block);
                } else {
                    insert_main(rm_q);
                }
            }           
        }

        /* Insert Block to start of Current Quicklist */
        insert_quick(quick_i, block_ptr);

        return;
    }

    /* ==================== MAIN FREE LIST ==================== */
    /* Free the Block */
    init_free_block(bsize, block_ptr);

    /* Coalesce with adjacent Blocks */
    new_free_block = coalesce(block_ptr);

    /* Add to Main Free List */
    insert_main(new_free_block);
}

void *
sf_realloc(void *pp, size_t rsize) {
    unsigned new_size = 0; // Size of the new block to be allocated
    unsigned padding = 0;

    /* ==================== ARGUMENT VALIDATION ==================== */
    /* If Block Pointer is Invalid */
    if(!(validate_arg((sf_block *)(pp - HEADER_SIZE)))) {
        sf_errno = EINVAL;
        return NULL;
    }

    if(!rsize) {
        sf_free(pp);
        return NULL;
    } else if((padding = ((new_size = (rsize + HEADER_SIZE)) % ROW_ALIGNMENT))) { 
        /* Align block size : double word aligned (16-Bytes) */
        new_size += ROW_ALIGNMENT - padding;
    }

    /* Block must be able to store at least: */
    /* 1 header, 1 footer, and 2 pointers when free (32-Bytes) */
    if(new_size < MIN_BLOCK_SIZE) {
        new_size += ROW_ALIGNMENT;
    }

    /* ==================== REALLOCATING : LARGER ==================== */
    sf_block *old_block = (sf_block *)(pp - HEADER_SIZE); // Old Allocated Block
    unsigned old_size = old_block->header.block_size & BLOCK_SIZE_MASK;
    sf_block *new_block = NULL; // New Allocated Block
    
    if(rsize > old_size) {
        /* Allocate Larger Block */
        if(!(new_block = (sf_block *)sf_malloc(rsize))) {
            return NULL;
        }

        /* Update new_block */
        new_block = (sf_block *)((void *)new_block - HEADER_SIZE);

        /* Update new block size */
        new_size = new_block->header.block_size & BLOCK_SIZE_MASK;

        /* Copy Contents of old block into the new one */
        new_block = memcpy(new_block, old_block, old_size);

        /* Update new block header */
        new_block->header.requested_size = rsize;
        if(new_block->header.block_size & PREV_BLOCK_ALLOCATED) {
            new_block->header.block_size = new_size; 
            new_block->header.block_size |= PREV_BLOCK_ALLOCATED;
        } else {    
            new_block->header.block_size = new_size; 
        }
        new_block->header.block_size |= THIS_BLOCK_ALLOCATED;

        /* Free old block */
        sf_free(pp);
    } else if (rsize <= old_size) {
        /* ==================== REALLOCATING : SMALLER ==================== */
        sf_block *new_free_block = NULL; // Remainder of Split Block

        /* Attempty to split the Block */
        if(!(new_free_block = split_block(old_block, new_size, rsize, 1))) {
            return &old_block->body.payload;
        }

        /* Initialze the remainder as a Free Block */
        init_free_block(old_size-new_size, new_free_block);

        /* Coalesce the split remainder */
        sf_block* new_c = coalesce(new_free_block);

        /* Insert remainder back into the Main Free List */
        insert_main(new_c);

        return &old_block->body.payload;
    } 

    return &new_block->body.payload;
}
