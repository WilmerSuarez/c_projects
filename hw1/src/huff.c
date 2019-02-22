#include <stdio.h>
#include <stdlib.h>
#include "const.h"
#include "huff.h"
#include "debug.h"

#ifdef _STRING_H
#error "Do not #include <string.h>. You will get a ZERO."
#endif

#ifdef _STRINGS_H
#error "Do not #include <strings.h>. You will get a ZERO."
#endif

#ifdef _CTYPE_H
#error "Do not #include <ctype.h>. You will get a ZERO."
#endif

#define BYTE    8
NODE *END; // END leaf node pointer

/*
 * @brief Post order travesral of Huffman Tree
 * @details Outputs the symbols of the leaves.
 * 
 * @param n Current node being evaluated
 * @param prev_sym Holds the previously evaluated symbol
*/
static void
sym_byte_seq(NODE *n, short prev_sym) {
    /* If leaf node (either left or right = NULL) */
    if(n->left == NULL) {
        /* END symbol output */
        if(!n->weight) {
            END = n; // Point to END leaf node - for use during encoding
            putchar(0xFF);
            putchar(0x00);
            return;
        }
        /* If previous symbol is 255, next byte ignored. */
        if(prev_sym == 0xFF) return;

        /* Output symbol */
        putchar(n->symbol);
        prev_sym = n->symbol;
        return;
    } 
    
    /* Go to Left child */
    sym_byte_seq(n->left, prev_sym);
    /* Go to Right child */
    sym_byte_seq(n->right, prev_sym);
}

/*
 * @brief Post order travesral of Huffman Tree
 * @details When a leaf is reached the output is 0.
 * Output is 1 when an internal node is reached. The 
 * bit sequence is then padded with the necessary 0's
 * to make it a multiple of 8 bits.
 * 
 * @param n Current node being evaluated
 * @param bit_sequence Holds the bit evaluation results of the nodes
*/

static void
bitseq(NODE *n, char *bit_sequence, int *bit_pos, int *bit_count) {
    /* If leaf node (either left or right = NULL) */
    if(n->left == NULL) {
        *bit_sequence &= ~(1 << (*bit_pos)--); // Clear next bit 
        /* If full byte filled */
        if(++(*bit_count) == BYTE) {
            putchar(*bit_sequence);
            *bit_pos = 7; // Go back to MSb
            *bit_count = 0; // Reset count
        }
        return;
    } 
    
    /* Go to Left child */
    bitseq(n->left, bit_sequence, bit_pos, bit_count);
    /* Go to Right child */
    bitseq(n->right, bit_sequence, bit_pos, bit_count);

    /* Internal node bit setting */
    *bit_sequence |= 1 << (*bit_pos)--; // Set next bit 
    /* If full byte filled */
    if(++(*bit_count) == BYTE) {
        putchar(*bit_sequence);
        *bit_pos = 7; // Go back to MSb
        *bit_count = 0; // Reset count
    } 
}

/*
 * @brief Emits a description of the Huffman tree used to compress the current block.
 * @details This function emits, to the standard output, a description of the
 * Huffman Tree used to compress the current block.
 * Huffman Tree Description: 
 *     1. Number of nodes: two-byte sequence in big-endian order
 *     2. Sequence of bits: 0 indicates leaf, 1 indicates internal node
 *     3. Sequence of bytes corresponding to the symbols in the Huffman Tree
 */
void 
emit_huffman_tree() {
    /* Emit number of nodes */
    short nnum = num_nodes;
    putchar((nnum >> BYTE) & 0xFF); // MSB 
    putchar(nnum & 0xFF); // LSB

    debug("nnum: %d", nnum);

    /* Emit node bit sequence */
    char bit_sequence = 0;
    int bit_count = 0; // Keeps track of number of bits
    int bit_pos = 7; // Bit Shift amount
    bitseq(nodes, &bit_sequence, &bit_pos, &bit_count);
    if(nnum % BYTE) {
        /* Zero padding */
        while(nnum % BYTE) {
            bit_sequence &= ~(1 << bit_pos--); // Clear next bit 
            ++nnum;
        }
        putchar(bit_sequence);
    }

    /* Emit symbol byte sequence */
    sym_byte_seq(nodes, 0);
}

/*
 * @brief Swap two Nodes
 * 
 * @param n1 Index of 1st Node
 * @param n2 Index of 2nd Node
*/
static void
swap(int n1, int n2) {
    NODE temp_node;

    temp_node = *(nodes+n2);
    *(nodes+n2) = *(nodes+n1);
    *(nodes+n1) = temp_node;
}

/*
 * @brief Recursively Heapify the subtree at the given nodes array 
 * with root at the given index.
 * 
 * @param i Index of current Node
 * @param heapnum The current size of the min-heap
*/
static void 
min_heapify(int i, const int heapnum) {
    int l = 2*i+1; // Left child index
    int r = 2*i+2; // Right child index
    int smallest = i; // Initial smallest value
    if(l < heapnum && (*(nodes+l)).weight < (*(nodes+i)).weight) 
        smallest = l;
    if(r < heapnum && (*(nodes+r)).weight < (*(nodes+smallest)).weight)
        smallest = r;
    if(smallest != i) {
        swap(i, smallest);
        min_heapify(smallest, heapnum);
    }
}

/*
 * @brief Removes the 2 minimum-weight nodes of the min-heap in the nodes
 * array. Re-heapifies.
 * 
 * @param *heapnum The current size of the min-heap
 * @param *n The number of nodes
*/
static void 
remove_min(int *heapnum, int *n) {
    NODE *temp_node1, *temp_node2;
    int p = (*n+1)/2-2; // Parent index of current nodes

    /* Store the root node at "upper end" of nodes array nodes[*n-1] */
    temp_node1 = nodes+(*n-1);
    *temp_node1 = *nodes;
    /* Replace the root node - 1st min-wieght node - with last node in min-heap */  
    *nodes = *(nodes+(*heapnum-1)); 
    *heapnum -= 1; // Decrement size of the min-heap after replacing root node
    min_heapify(0, *heapnum); // Re-heapify the min-heap

    /* Replace the root node - 2nd min-weight node - with last node in min-heap*/
    temp_node2 = nodes+(*n-2);
    *temp_node2 = *nodes; // Store root at nodes[*n-2]
    *nodes = *(nodes+(*heapnum-1)); 

    /* Create parent for the 2 min-weight nodes */
    (*(nodes+p)).left = temp_node1; // Left Child
    (*(nodes+p)).right = temp_node2;
    (*(nodes+p)).weight = temp_node1->weight + temp_node2->weight; // Parent wight = sum of children weight
    (*(nodes+p)).symbol = 'P'; // Arbitrary parent symbol
    *n = *n - 2; // Reduce size of huff tree 
    min_heapify(0, *heapnum);

    if(*heapnum == 1) *heapnum -= 1;
}

/*
 * @brief Sets the parent member pointers of the 
 * Nodes with parents in the nodes array. The function
 * also populates the node_for_symbol array.
 * 
 * @param n Address of current node being evaluated
 * @param prnt Address of current node's parent
*/
NODE **nfsptr = node_for_symbol; // Pointer to node_for_symbol array
static void
set_parents(NODE *n, NODE *prnt) {
    n->parent = prnt; // Assign current node's parent

    /* If leaf node (either left or right = NULL) */
    if(n->left == NULL) {
        /* 
            Set pointer to leaf nodes corresponding to 
            their symbol value.
        */
        *(nfsptr+((int)n->symbol)) = n; 
        return;
    } 
     
    /* Go to Left child */
    set_parents(n->left, n);
    /* Go to Right child */
    set_parents(n->right, n);
}

/*
 * @brief Output the compressed data representing the 
 * uncompressed data.
 * @details Traverse the Huffman Tree, outputting the code
 * bits for characters in the current block.
*/
static void
encode_data(NODE *nptr, NODE *naddr, unsigned *bit_pos, unsigned *bit_count, 
            unsigned *num_bits, char *bit_sequence) {
    /* Set-up bit sequence */
    while(nptr->parent) {
        naddr = nptr; // Store current node address
        nptr = nptr->parent; // Move to parent node

        /* Previous node: left or right child? */
        if(nptr->left == naddr) {
            nptr->weight = 0;
        } else if(nptr->right == naddr){
            nptr->weight = 1;
        }
    }

    /* Output encoded bit sequence  */ 
    while(nptr->left || nptr->right) {
        if(nptr->weight) {  // If direction is 1, go right
            *bit_sequence |= 1 << (*bit_pos)--; // Set next bit 
            (*num_bits)++;
            /* If full byte filled */
            if(++(*bit_count) == BYTE) {
                putchar(*bit_sequence);
                *bit_pos = 7; // Go back to MSb
                *bit_count = 0; // Reset count
            }
            nptr = nptr->right; // Go to next node in the sequence
        } else if(!nptr->weight) { // If direction is 0, go left
            *bit_sequence &= ~(1 << (*bit_pos)--); // Clear next bit 
            (*num_bits)++;
            /* If full byte filled */
            if(++(*bit_count) == BYTE) {
                putchar(*bit_sequence);
                *bit_pos = 7; // Go back to MSb
                *bit_count = 0; // Reset count
            }
            nptr = nptr->left; // Go to next node in the sequence
        }
    }
}

/*
 * @brief Output the compressed data representing the 
 * uncompressed data.
*/
static void
encode(int bbcnt) {
    char bit_sequence = 0;
    unsigned num_bits = 0; // Total number of bits in the sequence
    unsigned bit_pos = 7; // Bit Shift amount
    unsigned bit_count = 0; // Keeps track of number of bits
    unsigned char *cbptr = current_block; // Pointer to current block array
    unsigned char s; // Holds current character being compressed
    NODE *nptr = NULL; // Points to the current node used to compress "s"
    NODE *naddr = NULL; // Address of the previous node

    /* Encode Characters and output code bits */
    for(int i = 0; i < bbcnt; ++i) {
        s = *(cbptr+i); // Get next character to compress
        nptr = node_for_symbol[s]; // Pointer to node of symbol "s"

        encode_data(nptr, naddr, &bit_pos, &bit_count, &num_bits, &bit_sequence);
    }

    /* Encode END symbol */
    encode_data(END, naddr, &bit_pos, &bit_count, &num_bits, &bit_sequence);

    /* Zero padding */
    if(num_bits % BYTE) {
        while(num_bits % BYTE) {
            bit_sequence &= ~(1 << bit_pos--); // Clear next bit 
            ++num_bits;
        }
        putchar(bit_sequence);   
    } 
    
}

/*
 * @brief Resets the number of nodes and the nodes array.
 * @details Sets all the pointers in the NODE structs to NULL and 
 * resets the weight and symbol members to 0.
 */
void 
res_nodes(NODE *nptr) {
    for(int i = 0; i < (2*MAX_SYMBOLS-1); ++i) {
        nptr->left = NULL;
        nptr->right = NULL;
        nptr->parent = NULL;
        nptr->weight = 0;
        nptr->symbol = 0;
        nptr++; // Go to next node
    }

    /* Reset node count */
    num_nodes = 0;
}

/*
 * @brief Reads one block of data from standard input and emits corresponding
 * compressed data to standard output.
 * @details This function reads raw binary data bytes from the standard input
 * until the specified block size has been read or until EOF is reached.
 * It then applies a data compression algorithm to the block and outputs the
 * compressed block to the standard output.  The block size parameter is
 * obtained from the global_options variable.
 *
 * @return 0 if block compression completes without error, 1 if an error occurs.
 */
int 
compress_block() {
    int s; // Holds current character Symbol from stdin
    unsigned bbcnt = 0; // Keep track of block byte count
    const unsigned bsz = (unsigned)global_options >> 16; // Current Block Size 
    unsigned char *cbptr = current_block; // Pointer to block array
    int heapnum; // Number of nodes in the min-heap
    int done = 0; // Flag for file compression completion

    NODE *nptr = nodes+1; // Point to 2nd index of nodes array (1st index is END node)
    num_nodes++; // Initialize number of nodes to 1 (END node) 
    
    /* Read first byte of data to be compressed from standard input */
    if((s = getchar()) != EOF) { // Get first char from stdin
        *(cbptr++) = s; // Update current block storage
        bbcnt++; // Update Block Byte Count
    } else {
        return 1; // Empty file
    }
    for(;;) {
        if((s = getchar()) != EOF) { // Get next char from stdin
            *(cbptr++) = s; // Update current block storage
            bbcnt++; // Update Block Byte Count
        } 
        if(bbcnt > bsz || s == EOF) {
            /* Set "done" flag if End of File reached */
            if(s == EOF) done = 1;

            break;
        }
    }

    /* Create Symbol Histogram */
    cbptr = current_block; // Reset pointer to block array
    for(int i = 0; i < bbcnt; ++i) {
        s = *(cbptr+i);
        for(int j = 1; j < (2*MAX_SYMBOLS - 1); ++j) {
            /* If the symbol already exists in the node array or it does not */
            if(nptr->symbol == s || !nptr->symbol) {
                if(!nptr->symbol)
                    num_nodes++; // Increment the node count in the nodes array
                nptr->symbol = s; // Update Symbol
                nptr->weight++; // Incrememnt Symbol occurrence
                break;
            }
            nptr++; // Go to next node
        }
        nptr = nodes+1; // Point back to 2nd index
    }

    heapnum = num_nodes; // Number of nodes currently in the min-heap
    num_nodes = 2*num_nodes-1; // Number of nodes to be in Huffman tree of current block

    /* Histogram -> Min-Heap */
    for(int i = heapnum/2; i >= 0; i--) {
        min_heapify(i, heapnum);
    }

    /* Huffman Tree Construction */
    int n = num_nodes; // Copy of the number of nodes 
    while(heapnum) {
        /* Remove 2 minimum weight nodes - and Construct Huffman Tree*/
        remove_min(&heapnum, &n);
    }

    /* Set Parent Node Pointers & populate node_for_symbol array */ 
    set_parents(nodes, nodes);
    nodes->parent = NULL; // Root has no parent
    
    /* Output the Huffman Tree Description */
    emit_huffman_tree();

    /* Output compressed data bits */
    encode(bbcnt);

    return done;
}

/*
 * @brief Reads raw data from standard input, writes compressed data to
 * standard output.
 * @details This function reads raw binary data bytes from the standard input in
 * blocks of up to a specified maximum number of bytes or until EOF is reached,
 * it applies a data compression algorithm to each block, and it outputs the
 * compressed blocks to standard output.  The block size parameter is obtained
 * from the global_options variable.
 *
 * @return 0 if compression completes without error, 1 if an error occurs.
 */
int 
compress() {
    /* Compress all blocks */
    do {
        /* Reset Nodes Array */
        res_nodes(nodes);
    } while(!(compress_block()));

    /* Check for IO error */
    if(ferror(stdin)) return 1;

    /* Check if EOF is reached */
    if(feof(stdin)) return 0;

    return 1;
}

/*
 * @brief 
 * 
*/
static void
reconstruct() {

}

static int
decode_bit_seq() {
    int s; // Current bytes being analyzed
    int done = 0; // Flag for decompression completion of current block
    NODE *sp = nodes; // Stack pointer

    /* Get number of nodes in the huffman tree of the current compressed block */
    unsigned nnum = 0; // Number of nodes
    if((s = getchar()) != EOF) { // Get first byte from stdin
        nnum = (s << BYTE); // MSB 
    } else {
        return 1; // Empty file 
    }
    if((s = getchar()) != EOF) { // Get second byte from stdin
        nnum |= s; // LSB
    } else {
        return 1;
    }
    debug("nodes:%d", nnum);
    /* Numberof leaves in the huffman tree of the current compressed block */
    unsigned nleaf = (nnum+1)/2; 
    debug("leaves:%d", nleaf);

    /* Decode post-order bit sequence */
    unsigned bit_count = 0; // Per byte bit counter
    unsigned n_bits = 0; // Number of bits representing structure of the tree (post order traversal)
    char bit_val;
    while(n_bits != nnum) {
        if((s = getchar()) != EOF) { // Get next byte from stdin
        debug("s:%x", s);
            while(bit_count != BYTE) {
                bit_val = (s >> (BYTE - ((bit_count+1) % BYTE)) & 0x01); // Get value of next bit
                debug("bit_val:%d", bit_val);
                reconstruct(bit_val, sp); // Evaluate scanned value
                bit_count++; // Increment number of bits evaluated in current byte
                n_bits++; // Increment number of nodes decoded

                if(n_bits == nnum) break; // Skip padding

                if(bit_count == (BYTE - 1)) {
                    bit_val = s & 0x01; // Get value of last bit
                    debug("bit_val:%d", bit_val);
                    reconstruct(bit_val, sp); // Evaluate scanned valu
                    bit_count++; // Increment number of bits evaluated in current byte
                    n_bits++; // Increment number of nodes decoded
                }
            }

            /* Reset bit coutner */
            bit_count = 0;
        } 

        /* Invalid bit sequence - return error */
        if(s == EOF) {
            return 1;
        }
    }

    return 0;
}

/*
 * @brief Reads a description of a Huffman tree and reconstructs the tree from
 * the description.
 * @details  This function reads, from the standard input, the description of a
 * Huffman tree in the format produced by emit_huffman_tree(), and it reconstructs
 * the tree from the description. 
 *
 * @return 0 if the tree is read and reconstructed without error, otherwise 1
 * if an error occurs.
 */
int 
read_huffman_tree() {
    /* Decode post order bit sequence */
    if(decode_bit_seq()) {
        return 1;
    }

    /*  */

    return 0;
}



/*
 * @brief Reads one block of compressed data from standard input and writes
 * the corresponding uncompressed data to standard output.
 * @details This function reads one block of compressed data from the standard
 * input, decompresses the block, and it outputs the uncompressed data to
 * the standard output. The input data blocks are assumed to be in the format
 * produced by compress(). If EOF is encountered before a complete block has
 * been read, it is an error.
 *
 * @return 0 if decompression completes without error, 1 if an error occurs.
 */
int 
decompress_block() {
    /* Read and re-construct the Huffman Tree */
    if(read_huffman_tree()) {
        return 1;
    }

    /* De-compress block */
        
    return 0;
}

/*
 * @brief Reads compressed data from standard input, writes uncompressed
 * data to standard output.
 * @details This function reads blocks of compressed data from the standard
 * input until EOF is reached, it decompresses each block, and it outputs
 * the uncompressed data to the standard output.  The input data blocks
 * are assumed to be in the format produced by compress().
 *
 * @return 0 if decompression completes without error, 1 if an error occurs.
 */
int 
decompress() {
    /* Decompress all blocks */
    do {
        /* Reset Nodes Array */
        res_nodes(nodes);
    } while(!(decompress_block()));

    /* Check for IO error */
    if(ferror(stdin)) return 1;

    /* Check if EOF is reached */    
    if(feof(stdin)) return 0;

    return 1;
}
