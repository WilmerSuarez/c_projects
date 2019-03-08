#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <unistd.h>
#include "node.h"
#include "read.h"
#include "database.h"
#include "output.h"
#include "tags.h"

#define VERSION "2.1 (17 April 1995)"
#define USAGE "Usage: %s [-Hciv][-d <max-per-directory>][-s <individual> ...][-u <URL template>][-f <file-template>][-t <individual-template>][-T <index-template>] [--change-directory <dirname>] [-- <gedcom-file> ...]\n", argv[0]
#define OPTIONS " -v\t\t\t\tPrint version information.\n" \
" -c\t\t\t\tDisable automatic capitalization of surnames.\n" \
" -d max_per_directory\t\tSpecify number of individuals per subdirectory\n" \
"\t\t\t\t(0 means no subdirectories)\n" \
" -i\t\t\t\tCause an index file to be generated containing\n" \
"\t\t\t\tall the individuals in the input.\n" \
" -s individuals ...\t\tLimit the production of output files to a specified\n" \
"\t\t\t\tset of zero or more selected individuals.\n" \
" -u url_template\t\tSpecify a template string for the URL's used\n" \
"\t\t\t\tin anchors in the output files (default '%%s.html').\n" \
" -f file_template\t\tSpecify a template string for the names of the\n" \
"\t\t\t\tHTML files (default '%%s.html').\n" \
" -t individual_template\t\tSpecify an HTML template file for individuals.\n" \
" -T index_template\t\tSpecify an HTML template file for the index.\n" \
" -H\t\t\t\tPrint a brief message listing the available options.\n" \
" --change-directory dirname\tCreate HTML files at specified directory\n"

int generate_index;
int change_d;
char *output_path; // Name of existing directory to output HTML files
char **selected_individuals;
struct node head;

int main(int argc, char *argv[]) {
  struct node *np;
  int i, optc;
  extern char *optarg;
  extern int optind;
  int serial = 0;
#ifdef MSDOS
  int getopt(int argc, char *const *argv, const char *optstring);
  extern char *optarg;
  extern int optind;
#endif
  /* 
    Make sure the tag tables are in the correct 
    order. Will be used by bsearch() when reading
    the GEDCOM file(s). 
    (bsearch expects an assorted array)
  */
  validate_tags_tables();
#ifdef MSDOS
  if(argc <= 1) {
#ifdef MSWINDOWS
    fprintf(stderr, "Launch this program from the File Manager, using the\n");
    fprintf(stderr, "'Run' item under the 'File' menu to specify command-line\n");
    fprintf(stderr, "arguments.\n");
#endif
    fprintf(stderr, USAGE);
    exit(1);
  }
#endif

  /* Long Argument Options */
  const struct option long_options[] = {
    {"help", no_argument, NULL, 'H'},
    {"no-surname-caps", no_argument, NULL, 'c'},
    {"index", no_argument, NULL, 'i'},
    {"version", no_argument, NULL, 'v'},
    {"files-per-directory", required_argument, NULL, 'd'},
    {"select", required_argument, NULL, 's'},
    {"url-template", required_argument, NULL, 'u'},
    {"filename-template", required_argument, NULL, 'f'},
    {"individual-template", required_argument, NULL, 't'},
    {"index-template", required_argument, NULL, 'T'},
    {"change-directory", required_argument, NULL, 'g'},
    {0, 0, 0, 0}
  };

  /* Validate Arguments */
  while((optc = getopt_long(argc, argv, "Hcivd:s:u:h:f:t:T:", long_options, NULL)) != -1) {
    FILE *tempf;
    long size;
    char *temps, *tempe;
    int c;

    switch(optc) {
      case 'v':	/* Version */
        printf("GEDCOM to HTML translator, version %s by Gene Stark "
        "(stark@cs.sunysb.edu)\n", VERSION);
        exit(0);
      case 'c':   /* Disable automatic capitalization of surnames */
        capitalization = 0;
        break;
      case 'i':	/* Generate index */
        generate_index = 1;
        break;
      case 's':	/* Generate record(s) for selected individual(s) */
        {
          int i = 0;
          int j;
          while(argv[optind-1+i] && argv[optind-1+i][0] != '-')
            i++;
          if(!(selected_individuals = malloc((i+1) * sizeof(char *)))) out_of_memory();
          for (j = 0; j < i; j++)
            selected_individuals[j] = argv[optind-1+j];
          selected_individuals[j]=NULL;
          optind += i;
        }
        break;
      case 't':	/* Template file for individuals */
      case 'T':	/* Template file for the index */
        if((tempf = fopen(optarg, "r")) == NULL) {
          fprintf(stderr, "Can't open template file '%s'\n", optarg);
          break;
        }
        if(fseek(tempf, 0L, SEEK_END) == -1 || (size = ftell(tempf)) == -1){
          fprintf(stderr, "Can't determine length of template file '%s'\n",
            optarg);
          fclose(tempf);
          break;
        }
        rewind(tempf);
        if((temps = malloc((size_t) size+1)) == NULL) {
          fprintf(stderr, "Can't allocate memory for template string\n");
          fclose(tempf);
          break;
        }
        tempe = temps;
        while((c = fgetc(tempf)) != EOF && tempe-temps <= size) *tempe++ = c;
        *tempe = '\0';
        if(optc == 't')
          individual_template = temps;
        else
          index_template = temps;
        break;
      case 'd':	/* Specify max per directory */
        max_per_directory = strtol(optarg, NULL, 10);
        break;
      case 'u':	/* Template for URL's within HTML anchors */
        url_template = optarg;
        break;
      case 'f':	/* Template for file names */
        file_template = optarg;
        break;
      case 'g':
        change_d = 1; // Change directory flag set
        output_path = optarg; // HTML files output path
        break;
      case 'H':
        printf(USAGE);
        printf(OPTIONS);
        exit(0);
      case '?':
      default:
        fprintf(stderr, USAGE);
        exit(1);
    }
  }

  /* PHASE I - CREATE NODES */
  if(optind == argc) { 
    current_gedcom = "stdin";
    current_lineno = 0;
    read_gedcom(stdin, &head, 0);
  } else {
    for(np = &head; optind < argc; optind++) {
      FILE *gedcom_file;

      current_gedcom = argv[optind]; // GEDCOM file path
      current_lineno = 0;
      if((gedcom_file = fopen(argv[optind], "r")) == NULL) {
	      fprintf(stderr, "Can't open GEDCOM file '%s'.\n", argv[optind]);
	      continue;
      }
      read_gedcom(gedcom_file, np, 0);
      fclose(gedcom_file);
      while(np->siblings)
        np = np->siblings;
    } 
  } 

  /* 
    PHASE II:
      PROCESS NODES  
      ALLOCATE STRUCTURES OF PROPER TYPE 
  */
  if(head.siblings == NULL) {
    fprintf(stderr, "No valid GEDCOM lines found\n");
    exit(1);
  }
  process_records(head.siblings);

  /* PHASE III - FILL XREF STRUCTS */
  link_records(head.siblings);
  fprintf(stderr, "Processed %ld GEDCOM lines", gedcom_lines);
  if(total_individuals)
    fprintf(stderr, ", %d individuals", total_individuals);
  if(total_families)
    fprintf(stderr, ", %d families", total_families);
  if(total_events)
    fprintf(stderr, ", %d events", total_events);
  if(total_sources)
    fprintf(stderr, ", %d sources", total_sources);
  if(total_notes)
    fprintf(stderr, ", %d notes", total_notes);
  fprintf(stderr, "\n");

  /* 
    PHASE IV - OUTPUT HTML FILES
      - Determine individuals to be output, 
        and assign them serial numbers. 
  */

  /* Change the CWD */
  if(change_d) {
    if(chdir(output_path) == -1) {
      fprintf(stderr, "Bad directory path\n");
      exit(EXIT_FAILURE);
    }
  }

  for(i = 0; i < total_individuals; i++) {
    if(selected_individuals != NULL) {
      char **av;
      for(av = selected_individuals; *av != NULL; av++)
	    if(!strcmp(*av, all_individuals[i]->xref)) {
	      all_individuals[i]->serial = ++serial;
	    }
    } else {
      all_individuals[i]->serial = ++serial;
    }
  }
  /*
   * Generate index file
   */
  if (generate_index)
    output_index(*all_individuals);

  /*
   * Output individuals
   */
  if(individual_template == NULL) {
    int size = 0;
    if(max_per_directory) {
      for(i = 0; i < individual_template_subdir_size; i++)
	      size += strlen(individual_template_subdir[i]);
      if((individual_template = malloc(size+1)) == NULL) out_of_memory();
      *individual_template = '\0';
      for(i = 0; i < individual_template_subdir_size; i++) 
        strcat(individual_template, individual_template_subdir[i]);
      } else {
        for(i = 0; i < individual_template_nosubdir_size; i++)
	        size += strlen(individual_template_nosubdir[i]);
        if((individual_template = malloc(size+1)) == NULL) out_of_memory();
        *individual_template = '\0';
        for(i = 0; i < individual_template_nosubdir_size; i++)
	        strcat(individual_template, individual_template_nosubdir[i]);
      }
  }
  for(i = 0; i < total_individuals; i++) {
    if(all_individuals[i]->serial)
      output_individual(all_individuals[i]);
  }

  exit(0);
}
