/*
 * DO NOT MODIFY THE CONTENTS OF THIS FILE.
 * IT WILL BE REPLACED DURING GRADING
 */
#ifndef CONST_H
#define CONST_H

#include <stdio.h>
#include "huff.h"

#define USAGE(program_name, retcode) do{ \
fprintf(stderr, "USAGE: %s %s\n", program_name, \
"[-h] -c|-d [-b BLOCKSIZE]\n" \
"    -h       Help: displays this help menu.\n" \
"    -c       Compress: read raw data, output compressed data\n" \
"    -d       Decompress: read compressed data, output raw data\n" \
"    -b       For compression, specify blocksize in bytes (range [1024, 65536])\n"); \
exit(retcode); \
} while(0)

/* Options info, set by validargs. */
int global_options;

/*
 * @brief Validates command line arguments passed to the program.
 * @details This function will validate all the arguments passed to the
 * program, returning 0 if validation succeeds and 1 if validation fails.
 * Upon successful return, the selected program options will be set in the
 * global variable "global_options", where they will be accessible
 * elsewhere in the program.
 *
 * @param argc The number of arguments passed to the program from the CLI.
 * @param argv The argument strings passed to the program from the CLI.
 * @return 0 if validation succeeds and 1 if validation fails.
 * @modifies global variable "global_options" to contain a bitmap representing
 * the selected options.
 */
int validargs(int argc, char **argv);

/*
 * @brief Emits a description of the Huffman tree used to compress the current block.
 * @details This function emits, to the standard output, a description of the
 * Huffman tree used to compress the current block.  Refer to the assignment handout
 * for a detailed specification of the format of this description.
 */
void emit_huffman_tree();

/*
 * @brief Reads a description of a Huffman tree and reconstructs the tree from
 * the description.
 * @details  This function reads, from the standard input, the description of a
 * Huffman tree in the format produced by emit_huffman_tree(), and it reconstructs
 * the tree from the description.  First two bytes are read, and the number of
 * nodes n and the number of leaf nodes (n+1)/2 are computed from them.
 * Then, a sequence of n bits is read (8 bits per byte) and the tree structure is
 * built.  This is done using a stack of nodes: when a 0 bit is read, a node is
 * initialized as a leaf and pushed onto the stack, and when a 1 bit is read,
 * two nodes, first R then L, are popped from the stack and are made the right
 * and left children, respectively of a new internal node, which is then pushed
 * back onto the stack.  After n bits have been processed in this way, the one
 * node that remains on the stack is the root of the tree.  Finally, the sequence of
 * bytes that specifies the symbol values to be stored at the leaves of the tree
 * is read, and these values are stored at the leaves in left-to-right order.
 *
 * @return 0 if the tree is read and reconstructed without error, otherwise 1
 * if an error occurs.
 */
int read_huffman_tree();

/*
 * @brief Reads one block of data from standard input and emits corresponding
 * compressed data to standard output.
 * @details This function reads raw binary data bytes from the standard input
 * until the specified block size has been read or until EOF is reached.
 * It then applies a data compression algorithm to the block and outputs the
 * compressed block to the standard output.  The block size parameter is
 * obtained from the global_options variable.
 *
 * @return 0 if compression completes without error, 1 if an error occurs.
 */
int compress_block();

/*
 * @brief Reads one block of compressed data from standard input and writes
 * the corresponding uncompressed data to standard output.
 * @details This function reads one block of compressed data from the standard
 * inputk it decompresses the block, and it outputs the uncompressed data to
 * the standard output.  The input data blocks are assumed to be in the format
 * produced by compress().  If EOF is encountered before a complete block has
 * been read, it is an error.
 *
 * @return 0 if decompression completes without error, 1 if an error occurs.
 */
int decompress_block();

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
int compress();

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
int decompress();

#endif
