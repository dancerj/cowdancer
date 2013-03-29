/*BINFMTC: -g
 *
 * check memory leak
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
#include <mcheck.h>

void dump_procmap()
{
  FILE*f;
  int c;

  f=fopen("/proc/self/maps", "r");
  while (EOF!=(c=fgetc(f)))
    fputc(c,stdout);
  fclose(f);
}

int main(int argc, char** argv)
{
  int fd;
  int i;
  FILE* f;

  /* Initialize */
  if (-1==(fd=open("1/a", O_WRONLY)))
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

  /* try open/close 100 times. */
  mtrace();

  for (i=0; i<100; ++i)
    {
      if (-1==(fd=open("1/b", O_WRONLY)))
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

      if (!(f=fopen("1/c", "w")))
	{
	  return 1;
	}

      if (5 !=fwrite("test\n", 1, 5, f))
	{
	  return 1;
	}
      if (EOF==fclose(f))
	{
	  return 1;
	}
    }

  muntrace();

  dump_procmap();

  return 0;
}
