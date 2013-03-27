/*BINFMTC: cowbuilder.c parameter.c forkexec.c ilistcreate.c cowbuilder_util.c
 */

#define _GNU_SOURCE
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "file.h"
#include "parameter.h"

// utility to check hardlink count of file.
static int check_hardlink_count(const char* filename)
{
  struct stat buf;
  stat(filename, &buf);
  return buf.st_nlink;
}

int break_cowlink(const char* s);

void test_break_cowlink()
{
  char *temp = strdupa("/tmp/cowtestXXXXXX");
  char *temp2 = strdupa("/tmp/cowtestXXXXXX");
  close(mkstemp(temp));
  close(mkstemp(temp2));
  assert(check_hardlink_count(temp2) == 1);
  assert(check_hardlink_count(temp) == 1);
  assert(-1!=unlink(temp2));
  assert(-1!=link(temp, temp2));
  assert(check_hardlink_count(temp2) == 2);
  assert(check_hardlink_count(temp) == 2);
  break_cowlink(temp2);
  assert(check_hardlink_count(temp2) == 1);
  assert(check_hardlink_count(temp) == 1);
}

int main()
{
  test_break_cowlink();
  return 0;
}
