/*BINFMTC: file.c forkexec.c
 */

#define _GNU_SOURCE
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "file.h"
#include "parameter.h"

void test_copy_file()
{
  assert(copy_file("/proc/mounts", "test_file.c.test~")==0);
  assert(forkexeclp("diff", "diff", "-u", "/proc/mounts", 
		    "test_file.c.test~", NULL)==0);
  assert(copy_file("/proc/mounts", "/dev/path/does/not/exist")==-1);
}

void test_create_sparse_file()
{
  char* temp=strdupa("/tmp/sparseXXXXXX");
  mkstemp(temp);
  assert(create_sparse_file(temp,18UL*1UL<<30UL)==0);
}

int main() 
{
  test_copy_file();
  test_create_sparse_file();
  return 0;
}
