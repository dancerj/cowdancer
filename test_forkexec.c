/*BINFMTC: forkexec.c
 */
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <assert.h>
#include "parameter.h"

int main()
{
  char *const successvp[] = 
    {
      "/bin/true", 
      NULL
    };
  char *const failurevp[] = 
    {
      "/bin/false", 
      NULL
    };
  
  assert(forkexecvp(successvp) == 0);
  assert(forkexecvp(failurevp) == 1);

  assert(forkexeclp("/bin/true",
		    "/bin/true",
		    NULL) == 0);
  assert(forkexeclp("/bin/false",
		    "/bin/false",
		    NULL) == 1);
  
  return 0;
}


