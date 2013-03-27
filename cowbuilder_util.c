#include <mntent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cowbuilder_util.h"

/**
 * @return 0 if not mounted, 1 if something under the mountpoint is mounted.
 */
int check_mountpoint(const char* mountpoint) {
  /* Check if a subdirectory under buildplace is still mounted, which may happen when there's a stale bind mount or whatnot. */
  FILE *mtab = NULL;
  struct mntent * part = NULL;
  if ((mtab = setmntent("/etc/mtab", "r")) != NULL)
    {
      while ((part = getmntent(mtab) ) != NULL)
        {
          if ( part->mnt_fsname != NULL )
            {
              if (strstr(part->mnt_dir, mountpoint) == part->mnt_dir)
                {
                  printf("E: Something (%s) is still mounted under %s; unmount and remove %s manually\n",
			 part->mnt_dir, mountpoint, mountpoint);
                  endmntent(mtab);
                  return 1;
                }
            }
        }
      endmntent(mtab);
    }
  return 0;
}

/**
 * Remove double slash "//" from pc->buildplace and use "/"
 * dest needs to be large enough to hold buildplace string.
 */
void canonicalize_doubleslash(const char* buildplace,
			      char* dest) {
  char prev=0;
  int i, j;
  for (i = 0, j = 0; buildplace[i]; ++i)
    {
      if (prev == '/' && buildplace[i] == '/')
	{
	  continue;
	}
      prev = dest[j++] = buildplace[i];
    }
  dest[j] = '\0';
}

