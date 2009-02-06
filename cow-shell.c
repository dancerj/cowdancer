/*BINFMTC: -DLIBDIR='"/usr/lib"' ilistcreate.c
  Copy-on-write filesystem invocation.

  GPL v2 or later
  Copyright 2005-2009 Junichi Uekawa.
 */
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <sys/wait.h>
#include "ilist.h"

const char* ilist_PRGNAME="cow-shell";

/* 
 * remove ilist after use.
 */
void ilist_deleter(const char* ilistfile)
{
  if(fork()==0)
    {
      /* I am the child process */

      pid_t parent_pid=getppid();
      if (daemon(0,1) < 0)
	{
	  perror("cow-shell daemon");
	  exit (-1);
	}
      while (kill(parent_pid,0) >= 0)
	{
	  sleep(1);
	}
      if (unlink(ilistfile)==-1)
	{
	  perror("cow-shell unlink .ilist");
	  exit(1);
	}
      exit(0);
    }
}


int main(int ac, char** av)
{
  /* give me a command-line to exec, 
     and I will cow-keep what's under this directory. */
  const char* ilistpath="./.ilist";
  char*buf;
  struct stat st;
  int cowdancer_reuse;
  
  asprintf(&buf, "%s%s%s",
	   getenv("LD_PRELOAD")?:"",
	   getenv("LD_PRELOAD")?" ":"",
	   LIBDIR "/cowdancer/libcowdancer.so"
	   );

  cowdancer_reuse=getenv("COWDANCER_REUSE") && 
    !strcmp(getenv("COWDANCER_REUSE"),"yes") ;
  
  if (cowdancer_reuse && !stat(ilistpath, &st))
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
      if(ilistcreate(ilistpath, NULL))
	{
	  ilist_outofmemory(".ilist creation failed");
	  return 1;
	}
    }

  ilistpath=canonicalize_file_name(ilistpath);
    
  setenv("COWDANCER_ILISTFILE",
	  ilistpath,1);
  setenv("LD_PRELOAD",buf,1);
  unsetenv("COWDANCER_IGNORE");

  if (!cowdancer_reuse)
    {
      /* if reuse flag is not on, remove the file */
      ilist_deleter(ilistpath);
    }
  
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
