#include <stdio.h>
#include <criterion/criterion.h>

#define TESTALL_FILE "tests/rsrc/testall.ged"
#define ROYAL92_FILE "tests/rsrc/royal92.ged"
#define GARBAGE_FILE "tests/rsrc/garbage.ged"
#define REF_OUTPUT_DIR "tests/rsrc/testall"
#define VALGRIND_LOG_FILE "valgrind.out"

extern int errors, warnings;

Test(basic_suite, end_to_end_test1) {
    char cmd[500];
    char *htmldir = "end_to_end_test1_html";
    sprintf(cmd, "rm -fr output; mkdir -p %s; cd %s; ../bin/ged2html -i ../%s > ../end_to_end_test1.out 2>&1",
	    htmldir, htmldir, TESTALL_FILE);
    int err = system(cmd);
    cr_assert_eq(err, 0, "The program did not exit normally.\n");
    sprintf(cmd, "diff -r %s %s", htmldir, REF_OUTPUT_DIR);
    err = system(cmd);
    cr_assert_eq(err, 0, "The output was not what was expected.\n");
}

Test(basic_suite, end_to_end_test2) {
    char cmd[500];
    char *htmldir = "end_to_end_test2_html";
    sprintf(cmd, "rm -fr output; mkdir -p %s; cd %s; ../bin/ged2html -i ../%s > ../end_to_end_test2.out 2>&1",
	    htmldir, htmldir, ROYAL92_FILE);
    int err = system(cmd);
    cr_assert_eq(err, 0, "The program did not exit normally.\n");
}

Test(basic_suite, end_to_end_test3) {
    char cmd[500];
    char *htmldir = "end_to_end_test3_html";
    sprintf(cmd, "rm -fr output; mkdir -p %s; cd %s; ../bin/ged2html -i ../%s > ../end_to_end_test3.out 2>&1",
	    htmldir, htmldir, GARBAGE_FILE);
    int err = system(cmd);
    cr_assert_eq(WEXITSTATUS(err), 1, "The program did not exit with status 1 (was: %d).\n", WEXITSTATUS(err));
}

Test(basic_tests_suite, valgrind_test1){
    char cmd[500];
    char *htmldir = "valgrind_test1_html";
    char *valgrind_opts = "--leak-check=full --show-leak-kinds=definite,indirect,possible --errors-for-leak-kinds=definite,indirect,possible --error-exitcode=37";
    sprintf(cmd, "rm -fr output; mkdir -p %s; cd %s; valgrind %s ../bin/ged2html ../%s > ../%s 2>&1",
	    htmldir, htmldir, valgrind_opts, TESTALL_FILE, VALGRIND_LOG_FILE);
    int ret = system(cmd);
    cr_assert(ret == 0 || ret == 37, "The program did not exit normally.\n");
    cr_assert_neq(ret, 37, "Valgrind reported errors -- see %s", VALGRIND_LOG_FILE);
}

//############################################
// STUDENT UNIT TESTS SHOULD BE WRITTEN BELOW
// DO NOT DELETE THESE COMMENTS
//############################################
