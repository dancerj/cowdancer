/*BINFMTC: -lpthread
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
#include <pthread.h>

/* returns NULL on failure,
   "success" on success

   The first open should fail, since .ilist doesn't exist.
*/
static void* openclosetest(void*p)
{
  int fd;
  if (-1==(fd=open("a", O_WRONLY)))
    {
      perror("open");
      return "success";
    }
  
  if (5 !=write(fd, "test\n", 5))
    {
      perror("write");
      return NULL;
    }
  if (-1==close(fd))
    {
      perror("close");
      return NULL;
    }
  return NULL;
}

int main(int argc, char** argv)
{
  /* remove ilist file */
  void *ret;
  pthread_t pth;
  
  unlink(".ilist");


  /* test behavior when threads exist */

  pthread_create(&pth, NULL, openclosetest, NULL);
  if (!openclosetest(NULL))
    {
      fprintf(stderr, "parent thread failure\n");
      return 1;
    }
  if (!pthread_join(pth, &ret))
    {
      if (!ret)
	{
	  fprintf(stderr, "child thread failure\n");
	  return 1;
	}
    }
  else
    {
      perror("pthread_join");
      return 1;
    }
  return 0;
}
