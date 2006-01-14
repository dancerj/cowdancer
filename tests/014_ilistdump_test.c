/*BINFMTC:
 *
 * Debug program for dumping ilist file contents.
 * Run this with  < .ilist
 *
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
#include "../ilist.h"

int main(int argc, char** argv)
{
  struct ilist_struct s;
  fprintf (stderr, "ilist_struct size on this architecture: %i\n", (int)sizeof(struct ilist_struct));
  while(fread(&s, sizeof(struct ilist_struct), 1, stdin))
    {
      printf ("%li %li\n", (long)s.dev, (long)s.inode);
    }

  /* dummy call to quiet gcc down. */
  compare_ilist(&s,&s);
  return 0;
}
