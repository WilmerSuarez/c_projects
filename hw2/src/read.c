/*
 * Read in a GEDCOM file without interpreting the nodes
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "node.h"
#include "read.h"
#include "tags.h"

long gedcom_lines;
long current_lineno;
char *current_gedcom;

#if defined(MSDOS) || defined(LINUX)
char *fgetln(FILE *f, int *size);
#endif

struct node *
newnode()
{
  struct node *n;
  if((n = malloc(sizeof(*n))) == NULL)
    out_of_memory();
  memset(n, 0, sizeof(*n));
  return(n);
}

/*
 * Read a series of GEDCOM lines at the same level, and chain them
 * onto the given list.  If a line is encountered at a deeper level,
 * then recurse.
 *
 * prev is a pointer to the previous sibling at the current level
 */

struct node *
read_gedcom(FILE *f, struct node *prev, int level)
{
  char *line, *rest, *levp, *xrefp, *tagp;
  struct node *node = NULL;
  struct tag *tp;
  int size;

  while(prev && (line = fgetln(f, &size))) {
    gedcom_lines++;
    /*
     * Allocate node and copy line into it
     */
    node = newnode();
    node->lineno = ++current_lineno;
    if((node->line = malloc(size+1)) == NULL)
      out_of_memory();
    node->line[size] = '\0';
    do {
      --size;
      switch(line[size]) {
      case '\n':
      case '\r':
	node->line[size] = '\0';
	continue;
	break;
      default:
	node->line[size] = line[size];
	break;
      }
    } while(size);
    line = node->line;
    /*
     * Figure out level number
     */
    rest = line;
    while(*rest == ' ')
      rest++;
    if(*rest == '\0')
      continue;		/* Ignore blank line */
    levp = rest;
    while(*rest >= '0' && *rest <= '9')
      rest++;
    if(*rest != ' ') {
      fprintf(stderr, "%s: %d: Malformed GEDCOM line ignored\n",
	      current_gedcom, current_lineno);
      free(node);
      free(node->line);
      continue;
    }
    *rest++ = '\0';
    node->level = atoi(levp);
    /*
     * Extract XREF, if any
     */
    while(*rest == ' ')
      rest++;
    if(*rest == '\0') {
      fprintf(stderr, "%s: %d: Malformed GEDCOM line ignored\n",
	      current_gedcom, current_lineno);
      free(node->line);
      free(node);
      continue;
    }
    if(*rest == '@') {
      xrefp = ++rest;
      while(*rest != '\0' && *rest != '@')
	rest++;
      if(*rest != '@') {
	fprintf(stderr, "%s: %d: Non-terminated cross-reference -- line ignored\n",
		current_gedcom, current_lineno);
	free(node->line);
	free(node);
	continue;
      }
      *rest++ = '\0';
    } else {
      xrefp = NULL;
    }
    node->xref = xrefp;
    /*
     * Extract tag
     */
    while(*rest == ' ')
      rest++;
    if(*rest == '\0') {
      fprintf(stderr, "%s: %d: Ignored GEDCOM line with no tag\n",
	      current_gedcom, current_lineno);
      free(node->line);
      free(node);
      continue;
    }
    tagp = rest;
    while(*rest != '\0' && *rest != ' ')
      rest++;
    if(*rest)
      *rest++ = '\0';
    if(tp = findtag(tagp, gedcom_tags, gedcom_tags_size))
      node->tag = tp;
    while(*rest == ' ')
      rest++;
    node->rest = rest;
    /*
     * The line is parsed, now take care of linking it in to
     * the data structure
     */
    if(node->level < level) {
      return(node);
    } else if(node->level == level) {
      prev->siblings = node;
      prev = node;
      continue;
    } else {
      if(node->level > level+1)
	fprintf(stderr, "%s: %d: Level number increased by more than one\n",
		current_gedcom, current_lineno);
      prev->children = node;
      node = read_gedcom(f, node, node->level);
      if(node == NULL) {
	fprintf(stderr, "%s: %d GEDCOM file does not end at level 0\n",
		current_gedcom, current_lineno);
	return(NULL);
      }
      if(node->level < level)
	return(node);
      prev->siblings = node;
      prev = prev->siblings;
    }
  }
  if(!feof(f)) {
    if(errno == ENOMEM)
      out_of_memory();
    else
      fprintf(stderr, "%s: %d: Error reading GEDCOM file\n",
	      current_gedcom, current_lineno);
  }
  return(NULL);
}

void out_of_memory()
{
  fprintf(stderr, "Insufficient memory available for processing.\n");
  exit(1);
}
                   
#if defined(MSDOS) || defined(LINUX)
char *fgetln(FILE *f, int *size)
{
   static char *l = NULL;
   char *lp, c;
   static int max = 4;
   int s = 0;
   
    
   if(l == NULL) {
     if((l = malloc(max)) == NULL)
   	   out_of_memory();
   }
   if(feof(f) || ferror(f)) {
     *size = 0;
     return(NULL);
   }
   lp = l;
   while((c = fgetc(f)) != EOF) {
     if(s >= max-1) {
        max = 2*max;
        if((l = realloc(l, max)) == NULL)
          out_of_memory();
        lp = l + s;   
     }
     *lp++ = c;
     s++;
     if(c == '\n')
        break;
   }
   *lp++ = '\0';
   *size = s;
   if(s == 0)
     return(NULL);
   return(l);
}
#endif

                   
