/*
 * XREF ID --> pointer index routines
 *
 * If too much time is being spent in this code, replace it with something
 * more sophisticated.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "index.h"

struct ientry {
  char *id;
  void *value;
  struct ientry *next;
};

void out_of_memory();

#define HASHSIZE 5003
#define HASHMULT 251

struct ientry *hashtab[HASHSIZE];

int hash(char *id) {
  unsigned int h = 0;
  while(*id) {
    h = (h * HASHMULT) + *id;
    id++;
  }
  return(h % HASHSIZE);
}

int index_enter(char *id, void *value) {
  int h;
  struct ientry *ip, *pip, *new;

  h = hash(id);
  for(ip = hashtab[h], pip = NULL; ip != NULL; pip = ip, ip = ip->next) {
    if(!strcmp(ip->id, id)) {
      fprintf(stderr, "Multiply defined cross-reference ID: %s\n", id);
      return(-1);
    }
  }
  if((new = malloc(sizeof(struct ientry))) == NULL) out_of_memory();
  new->id = strdup(id);
  new->value = value;
  new->next = NULL;
  if(pip == NULL) {
    hashtab[h] = new;
  } else {
    pip->next = new;
  }
}

void *index_find(char *id) {
  int h;
  struct ientry *ip;

  h = hash(id);
  for(ip = hashtab[h]; ip != NULL; ip = ip->next) {
    if(!strcmp(ip->id, id))
      return(ip->value);
  }
  return(NULL);
}
