/*
 * cowdancer -- a Copy-on-write data-access; No-cow-easy-replacement
 *
 * Copyright 2005 Junichi Uekawa
 * GPL v2 or later.
 */
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
static FILE* (*origlibc_fopen)(const char *, const char*) = NULL;
static FILE* (*origlibc_fopen64)(const char *, const char*) = NULL;

struct ilist_struct
{
  dev_t dev;
  ino_t inode;
};

static struct ilist_struct* ilist=NULL;
static long ilist_len=0;

/* I will die when things get too wrong. */
static void outofmemory(const char* msg)
{
  fprintf (stderr, "%s: %s\n", PRGNAME, msg);
}

/* return 1 on error */
static int load_ilist(void)
{
  int i=0;
  FILE* f=0;
  int fd=0;
  long dev, ino;

  if (!(ilist=malloc(2000*sizeof(struct ilist_struct))))
    {
      outofmemory("load_ilist initialize");
      return 1;
    }
  
  ilist_len=2000;

  if (!getenv("COWDANCER_ILISTFILE"))
    {
      fprintf(stderr, "env var COWDANCER_ILISTFILE not defined\n");
      return 1;
    }
  
  if (-1==(fd=origlibc_open(getenv("COWDANCER_ILISTFILE"),O_RDONLY,0)))
    {
      fprintf(stderr, "cannot open ilistfile %s\n", getenv("COWDANCER_ILISTFILE"));
      return 1;
    }
  f=fdopen(fd, "r");
  
  while (fscanf(f,"%li %li",&dev, &ino)>0)
    {
      (ilist+i)->dev=(dev_t)dev;
      (ilist+i)->inode=(ino_t)ino;

      i++;
      if (i>=ilist_len)
	{
	  ilist=realloc(ilist, (ilist_len*=2)*sizeof(struct ilist_struct));
	  if (!ilist)
	    {
	      outofmemory("load_ilist initialize realloc ");
	      fclose(f);
	      return 1;
	    }
	}
    }
  ilist_len=i;
  fclose(f);
  return 0;
}


static void debug_cowdancer (const char * s)
{
  if (0) fprintf (stderr, PRGNAME ": DEBUG %s\n", s);
}

static void debug_cowdancer_2 (const char * s, const char*e)
{
  if (0) fprintf (stderr, PRGNAME ": DEBUG %s:%s\n", s, e);
}

/* return 1 on error */
static int initialize_functions ()
{
  static int initialized = 0;
  /* do I need to make this code reentrant? */
  
  if (!initialized)
    {
      origlibc_open = dlsym(RTLD_NEXT, "open");
      origlibc_open64 = dlsym(RTLD_NEXT, "open64");
      origlibc_creat = dlsym(RTLD_NEXT, "creat");
      origlibc_creat64 = dlsym(RTLD_NEXT, "creat64");
      origlibc_fopen = dlsym(RTLD_NEXT, "fopen64");
      origlibc_fopen64 = dlsym(RTLD_NEXT, "fopen64");

      /* load the ilist */
      if (load_ilist())
	return 1;

      initialized = 1;
      debug_cowdancer ("Initialization successfully finished.\n");
    }  
  return 0;
}

/* check if i-node is to be protected, and if so, copy the file*/
static void check_inode_and_copy(const char* s)
{
  struct stat buf;
  struct ilist_struct search_target;
  char *canonical=NULL;		/* the canonical filename */
  

  debug_cowdancer_2("DEBUG: test for", s);
  if(lstat(s, &buf))
    return;			/* if stat fails, the file probably 
				   doesn't exist; return */

  if (S_ISLNK(buf.st_mode))
    {
      /* for symbollic link, canonicalize and get the real filename */
      if (!(canonical=canonicalize_file_name(s)))
	return;			/* if canonicalize_file_name fails, 
				   the file probably doesn't exist. */
      
      if(stat(canonical, &buf))	/* if I can't stat this file, I don't think 
				   I can write to it; ignore */
	return;
    }
      
  search_target.inode = buf.st_ino;
  search_target.dev = buf.st_dev;

  if(memmem(ilist, ilist_len*(sizeof(struct ilist_struct)), 
	    &search_target, sizeof(search_target)) &&
     S_ISREG(buf.st_mode))
    {
      /* There is a file that needs to be protected, 
	 Copy-on-write hardlink files.
	 we move the file away and create a copy of that file
	 using cp; and remove the original file.
      */
      char* buf=NULL;
      char* tilde=NULL;		/* filename of backup file */

      if (asprintf(&tilde, "%s~~", canonical?:s)==-1)
	outofmemory("out of memory in check_inode_and_copy, 1");
      if (rename(canonical?:s, tilde)==-1)
	{
	  perror (PRGNAME " backup file generation");
	  fprintf(stderr, "while trying %s\n", tilde);
	  /* FIXME: should error-handle. */
	}
      else
	{
	  if (asprintf(&buf, "COWDANCER_IGNORE=yes /bin/cp -a %s~~ %s",
		       canonical?:s, canonical?:s)==-1)
	    outofmemory("out of memory in check_inode_and_copy, 2");
	  system(buf);
	  free(buf);
	  if (unlink(tilde)==-1)
	    perror(PRGNAME " unlink backup");
	}
      free(tilde);
    }
  else				
    debug_cowdancer_2("DEBUG: did not match ", s);

  if (canonical) free(canonical);
}

int open(const char * a, int flags, ...)
{
  int fd;
  mode_t mode;
  va_list args;
  va_start(args, flags);
  mode = va_arg(args, mode_t);
  va_end(args);
  if (initialize_functions())
    return -1;			/* FIXME: should set errno */
  
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
  if (initialize_functions())
    return -1;			/* FIXME: should set errno */
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
  if (initialize_functions())
    return -1;			/* FIXME: should set errno */
  if(!getenv("COWDANCER_IGNORE"))
    {
      debug_cowdancer_2 ("creat64", a);
      check_inode_and_copy(a);
    }
  fd = origlibc_creat64 (a, mode);
  return fd;
}

static int likely_fopen_write(const char *t)
{
  /* checks if it is likely to be a write fopen */
  return strspn(t, "aw+");
}

#undef fopen
FILE* fopen(const char* s, const char* t)
{
  FILE *f;
  if (initialize_functions())
    return NULL;
  if(!getenv("COWDANCER_IGNORE")&&
     likely_fopen_write(t))
    {
      debug_cowdancer_2 ("fopen", s);
      check_inode_and_copy(s);
    }
  f = origlibc_fopen (s, t);
  return f;
}

#undef fopen64
FILE* fopen64(const char* s, const char* t)
{
  FILE *f;
  if(initialize_functions())
    return NULL;
  if(!getenv("COWDANCER_IGNORE")&&
     likely_fopen_write(t))
    {
      debug_cowdancer_2 ("fopen64", s);
      check_inode_and_copy(s);
    }
  f = origlibc_fopen64(s, t);
  return f;
}

