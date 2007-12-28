/*BINFMTC: -g -DILISTCREATEBENCH
 * cowdancer -- a Copy-on-write data-access; No-cow-easy-replacement
 *
 * Copyright 2007 Junichi Uekawa
 * GPL v2 or later.
 */
#include <stdlib.h>
#include <stdio.h>
#include "ilist.h"

/* I will die when things get too wrong. */
void ilist_outofmemory(const char* msg)
{
  fprintf (stderr, "%s: %s\n", ilist_PRGNAME, msg);
}

/* return 1 on error, 0 on success */
int ilistcreate(const char* ilistpath, const char* findcommandline)
{
  int i=0;
  long dev, ino;
  FILE* inf;
  FILE* outf;
  struct ilist_struct* ilist=NULL;  
  long ilist_len=0;
  if(!findcommandline)
    findcommandline="find . -xdev \\( -type l -o -type f \\) -a -links +1 -print0 | xargs -0 stat --format '%d %i '";
  
  if (!(ilist=calloc(2000,sizeof(struct ilist_struct))))
    {
      ilist_outofmemory("memory allocation failed");
      return 1;
    }
  ilist_len=2000;
  if (NULL==(inf=popen(findcommandline, "r")))
    {
      ilist_outofmemory("popen find failed");
      return 1;
    }

  while (fscanf(inf,"%li %li", &dev, &ino)>0)
    {
      (ilist+i)->dev=(dev_t)dev;
      (ilist+i)->inode=(ino_t)ino;

      if (getenv("COWDANCER_DEBUG")) 
	printf("%li %li \n ", (long int)dev, (long int)ino);
      
      i++;
      if (i>=ilist_len)
	{
	  ilist=realloc(ilist, (ilist_len*=2)*sizeof(struct ilist_struct));
	  if (!ilist)
	    {
	      ilist_outofmemory("realloc failed");
	      pclose(inf);
	      return 1;
	    }
	}
    }
  ilist_len=i;
  if (pclose(inf))
    {
      ilist_outofmemory("pclose returned non-zero, probably the directory contains no hardlinked file, don't bother using cow-shell here.");
      return 1;
    }

  /* sort the ilist */
  qsort(ilist, ilist_len, sizeof(struct ilist_struct), compare_ilist);

  /* write out the ilist file */
  if (NULL==(outf=fopen(ilistpath,"w")))
    {
      ilist_outofmemory("cannot open .ilist file");
      return 1;
    }
  
  if (ilist_len != fwrite(ilist, sizeof(struct ilist_struct), ilist_len, outf))
    {
      ilist_outofmemory("failed writing to .ilist file");
      return 1;
    }
  
  if (fclose (outf))
    {
      ilist_outofmemory("error flushing to .ilist file");
      return 1;
    }
  return 0;
}


/* comparison function for qsort/bsearch of ilist
 */
int
compare_ilist (const void *a, const void *b)
{
  const struct ilist_struct * ilista = (const struct ilist_struct*) a;
  const struct ilist_struct * ilistb = (const struct ilist_struct*) b;
  int ret;
  
  ret = ilista->inode - ilistb->inode;
  if (!ret)
    {
      ret = ilista->dev - ilistb->dev;
    }
  return ret;
}

#ifdef ILISTCREATEBENCH
/* test code for performance tuning */
const char* ilist_PRGNAME="testbench";

int 
main()
{
  int i;
  
  if (-1==chdir("/home/dancer/shared/git/linux-2.6/"))
    exit (1);
  
  for(i=0; i<100; ++i)
    ilistcreate("/home/dancer/shared/git/linux-2.6/.ilist", NULL);

  exit (0);
}

#endif
