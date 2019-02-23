/*
 * GEDCOM lineage-linked database structure definitions
 */

/*
 * Counts of various kinds of top-level records
 */

extern int total_individuals;
extern int total_families;
extern int total_events;
extern int total_sources;
extern int total_notes;
extern int total_repositories;
extern int total_submitters;

/*
 * Flag controlling capitalization of surnames
 */

int capitalization = 1;

/*
 * Arrays for each access to top-level records
 */

struct individual_record **all_individuals;
struct family_record **all_familes;

/*
 * Database structure definitions
 */

struct individual_record {
  int serial;
  char *xref;
  struct name_structure *personal_name;
  char *title;
  char sex;
  char *refn;
  char *rfn;
  char *afn;
  struct xref *fams, *lastfams;
  struct xref *famc, *lastfamc;
  struct xref *sources, *lastsource;
  struct note_structure *notes, *lastnote;
  struct event_structure *events, *lastevent;
  struct individual_record *next;
};

struct family_record {
  char *xref;
  char *refn;
  struct xref *husband;
  struct xref *wife;
  struct xref *children, *lastchild;
  struct xref *sources, *lastsource;
  struct note_structure *notes, *lastnote;
  struct event_structure *events, *lastevent;
  struct family_record *next;
};

struct source_record {
  char *xref;
  char *text;
  struct continuation *cont;
};

struct name_structure {
  char *name;
  int surname_start;
  int surname_end;
};

struct place_structure {
  char *name;
  struct note_structure *notes, *lastnote;
};

struct note_structure {
   char *xref;
   char *text;
   struct continuation *cont;
   struct note_structure *next;
};

struct event_structure {
  struct tag *tag;
  char *date;
  struct place_structure *place;
  struct event_structure *next;
};

struct xref {
  char *id;
  union {
    struct individual_record *individual;
    struct family_record *family;
    struct source_record *source;
  } pointer;
  struct xref *next;
};

struct continuation {
  char *text;
  struct continuation *next;
};

/*
 * Function prototypes
 */

void process_records(struct node *np);
void process_individual_record(struct node *np);
void process_family_record(struct node *np);
void process_event_record(struct node *np);
void process_note_record(struct node *np);
void process_submitter_record(struct node *np);
void process_repository_record(struct node *np);
void process_source_record(struct node *np);
struct event_structure *process_event(struct node *np);
struct note_structure *process_note(struct node *np);
struct xref *process_xref(struct node *np);
struct name_structure *process_name(struct node *np);
void link_records(struct node *np);
void link_individual_record(struct node *np);
void link_family_record(struct node *np);
int compare_name(struct individual_record **ipp1,
		 struct individual_record **ipp2);

