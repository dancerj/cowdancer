/* cowdancer test */
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
#define PRGNAME "cowdancer"

/* libc functions. */
static int (*origlibc_open)(const char *, int, ...) = NULL;
static int (*origlibc_open64)(const char *, int, ...) = NULL;
static int (*origlibc_creat)(const char *, mode_t) = NULL;
static int (*origlibc_creat64)(const char *, mode_t) = NULL;

struct ilist_struct
{
  dev_t dev;
  ino_t inode;
};

struct ilist_struct* ilist=NULL;
long ilist_len=0;

/* I will die when things get too wrong. */
static void outofmemory(const char* msg)
{
  fprintf (stderr, "%s: %s\n", PRGNAME, msg);
  kill(getpid(), SIGSEGV);
}

static void load_ilist(void)
{
  int i=0;
  FILE* f=0;
  int fd=0;
  long dev, ino;

  ilist=malloc(2000*sizeof(struct ilist_struct));
  ilist_len=2000;

  fd=origlibc_open(getenv("COWDANCER_ILISTFILE"),O_RDONLY,0);
  f=fdopen(fd, "r");
  
  while (fscanf(f,"%li %li",&dev, &ino)>0)
    {
      (ilist+i)->dev=(dev_t)dev;
      (ilist+i)->inode=(ino_t)ino;

      i++;
      if (i>ilist_len)
	{
	  ilist=realloc(ilist, (ilist_len*=2)*sizeof(struct ilist_struct));
	  if (!ilist)
	    {
	      /* FIXME: erm.... out of memory? How do I signal error? */
	      outofmemory("ilist_load");
	    }
	}
    }
  ilist_len=i;
  fclose(f);
}


static void debug_cowdancer (const char * s)
{
  if (0) fprintf (stderr, PRGNAME ": DEBUG %s\n", s);
}

static void debug_cowdancer_2 (const char * s, const char*e)
{
  if (0) fprintf (stderr, PRGNAME ": DEBUG %s:%s\n", s, e);
}

static void initialize_functions ()
{
  static int initialized = 0;
  /* do I need to make this code reentrant? */
  
  if (!initialized)
    {
      origlibc_open = dlsym(RTLD_NEXT, "open");
      origlibc_open64 = dlsym(RTLD_NEXT, "open64");
      origlibc_creat = dlsym(RTLD_NEXT, "creat");
      origlibc_creat64 = dlsym(RTLD_NEXT, "creat64");

      /* load the ilist */
      load_ilist();

      initialized = 1;
      debug_cowdancer ("Initialization successfully finished.\n");
    }  
}

/* check if i-node is to be protected, and if so, copy the file*/
static void check_inode_and_copy(const char* s)
{
  struct stat buf;
  struct ilist_struct search_target;

  debug_cowdancer_2("DEBUG: test for", s);
  stat(s, &buf);
  search_target.inode = buf.st_ino;
  search_target.dev = buf.st_dev;

  if(memmem(ilist, ilist_len*(sizeof(struct ilist_struct)), 
	    &search_target, sizeof(search_target)))
    {
      /* There is a file that needs to be protected, 
	 Copy-on-write hardlink files.
	 we move the file away and create a copy of that file
	 using cp; and remove the original file.
      */
      char* buf=NULL;
      char* tilde=NULL;		/* filename of backup file */

      if (asprintf(&tilde, "%s~~", s)==-1)
	outofmemory("out of memory in check_inode_and_copy, 1");
      if (rename(s, tilde)==-1)
	perror (PRGNAME " backup file generation");
      if (asprintf(&buf, "COWDANCER_IGNORE=yes /bin/cp -a %s~~ %s",
		   s, s)==-1)
	outofmemory("out of memory in check_inode_and_copy, 2");
      debug_cowdancer_2("Command-line", buf);
      system(buf);
      if (unlink(tilde)==-1)
	perror(PRGNAME " unlink backup");
      free(buf);
      free(tilde);
    }
  else				
    debug_cowdancer_2("DEBUG: did not match ", s);
}

int open(const char * a, int flags, ...)
{
  int fd;
  mode_t mode;
  va_list args;
  va_start(args, flags);
  mode = va_arg(args, mode_t);
  va_end(args);
  initialize_functions();
  if(!getenv("COWDANCER_IGNORE"))
    {
      debug_cowdancer_2 ("open", a);
      switch(flags & O_ACCMODE)
	{
	case O_WRONLY:
	case O_RDWR:
	  check_inode_and_copy(a);
	  break;
	}
    }
  fd = origlibc_open (a, flags, mode);
  return fd;
}

int open64(const char * a, int flags, ...)
{
  int fd;
  mode_t mode;
  va_list args;
  va_start(args, flags);
  mode = va_arg(args, mode_t);
  va_end(args);
  initialize_functions();
  if(!getenv("COWDANCER_IGNORE"))
    {
      debug_cowdancer_2 ("open64", a);
      switch(flags & O_ACCMODE)
	{
	case O_WRONLY:
	case O_RDWR:
	  check_inode_and_copy(a);
	  break;
	}
    }
  fd = origlibc_open64 (a, flags, mode);
  return fd;
}

int creat(const char * a, mode_t mode)
{
  int fd;
  initialize_functions();
  if(!getenv("COWDANCER_IGNORE"))
    {
      debug_cowdancer_2 ("creat", a);
      check_inode_and_copy(a);
    }
  fd = origlibc_creat (a, mode);
  return fd;
}

int creat64(const char * a, mode_t mode)
{
  int fd;
  initialize_functions();
  if(!getenv("COWDANCER_IGNORE"))
    {
      debug_cowdancer_2 ("creat64", a);
      check_inode_and_copy(a);
    }
  fd = origlibc_creat64 (a, mode);
  return fd;
}
