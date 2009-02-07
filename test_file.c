/*BINFMTC: file.c forkexec.c
 */

#include <stdlib.h>
#include <assert.h>
#include "file.h"
#include "parameter.h"

int main() 
{
  assert(copy_file("/proc/mounts", "test_file.c.test~")==0);
  assert(forkexeclp("diff", "diff", "-u", "/proc/mounts", 
		    "test_file.c.test~", NULL)==0);
  assert(copy_file("/proc/mounts", "/dev/path/does/not/exist")==-1);
  
  return 0;
}
