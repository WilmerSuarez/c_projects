/*
 * Build lineage-linked database from lines of GEDCOM file.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "node.h"
#include "database.h"
#include "index.h"
#include "tags.h"

/*
 * Counts of various kinds of top-level records
 */
int total_individuals;
int total_families;
int total_events;
int total_sources;
int total_notes;
int total_repositories;
int total_submitters;

/*
 * Flag controlling capitalization of surnames.
 */
int capitalization = 1;

/*
 * Arrays for each access to top-level records
 */
struct individual_record **all_individuals;
struct family_record **all_families;

void extract_xref(struct node *np);

/*
 * Pass I: Allocate database records and attach them to the GEDCOM nodes.
 */
void
process_records(struct node *np) {
  for( ; np != NULL; np = np->siblings) {
    if(np->tag == NULL) continue;
    switch(np->tag->value) {
      case INDI:
        total_individuals++;
        process_individual_record(np);
        break;
      case FAM:
        total_families++;
        process_family_record(np);
        break;
      case EVEN:
        total_events++;
        process_event_record(np);
        break;
      case NOTE:
        total_notes++;
        process_note_record(np);
        break;
      case SOUR:
        total_sources++;
        process_source_record(np);
        break;
      case REPO:
        total_repositories++;
        process_repository_record(np);
        break;
      case SUBM:
        total_submitters++;
        process_submitter_record(np);
        break;
      default:
        /* Skip unrecognized records */
        break;
    }
  }
}

void
process_individual_record(struct node *np) {
  struct individual_record *ip;
  struct note_structure *ntp;
  struct xref *xp;
 
  if((ip = malloc(sizeof(*ip))) == NULL) out_of_memory();
  np->hook = ip;
  ip->xref = np->xref;
  index_enter(ip->xref, ip);
  for(np = np->children ; np != NULL; np = np->siblings) {
    switch(np->tag->value) {
      case NAME:
        ip->personal_name = process_name(np);
        break;
      case FAMS:
        xp = process_xref(np);
        if(ip->fams == NULL)
          ip->fams = ip->lastfams = xp;
        else {
          ip->lastfams->next = xp;
          ip->lastfams = xp;
        }
        break;
      case FAMC:
        xp = process_xref(np);
        if(ip->famc == NULL)
          ip->famc = ip->lastfamc = xp;
        else {
          ip->lastfamc->next = xp;
          ip->lastfamc = xp;
        }
        break;
      case SOUR:
        xp = process_xref(np);
        if(ip->sources == NULL)
          ip->sources = ip->lastsource = xp;
        else {
          ip->sources->next = xp;
          ip->lastsource = xp;
        }
        break;
      case REFN:
        ip->refn = np->rest;
        break;
      case RFN:
        ip->rfn = np->rest;
        break;
      case AFN:
        ip->afn = np->rest;
        break;
      case NOTE:
        ntp = process_note(np);
        if(ip->notes == NULL)
          ip->notes = ip->lastnote = ntp;
        else {
          ip->lastnote->next = ntp;
          ip->lastnote = ntp;
        }
        break;
      case TITL:
        ip->title = np->rest;
        break;
      case SEX:
        if(*np->rest == 'M')
          ip->sex = 'M';
        else if(*np->rest == 'F')
          ip->sex = 'F';
        break;
      case CENS: case MARR: case MARB: case MARC: case MARL: case MARS:
      case ENGA: case BAPM: case BARM: case BASM: case BIRT: case BLES:
      case BURI: case CHR: case CHRA: case CONF: case DEAT: case EMIG:
      case GRAD: case IMMI: case NATU: case ORDN: case RETI: case PROB:
      case WILL: case ANUL: case DIV: case DIVF:
        if(ip->events == NULL)
          ip->events = ip->lastevent = process_event(np);
        else {
          ip->lastevent->next = process_event(np);
          ip->lastevent = ip->lastevent->next;
        }
        break;
      default:
        /* Skip unrecognized substructures */
        break;
    }
  }
}

void
process_family_record(struct node *np) {
  struct family_record *frp;
  struct note_structure *ntp;
  struct xref *xp;

  if((frp = malloc(sizeof(*frp))) == NULL) out_of_memory();
  memset(frp, 0, sizeof(*frp));
  np->hook = frp;
  frp->xref = np->xref;
  index_enter(frp->xref, frp);
  for(np = np->children ; np != NULL; np = np->siblings) {
    if(np->tag == NULL) continue;
    switch(np->tag->value) {
      case HUSB:
        frp->husband = process_xref(np);
        break;
      case WIFE:
        frp->wife = process_xref(np);
        break;
      case CHIL:
        xp = process_xref(np);
        if(frp->children == NULL)
          frp->children = frp->lastchild = xp;
        else {
          frp->lastchild->next = xp;
          frp->lastchild = xp;
        }
        break;
      case SOUR:
        xp = process_xref(np);
        if(frp->sources == NULL)
          frp->sources = frp->lastsource = xp;
        else {
          frp->sources->next = xp;
          frp->lastsource = xp;
        }
        break;
      case REFN:
        frp->refn = np->rest;
        break;
      case CENS: case MARR: case MARB: case MARC: case MARL: case MARS:
      case ENGA: case BAPM: case BARM: case BASM: case BIRT: case BLES:
      case BURI: case CHR: case CHRA: case CONF: case DEAT: case EMIG:
      case GRAD: case IMMI: case NATU: case ORDN: case RETI: case PROB:
      case WILL: case ANUL: case DIV: case DIVF:
        if(frp->events == NULL)
          frp->events = frp->lastevent = process_event(np);
        else {
          frp->lastevent->next = process_event(np);
          frp->lastevent = frp->lastevent->next;
        }
        break;
      case NOTE:
        ntp = process_note(np);
        if(frp->notes == NULL)
          frp->notes = frp->lastnote = ntp;
        else {
          frp->lastnote->next = ntp;
          frp->lastnote = ntp;
        }
        break;
      default:
        /* Skip unrecognized substructures */
        break;
    }
  }
}

void
process_source_record(struct node *np) {
  struct source_record *sp;
  struct continuation *cp;
  int cont = 0;

  if((sp = malloc(sizeof(*sp))) == NULL) out_of_memory();
  memset(sp, 0, sizeof(*sp));
  np->hook = sp;
  sp->xref = np->xref;
  index_enter(sp->xref, sp);
  sp->text = np->rest;
  for(np = np->children ; np != NULL; np = np->siblings) {
    if(np->tag == NULL)
      continue;
    switch(np->tag->value) {
      case CONT:
        if(cont == 0) {
          cont++;
          if((sp->cont = malloc(sizeof(*sp->cont))) == NULL) out_of_memory();
            cp = sp->cont;
        } else {
          if((cp->next = malloc(sizeof(*cp->next))) == NULL) out_of_memory();
          cp = cp->next;
        }
        memset(cp, 0, sizeof(*cp));
        np->hook = cp;
        cp->text = np->rest;
        break;
    }
  }
}

void
process_event_record(struct node *np) {
  (void)np;
}

void
process_note_record(struct node *np) {
  (void)np;
}

void
process_repository_record(struct node *np) {
  (void)np;
}

void
process_submitter_record(struct node *np) {
  (void)np;
}

struct event_structure *
process_event(struct node *np) {
  struct event_structure *ep;
  struct place_structure *pp;

  if((ep = malloc(sizeof(*ep))) == NULL) out_of_memory();
  memset(ep, 0, sizeof(*ep));
  np->hook = ep;
  ep->tag = np->tag;
  for(np = np->children; np != NULL; np = np->siblings) {
    if(np->tag == NULL) continue;
    switch(np->tag->value) {
      case DATE:
        ep->date = np->rest;
        break;
      case PLAC:
        if((pp = malloc(sizeof(*pp))) == NULL) out_of_memory();
        memset(pp, 0, sizeof(*pp));
        pp->name = np->rest;
        ep->place = pp;
        break;
      default:
        break;
    }
  }
  return(ep);
}

struct note_structure *
process_note(struct node *np) {
  struct note_structure *ntp;
  struct continuation *ntpc;
  int cont = 0;
 
  if((ntp = malloc(sizeof(*ntp))) == NULL) out_of_memory();
  memset(ntp, 0, sizeof(*ntp));
  np->hook = ntp;
  ntp->text = np->rest;
  for(np = np->children; np != NULL; np = np->siblings) {
    if(np->tag == NULL) continue;
    switch(np->tag->value) {
      case CONT:
        if(cont == 0) {
          cont++;
          if((ntp->cont = malloc(sizeof(*ntp->cont))) == NULL) out_of_memory();
          ntpc = ntp->cont;
        } else {
          if((ntpc->next = malloc(sizeof(*ntpc->next))) == NULL) out_of_memory();
          ntpc = ntpc->next;
        }
        memset(ntpc, 0, sizeof(*ntpc));
        np->hook = ntpc;
        ntpc->text = np->rest;
        break;
      default:
        break;
    }
  }
  return(ntp);
}

struct xref *
process_xref(struct node *np) {
  struct xref *xp;

  extract_xref(np);
  if((xp = malloc(sizeof(*xp))) == NULL) out_of_memory();
  memset(xp, 0, sizeof(*xp));
  xp->id = np->rest;
  return(xp);
}

struct name_structure *
process_name(struct node *np) {
  char *cp, *p;
  int i, surname=0;
  struct name_structure *nsp;

  for(i = 0, cp = np->rest; *cp != '\0'; cp++, i++);
  if((p = malloc(i+1)) == NULL) out_of_memory();
  if((nsp = malloc(sizeof(*nsp))) == NULL) out_of_memory();
  memset(nsp, 0, sizeof(*nsp));
  nsp->name = p;
  for(i = 0, cp = np->rest; *cp != '\0'; cp++, i++) {
    if(*cp == '/') {
      surname = 1 - surname;
      if(surname)
	      nsp->surname_start = i;
      else
	      nsp->surname_end = i;
      *p++ = ' ';
      continue;
    } else {
      if(surname && capitalization)
	      *p++ = islower(*cp) ? toupper(*cp) : *cp;
      else
	      *p++ = *cp;
    }
  }
  *p = '\0';
  return(nsp);
}

/*
 * Pass II: Create lineage-linked structure on database nodes.
 */
void
link_records(struct node *np) {
  struct individual_record **ip;
  struct family_record **fp;
  int i;

  if((ip = all_individuals = malloc(total_individuals *
			   sizeof(struct individual_record **))) == NULL)
    out_of_memory();
  if((fp = all_families = malloc(total_families *
			   sizeof(struct family_record **))) == NULL)
    out_of_memory();
  for( ; np != NULL; np = np->siblings) {
    if(np->tag == NULL) continue;
    switch(np->tag->value) {
      case INDI:
        *ip++ = (struct individual_record *)np->hook;
        link_individual_record(np);
        break;
      case FAM:
        *fp++ = (struct family_record *)np->hook;
        link_family_record(np);
        break;
      case SOUR:
      case EVEN:
      case NOTE:
      case REPO:
      case SUBM:
      default:
        /* Skip unrecognized records */
        break;
    }
  }
  qsort(all_individuals, total_individuals,
	sizeof(struct individual_record *),
	(int (*)(const void *, const void *)) compare_name);
  /*
   * Link individuals for the benefit of the output interpreter
   */
  for(i = 0; i < total_individuals-1; i++)
    all_individuals[i]->next = all_individuals[i+1];
  all_individuals[total_individuals-1]->next = NULL;
}

void
link_individual_record(struct node *np) {
  struct individual_record *ip;
  struct xref *xp;

  ip = (struct individual_record *)np->hook;
  for(xp = ip->fams; xp != NULL; xp = xp->next)
    xp->pointer.family = index_find(xp->id);
  for(xp = ip->famc; xp != NULL; xp = xp->next)
    xp->pointer.family = index_find(xp->id);
  for(xp = ip->sources; xp != NULL; xp = xp->next)
    xp->pointer.source = index_find(xp->id);
}

void
link_family_record(struct node *np) {
  struct family_record *fp;
  struct xref *xp;

  fp = (struct family_record *)np->hook;
  if((xp = fp->husband))
    xp->pointer.family = index_find(xp->id);
  if((xp = fp->wife))
    xp->pointer.family = index_find(xp->id);
  for(xp = fp->children; xp != NULL; xp = xp->next)
    xp->pointer.individual = index_find(xp->id);
}

/*
 * Compare personal names for lexicographic order.
 * Used to sort individual records.
 */
int compare_name(struct individual_record **ipp1,
		 struct individual_record **ipp2)
{
  char *p1, *p2, *ep1, *ep2;
  struct individual_record *ip1 = *ipp1;
  struct individual_record *ip2 = *ipp2;

  /*
   * Handle case in which names are missing
   */
  if(ip1->personal_name == NULL) {
    if(ip2->personal_name == NULL) {
      return(0);
    } else {
      return(1);
    }
  } else if(ip2->personal_name == NULL) {
    return(-1);
  }

  /*
   * Normal case -- names present
   */
  p1 = ip1->personal_name->name + ip1->personal_name->surname_start;
  p2 = ip2->personal_name->name + ip2->personal_name->surname_start;
  ep1 = ip1->personal_name->name + ip1->personal_name->surname_end;
  ep2 = ip2->personal_name->name + ip2->personal_name->surname_end;
  while(p1 < ep1 && p2 < ep2) {
    if(*p1 < *p2) return(-1);
    if(*p1 > *p2) return(1);
    p1++;
    p2++;
  }
  if(p1 != ep1)
    return(-1);
  else if(p2 != ep2)
    return(1);
  else
    return(strcmp(ip1->personal_name->name, ip2->personal_name->name));
}

/*
 * Adjust np->rest in case an XREF constitutes the rest of the GEDCOM line
 */
void
extract_xref(struct node *np) {
  if(*np->rest == '@') {
    char *cp;
    np->rest++;
    for(cp = np->rest; *cp != '\0' && *cp != '@'; cp++);
    *cp = '\0';
  }
}
