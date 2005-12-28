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
#include <sys/mman.h>
#include <sched.h>
#define PRGNAME "cowdancer"
#include "ilist.h"

/* libc functions. */
static int (*origlibc_open)(const char *, int, ...) = NULL;
static int (*origlibc_open64)(const char *, int, ...) = NULL;
static int (*origlibc_creat)(const char *, mode_t) = NULL;
static int (*origlibc_creat64)(const char *, mode_t) = NULL;
static FILE* (*origlibc_fopen)(const char *, const char*) = NULL;
static FILE* (*origlibc_fopen64)(const char *, const char*) = NULL;

static int (*origlibc_chown)(const char *, uid_t, gid_t) = NULL;
static int (*origlibc_fchown)(int fd, uid_t, gid_t) = NULL;
static int (*origlibc_lchown)(const char *, uid_t, gid_t) = NULL;
static int (*origlibc_chmod)(const char *, mode_t) = NULL;
static int (*origlibc_fchmod)(int fd, mode_t) = NULL;

static struct ilist_struct* ilist=NULL;
static long ilist_len=0;

/* I will die when things get too wrong. */
static void outofmemory(const char* msg)
{
  fprintf (stderr, "%s: %s\n", PRGNAME, msg);
}

/* load ilist file, 
   return 1 on error */
static int load_ilist(void)
{
  FILE* f=0;
  int fd=0;
  struct stat stbuf;
  struct ilist_struct* local_ilist=NULL;
  long local_ilist_len=0;  

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
  if (-1==fstat(fd,&stbuf))
    {
      fprintf(stderr, "cannot fstat ilistfile %s\n", getenv("COWDANCER_ILISTFILE"));
      return 1;
    }

  local_ilist_len=stbuf.st_size / sizeof(struct ilist_struct);

  if (stbuf.st_size != (sizeof(struct ilist_struct) * local_ilist_len))
    {
      outofmemory(".ilist size unexpected");
      return 1;
    }
  
  if (((void*)-1)==
      (local_ilist=mmap(NULL, stbuf.st_size, PROT_READ, MAP_PRIVATE, 
		  fd, 0)))
    {
      perror("mmap failed, failback to other method");
      /* fall back to non-mmap method. */
      f=fdopen(fd, "r");
      local_ilist=malloc(stbuf.st_size);
      fread(local_ilist, sizeof(struct ilist_struct), local_ilist_len, f);
      fclose(f);
    }

  /* commit, this should really be atomic, 
     but I don't want to have a lock here.
     Pray that same result will result in multiple invocations,
     and ask for luck. It shouldn't change the result very much if it was called multiple times.
  */
  sched_yield();
  /* Do I need to make them volatile? I want the following assignment to happen
     at this exact timing, to make this quasi-lock-free logic to be remotely successful. */
  ilist_len = local_ilist_len;
  ilist = local_ilist;
  
  return 0;
}

static void debug_cowdancer (const char * s)
{
  if (getenv("COWDANCER_DEBUG")) 
    fprintf (stderr, PRGNAME ": DEBUG %s\n", s);
}

static void debug_cowdancer_2 (const char * s, const char*e)
{
  if (getenv("COWDANCER_DEBUG"))
    fprintf (stderr, PRGNAME ": DEBUG %s:%s\n", s, e);
}

/* return 1 on error */
static int initialize_functions ()
{
  static volatile int initialized = 0;
  /* this code is quasi-reentrant; 
     wouldn't suffer too much if it is called multiple times. */
  
  if (!initialized)
    {
      initialized = 1;
      origlibc_open = dlsym(RTLD_NEXT, "open");
      origlibc_open64 = dlsym(RTLD_NEXT, "open64");
      origlibc_creat = dlsym(RTLD_NEXT, "creat");
      origlibc_creat64 = dlsym(RTLD_NEXT, "creat64");
      origlibc_fopen = dlsym(RTLD_NEXT, "fopen64");
      origlibc_fopen64 = dlsym(RTLD_NEXT, "fopen64");
      origlibc_chown = dlsym(RTLD_NEXT, "chown");
      origlibc_fchown = dlsym(RTLD_NEXT, "fchown");
      origlibc_lchown = dlsym(RTLD_NEXT, "lchown");
      origlibc_chmod = dlsym(RTLD_NEXT, "chmod");
      origlibc_fchmod = dlsym(RTLD_NEXT, "fchmod");

      /* load the ilist */
      if (!ilist)
	{
	  if (load_ilist())
	    return 1;
	}

      initialized = 2;
      debug_cowdancer ("Initialization successfully finished.\n");
    }  
  /* wait until somebody else finishes his job */
  while (initialized == 1)
    sched_yield();
  
  return 0;
}

/* check if i-node is to be protected, and if so, copy the file*/
static void check_inode_and_copy(const char* s)
{
  struct ilist_struct search_target;
  char *canonical=NULL;		/* the canonical filename */
  struct stat buf;
  
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
      
  memset(&search_target, 0, sizeof(search_target));
  search_target.inode = buf.st_ino;
  search_target.dev = buf.st_dev;
  if(bsearch(&search_target, ilist, ilist_len, 
	     sizeof(search_target), compare_ilist) &&
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

#undef chown
int chown(const char* s, uid_t u, gid_t g)
{
  int ret;
  if(initialize_functions())
    return -1;
  if(!getenv("COWDANCER_IGNORE"))
    {
      debug_cowdancer_2 ("chown", s);
      check_inode_and_copy(s);
    }
  ret = origlibc_chown(s, u, g);
  return ret;
}

/* Check out file descriptor
 *
 * @return 1 on failure
 */
int check_fd_inode_and_warn(int fd)
{
  struct stat buf;
  struct ilist_struct search_target;

  fstat(fd, &buf);
  memset(&search_target, 0, sizeof(search_target));
  search_target.inode = buf.st_ino;
  search_target.dev = buf.st_dev;
  if(bsearch(&search_target, ilist, ilist_len, 
	     sizeof(search_target), compare_ilist) &&
     S_ISREG(buf.st_mode))
    {
      /* Someone opened file read-only, and called
	 fchown/fchmod; I don't really know how to do
	 salvation in that case, since the original filename is 
	 probably not available, and file is already open.

	 If there is any better way, I'd like to know.
       */
      fprintf(stderr, "Warning: cowdancer: unsupported operation, read-only open and fchown/fchmod: %li:%li\n", 
	      (long)buf.st_dev, (long)buf.st_ino);
      /* emit a warning and do not fail, 
	 if you want to make it fail, add a return 1;
	 apt seems to want to use this operation; thus apt will always fail.
       */
    }
  return 0;
}

#undef fchown
int fchown(int fd, uid_t u, gid_t g)
{
  int ret;
  if(initialize_functions())
    return -1;
  if(!getenv("COWDANCER_IGNORE"))
    {
      debug_cowdancer ("fchown");
      if (check_fd_inode_and_warn(fd))
	return -1;
    }
  ret = origlibc_fchown(fd, u, g);
  return ret;
}

#undef lchown
int lchown(const char* s, uid_t u, gid_t g)
{
  int ret;
  if(initialize_functions())
    return -1;
  if(!getenv("COWDANCER_IGNORE"))
    {
      debug_cowdancer_2 ("lchown", s);
      check_inode_and_copy(s);
    }
  ret = origlibc_lchown(s, u, g);
  return ret;
}

#undef chmod
int chmod(const char* s, mode_t mode)
{
  int ret;
  if(initialize_functions())
    return -1;
  if(!getenv("COWDANCER_IGNORE"))
    {
      debug_cowdancer_2 ("chmod", s);
      check_inode_and_copy(s);
    }
  ret = origlibc_chmod(s, mode);
  return ret;
}

#undef fchmod
int fchmod(int fd, mode_t mode)
{
  int ret;
  if(initialize_functions())
    return -1;
  if(!getenv("COWDANCER_IGNORE"))
    {
      debug_cowdancer ("fchmod");
      if (check_fd_inode_and_warn(fd))
	return -1;
    }
  ret = origlibc_fchmod(fd, mode);
  return ret;
}
