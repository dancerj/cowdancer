/*BINFMTC: ilistcreate.c
 *
 * Debug program for dumping ilist file contents.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "ilist.h"

const char* ilist_PRGNAME="cowdancer-ilistdump";

int main(int argc, char** argv)
{
  struct ilist_struct s;
  struct ilist_header h;

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
  
  fread(&h, sizeof(struct ilist_header), 1, f);
  fprintf(stderr, 
	  "Signature: %x (expect %x)\n"
	  "Revision: %i (expect %i)\n"
	  "Struct size: %i\n\n",
	  h.ilistsig, ILISTSIG, 
	  h.revision, ILISTREVISION, 
	  h.ilist_struct_size
	  );
  
  while(fread(&s, sizeof(struct ilist_struct), 1, f))
    {
      printf ("%li %li\n", (long)s.dev, (long)s.inode);
    }

  fclose(f);
  
  return 0;
}
