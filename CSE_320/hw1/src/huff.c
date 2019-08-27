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
    /* If leaf node (either left or right == NULL) */
    if(n->left == NULL) {
        /* END symbol output */
        if(!n->weight) {
            END = n; // Point to END leaf node - for use during encoding
            /* END symbol is a two byte symbol: 0xFF00 */
            putchar(0xFF);
            putchar(0x00);
            return;
        }
        /* If previous symbol is 255, the byte must be any value that's not 0x00 */
        if(prev_sym == 0xFF) {
            putchar(0x01);
            return;
        }

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
 * Output is 1 when an internal node is reached.
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
            *bit_pos = 7;   // Go back to MSb
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
        *bit_pos = 7;   // Go back to MSb
        *bit_count = 0; // Reset count
    } 
}

/*
 * @brief Emits the description of the Huffman tree used to compress the current block.
 * Huffman Tree Description: 
 *     1. Number of nodes: two-byte sequence in big-endian order
 *     2. Sequence of bits: 0 indicates leaf, 1 indicates internal node
 *     3. Sequence of bytes corresponding to the symbols in the Huffman Tree
 */
void 
emit_huffman_tree() {
    /* Emit number of nodes */
    short nnum = num_nodes;         // Copy of number of nodes
    putchar((nnum >> BYTE) & 0xFF); // MSB 
    putchar(nnum & 0xFF);           // LSB

    /* Emit node bit sequence */
    char bit_sequence = 0; // Holds bit sequence to be outputted
    int bit_count = 0;     // Keeps track of number of bits
    int bit_pos = 7;       // Bit Shift amount
    /* Generate the bit sequence */
    bitseq(nodes, &bit_sequence, &bit_pos, &bit_count);
    /* Zero-padding the last byte in the bit sequence to make it a multiple of 8 bits */
    if(nnum % BYTE) { // Zero-padding only when not a multiple of 8 bits
        while(nnum % BYTE) {
            bit_sequence &= ~(1 << bit_pos--); // Clear next bit 
            ++nnum;
        }
        putchar(bit_sequence); // Output last zero-padded byte
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
    int l = 2*i+1;    // Left child index
    int r = 2*i+2;    // Right child index
    int smallest = i; // Initial smallest value
    if(l < heapnum && (*(nodes+l)).weight < (*(nodes+i)).weight) 
        smallest = l;
    if(r < heapnum && (*(nodes+r)).weight < (*(nodes+smallest)).weight)
        smallest = r;
    if(smallest != i) {
        swap(i, smallest); // Swap the previous smalles with current smallest
        min_heapify(smallest, heapnum); 
    }
}

/*
 * @brief Removes the 2 minimum-weight nodes of the min-heap in the nodes
 * array. Re-heapifies.
 * 
 * @param *heapnum The current size of the min-heap
 * @param *n The number of nodes
 * @return 0 when Huffman tree constructed, 1 when not
*/
static int 
remove_min(int *heapnum, int *n) {
    NODE *temp_node1, *temp_node2;
    int p = (*n+1)/2-2; // Parent index of current nodes

    /* Store the root node at "upper end" of nodes array nodes[*n-1] */
    temp_node1 = nodes+(*n-1);
    *temp_node1 = *nodes;
    /* Replace the root node - 1st min-wieght node - with last node in min-heap */  
    *nodes = *(nodes+(*heapnum-1)); 
    *heapnum -= 1; // Decrement size of the min-heap after replacing root node
    min_heapify(0, *heapnum); // Fix the min-heap

    /* Replace the root node - 2nd min-weight node - with last node in min-heap*/
    temp_node2 = nodes+(*n-2);
    *temp_node2 = *nodes; // Store root at nodes[*n-2]
    *nodes = *(nodes+(*heapnum-1)); 

    /* Create parent for the 2 min-weight nodes */
    (*(nodes+p)).left = temp_node1;  // Left Child
    (*(nodes+p)).right = temp_node2; // Right Child
    (*(nodes+p)).weight = temp_node1->weight + temp_node2->weight; // Parent wight = sum of children weight
    (*(nodes+p)).symbol = 'P'; // Arbitrary parent symbol
    *n = *n - 2;               // Reduce temporary size of Huff tree
    min_heapify(0, *heapnum);  // Fix the min-heap

    if(*heapnum == 1) return 0; // Huffman Tree Complete

    return 1; // Continue Huffman Tree construction
}

/*
 * @brief Sets the parent pointers of the 
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
        /* Set pointer to leaf nodes corresponding to their symbol value. */
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
 * bits for the current symbol pointed to by nptr.
 * The weight fields of the nodes are used to store the directions
 * of the tree edges.
 * 
 * @param nptr: Pointer to current symbol to be encoded
 * @param naddr: Address of the previous node
 * @param bit_pos: Bit shift amount
 * @param bit_count: Number of bits processed
 * @param num_bits: Total number of bits in the encoded data
 * @param bit_sequence: Holds bit sequence to be outputted
*/
static void
encode_data(NODE *nptr, NODE *naddr, unsigned *bit_pos, unsigned *bit_count, 
            unsigned *num_bits, char *bit_sequence) {
    /* Set-up bit sequence */
    while(nptr->parent) {
        naddr = nptr;        // Store current node address
        nptr = nptr->parent; // Move to parent node

        /* Previous node: left or right child? */
        if(nptr->left == naddr) {
            nptr->weight = 0;
        } else if(nptr->right == naddr){
            nptr->weight = 1;
        }
    }

    /* Output encoded bit sequence for current symbol */ 
    while(nptr->left || nptr->right) {
        if(nptr->weight) {                      // If direction is 1, go right
            *bit_sequence |= 1 << (*bit_pos)--; // Set next bit 
            (*num_bits)++;
            /* If full byte filled */
            if(++(*bit_count) == BYTE) {
                putchar(*bit_sequence);
                *bit_pos = 7;   // Go back to MSb
                *bit_count = 0; // Reset count
            }
            nptr = nptr->right;    // Go to next node in the sequence
        } else if(!nptr->weight) { // If direction is 0, go left
            *bit_sequence &= ~(1 << (*bit_pos)--); // Clear next bit 
            (*num_bits)++;
            /* If full byte filled */
            if(++(*bit_count) == BYTE) {
                putchar(*bit_sequence);
                *bit_pos = 7;   // Go back to MSb
                *bit_count = 0; // Reset count
            }
            nptr = nptr->left;  // Go to next node in the sequence
        }
    }
}

/*
 * @brief Output the compressed data representing the 
 * uncompressed data.
 * @details Traverses the Huffman Tree using the current_block array
 * content as the index to the node_for_symbol array to reference the leaf nodes 
 * of the Huffman Tree.
 * 
 * @param bbcnt Size of block
*/
static void
encode(int bbcnt) {
    unsigned char *cbptr = current_block; // Pointer to current block array
    char bit_sequence = 0;  // Holds bit sequence to be outputted
    unsigned num_bits = 0;  // Total number of bits in the sequence
    unsigned bit_pos = 7;   // Bit Shift amount
    unsigned bit_count = 0; // Keeps track of number of bits
    unsigned char s;        // Holds current character being compressed
    NODE *nptr = NULL;      // Points to the current node used to compress "s"
    NODE *naddr = NULL;     // Address of the previous node

    /* Encode Characters and output code bits */
    for(int i = 0; i < bbcnt; ++i) {
        s = *(cbptr+i);              // Get next character to compress
        nptr = *(node_for_symbol+s); // Pointer to node of symbol "s"
        encode_data(nptr, naddr, &bit_pos, &bit_count, &num_bits, &bit_sequence);
    }

    /* Encode END symbol */
    encode_data(END, naddr, &bit_pos, &bit_count, &num_bits, &bit_sequence);

    /* Zero-padding the last byte in the bit sequence to make it a multiple of 8 bits */
    if(num_bits % BYTE) { // Zero-padding only when not a multiple of 8 bits
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
    const unsigned bsz = (unsigned)global_options >> 16; // Current Block Size 
    unsigned char *cbptr = current_block;                // Pointer to block array
    int s;              // Holds current character Symbol from stdin
    int heapnum;        // Number of nodes in the min-heap
    int done = 0;       // Flag for file compression completion
    unsigned bbcnt = 0; // Keep track of block byte count
    
    NODE *nptr = nodes+1; // Point to 2nd index of nodes array (1st index is END node)
    num_nodes++;          // Initialize number of nodes to 1 (END node) 
    
    /* Read first byte of data to be compressed from standard input */
    if((s = getchar()) != EOF) { // Get first char from stdin
        *(cbptr++) = s; // Update current block storage
        bbcnt++;        // Update Block Byte Count
    } else {
        return 1;       // Empty file
    }

    /* Read the remaining data */
    for(;;) {
        if((s = getchar()) != EOF) { // Get next char from stdin
            *(cbptr++) = s; // Update current block storage
            bbcnt++;        // Update Block Byte Count
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
        s = *(cbptr+i);    // Get the next symbol
        for(int j = 1; j < (2*MAX_SYMBOLS - 1); ++j) {
            /* If the symbol already exists in the node array or if it does not */
            if(nptr->symbol == s || !nptr->symbol) {
                if(!nptr->symbol)
                    num_nodes++;  // Increment the node count in the nodes array
                nptr->symbol = s; // Update Symbol
                nptr->weight++;   // Incrememnt Symbol occurrence
                break;
            }
            nptr++;     // Go to next node
        }
        nptr = nodes+1; // Point back to 2nd index
    }

    heapnum = num_nodes;       // Number of nodes currently in the min-heap
    num_nodes = 2*num_nodes-1; // Number of nodes to be in Huffman tree of current block

    /* Histogram to Min-Heap */
    for(int i = heapnum/2; i >= 0; i--) {
        min_heapify(i, heapnum);
    }

    /* Huffman Tree Construction */
    int n = num_nodes; // Copy of the number of nodes 
    /* Repeatedly remove 2 minimum weight nodes until Huffman Tree Constructed */
    while(remove_min(&heapnum, &n));

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

    return 0;
}

/*
 * @brief Use a stack to rebuild the Huffman Tree
 * according to the bit sequence in the Tree
 * description.
 * @details When the bit_val is 0 a new leaf is created and pushed on the stack.
 * If it is 1 then two nodes are popped and are made children of a new node.
 * 
 * @param bit_val - Scanned bit value used to determine direction of child nodes
 * @param sp - Stack pointer
 * @param nnum - Current number of nodes
*/
static void
push_pop(char bit_val, NODE **sp, int *nnum) {
    /* If 0: create new leaf node and  */
    if(!bit_val) {
        /* Initialize New Leaf */
        (*sp)->left = NULL;
        (*sp)->right = NULL;
        ++(*sp); // Increment stack pointer ("pushing" new leaf node)
    } else { 
    /* If 1: pop two nodes - give them a parent */
        NODE *R, *L;

        /* The two popped nodes go to Upper end of nodes array */
        R = nodes+(*nnum-1);
        L = nodes+(*nnum-2);

        --(*sp); // Decrement stack pointer
        *R = *(*sp); // Store first popped node
        --(*sp); // Decrement stack pointer
        *L = *(*sp); // Store second popped node

        /* Create Parent of popped nodes */
        (*sp)->right = R;
        (*sp)->left = L;
        
        (*sp)++; // Increment stack pointer (pushing new parent node)
        
        *nnum -= 2; // Reduce temprary size of Huff Tree
    }
}

/*
 * @brief Decodes the post-order bit sequence 
 * generated by compress() in order to re-construct the 
 * Huffman Tree.
 * 
*/
static int
decode_bit_seq() {
    int s; // Current byte being analyzed
    NODE *sp = nodes; // Stack pointer

    /* Get number of nodes in the huffman tree of the current compressed block */
    if((s = getchar()) != EOF) { // Get first byte from stdin
        num_nodes = (s << BYTE); // MSB 
    } else {
        return 1; // Empty file 
    }
    if((s = getchar()) != EOF) { // Get second byte from stdin
        num_nodes |= s; // LSB
    } else {
        return 1;
    }

    /* Decode post-order bit sequence */
    unsigned bit_count = 0; // Per byte bit counter
    unsigned n_bits = 0; // Number of bits representing structure of the tree (post order traversal)
    char bit_val; // Holds Current bit being evaluated 
    int nnum = num_nodes; // Copy of num_nodes used in reconstruct()
    while(n_bits != num_nodes) {
        if((s = getchar()) != EOF) { // Get next byte from stdin
            while(bit_count != BYTE) {
                bit_val = (s >> (BYTE - ((bit_count+1) % BYTE)) & 0x01); // Get value of next bit

                push_pop(bit_val, &sp, &nnum); // Evaluate scanned value

                bit_count++; // Increment number of bits evaluated in current byte
                n_bits++; // Increment number of nodes decoded

                if(n_bits == num_nodes) break; // Skip padding

                if(bit_count == (BYTE - 1)) {
                    bit_val = s & 0x01; // Get value of last bit

                    push_pop(bit_val, &sp, &nnum); // Evaluate scanned value

                    bit_count++; // Increment number of bits evaluated in current byte
                    n_bits++; // Increment number of nodes decoded
                }
            }

            /* Reset bit coutner */
            bit_count = 0;
        } else return 1; /* Invalid bit sequence - return error */
    }

   return 0;
}

/*
 * @brief Recursive post order travesral of the reconstructed
 * Huffman Tree.
 * @details Adds the symbols to the leaves.
 * 
 * @param n Current node being evaluated
 * @param s Symbol
 * 
 * @return 1 if EOF reached (error), 0 if successful symbol restoration
*/
static int 
restores_symbols(NODE *n, int s) {
    /* If leaf node (either left or right == NULL) */
    if(n->left == NULL) {
        if((s = getchar()) != EOF) { 
            if(s == 0xFF) { 
                if((s = getchar()) != EOF) {  // Get next byte after 0xFF
                    if(s == 0x00) { // Check for END node
                        END = n; // Pointer to END node
                        return 0;
                    } else {
                        n->symbol = 0xFF; // Assign 0xFF if next byte != 0x00
                    }
                } else {
                    return 1;
                }
            }
            n->symbol = s; // Assign symbol
        } else {
            return 1;
        }

        return 0;
    } 
    
    /* Go to Left child */
    if(restores_symbols(n->left, s)) return 1; // If EOF reached = error
    /* Go to Right child */
    if(restores_symbols(n->right, s)) return 1; // If EOF reached = error

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
    if(decode_bit_seq()) return 1;

    /* Restore the symbol values of the leaf nodes */
    return restores_symbols(nodes, 0);
}

/*
 * @brief Decode bits using the Huffman Tree. Output the
 * decoded symbols to stdout.
 * 
 * @param nptr pointer to current node being evaluated
 * @param bit_val direction of Huffman Tree traversal
 * 
 * @return 0: continue decoding next bit, 1: error (invalid bit sequence), 2: END node reached
*/
static int
decode_data (NODE **nptr, char bit_val) {
    /* If bit_val is 1 -> go right. If 0 -> go left */
    if(bit_val) {
        if((*(*nptr)).right) {
            *nptr = (*(*nptr)).right;
            if(!(*(*nptr)).right) {        // Check if leaf node
                if(*nptr == END) return 2; // If END node - De-compression complete 
            
                putchar((*(*nptr)).symbol); // Output decoded symbol
                *nptr = nodes;              // Reset pointer back to root of Huffman Tree
            }
        } else {
            return 1;
        }
    } else {
        if((*(*nptr)).left) {
            *nptr = (*(*nptr)).left;
            if(!(*(*nptr)).left) {          // Check if leaf node
                if(*nptr == END) return 2;  // If END node - De-compression complete 
                putchar((*(*nptr)).symbol); // Output decoded symbol
                *nptr = nodes;              // Reset pointer back to root of Huffman Treeo
            }
        } else {
            return 1;
        }
    }

    return 0;
}

/*
 * @brief Decode and output compressed data.
 * @details Go through the bit seqeunce and decode them 
 * to form the original characters of the uncompressed data.
*/
static int
decode() {
    int s;        // Current byte being analyze
    char bit_val; // Holds Current bit being evaluated 
    unsigned bit_count = 0; // Per byte bit counter
    NODE *nptr = nodes;     // Pointer to root of Huffman Tree

    for(;;) {
        if((s = getchar()) != EOF) { // Get next byte from stdin
            while(bit_count != BYTE) {
                bit_val = (s >> (BYTE - ((bit_count+1) % BYTE)) & 0x01); // Get value of next bit
                bit_count++; // Increment number of bits evaluated in current byte

                /* Evaluate decoding completion */
                switch(decode_data(&nptr, bit_val)) { // Decode bit
                    case 0: break;    // Continue decoding next bit
                    case 1: return 1; // Error
                    case 2: return 0; // END node reached - De-compression complete
                }

                if(bit_count == (BYTE - 1)) {
                    bit_val = s & 0x01; // Get value of last bit
                    bit_count++;        // Increment number of bits evaluated in current byte

                    /* Evaluate decoding completion */
                    switch(decode_data(&nptr, bit_val)) { // Decode bit
                        case 0: break;    // Continue decoding next bit
                        case 1: return 1; // Error 
                        case 2: return 0; // END node reached - De-compression complete
                    }
                }
            }

            /* Reset bit coutner */
            bit_count = 0;
        } else { return 1; } /* Invalid bit sequence - return error */
    }
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
    if(read_huffman_tree()) return 1;

    /* De-compress block */
    if(decode()) return 1;

    return 0;
}

/*
 * @brief Reads compressed data from standard input, writes uncompressed
 * data to standard output.
 * @details This function reads blocks of compressed data from the standard
 * input until EOF is reached, it decompresses each block, and it outputs
 * the uncompressed data to the standard output. The input data blocks
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

    return 0;
}
