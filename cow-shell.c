/*BINFMTC:
  Copy-on-write filesystem invocation.
 */
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int ac, char** av)
{
  /* give me a command-line to exec. */
  char*buf;
  asprintf(&buf, "%s%s%s",
	   getenv("LD_PRELOAD")?:"",
	   getenv("LD_PRELOAD")?" ":"",
	   "/usr/lib/libcowdancer/libcowdancer.so"
	   );
  /* assume current directory */
  system("find . -print0 -type f | xargs -0 stat --format '%d %i' > .ilist");
  setenv ("COWDANCER_ILISTFILE",
	  canonicalize_file_name("./.ilist"),1);
  setenv ("LD_PRELOAD",buf,1);
  if (ac>1)
    execvp(av[1], av+1);
  else
    execl(getenv("SHELL")?:"/bin/sh",
	  getenv("SHELL")?:"/bin/sh");
  return 1;
}
