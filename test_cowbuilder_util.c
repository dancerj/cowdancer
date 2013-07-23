/*BINFMTC: cowbuilder_util.c
 */
#include "cowbuilder_util.h"

#include <stdio.h>
#include <assert.h>
#include <string.h>


void test_check_mountpoint() {
  // something is usually mounted under /.
  if(0 == check_mountpoint("/")) {
    fprintf(stderr, 
	    "*******************************************************************************\n"
	    "* '/' is not mounted, something is wrong with the system or the code\n"
	    "*******************************************************************************\n");
  }
  // something is usually mounted under /run, but can't be sure.
  // assert(1 == check_mountpoint("/run")); // commented out so that I don't break automatic builds.
  // usually nothing is mounted under /tmp/nothing-is-mounted
  fprintf(stderr, "The following message is expected:\n");
  assert(0 == check_mountpoint("/tmp/nothing-is-mounted"));
}

/** 
 * make sure strings are equal, and return true when they are. Just to
 * save myself from strcmp being a macro.
 */
int verify_string_equal(const char* a, const char* b) {
  return !strcmp(a, b);
}

void test_canonicalize_doubleslash() {
  char dest[256];
  canonicalize_doubleslash("/no/double/slash", dest);
  assert(verify_string_equal(dest, "/no/double/slash"));
  canonicalize_doubleslash("/trailing/slash/", dest);
  assert(verify_string_equal(dest, "/trailing/slash/"));
  canonicalize_doubleslash("//starting/double/slash/", dest);
  assert(verify_string_equal(dest, "/starting/double/slash/"));
  canonicalize_doubleslash("/double//slash", dest);
  assert(verify_string_equal(dest, "/double/slash"));
  canonicalize_doubleslash("/more//double//slash", dest);
  assert(verify_string_equal(dest, "/more/double/slash"));
  canonicalize_doubleslash("no/starting/slash//", dest);
  assert(verify_string_equal(dest, "no/starting/slash/"));
  canonicalize_doubleslash("///", dest);
  assert(verify_string_equal(dest, "/"));

  const char* test_buffer_overrun = "/some/string";
  strcpy(dest,                      "-some-string+g");
  canonicalize_doubleslash(test_buffer_overrun, dest);
  assert(verify_string_equal(dest, "/some/string"));
  assert(strlen(dest) == 12);
  assert(dest[12] == 0);
  assert(dest[13] == 'g');
}

int main()
{
  test_check_mountpoint();
  test_canonicalize_doubleslash();

  return 0;
}
