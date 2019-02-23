/*
 * Structure of a generic GEDCOM node
 */

typedef struct node {
  long lineno;			/* Source line number */
  int  level;			/* Level number at which line appears */
  char *xref;			/* Cross-reference ID */
  struct tag *tag;		/* tag structure for tag */
  char *line;			/* The whole line of input */
  char *rest;			/* The rest of the line after the tag */
  void *hook;			/* Hook to corresponding database node */
  struct node *children;	/* List of subsidiary nodes */
  struct node *siblings;	/* List of siblings */
} NODE;

void out_of_memory();

