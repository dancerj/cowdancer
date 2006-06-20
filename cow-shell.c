/*BINFMTC:
  Copy-on-write filesystem invocation.

  GPL v2 or later
  Copyright 2005,2006 Junichi Uekawa.
 */
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <string.h>
#define PRGNAME "cow-shell"
#include "ilist.h"
  
/* I will die when things get too wrong. */
static void outofmemory(const char* msg)
{
  fprintf (stderr, "%s: %s\n", PRGNAME, msg);
}

/* return 1 on error, 0 on success */
static int ilistcreate(const char* ilistpath)
{
  int i=0;
  long dev, ino;
  FILE* inf;
  FILE* outf;
  struct ilist_struct* ilist=NULL;
  long ilist_len=0;

  if (!(ilist=calloc(2000,sizeof(struct ilist_struct))))
    {
      outofmemory("memory allocation failed");
      return 1;
    }
  ilist_len=2000;
  if (NULL==(inf=popen("find . -xdev -type f -links +1 -print0 | xargs -0 stat --format '%d %i '","r")))
    {
      outofmemory("popen find failed");
      return 1;
    }

  while (fscanf(inf,"%li %li",&dev, &ino)>0)
    {
      (ilist+i)->dev=(dev_t)dev;
      (ilist+i)->inode=(ino_t)ino;

      i++;
      if (i>=ilist_len)
	{
	  ilist=realloc(ilist, (ilist_len*=2)*sizeof(struct ilist_struct));
	  if (!ilist)
	    {
	      outofmemory("realloc failed");
	      pclose(inf);
	      return 1;
	    }
	}
    }
  ilist_len=i;
  if (pclose(inf))
    {
      outofmemory("pclose returned non-zero, probably the directory contains no hardlinked file, don't bother using cow-shell here.");
      return 1;
    }

  /* sort the ilist */
  qsort(ilist, ilist_len, sizeof(struct ilist_struct), compare_ilist);

  /* write out the ilist file */
  if (NULL==(outf=fopen(ilistpath,"w")))
    {
      outofmemory("cannot open .ilist file");
      return 1;
    }
  
  if (ilist_len != fwrite(ilist, sizeof(struct ilist_struct), ilist_len, outf))
    {
      outofmemory("failed writing to .ilist file");
      return 1;
    }
  
  if (fclose (outf))
    {
      outofmemory("error flushing to .ilist file");
      return 1;
    }
  return 0;
}

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
	   "/usr/lib/cowdancer/libcowdancer.so"
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
