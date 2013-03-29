/*
 * cowdancer -- a Copy-on-write data-access; No-cow-easy-replacement
 *
 * Copyright 2005-2009 Junichi Uekawa
 * GPL v2 or later.
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dlfcn.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sched.h>
#include <errno.h>
#include "ilist.h"

const char* ilist_PRGNAME="cowdancer";

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
static int (*origlibc_flock)(int fd, int) = NULL;

static struct ilist_struct* ilist=NULL;
static long ilist_len=0;

/*
   verify the header content
 */
static int verify_ilist_header(struct ilist_header header)
{
  if (header.revision != ILISTREVISION ||
      header.ilistsig != ILISTSIG ||
      header.ilist_struct_size != sizeof (struct ilist_struct))
    {
      ilist_outofmemory(".ilist header unexpected");
      return 1;
    }
  return 0;
}

/* load ilist file

   @return 1 on error, 0 on success */
static int load_ilist(void)
{
  FILE* f=0;
  int fd=0;
  struct stat stbuf;
  struct ilist_struct* local_ilist=NULL;
  struct ilist_header header;
  long local_ilist_len=0;

  if (!getenv("COWDANCER_ILISTFILE"))
    {
      fprintf(stderr, "env var COWDANCER_ILISTFILE not defined\n");
      return 1;
    }

  if (-1==(fd=origlibc_open(getenv("COWDANCER_ILISTFILE"),O_RDONLY,0)))
    {
      fprintf(stderr, "%s: cannot open ilistfile %s\n", ilist_PRGNAME, getenv("COWDANCER_ILISTFILE"));
      return 1;
    }
  if (-1==fstat(fd,&stbuf))
    {
      fprintf(stderr, "%s: cannot fstat ilistfile %s\n", ilist_PRGNAME, getenv("COWDANCER_ILISTFILE"));
      return 1;
    }

  local_ilist_len=(stbuf.st_size - sizeof (struct ilist_header)) / sizeof(struct ilist_struct);

  if (stbuf.st_size != (sizeof(struct ilist_struct) * local_ilist_len + sizeof (struct ilist_header)))
    {
      fprintf(stderr, "%s: .ilist size: %li\n", ilist_PRGNAME, (long)stbuf.st_size);
      ilist_outofmemory(".ilist size unexpected");
      return 1;
    }

  if (((void*)-1)==
      (local_ilist=mmap(NULL, stbuf.st_size, PROT_READ, MAP_PRIVATE,
		  fd, 0)))
    {
      perror("mmap failed, failback to other method");
      /* fall back to non-mmap method. */
      if (NULL==(f=fdopen(fd, "r")))
	{
	  fprintf(stderr, "%s: cannot fdopen ilistfile %s\n", ilist_PRGNAME, getenv("COWDANCER_ILISTFILE"));
	  return 1;
	}

      if (NULL==(local_ilist=malloc(stbuf.st_size)))
	{
	  fprintf(stderr, "%s: out of memory while trying to allocate memory for ilist\n", ilist_PRGNAME);
	  return 1;
	}
      fread(&header, sizeof(struct ilist_header), 1, f);
      if(verify_ilist_header(header)) return 1;
      fread(local_ilist, sizeof(struct ilist_struct), local_ilist_len, f);
      fclose(f);
    }
  else
    {
      if (verify_ilist_header(*(struct ilist_header *)local_ilist)) return 1;
      local_ilist=(void*)local_ilist+sizeof (struct ilist_header);
      close(fd);
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
    fprintf (stderr, "%s: DEBUG %s\n", ilist_PRGNAME, s);
}

static void debug_cowdancer_2 (const char * s, const char*e)
{
  if (getenv("COWDANCER_DEBUG"))
    fprintf (stderr, "%s: DEBUG %s:%s\n", ilist_PRGNAME, s, e);
}

/**
    initialize functions to override libc functions.

    @return 1 on error
 */
__attribute__ ((warn_unused_result))
static int initialize_functions ()
{
  static volatile int initialized = 0;
  /* this code is quasi-reentrant;
     shouldn't suffer too much if it is called multiple times. */

  /* this code is __unlikely__ to be true
     It is initialized==0 only for the first time
     so should be !initialized==0 most of the time.
  */
  if (__builtin_expect(!initialized,0))
    {
      initialized = 1;
      origlibc_open = dlsym(RTLD_NEXT, "open");
      origlibc_open64 = dlsym(RTLD_NEXT, "open64");
      origlibc_creat = dlsym(RTLD_NEXT, "creat");
      origlibc_creat64 = dlsym(RTLD_NEXT, "creat64");
      origlibc_fopen = dlsym(RTLD_NEXT, "fopen64");
      origlibc_fopen64 = dlsym(RTLD_NEXT, "fopen64");
      dlerror();
      if (!(origlibc_chown = dlvsym(RTLD_NEXT, "chown", "GLIBC_2.1")))
	{
	  /* I should really check dlerror, but due to a possible bug in glibc,
	     dlerror doesn't seem to work at all.
	   */
	  const char* d=dlerror();
	  if(!d)
	    {
	      debug_cowdancer("dlerror does not return anything, chown returned NULL but OK");
	      /* success */
	    }
	  else
	    {
	      debug_cowdancer(d);
	    }


	  /* fallback to loading unversioned symbol doing it anyway
	     since glibc does not seem to set dlerror on dlsym failure.
	  */
	  origlibc_chown = dlsym(RTLD_NEXT, "chown");

	}
      origlibc_fchown = dlsym(RTLD_NEXT, "fchown");
      origlibc_lchown = dlsym(RTLD_NEXT, "lchown");
      origlibc_chmod = dlsym(RTLD_NEXT, "chmod");
      origlibc_fchmod = dlsym(RTLD_NEXT, "fchmod");
      origlibc_flock = dlsym(RTLD_NEXT, "flock");

      if (getenv("COWDANCER_DEBUG"))
	{
	  fprintf(stderr,
		  "chown:%p lchown:%p\n", origlibc_chown, origlibc_lchown);
	}

      /* load the ilist */
      if (!ilist)
	{
	  if (load_ilist())
	    {
	      initialized = 0;
	      return 1;
	    }
	  initialized = 2;
	  debug_cowdancer ("Initialization successfully finished.\n");
	}
    }
  /*
     Wait until somebody else finishes his job
     This is very unlikely
  */
  while (__builtin_expect(initialized == 1,0))
    sched_yield();

  if (initialized==0)
    return 1;			/* that somebody else failed */
  else
    return 0;
}

/* the constructor for this library */
__attribute__ ((constructor))
     void ctor()
{
  if (initialize_functions())
    {
      fprintf(stderr,
	      "%s: Fatal, initialize_functions failed\n", ilist_PRGNAME);
    }
}

/**
   check if i-node is to be protected, and if so, copy the file.  This
   function may fail, but the error cannot really be recovered; how
   should the default be ?

   canonicalize flag should be 1 except for non-symlink-following functions.

   @return 1 on failure, 0 on success
*/
__attribute__ ((warn_unused_result))
static int check_inode_and_copy(const char* s, int canonicalize)
{
  struct ilist_struct search_target;
  char* canonical=NULL;/* the canonical filename, the filename of the protected inode and the newly generated*/
  struct stat buf;
  char* backup_file=NULL;/* filename of backup file, the new file with new inode that will be used for future writes */
  int ret;
  pid_t pid;
  int status;
  sigset_t newmask, omask;

  debug_cowdancer_2("DEBUG: test for", s);
  if(lstat(s, &buf))
    return 0;			/* if stat fails, the file probably
				   doesn't exist; return, success */
  if (canonicalize && S_ISLNK(buf.st_mode))
    {
      /* for symbollic link, canonicalize and get the real filename */
      if (!(canonical=canonicalize_file_name(s)))
	return 0;			/* if canonicalize_file_name fails,
					   the file probably doesn't exist. */

      if(stat(canonical, &buf))	/* if I can't stat this file, I don't think
				   I can write to it; ignore */
	return 0;
    }
  else
    canonical=strdup(s);

  memset(&search_target, 0, sizeof(search_target));
  search_target.inode = buf.st_ino;
  search_target.dev = buf.st_dev;

  //do some hardcore debugging here:
  if (getenv("COWDANCER_DEBUG"))
    {
      fprintf (stderr,
	       "ciac: s:%s=canonical:%s nlink:%i reg:%i lnk:%i match:%p\n", s, canonical, (int)buf.st_nlink, S_ISREG(buf.st_mode), S_ISLNK(buf.st_mode),
	      bsearch(&search_target, ilist, ilist_len,
		      sizeof(search_target), compare_ilist));
    }

  if((buf.st_nlink > 1) && 	/* it is hardlinked */
     (S_ISREG(buf.st_mode) ||
      S_ISLNK(buf.st_mode)) && 	/* it is a regular file or a symbollic link */
     bsearch(&search_target, ilist, ilist_len,
	     sizeof(search_target), compare_ilist)) /* and a match is
						       found in ilist
						       file */
    {
      /* There is a file that needs to be protected,
	 Copy-on-write hardlink files.

	 we copy the file first to a backup place, and then mv the
	 backup to the original file, to break the hardlink.
      */

      if (asprintf(&backup_file, "%sXXXXXX", canonical)==-1)
	{
	  ilist_outofmemory("out of memory in check_inode_and_copy, 1");
	  goto error_canonical;
	}

      close(ret=mkstemp(backup_file));
      if (ret==-1)
	{
	  perror("mkstemp");
	  goto error_buf;
	}

      /* mask the SIGCHLD signal, I want waitpid to work.
       */
      sigemptyset (&newmask);
      sigaddset (&newmask, SIGCHLD);
      if (sigprocmask (SIG_BLOCK, &newmask, &omask) < 0)
	{
	  perror("sigprocmask");
	  goto error_buf;
	}

      /* let cp do the task,
	 it probably knows about filesystem details more than I do.

	 forking here is really difficult; I should rewrite this code
	 to not fork/exec. Signal handling and process waiting is too
	 unreliable.
      */
      switch(pid=fork())
	{
	case 0:
	  /* child process, run cp */
	  putenv("COWDANCER_IGNORE=yes");
	  sigprocmask (SIG_SETMASK, &omask, (sigset_t *) NULL); /* unmask SIGCHLD signal */
	  execl("/bin/cp", "/bin/cp", "-a", canonical, backup_file, NULL);
	  perror("execl:cp:");
	  exit(EXIT_FAILURE);
	case -1:
	  /* error condition in fork(); something is really wrong */
	  ilist_outofmemory("out of memory in check_inode_and_copy, 2");
	  goto error_spm;
	default:
	  /* parent process, waiting for cp -a to terminate */
	  if(-1==waitpid(pid, &status, 0))
	    {
	      perror("waitpid:cp");
	      fprintf(stderr, "%s: unexpected waitpid error when waiting for process %i with status %x\n",
		      ilist_PRGNAME, pid, status);
	      goto error_spm;
	    }

	  if (!WIFEXITED(status))
	    {
	      /* something unexpected */
	      fprintf(stderr, "%s: unexpected WIFEXITED status in waitpid: %x\n", ilist_PRGNAME,
		      (int)status);
	      goto error_spm;
	    }
	  else if (WEXITSTATUS(status))
	    {
	      /* cp -a failed */
	      fprintf(stderr, "%s: cp -a failed for %s\n", ilist_PRGNAME, backup_file);
	      goto error_spm;
	    }
	  /* when cp -a succeeded, overwrite the target file from the temporary file with rename */
	  else if (-1==rename(backup_file, canonical))
	    {
	      perror ("file overwrite with rename");
	      fprintf(stderr, "%s: while trying rename of %s to %s\n",  ilist_PRGNAME, canonical, backup_file);
	      goto error_spm;
	    }
	}
      free(backup_file);
      sigprocmask (SIG_SETMASK, &omask, (sigset_t *) NULL);
    }
  else
    debug_cowdancer_2("DEBUG: did not match ", s);

  free(canonical);
  return 0;

  /* error-processing routine. */
 error_spm:
  sigprocmask (SIG_SETMASK, &omask, (sigset_t *) NULL);
 error_buf:
  free(backup_file);
 error_canonical:
  free(canonical);
  return 1;
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
    {
      errno=ENOMEM;
      return -1;
    }

  if(!getenv("COWDANCER_IGNORE"))
    {
      debug_cowdancer_2 ("open", a);
      switch(flags & O_ACCMODE)
	{
	case O_WRONLY:
	case O_RDWR:
	  if (check_inode_and_copy(a,1))
	    {
	      errno=ENOMEM;
	      return -1;
	    }
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
    {
      errno=ENOMEM;
      return -1;
    }
  if(!getenv("COWDANCER_IGNORE"))
    {
      debug_cowdancer_2 ("open64", a);
      switch(flags & O_ACCMODE)
	{
	case O_WRONLY:
	case O_RDWR:
	  if (check_inode_and_copy(a,1))
	    {
	      errno=ENOMEM;
	      return -1;
	    }
	  break;
	}
    }
  fd = origlibc_open64 (a, flags, mode);
  return fd;
}

int creat(const char * a, mode_t mode)
{
  int fd;
  if (initialize_functions())
    {
      errno=ENOMEM;
      return -1;
    }
  if(!getenv("COWDANCER_IGNORE"))
    {
      debug_cowdancer_2 ("creat", a);
      if (check_inode_and_copy(a,1))
	{
	  errno=ENOMEM;
	  return -1;
	}
    }
  fd = origlibc_creat (a, mode);
  return fd;
}

int creat64(const char * a, mode_t mode)
{
  int fd;
  if (initialize_functions())
    {
      errno=ENOMEM;
      return -1;
    }
  if(!getenv("COWDANCER_IGNORE"))
    {
      debug_cowdancer_2 ("creat64", a);
      if (check_inode_and_copy(a,1))
	    {
	      errno=ENOMEM;
	      return -1;
	    }
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
    {
      errno=ENOMEM;
      return NULL;
    }
  if(!getenv("COWDANCER_IGNORE")&&
     likely_fopen_write(t))
    {
      debug_cowdancer_2 ("fopen", s);
      if (check_inode_and_copy(s,1))
	{
	  errno=ENOMEM;
	  return NULL;
	}
    }
  f = origlibc_fopen (s, t);
  return f;
}

#undef fopen64
FILE* fopen64(const char* s, const char* t)
{
  FILE *f;
  if(initialize_functions())
    {
      errno=ENOMEM;
      return NULL;
    }
  if(!getenv("COWDANCER_IGNORE")&&
     likely_fopen_write(t))
    {
      debug_cowdancer_2 ("fopen64", s);
      if(check_inode_and_copy(s,1))
	{
	  errno=ENOMEM;
	  return NULL;
	}
    }
  f = origlibc_fopen64(s, t);
  return f;
}

#undef chown
int chown(const char* s, uid_t u, gid_t g)
{
  int ret;
  if(initialize_functions())
    {
      errno=ENOMEM;
      return -1;
    }
  if(!getenv("COWDANCER_IGNORE"))
    {
      debug_cowdancer_2 ("chown", s);
      if(check_inode_and_copy(s,1))
	{
	  errno=ENOMEM;
	  return -1;
	}
    }
  ret = origlibc_chown(s, u, g);
  debug_cowdancer_2 ("end-chown", s);
  return ret;
}

/* Check out file descriptor
 *
 * This function is currently a dummy, which does not return 1.
 *
 * @return 1 on failure, 0 on success
 *
 */
static int check_fd_inode_and_warn(int fd, const char* operation)
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
	 fchown/fchmod/flock; I don't really know how to do
	 salvation in that case, since the original filename is
	 probably not available, and file is already open.

	 If there is any better way, I'd like to know.
       */
      fprintf(stderr,
	      "W: cowdancer: unsupported operation %s, read-only open and fchown/fchmod/flock are not supported: tried opening dev:inode of %li:%li\n",
	      operation, (long)buf.st_dev, (long)buf.st_ino);
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
      if (check_fd_inode_and_warn(fd, "fchown"))
	{
	  errno=ENOMEM;
	  return -1;
	}
    }
  ret = origlibc_fchown(fd, u, g);
  return ret;
}

#undef lchown
/* chown that does not follow the symbollic links. */
int lchown(const char* s, uid_t u, gid_t g)
{
  int ret;
  if(initialize_functions())
    {
      errno=ENOMEM;
      return -1;
    }
  if(!getenv("COWDANCER_IGNORE"))
    {
      debug_cowdancer_2 ("lchown", s);
      if (check_inode_and_copy(s,0))
	{
	  errno=ENOMEM;
	  return -1;
	}
    }
  ret = origlibc_lchown(s, u, g);
  debug_cowdancer_2 ("end-lchown", s);
  return ret;
}

#undef chmod
int chmod(const char* s, mode_t mode)
{
  int ret;
  if(initialize_functions())
    {
      errno=ENOMEM;
      return -1;
    }
  if(!getenv("COWDANCER_IGNORE"))
    {
      debug_cowdancer_2 ("chmod", s);
      if (check_inode_and_copy(s,1))
	{
	  errno=ENOMEM;
	  return -1;
	}
    }
  ret = origlibc_chmod(s, mode);
  return ret;
}

#undef fchmod
int fchmod(int fd, mode_t mode)
{
  int ret;
  if(initialize_functions())
    {
      errno=ENOMEM;
      return -1;
    }
  if(!getenv("COWDANCER_IGNORE"))
    {
      debug_cowdancer ("fchmod");
      if (check_fd_inode_and_warn(fd, "fchmod"))
	{
	  errno=ENOMEM;
	  return -1;
	}
    }
  ret = origlibc_fchmod(fd, mode);
  return ret;
}

#undef flock
int flock(int fd, int operation)
{
  int ret;
  if(initialize_functions())
    {
      errno=ENOMEM;
      return -1;
    }
  if(!getenv("COWDANCER_IGNORE"))
    {
      debug_cowdancer ("flock");
      if (check_fd_inode_and_warn(fd, "flock"))
	{
	  errno=ENOMEM;
	  return -1;
	}
    }
  ret = origlibc_flock(fd, operation);
  return ret;
}
