/*
 * HTML Output Routines
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include "tags.h"
#include "node.h"
#include "database.h"
#include "output.h"

#ifndef FILENAME_MAX
#define FILENAME_MAX 1024
#endif

char *url_template="%s.html";
char *file_template="%s.html";
char *output_path="./";

#ifdef MSDOS
int max_per_directory = 100;
#else
int max_per_directory = 0;
#endif

/*
 * I don't like repeating these two templates, which are the same
 * except for "INDEX" versus "../INDEX", but I am tired of working on this
 * right now and I want to get something out that works right.
 */

/*
 * The templates are split into several strings, because the
 * "stone knives and bearskins" Microsoft compiler has a limit on
 * the length of a string literal.
 */
char *individual_template_subdir[] = {
  "<HTML>\n" \
  "<HEAD>\n" \
  "!IF @.TITLE\n" \
    "<TITLE>${@.XREF}: ${@.NAME} (${@.TITLE})</TITLE>\n" \
  "!ELSE\n" \
    "<TITLE>${@.XREF}: ${@.NAME}</TITLE>\n" \
  "!ENDIF\n" \
  "</HEAD>\n" \
  "<BODY>\n"
  "!IF @.TITLE\n" \
    "<H1><A HREF=\"${@.&}\">${@.NAME} (${@.TITLE})</A></H1>\n" \
  "!ELSE\n" \
    "<H1><A HREF=\"${@.&}\">${@.NAME}</A></H1>\n" \
  "!ENDIF\n" \
  "!INCLUDE @.img\n" \
  "!IF @.EVENT\n" \
    "<UL>\n" \
    "!RESET i\n" \
    "!WHILE @.EVENT[i]\n" \
      "!IF @.EVENT[i].DATE\n" \
        "!IF @.EVENT[i].PLACE\n" \
          "<LI><EM>${@.EVENT[i].TAG}</EM>: ${@.EVENT[i].DATE}, ${@.EVENT[i].PLACE.NAME}\n" \
        "!ELSE\n" \
          "<LI><EM>${@.EVENT[i].TAG}</EM>: ${@.EVENT[i].DATE}\n" \
        "!ENDIF\n" \
      "!ELSE\n" \
        "<LI><EM>${@.EVENT[i].TAG}</EM>: ${@.EVENT[i].PLACE.NAME}\n" \
      "!ENDIF\n" \
      "!INCREMENT i\n" \
    "!END\n" \
    "</UL>\n" \
   "!ENDIF\n" \
   "!IF @.FATHER\n" \
     "<EM>Father:</EM> <A HREF=\"${@.FATHER.&}\">${@.FATHER.NAME}</A><BR>\n" \
  "!ENDIF\n" \
  "!IF @.MOTHER\n" \
    "<EM>Mother:</EM> <A HREF=\"${@.MOTHER.&}\">${@.MOTHER.NAME}</A><BR>\n" \
  "!ENDIF\n" \
  "<BR>\n" \
  "!RESET i\n" \
  "!WHILE @.FAMS[i]\n" \
    "</EM>Family ${i}</EM>:\n" \
    "!IF @.ISMALE\n" \
      "!IF @.FAMS[i].FAMILY.WIFE\n" \
        "<A HREF=\"${@.FAMS[i].FAMILY.WIFE.&}\">${@.FAMS[i].FAMILY.WIFE.NAME}</A>\n" \
      "!ENDIF\n" \
    "!ENDIF\n"
    "!IF @.ISFEMALE\n" \
      "!IF @.FAMS[i].FAMILY.HUSBAND\n" \
        "<A HREF=\"${@.FAMS[i].FAMILY.HUSBAND.&}\">${@.FAMS[i].FAMILY.HUSBAND.NAME}</A>\n" \
      "!ENDIF\n" \
    "!ENDIF\n" \
    "!IF @.FAMS[i].FAMILY.EVENT\n" \
      "<UL>\n" \
      "!RESET j\n" \
      "!WHILE @.FAMS[i].FAMILY.EVENT[j]\n" \
        "<LI><EM>${@.FAMS[i].FAMILY.EVENT[j].TAG}</EM>: " \
        "${@.FAMS[i].FAMILY.EVENT[j].DATE}, ${@.FAMS[i].FAMILY.EVENT[j].PLACE.NAME}\n" \
        "!INCREMENT j\n" \
      "!END\n" \
      "</UL>\n" \
    "!ELSE\n" \
      "<BR>\n" \
    "!ENDIF\n",

    "!IF @.FAMS[i].FAMILY.CHILDREN\n" \
      "<OL>\n" \
      "!RESET j\n" \
      "!WHILE @.FAMS[i].FAMILY.CHILDREN[j]\n" \
        "<LI><A HREF=\"${@.FAMS[i].FAMILY.CHILDREN[j].INDIV.&}\">" \
        "${@.FAMS[i].FAMILY.CHILDREN[j].INDIV.NAME}</A>\n" \
        "!INCREMENT j\n" \
      "!END\n" \
      "</OL>\n" \
    "!ENDIF\n" \
    "!INCREMENT i\n" \
  "!END\n" \
  "<BR>\n" \
  "<A HREF=\"../INDEX.html\">INDEX</A><BR><BR>\n" \
  "!IF @.NOTE\n" \
    "<H2>Notes</H2>\n" \
    "!RESET i\n" \
    "!WHILE @.NOTE[i]\n" \
      "${@.NOTE[i].TEXT}\n" \
      "!RESET j\n" \
      "!WHILE @.NOTE[i].CONT[j]\n" \
        "${@.NOTE[i].CONT[j].TEXT}\n" \
/*        "<BR>\n" \  */
        "!INCREMENT j\n" \
      "!END\n" \
      "<BR>\n" \
      "!INCREMENT i\n" \
    "!END\n" \
  "!ENDIF\n" \
  "<BR>\n" \
  "!IF @.SOURCE\n" \
    "<H2>Sources</H2>\n" \
    "!RESET i\n" \
    "!WHILE @.SOURCE[i]\n" \
      "${@.SOURCE[i].SOURCE.TEXT}\n" \
      "!RESET j\n" \
      "!WHILE @.SOURCE[i].SOURCE.CONT[j]\n" \
        "${@.SOURCE[i].SOURCE.CONT[j].TEXT}\n" \
        "<BR>\n" \
        "!INCREMENT j\n" \
      "!END\n" \
      "<BR>\n" \
      "!INCREMENT i\n" \
    "!END\n" \
  "!ENDIF\n" \
  "!INCLUDE @.inc\n" \
  "</BODY>\n" \
  "</HTML>\n"
  };

int individual_template_subdir_size =
  sizeof(individual_template_subdir)/sizeof(char *);

char *individual_template_nosubdir[] = {
  "<HTML>\n" \
  "<HEAD>\n" \
  "!IF @.TITLE\n" \
    "<TITLE>${@.XREF}: ${@.NAME} (${@.TITLE})</TITLE>\n" \
  "!ELSE\n" \
    "<TITLE>${@.XREF}: ${@.NAME}</TITLE>\n" \
  "!ENDIF\n" \
  "</HEAD>\n" \
  "<BODY>\n"
  "!IF @.TITLE\n" \
    "<H1><A HREF=\"${@.&}\">${@.NAME} (${@.TITLE})</A></H1>\n" \
  "!ELSE\n" \
    "<H1><A HREF=\"${@.&}\">${@.NAME}</A></H1>\n" \
  "!ENDIF\n" \
  "!INCLUDE @.img\n" \
  "!IF @.EVENT\n" \
    "<UL>\n" \
    "!RESET i\n" \
    "!WHILE @.EVENT[i]\n" \
      "!IF @.EVENT[i].DATE\n" \
        "!IF @.EVENT[i].PLACE\n" \
          "<LI><EM>${@.EVENT[i].TAG}</EM>: ${@.EVENT[i].DATE}, ${@.EVENT[i].PLACE.NAME}\n" \
        "!ELSE\n" \
          "<LI><EM>${@.EVENT[i].TAG}</EM>: ${@.EVENT[i].DATE}\n" \
        "!ENDIF\n" \
      "!ELSE\n" \
        "<LI><EM>${@.EVENT[i].TAG}</EM>: ${@.EVENT[i].PLACE.NAME}\n" \
      "!ENDIF\n" \
      "!INCREMENT i\n" \
    "!END\n" \
    "</UL>\n" \
   "!ENDIF\n" \
   "!IF @.FATHER\n" \
     "<EM>Father:</EM> <A HREF=\"${@.FATHER.&}\">${@.FATHER.NAME}</A><BR>\n" \
  "!ENDIF\n" \
  "!IF @.MOTHER\n" \
    "<EM>Mother:</EM> <A HREF=\"${@.MOTHER.&}\">${@.MOTHER.NAME}</A><BR>\n" \
  "!ENDIF\n" \
  "<BR>\n" \
  "!RESET i\n" \
  "!WHILE @.FAMS[i]\n" \
    "</EM>Family ${i}</EM>:\n" \
    "!IF @.ISMALE\n" \
      "!IF @.FAMS[i].FAMILY.WIFE\n" \
        "<A HREF=\"${@.FAMS[i].FAMILY.WIFE.&}\">${@.FAMS[i].FAMILY.WIFE.NAME}</A>\n" \
      "!ENDIF\n" \
    "!ENDIF\n"
    "!IF @.ISFEMALE\n" \
      "!IF @.FAMS[i].FAMILY.HUSBAND\n" \
        "<A HREF=\"${@.FAMS[i].FAMILY.HUSBAND.&}\">${@.FAMS[i].FAMILY.HUSBAND.NAME}</A>\n" \
      "!ENDIF\n" \
    "!ENDIF\n" \
    "!IF @.FAMS[i].FAMILY.EVENT\n" \
      "<UL>\n" \
      "!RESET j\n" \
      "!WHILE @.FAMS[i].FAMILY.EVENT[j]\n" \
        "<LI><EM>${@.FAMS[i].FAMILY.EVENT[j].TAG}</EM>: " \
        "${@.FAMS[i].FAMILY.EVENT[j].DATE}, ${@.FAMS[i].FAMILY.EVENT[j].PLACE.NAME}\n" \
        "!INCREMENT j\n" \
      "!END\n" \
      "</UL>\n" \
    "!ELSE\n" \
      "<BR>\n" \
    "!ENDIF\n",

    "!IF @.FAMS[i].FAMILY.CHILDREN\n" \
      "<OL>\n" \
      "!RESET j\n" \
      "!WHILE @.FAMS[i].FAMILY.CHILDREN[j]\n" \
        "<LI><A HREF=\"${@.FAMS[i].FAMILY.CHILDREN[j].INDIV.&}\">" \
        "${@.FAMS[i].FAMILY.CHILDREN[j].INDIV.NAME}</A>\n" \
        "!INCREMENT j\n" \
      "!END\n" \
      "</OL>\n" \
    "!ENDIF\n" \
    "!INCREMENT i\n" \
  "!END\n" \
  "<BR>\n" \
  "<A HREF=\"INDEX.html\">INDEX</A><BR><BR>\n" \
  "!IF @.NOTE\n" \
    "<H2>Notes</H2>\n" \
    "!RESET i\n" \
    "!WHILE @.NOTE[i]\n" \
      "${@.NOTE[i].TEXT}\n" \
      "!RESET j\n" \
      "!WHILE @.NOTE[i].CONT[j]\n" \
        "${@.NOTE[i].CONT[j].TEXT}\n" \
/*        "<BR>\n" \  */
        "!INCREMENT j\n" \
      "!END\n" \
      "<BR>\n" \
      "!INCREMENT i\n" \
    "!END\n" \
  "!ENDIF\n" \
  "<BR>\n" \
  "!IF @.SOURCE\n" \
    "<H2>Sources</H2>\n" \
    "!RESET i\n" \
    "!WHILE @.SOURCE[i]\n" \
      "${@.SOURCE[i].SOURCE.TEXT}\n" \
      "!RESET j\n" \
      "!WHILE @.SOURCE[i].SOURCE.CONT[j]\n" \
        "${@.SOURCE[i].SOURCE.CONT[j].TEXT}\n" \
        "<BR>\n" \
        "!INCREMENT j\n" \
      "!END\n" \
      "<BR>\n" \
      "!INCREMENT i\n" \
    "!END\n" \
  "!ENDIF\n" \
  "!INCLUDE @.inc\n" \
  "</BODY>\n" \
  "</HTML>\n"
};

int individual_template_nosubdir_size =
  sizeof(individual_template_subdir)/sizeof(char *);

char *individual_template = NULL;

char *index_template =
  "<HTML>\n<HEAD>\n<TITLE>Index of Persons</TITLE>\n</HEAD>\n" \
  "<BODY>\n<H1>Index of Persons</H1>\n<P>\n" \
  "!WHILE @\n" \
    "!IF @.TITLE\n" \
      "<A HREF=\"${@.&}\">${@.NAME} (${@.TITLE})</A>\n" \
    "!ELSE\n" \
      "<A HREF=\"${@.&}\">${@.NAME}</A>\n" \
    "!ENDIF\n" \
    " (${@.BIRTH.DATE} - ${@.DEATH.DATE})<BR>\n" \
    "!NEXT\n" \
  "!END\n" \
  "</P>\n</BODY>\n</HTML>\n";

/*
 * Record types
 */
typedef enum {
  T_INTEGER, T_STRING, T_PLACE, T_NOTE, T_XREF, T_SOURCE,
  T_EVENT, T_INDIV, T_FAMILY, T_CONT, T_URL
} record_type;

/*
 * Interpreter state
 */
int skipping;

#define CONTROL_STACK_SIZE 100
char *while_stack[CONTROL_STACK_SIZE];
int while_stack_top;
int if_stack[CONTROL_STACK_SIZE];
int if_stack_top;

struct individual_record *root;

char *template;
char *template_start;
int current_index;
record_type current_type = T_INTEGER;
union value {
  int integer;
  char *string;
  struct place_structure *place;
  struct note_structure *note;
  struct xref *xref;
  struct event_structure *event;
  struct individual_record *indiv;
  struct family_record *family;
  struct continuation *cont;
  struct source_record *source;
  char *url;
} current_value;

char current_url[FILENAME_MAX+1];
int doing_index;

void interpret(FILE *ofile);
void variable(FILE *ofile);
void command(FILE *ofile);
void collect_identifier(char **ret);
void skip_white_space();
void output_error(char *msg);

void set_variable(char *name, int value);
int get_variable(char *name);

void family_select(char *field);
void indiv_select(char *field);
void event_select(char *field);
void note_select(char *field);
void source_select(char *field);
void cont_select(char *field);
void place_select(char *field);
void xref_select(char *field);

void construct_url(char *dest, struct individual_record *indiv);

void output_individual(struct individual_record *rt) {
  FILE *ofile;
  char path[FILENAME_MAX+1];
  char url[FILENAME_MAX+1];

  if(max_per_directory) {
    sprintf(path, "D%07d", rt->serial / max_per_directory);
    mkdir(path, 0777);
    strcat(path, "/");
  } else {
    sprintf(path, "%s", "");
  }
  sprintf(url, file_template, rt->xref);
  strcat(path, url);
  if((ofile = fopen(path, "w")) == NULL) {
    fprintf(stderr, "Failed to create individual file %s\n", path);
    return;
  }
#ifdef MSDOS
  fprintf(stderr, "Created %s\n", path);
#endif
  template = template_start = individual_template;
  root = rt;
  interpret(ofile);
  fclose(ofile);
}

void output_index(struct individual_record *rt) {
  FILE *ofile;
  char path[FILENAME_MAX+1];

  sprintf(path, file_template, "INDEX");
  if((ofile = fopen(path, "w")) == NULL) {
    fprintf(stderr, "Failed to create index file %s\n", path);
    return;
  }
  template = template_start = index_template;
  root = rt;
  doing_index = 1;
  interpret(ofile);
  doing_index = 0;
  fclose(ofile);
}

/*
 * Interpret the template stored in "template", outputting results
 * to "ofile".  The individual "root" is the starting point for
 * the interpretation of variables.
 */
void interpret(FILE *ofile) {
  char c;
  int start_of_line = 1;

  while((c = *template++) != '\0') {
    switch(c) {
    case '\n':
      start_of_line = 1;
      /* fall through intentional */
    case ' ':
    case '\t':
      if(!skipping) fputc(c, ofile);
      continue;
    case '!':
      if(!start_of_line) {
        if(!skipping) fputc(c, ofile);
        continue;
      }
      template--;
      command(ofile);
      continue;
    case '$':
      start_of_line = 0;
      variable(ofile);
      if(!skipping) {
        if(current_type == T_STRING) {
          fprintf(ofile, "%s", current_value.string);
        } else if(current_type == T_URL) {
          fprintf(ofile, "%s", current_value.url);
        } else if(current_type == T_INTEGER) {
          /* Integer variables start from 1 */
          fprintf(ofile, "%d", current_value.integer + 1);
        } else {
          output_error("Attempt to output something not an integer or string");
        }
      }
      continue;
    default:
      start_of_line = 0;
      if(!skipping) fputc(c, ofile);
      continue;
    }
  }
}

/*
 * After having seen the initial $, interpret a simple or compound variable. 
 */
void variable(FILE *ofile) {
  char c, *selector;
  int braces = 0;
  int first = 1;

  /*
   * $$ means output a single $
   */
  if(*template == '$') {
    if(!skipping)
      fputc(*template++, ofile);
    return;
  }
  while((c = *template) != '\0') {
    switch(c) {
      /*
       * An '@' indicates the current individual
       */
    case '@':
      first = 0;
      template++;
      if(!skipping) {
	      current_value.indiv = root;
	      current_type = T_INDIV;
      }
      if(*template == '.') {
	      template++;
	      continue;
      } else if(*template == '[') 
	      continue;
      else
        return;

      /*
       * Braces in variables simply serve as delimiters
       */
    case '{':
      template++;
      braces++;
      continue;
    case '}':
      template++;
      if(braces) {
      	braces--;
	      if(braces)
	        continue;
	      else
	        return;
      } else {
	      if(!skipping) fputc(c, ofile);
	      return;
      }

      /*
       * Brackets indicate integer subscripts
       */
    case '[':
      first = 0;
      template++;

      /*
       * Integer constant
       */
      if(*template >= '0' && *template <= '9') {
      /*
      * Convert integer value and leave on top of stack
      * (if not skipping)
      */
      }
      /*
       * Variable subscript
       */
      else {
	      record_type previous_type;
	      union value previous_value;
	
        previous_type = current_type;
        previous_value = current_value;
        variable(ofile);
	      if(!skipping && current_type != T_INTEGER)
	        output_error("Subscript is not an integer variable");
	      else {
          current_index = current_value.integer;
          current_type = previous_type;
          current_value = previous_value;
	      }
        if(*template != ']') {
          output_error("Subscript fails to end with ']'");
        } else {
          template++;
        }
      }
      if(!skipping) {
        switch(current_type) {
          case T_INTEGER:
          case T_STRING:
          case T_URL:
            output_error("Can't apply subscript to an integer, string, or URL");
            break;
          case T_PLACE:
            while(current_index--) place_select("NEXT");
            break;
          case T_NOTE:
            while(current_index--) note_select("NEXT");
            break;
          case T_SOURCE:
            while(current_index--) source_select("NEXT");
            break;
          case T_CONT:
            while(current_index--) cont_select("NEXT");
            break;
          case T_EVENT:
            while(current_index--) event_select("NEXT");
            break;
          case T_INDIV:
            while(current_index--) indiv_select("NEXT");
            break;
          case T_FAMILY:
            while(current_index--) family_select("NEXT");
            break;
          case T_XREF:
            while(current_index--) xref_select("NEXT");
            break;
	      }
      }
      if(*template == '.') {
        template++;
        continue;
      } else if(*template == '[' || *template == '}') 
	      continue;
      else
	      return;
      /*
       * An ampersand means turn the current individual into a URL
       */
    case '&':
      template++;
      if(!skipping) {
        if(current_type == T_INDIV) {
          current_type = T_URL;
          construct_url(current_url, current_value.indiv);
          current_value.url = current_url;
        } else
          output_error("Can only make a URL from an individual\n");
      }
      if(*template == '}')
	      continue;
      else
	      return;
      /*
       * Alphabetic characters indicate selector name.
       * Anything else is a delimiter.
       */
    default:
      collect_identifier(&selector);
      if(*selector == '\0') return;
      if(first) {
        if(!skipping) {
          current_value.integer = get_variable(selector);
          current_type = T_INTEGER;
        }
        if(*template == '}')
          continue;
        else
          return;
      }
      if(!skipping) {
        switch(current_type) {
        case T_INTEGER:
        case T_STRING:
        case T_URL:
          output_error("Can't apply selector to an integer, string, or URL");
          break;
        case T_PLACE:
          place_select(selector);
          break;
        case T_NOTE:
          note_select(selector);
          break;
        case T_SOURCE:
          source_select(selector);
          break;
        case T_CONT:
          cont_select(selector);
          break;
        case T_EVENT:
          event_select(selector);
          break;
        case T_INDIV:
          indiv_select(selector);
          break;
        case T_FAMILY:
          family_select(selector);
          break;
        case T_XREF:
          xref_select(selector);
          break;
        }
      }
      if(*template == '.') {
        template++;
        continue;
      } else if(*template == '[' || *template == '}') 
	      continue;
      else
	      return;
    }
  }
}

/*
 * Record field selection operations
 */
void family_select(char *field) {
  struct family_record *r = current_value.family;
  if(!strcmp(field, "XREF")) {
    current_type = T_STRING;
    current_value.string = (r && r->xref) ? r->xref: "";
  } else if(!strcmp(field, "REFN")) {
    current_type = T_STRING;
    current_value.string = (r && r->refn) ? r->refn: "";
  } else if(!strcmp(field, "HUSBAND")) {
    current_type = T_INDIV;
    current_value.indiv =
      (r && r->husband) ? r->husband->pointer.individual : NULL;
  } else if(!strcmp(field, "WIFE")) {
    current_type = T_INDIV;
    current_value.indiv =
      (r && r->wife) ? r->wife->pointer.individual: NULL;
  } else if(!strcmp(field, "CHILDREN")) {
    current_type = T_XREF;
    current_value.xref =
      (r && r->children) ? r->children: NULL;
  } else if(!strcmp(field, "NOTE")) {
    current_type = T_NOTE;
    current_value.note = r ? r->notes: NULL;
  } else if(!strcmp(field, "EVENT")) {
    current_type = T_EVENT;
    current_value.event = r ? r->events: NULL;
  } else if(!strcmp(field, "NEXT")) {
    current_value.family = r ? r->next: NULL;
  } else {
    output_error("Unrecognized selector applied to family record");
  }
}

void indiv_select(char *field) {
  struct individual_record *r = current_value.indiv;
  if(!strcmp(field, "XREF")) {
    current_type = T_STRING;
    current_value.string = (r && r->xref) ? r->xref: "";
  } else if(!strcmp(field, "NAME")) {
    current_type = T_STRING;
    current_value.string = (r && r->personal_name) ?
      r->personal_name->name: "???";
  } else if(!strcmp(field, "TITLE")) {
    current_type = T_STRING;
    current_value.string = (r && r->title) ? r->title: "";
  } else if(!strcmp(field, "ISMALE")) {
    current_type = T_INTEGER;
    current_value.integer = (r && r->sex == 'M');
  } else if(!strcmp(field, "ISFEMALE")) {
    current_type = T_INTEGER;
    current_value.integer = (r && r->sex == 'F');
  } else if(!strcmp(field, "REFN")) {
    current_type = T_STRING;
    current_value.string = (r && r->refn) ? r->refn: "";
  } else if(!strcmp(field, "RFN")) {
    current_type = T_STRING;
    current_value.string = (r && r->rfn) ? r->rfn: "";
  } else if(!strcmp(field, "AFN")) {
    current_type = T_STRING;
    current_value.string = (r && r->afn) ? r->afn: "";
  } else if(!strcmp(field, "FAMC")) {
    current_type = T_XREF;
    current_value.xref = r ? r->famc: NULL;
  } else if(!strcmp(field, "FAMS")) {
    current_type = T_XREF;
    current_value.xref = r ? r->fams: NULL;
  } else if(!strcmp(field, "FATHER")) {
    current_value.indiv =
      (r && r->famc && r->famc->pointer.family
       && r->famc->pointer.family->husband)
	? r->famc->pointer.family->husband->pointer.individual: NULL;
  } else if(!strcmp(field, "MOTHER")) {
    current_value.indiv =
      (r && r->famc && r->famc->pointer.family
       && r->famc->pointer.family->wife)
	? r->famc->pointer.family->wife->pointer.individual: NULL;
  } else if(!strcmp(field, "NOTE")) {
    current_type = T_NOTE;
    current_value.note = r ? r->notes: NULL;
  } else if(!strcmp(field, "SOURCE")) {
    current_type = T_XREF;
    current_value.xref =
      (r && r->sources) ? r->sources: NULL;
  } else if(!strcmp(field, "EVENT")) {
    current_type = T_EVENT;
    current_value.event = r ? r->events: NULL;
  } else if(!strcmp(field, "BIRTH")) {
    struct event_structure *ep;
    current_type = T_EVENT;
    current_value.event = NULL;
    for(ep = r->events; ep != NULL; ep = ep->next) {
      if(ep->tag->value == BIRT)
	current_value.event = ep;
    }
  } else if(!strcmp(field, "DEATH")) {
    struct event_structure *ep;
    current_type = T_EVENT;
    current_value.event = NULL;
    for(ep = r->events; ep != NULL; ep = ep->next) {
      if(ep->tag->value == DEAT)
	current_value.event = ep;
    }
  } else if(!strcmp(field, "NEXT")) {
    current_value.indiv = r ? r->next: NULL;
  } else {
    output_error("Unrecognized selector applied to individual record");
  }
}

void event_select(char *field) {
  struct event_structure *r = current_value.event;
  if(!strcmp(field, "TAG")) {
    current_type = T_STRING;
    current_value.string = (r && r->tag) ? r->tag->pname[default_language]: "";
  } else if(!strcmp(field, "DATE")) {
    current_type = T_STRING;
    current_value.string = (r && r->date) ? r->date: "";
  } else if(!strcmp(field, "PLACE")) {
    current_type = T_PLACE;
    current_value.place = r ? r->place: NULL;
  } else if(!strcmp(field, "NEXT")) {
    current_value.event = r ? r->next: NULL;
  } else {
    output_error("Unrecognized selector applied to event structure");
  }
}

void note_select(char *field) {
  struct note_structure *r = current_value.note;

  if(!strcmp(field, "XREF")) {
    current_type = T_STRING;
    current_value.string = (r && r->xref) ? r->xref: "";
  } else if(!strcmp(field, "TEXT")) {
    current_type = T_STRING;
    current_value.string = (r && r->text) ? r->text: "";
  } else if(!strcmp(field, "NEXT")) {
    current_value.note = r ? r->next : NULL;
  } else if(!strcmp(field, "CONT")) {
    current_type = T_CONT;
    current_value.cont = r ? r->cont: NULL;
  } else {
    output_error("Unrecognized selector applied to note structure");
  }
}

void source_select(char *field) {
  struct source_record *r = current_value.source;

  if(!strcmp(field, "XREF")) {
    current_type = T_STRING;
    current_value.string = (r && r->xref) ? r->xref: "";
  } else if(!strcmp(field, "TEXT")) {
    current_type = T_STRING;
    current_value.string = (r && r->text) ? r->text: "";
  } else if(!strcmp(field, "CONT")) {
    current_type = T_CONT;
    current_value.cont = r ? r->cont: NULL;
  } else {
    output_error("Unrecognized selector applied to source record");
  }
}

void cont_select(char *field) {
  struct continuation *c = current_value.cont;

  if(!strcmp(field, "TEXT")) {
    current_type = T_STRING;
    current_value.string = (c && c->text) ? c->text: "";
  } else if(!strcmp(field, "NEXT")) {
    current_value.cont = c ? c->next: NULL;
  } else {
    output_error("Unrecognized selector applied to continuation structure");
  }
}

void place_select(char *field) {
  struct place_structure *r = current_value.place;
  if(!strcmp(field, "NAME")) {
    current_type = T_STRING;
    current_value.string = (r && r->name) ? r->name: "";
  } else if(!strcmp(field, "NOTE")) {
    current_type = T_NOTE;
    current_value.note = r ? r->notes: NULL;
  } else {
    output_error("Unrecognized selector applied to place structure");
  }
}

void xref_select(char *field) {
  struct xref *r = current_value.xref;
  if(!strcmp(field, "INDIV")) {
    current_type = T_INDIV;
    current_value.indiv = r ? r->pointer.individual: NULL;
  } else if(!strcmp(field, "FAMILY")) {
    current_type = T_FAMILY;
    current_value.family = r ? r->pointer.family: NULL;
  } else if(!strcmp(field, "SOURCE")) {
    current_type = T_SOURCE;
    current_value.source = r ? r->pointer.source: NULL;
  } else if(!strcmp(field, "NEXT")) {
    current_value.xref = r ? r->next: NULL;
  } else {
    output_error("Unrecognized selector applied to cross-reference");
  }
}

/*
 * Perform a control command.
 */
void command(FILE *ofile) {
  char *buf;
  char *start = template++; 

  collect_identifier(&buf);
  skip_white_space();
  if(!strcmp(buf, "RESET")) {
    collect_identifier(&buf);
    if(*template == '\n')
      template++;
    else {
      output_error("Newline expected");
    }
    set_variable(buf, 0);
  } else if(!strcmp(buf, "INCREMENT")) {
    collect_identifier(&buf);
    if(*template == '\n')
      template++;
    else {
      output_error("Newline expected");
    }
    set_variable(buf, get_variable(buf)+1);
  } else if(!strcmp(buf, "IF")) {
    variable(ofile);
    if(*template == '\n') {
      template++;
      if(current_type == T_STRING) {
	      if(!strcmp(current_value.string, "")) {
          skipping++;
          if_stack[if_stack_top++] = 1;
        } else {
          if_stack[if_stack_top++] = 0;
        }
      } else {
        if(!current_value.integer) {
          skipping++;
          if_stack[if_stack_top++] = 1;
        } else {
          if_stack[if_stack_top++] = 0;
        }
      }
    } else {
      output_error("Newline expected");
    }
  } else if(!strcmp(buf, "ELSE")) {
    if(*template == '\n') {
      template++;
      if(if_stack[if_stack_top-1]) {
        skipping--;
        if_stack[if_stack_top-1] = 0;
      } else {
        skipping++;
        if_stack[if_stack_top-1] = 1;
      }
    } else {
      output_error("Newline expected");
    }
  } else if(!strcmp(buf, "ENDIF")) {
    if(*template == '\n') {
      template++;
      if(if_stack[--if_stack_top]) skipping--;
    } else {
      output_error("Newline expected");
    }
  } else if(!strcmp(buf, "WHILE")) {
    variable(ofile);
    if(*template == '\n') {
      template++;
      while_stack[while_stack_top++] = start;
      if(!skipping) {
        if(current_type == T_STRING) {
          if(!strcmp(current_value.string, ""))
            skipping++;
        } else {
          if(!current_value.integer)
            skipping++;
        }
      } else {
	      skipping++;
      }
    } else {
      output_error("Newline expected");
    }
  } else if(!strcmp(buf, "END")) {
    if(*template == '\n') {
      template++;
      if(skipping) {
	      skipping--;
        if(while_stack_top) while_stack_top--;
      } else {
        if(while_stack_top)
	        template = while_stack[--while_stack_top];
      }
    } else {
      output_error("Newline expected");
    }
  } else if(!strcmp(buf, "NEXT")) {
    if(*template == '\n') {
      template++;
      if(!skipping && root)
	      root = root->next;
    }
  } else if(!strcmp(buf, "INCLUDE")) {
    char path[FILENAME_MAX+1], *pp;
    FILE *incf;

    skip_white_space();
    for(pp = path; pp - path <= FILENAME_MAX
	      && *template && *template != '\n'; ) {
      if(*template == '@') {
      	template++;
        if(*template == '@') {
          template++;
          *pp++ = '@';
        } else {
          char *id = root->xref;
          while(*id && pp - path <= FILENAME_MAX)
            *pp++ = *id++;
        }
      } else {
	      *pp++ = *template++;
      }
    }
    *pp = '\0';
    if((incf = fopen(path, "r")) != NULL) {
      int c;
      while((c = fgetc(incf)) != EOF) fputc(c, ofile);
      fclose(incf);
    }
  } else {
    output_error("Unrecognized control command");
  }
}

/*
 * Pick up the next identifier in the template.
 */
#define IDENTIFIER_MAX 63

void collect_identifier(char **ret) {
  static char id[IDENTIFIER_MAX+1];
  char *ip, c;

  ip = id;
  while(((c = *template) >= 'A' && c <= 'Z')
	      || (c >= 'a' && c <= 'z')) {
    template++;
    if(ip - id < IDENTIFIER_MAX) *ip++ = c;
  }
  *ip = '\0';
  *ret = id;
}

void skip_white_space() {
  while(*template == ' ' || *template == '\t') template++;
}

struct binding {
  char *name;
  int value;
  struct binding *next;
} *environment;

void set_variable(char *name, int value) {
  struct binding *b;
  for(b = environment; b != NULL; b = b->next) {
    if(!strcmp(name, b->name)) {
      b->value = value;
      return;
    }
  }
  if((b = malloc(sizeof(struct binding))) == NULL) out_of_memory();
  b->name = strdup(name);
  b->value = value;
  b->next = environment;
  environment = b;
}

int get_variable(char *name) {
  struct binding *b;
  for(b = environment; b != NULL; b = b->next) {
    if(!strcmp(name, b->name)) return(b->value);
  }
  set_variable(name, 0);
  return(0);
}

void output_error(char *msg) {
  char *tp;
  int line = 1;

  for(tp = template_start; tp < template; tp++)
    if(*tp == '\n') line++;
  fprintf(stderr, "Output error: ");
  fprintf(stderr, "%s template line %d: %s\n",
	  template_start == individual_template ? "individual" : "index",
	  line, msg);
}

void construct_url(char *dest, struct individual_record *indiv) {
  char url[FILENAME_MAX+1];

  if(max_per_directory) {
    if(!doing_index)
      sprintf(dest, "../");
      sprintf(url, "D%07d/", indiv->serial / max_per_directory);
  } else {
    sprintf(url, "%s", "");
  }
  strcat(dest, url);
  sprintf(url, url_template, indiv->xref);
  strcat(dest, url);
}
