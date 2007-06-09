/*BINFMTC:
  Copy-on-write filesystem invocation.

  GPL v2 or later
  Copyright 2005,2006,2007 Junichi Uekawa.
 */
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <string.h>
#include "ilist.h"
const char* PRGNAME="cow-shell";

int main(int ac, char** av)
{
  /* give me a command-line to exec, 
     and I will cow-keep what's under this directory. */
  const char* ilistpath="./.ilist";
  char*buf;
  struct stat st;

  asprintf(&buf, "%s%s%s",
	   getenv("LD_PRELOAD")?:"",
	   getenv("LD_PRELOAD")?" ":"",
	   LIBDIR "/cowdancer/libcowdancer.so"
	   );

  if (getenv("COWDANCER_REUSE") && 
      !strcmp(getenv("COWDANCER_REUSE"),"yes") && 
      !stat(ilistpath, &st))
    {
      /* if reuse flag is on and file already exists 
	 do nothing */
    }
  else
    {
      if (unlink(ilistpath)==-1)
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
      if(ilistcreate(ilistpath))
	{
	  outofmemory(".ilist creation failed");
	  return 1;
	}
    }
  
  setenv("COWDANCER_ILISTFILE",
	  canonicalize_file_name(ilistpath),1);
  setenv("LD_PRELOAD",buf,1);
  unsetenv("COWDANCER_IGNORE");

  if (ac>1)
    execvp(av[1], av+1);
  else
    {
      const char* myshell=getenv("SHELL")?:"/bin/sh";
      fprintf(stderr, "Invoking %s\n", myshell);
      
      execlp(myshell,
	     myshell,
	     NULL);
      perror("cow-shell: exec");

      fprintf(stderr, "Falling back to /bin/sh\n");
      execlp("/bin/sh",
	     "/bin/sh",
	     NULL);
    }
  perror("cow-shell: exec");
  return 1;
}
