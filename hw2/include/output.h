extern char *individual_template;
extern char *individual_template_subdir[];
extern int individual_template_subdir_size;
extern char *individual_template_nosubdir[];
extern int individual_template_nosubdir_size;
extern char *index_template;
extern char *url_template;
extern char *file_template;
extern int max_per_directory;

void output_individual(struct individual_record *ip);
void output_index(struct individual_record *ip);
