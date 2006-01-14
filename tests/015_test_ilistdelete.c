/*BINFMTC:
 *
 * Check deleting .ilist file is handled gracefully.
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

int main(int argc, char** argv)
{
  /* remove ilist file */
  int fd;
  
  unlink(".ilist");
  
  if (-1==(fd=open("a", O_WRONLY)))
    {
      perror("open");
      return 1;
    }
  
  if (5 !=write(fd, "test\n", 5))
    {
      perror("write");
      return 1;
    }
  if (-1==close(fd))
    {
      perror("close");
      return 1;
    }

  return 0;
}
