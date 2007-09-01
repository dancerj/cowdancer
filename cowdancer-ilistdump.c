/*BINFMTC: ilistcreate.c
 *
 * Debug program for dumping ilist file contents.
 */

#include <stdio.h>
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dlfcn.h>
#include <dirent.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/signal.h>
#include <sys/mman.h>
#include "ilist.h"
const char* ilist_PRGNAME="cowdancer-ilistdump";

int main(int argc, char** argv)
{
  struct ilist_struct s;
  FILE*f;
  
  if (argc != 2)
    {
      fprintf (stderr,
	       "%s ilist-path \n\n\tdump contents of .ilist file\n\n",
	       argv[0]);
      return 1;
    }
  
  fprintf (stderr, "ilist_struct size on this architecture: %i\n", (int)sizeof(struct ilist_struct));

  f=fopen (argv[1], "r");
  if (!f)
    {
      fprintf (stderr,
	       "%s: cannot open file %s\n",
	       argv[0], argv[1]);
      return 1;
    }
  
  while(fread(&s, sizeof(struct ilist_struct), 1, f))
    {
      printf ("%li %li\n", (long)s.dev, (long)s.inode);
    }

  fclose(f);
  
  return 0;
}
