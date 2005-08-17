/*BINFMTC:
  Copy-on-write filesystem invocation.

  GPL v2 or later
  Copyright 2005 Junichi Uekawa.
 */
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

int main(int ac, char** av)
{
  /* give me a command-line to exec, 
     and I will cow-keep what's under this directory. */
  char*buf;
  asprintf(&buf, "%s%s%s",
	   getenv("LD_PRELOAD")?:"",
	   getenv("LD_PRELOAD")?" ":"",
	   "/usr/lib/cowdancer/libcowdancer.so"
	   );

  if (unlink(".ilist")==-1)
    {
      if (errno == ENOENT)
	{
	  /* expected */
	}
      else
	{
	  perror("cow-shell: unlink of .ilist failed");
	  return 1;
	}
    }
      
  system("find . -xdev -print0 -type f | xargs -0 stat --format '%d %i' > .ilist");
  setenv("COWDANCER_ILISTFILE",
	  canonicalize_file_name("./.ilist"),1);
  setenv("LD_PRELOAD",buf,1);
  unsetenv("COWDANCER_IGNORE");

  if (ac>1)
    execvp(av[1], av+1);
  else
    {
      const char* myshell=getenv("SHELL")?:"/bin/sh";
      fprintf(stderr, "Invoking %s\n", myshell);
      
      execl(myshell,
	    myshell,
	    NULL);
    }
  perror("cow-shell: exec");
  return 1;
}
