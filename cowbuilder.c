/*BINFMTC:
 *  cowbuilder / pbuilder with cowdancer
 *  Copyright (C) 2005-2006 Junichi Uekawa
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 *
 */
/* 
  sudo ./cowbuilder --create --distribution sid --basepath /mnt/120g-1/dancer/cow/cow
  sudo ./cowbuilder --update --basepath /mnt/120g-1/dancer/cow/cow

  sudo mkdir /mnt/120g-1/dancer/cow/cow
  cd /mnt/120g-1/dancer/cow/cow
  sudo tar xfzp /var/cache/pbuilder/base.tgz
  sudo chroot . dpkg -i tmp/cowdancer_0.3_i386.deb

  sudo ./cowbuilder.c --build --basepath /mnt/120g-1/dancer/cow/cow $FILENAME

i.e. requires cowdancer inside the chroot.



How to find out changed times of chroot:
$ sudo find /var/cache/pbuilder/build/cow -mtime -1 -ls
will show the list of files changed.

 */

/* 

pbuilder update time measurement on iBook G4 1GHz:

cowbuilder:
real    0m16.792s
user    0m4.395s
sys     0m3.568s

pbuilder:
real    2m30.573s
user    0m38.957s
sys     0m6.523s

pbuilder build time with network down (no package install) on iBook G4
1GHz:

cowbuilder:
real    0m18.694s
user    0m7.899s
sys     0m2.872s

pbuilder:
real    1m20.407s
user    0m9.688s
sys     0m4.142s

with network up:
cowbuilder:
real    1m26.027s
user    0m21.582s
sys     0m7.377s

pbuilder:
real    2m57.126s
user    0m22.255s
sys     0m7.961s


pbuilder login
cowbuilder:
real    0m4.488s
user    0m1.039s
sys     0m1.393s

pbuilder:
real    1m20.058s
user    0m4.526s
sys     0m3.689s



ibookg4 on batteries, building cowdancer sources.
time pdebuild --pbuilder cowbuild
real    0m41.501s
user    0m22.473s
sys     0m8.473s

real    0m45.087s
user    0m22.422s
sys     0m8.434s

time debuild
real    0m51.879s
user    0m18.079s
sys     0m4.401s

2nd try:
real    0m24.262s
user    0m18.113s
sys     0m3.792s

 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <stdarg.h>

typedef struct pbuilderconfig 
{
  int mountproc;
  int mountdev;
  int mountdevpts;
  int save_after_login;
  char* buildplace;		/* /var/cache/pbuilder/build/XXX.$$ */
  char* basepath;		/* /var/cache/pbuilder/cow */
  enum {
    pbuilder_do_nothing=0,
    pbuilder_help,
    pbuilder_build,
    pbuilder_create,
    pbuilder_update,
    pbuilder_execute,
    pbuilder_login
  } operation;
}pbuilderconfig;

/* 
   
The pbuilder command-line to pass

0: pbuilder
1: build/create/login etc.
offset: the next command

The last-command will be
PBUILDER_ADD_PARAM(NULL);

 */
#define MAXPBUILDERCOMMANDLINE 256
char* pbuildercommandline[MAXPBUILDERCOMMANDLINE];
int offset=2;
#define PBUILDER_ADD_PARAM(a) \
 if(offset<(MAXPBUILDERCOMMANDLINE-1)) \
 {pbuildercommandline[offset++]=a;} \
 else \
 {pbuildercommandline[offset]=NULL; fprintf(stderr, "pbuilder-command-line: Max command-line exceeded\n");}


/*
  execvp that does fork.
  
  @return < 0 for failure, exit code for other cases.
 */
static int
forkexecvp (char *const argv[])
{
  int ret;
  pid_t pid;
  int status;
  
  /* DEBUG: */
  {
    int i=0;
    while(argv[i])
      {
	//printf("DEBUG: %i: %s\n", i, argv[i]);
	i++;
      }
  }

  switch(pid=fork())
    {
    case 0:
      execvp(argv[0], (char*const*)argv);
      perror("cowbuilder: execvp");
      exit(EXIT_FAILURE);
    case -1:
      /* error condition in fork(); something is really wrong */
      perror("cowbuilder: fork");
      return -1;
    default:
      /* parent process, waiting for termination */
      waitpid(pid, &status, 0);
      if (!WIFEXITED(status))
	{
	  /* something unexpected */
	  return -1;
	}
      ret = WEXITSTATUS(status);
    }
  return ret;
}

/*
  execlp that does fork.
  
  NULL-terminated list of parameters.

  cf. execl from FreeBSD sources, and glibc posix/execl.c, 
  and cygwin exec.cc

  @return < 0 for failure, exit code for other cases.
 */
static int
forkexeclp (const char *path, const char *arg0, ...)
{
  int i, ret;
  va_list args;
  const char *argv[1024];
  pid_t pid;
  int status;

  va_start(args, arg0);
  argv[0] = arg0;
  i = 1;
  do
    {
      argv[i] = va_arg(args, const char *);
      
      if ( i >= 1023 ) 
	{
	  return -1;
	}
      
    }
  while (argv[i++] != NULL);
  va_end (args);

  switch(pid=fork())
    {
    case 0:
      execvp(path, (char*const*)argv);
      perror("cowbuilder: execlp");
      exit(EXIT_FAILURE);
    case -1:
      /* error condition in fork(); something is really wrong */
      perror("cowbuilder: fork");
      return -1;
    default:
      /* parent process, waiting for termination */
      waitpid(pid, &status, 0);
      if (!WIFEXITED(status))
	{
	  /* something unexpected */
	  return -1;
	}
      ret = WEXITSTATUS(status);
    }
  return ret;
}

/* 
   @return 0 on success, 1 on failure.
 */
static int cpbuilder_internal_cowcopy(const struct pbuilderconfig* pc)
{
  printf(" -> Copying COW directory\n");
  if (0!=forkexeclp("rm", "rm", "-rvf", pc->buildplace, NULL))
    return 1;
  if (0!=forkexeclp("cp", "cp", "-al", pc->basepath, pc->buildplace, NULL))
    return 1;
  return 0;
}

/* 
   @return 0 on success, 1 on failure.
 */
static int cpbuilder_internal_cleancow(const struct pbuilderconfig* pc)
{
  printf(" -> Cleaning COW directory\n");
  if (0!=forkexeclp("rm", "rm", "-rf", pc->buildplace, NULL))
    return 1;
  return 0;
}

/*
 * @return: return code of pbuilder, or <0 on failure
 */
int cpbuilder_build(const struct pbuilderconfig* pc, const char* dscfile_)
{
  const char* dscfile=canonicalize_file_name(dscfile_);
  char* ilistfile;
  char* buf_chroot=NULL;
  int ret;

  if (!dscfile)
    {
      fprintf(stderr, "File not found: %s\n", dscfile_);
      return 1;
    }

  if (cpbuilder_internal_cowcopy(pc))
    {
      fprintf(stderr, "Failed cowcopy.\n");
      return 1;
    }

  
  if (0>asprintf(&buf_chroot, "chroot %s cow-shell", pc->buildplace))
    {
      /* outofmemory */
      fprintf(stderr, "cowdancer: out of memory.\n");
      return -1;
    }
  
  /* delete existing ilist file and use COWDANCER_REUSE */
  asprintf(&ilistfile, "%s/.ilist", pc->buildplace);  
  if (unlink(ilistfile))
    {
      /* no ilist file, but this is no problem */
    }
    
  free(ilistfile);
  setenv("COWDANCER_REUSE","yes",1);

  printf(" -> Invoking pbuilder\n");
  pbuildercommandline[1]="build";
  PBUILDER_ADD_PARAM("--buildplace");
  PBUILDER_ADD_PARAM(pc->buildplace);
  PBUILDER_ADD_PARAM("--no-targz");
  PBUILDER_ADD_PARAM("--internal-chrootexec");
  PBUILDER_ADD_PARAM(buf_chroot);
  PBUILDER_ADD_PARAM(strdup(dscfile));
  PBUILDER_ADD_PARAM(NULL);
  ret=forkexecvp(pbuildercommandline);
  free(buf_chroot);
  if (ret < 128)
    cpbuilder_internal_cleancow(pc);
  else
    printf("pbuilder build aborted, not cleaning\n");

  return ret;
}

int cpbuilder_create(const struct pbuilderconfig* pc)
{
  int ret;
  
  mkdir(pc->basepath,0777);
  printf(" -> Invoking pbuilder\n");

  pbuildercommandline[1]="create";
  PBUILDER_ADD_PARAM("--buildplace");
  PBUILDER_ADD_PARAM(pc->basepath);
  PBUILDER_ADD_PARAM("--no-targz");
  PBUILDER_ADD_PARAM("--extrapackages");
  PBUILDER_ADD_PARAM("cowdancer");
  PBUILDER_ADD_PARAM(NULL);
  ret=forkexecvp(pbuildercommandline);
  return ret;
}


/*
  mv basepath basepath-
  mv buildplace basepath
  rm -rf basepath-

  return 0 on success, nonzero on error.
 */
int cpbuilder_internal_saveupdate(const struct pbuilderconfig* pc)
{
  int ret=0;
  char* buf_tmpfile=NULL;
  
  if (0>asprintf(&buf_tmpfile, "%s-%i-tmp", pc->buildplace, (int)getpid()))
    {
      /* outofmemory */
      return -1;
    }
  if(-1==rename(pc->basepath, buf_tmpfile))
    {
      perror("rename");
      ret=-1;
      goto out;
    }
  if(-1==rename(pc->buildplace, pc->basepath))
    {
      perror("rename");
      ret=-1;
      goto out;
    }
  if (0!=forkexeclp("rm", "rm", "-rf", buf_tmpfile, NULL))
    {
      printf("Could not remove original tree\n");
      ret=-1;
      goto out;
    }
 out:
  free(buf_tmpfile);
  return ret;
}


int cpbuilder_login(const struct pbuilderconfig* pc)
{
  char *buf_chroot;
  int ret;
  
  if (cpbuilder_internal_cowcopy(pc))
    {
      fprintf(stderr, "Failed cowcopy.\n");
      return 1;
    }
  
  if (0>asprintf(&buf_chroot, "chroot %s cow-shell", pc->buildplace))
    {
      /* outofmemory */
      return -1;
    }
  printf(" -> Invoking pbuilder\n");
  pbuildercommandline[1]="login";
  PBUILDER_ADD_PARAM("--buildplace");
  PBUILDER_ADD_PARAM(pc->buildplace);
  PBUILDER_ADD_PARAM("--no-targz");
  PBUILDER_ADD_PARAM("--internal-chrootexec");
  PBUILDER_ADD_PARAM(buf_chroot);
  PBUILDER_ADD_PARAM(NULL);
  ret=forkexecvp(pbuildercommandline);
  free(buf_chroot);
  if (ret < 128)
    {
      
      if (pc->save_after_login)
	{
	  if (0!=forkexeclp("chroot", "chroot", pc->buildplace, "apt-get", "clean", NULL))
	    ret=-1;
	  if (cpbuilder_internal_saveupdate(pc))

	    ret=-1;
	}
      else
	{
	  cpbuilder_internal_cleancow(pc);
	}
    }
  else
    printf("pbuilder login aborted, not cleaning\n");

  return ret;
}

/* 

Mostly a copy of pbuilder login, executes a script.

 */
int cpbuilder_execute(const struct pbuilderconfig* pc, char** av)
{
  char *buf_chroot;
  int ret;
  int i=0;
  
  if (cpbuilder_internal_cowcopy(pc))
    {
      fprintf(stderr, "Failed cowcopy.\n");
      return 1;
    }
  
  if (0>asprintf(&buf_chroot, "chroot %s cow-shell", pc->buildplace))
    {
      /* outofmemory */
      return -1;
    }
  printf(" -> Invoking pbuilder\n");
  pbuildercommandline[1]="execute";
  PBUILDER_ADD_PARAM("--buildplace");
  PBUILDER_ADD_PARAM(pc->buildplace);
  PBUILDER_ADD_PARAM("--no-targz");
  PBUILDER_ADD_PARAM("--internal-chrootexec");
  PBUILDER_ADD_PARAM(buf_chroot);

  /* add all additional parameters */
  while(av[i])
    {
      PBUILDER_ADD_PARAM(av[i]);
      i++;
    }
  PBUILDER_ADD_PARAM(NULL);
  ret=forkexecvp(pbuildercommandline);
  free(buf_chroot);
  if (ret < 128)
    {
      if (pc->save_after_login)
	{
	  if (0!=forkexeclp("chroot", "chroot", pc->buildplace, "apt-get", "clean", NULL))
	    ret=-1;
	  if (cpbuilder_internal_saveupdate(pc))
	    ret=-1;
	}
      else
	{
	  cpbuilder_internal_cleancow(pc);
	}
    }
  else
    printf("pbuilder execute aborted, not cleaning\n");
  
  return ret;
}


/**
   implement pbuilder update

   @return 0 on success, other values on failure.
 */
int cpbuilder_update(const struct pbuilderconfig* pc)
{
  char * buf_chroot;
  int ret;
  
  /* 
     I want the following actions done:
     NEW=buildplace OLD=basepath

     cp -al OLD NEW
     update in NEW
     mv OLD OLD-
     mv NEW OLD
     rm -rf OLD-

     But if you ignore that part, it's really simple.
  */

  /* This simple process will not disrupt already-running pbuilder-cow session,
     but it will not work with new pbuilder-cow sessions until this process finishes, 
     and any failure to pbuilder update will leave crap.
  */
  if (0>asprintf(&buf_chroot, "chroot %s cow-shell", pc->buildplace))
    {
      /* outofmemory */
      return -1;
    }

  if (cpbuilder_internal_cowcopy(pc))
    {
      fprintf(stderr, "Failed cowcopy.\n");
      return 1;
    }

  printf(" -> Invoking pbuilder\n");

  pbuildercommandline[1]="update";
  PBUILDER_ADD_PARAM("--buildplace");
  PBUILDER_ADD_PARAM(pc->buildplace);
  PBUILDER_ADD_PARAM("--no-targz");
  PBUILDER_ADD_PARAM("--internal-chrootexec");
  PBUILDER_ADD_PARAM(buf_chroot);
  PBUILDER_ADD_PARAM(NULL);

  ret=forkexecvp(pbuildercommandline);
  if (ret)
    {
      if (ret < 128)
	{
	  printf("pbuilder update failed\n");
	  if (0!=forkexeclp("rm", "rm", "-rf", pc->buildplace, NULL))
	    {
	      printf("Could not remove original tree\n");
	    }
	}
      else
	  printf("pbuilder update aborted, not cleaning\n");
      
      /* either way, I don't want to touch the original tree */
      goto out;
      
    }
  printf(" -> removing cowbuilder working copy\n");
  cpbuilder_internal_saveupdate(pc);
 out:
  free(buf_chroot);
  return ret;
}

int cpbuilder_help(void)
{
  printf("cowbuilder [operation] [options]\n"
	 "operation:\n"
	 " --build\n"
	 " --create\n"
	 " --update\n"
	 " --login\n"
	 " --execute\n"
	 " --help\n"
	 "options:\n"
	 " --basepath:\n"
	 " --buildplace:\n"
	 " --distribution:\n"
	 " ... and other pbuilder options \n"
	 );
  return 0;
}

int main(int ac, char** av)
{
  int c;			/* option */
  int index_point;
  char * cmdstr=NULL;
  
  static pbuilderconfig pc;

  static struct option long_options[]=
  {
    {"basepath", required_argument, 0, 'b'},
    {"buildplace", required_argument, 0, 'B'},
    {"mountproc", no_argument, &pc.mountproc, 1},
    {"mountdev", no_argument, &pc.mountdev, 1},
    {"mountdevpts", no_argument, &pc.mountdevpts, 1},
    {"nomountproc", no_argument, &pc.mountproc, 0},
    {"nomountdev", no_argument, &pc.mountdev, 0},
    {"nomountdevpts", no_argument, &pc.mountdevpts, 0},
    {"save-after-login", no_argument, &pc.save_after_login, 1},
    {"save-after-exec", no_argument, &pc.save_after_login, 1},
    {"build", no_argument, (int*)&pc.operation, pbuilder_build},
    {"create", no_argument, (int*)&pc.operation, pbuilder_create},
    {"update", no_argument, (int*)&pc.operation, pbuilder_update},
    {"login", no_argument, (int*)&pc.operation, pbuilder_login},
    {"execute", no_argument, (int*)&pc.operation, pbuilder_execute},
    {"help", no_argument, (int*)&pc.operation, pbuilder_help},
    {"version", no_argument, 0, 'v'},

    /* verbatim options, synced as of pbuilder 0.153 */
    {"mirror", required_argument, 0, 'M'},
    {"othermirror", required_argument, 0, 'M'},
    {"buildresult", required_argument, 0, 'M'},
    {"http-proxy", required_argument, 0, 'M'},
    {"aptcache", required_argument, 0, 'M'},
    {"extrapackages", required_argument, 0, 'M'},
    {"configfile", required_argument, 0, 'M'},
    {"hookdir", required_argument, 0, 'M'},
    {"debemail", required_argument, 0, 'M'},
    {"debbuildopts", required_argument, 0, 'M'},
    {"logfile", required_argument, 0, 'M'},
    {"aptconfdir", required_argument, 0, 'M'},
    {"timeout", required_argument, 0, 'M'},
    {"bindmounts", required_argument, 0, 'M'},
    {"debootstrapopts", required_argument, 0, 'M'},
    {"debootstrap", required_argument, 0, 'M'},
    {"distribution", required_argument, 0, 'M'},

    {"removepackages", no_argument, 0, 'm'},
    {"override-config", no_argument, 0, 'm'},
    {"pkgname-logfile", no_argument, 0, 'm'},
    {"binary-arch", no_argument, 0, 'm'},
    {"preserve-buildplace", no_argument, 0, 'm'},
    {"debug", no_argument, 0, 'm'},
    {"autocleanaptcache", no_argument, 0, 'm'},

    {0,0,0,0}
  };

  /* define pc to be clear. */
  memset (&pc, 0, sizeof(pbuilderconfig));
  /* default command-line component */
  pbuildercommandline[0]="pbuilder";

  /* load config files here. */
  while((c = getopt_long (ac, av, "b:d:Mmhv", long_options, &index_point)) != -1)
    {
      switch (c)
	{
	case 'b':		/* basepath */
	  if (pc.operation == pbuilder_create)
	    {
	      if (mkdir(optarg, 0777)<0)
		{
		  perror("mkdir");
		  return 1;
		}
	    }
	  else if (!pc.operation)
	    {
	      fprintf(stderr, "need to specify operation before --basepath option\n");
	      return 1;
	    }
	  if (!(pc.basepath = canonicalize_file_name(optarg)))
	    {
	      fprintf(stderr, "cannot canonicalize filename %s, does not exist\n", optarg);
	      return 1;
	    }
	  break;
	case 'B':		/* buildplace */
	  pc.buildplace = strdup(optarg);
	  break;
	case 'M':		/* pass through to pbuilder: duplicate with param */
	  if (0>asprintf(&cmdstr, "--%s", long_options[index_point].name))
	    {
	      /* error */
	      fprintf(stderr, "out of memory constructing command-line options\n");
	      exit (1);
	    }
	  PBUILDER_ADD_PARAM(cmdstr);
	  PBUILDER_ADD_PARAM(strdup(optarg));
	  break;
	case 'm':		/* pass through to pbuilder: duplicate without param */
	  if (0>asprintf(&cmdstr, "--%s", long_options[index_point].name))
	    {
	      /* error */
	      fprintf(stderr, "out of memory constructing command-line options\n");
	      exit (1);
	    }
	  PBUILDER_ADD_PARAM(cmdstr);
	  break;
	case 0:
	  /* other cases with long option with flags,
	     this is expected behavior, so ignore it.
	  */
	  break;	  
	case 'h':		/* -h */
	case 'v':		/* -v --version */
	  pc.operation=pbuilder_help;
	  break;
	default:
	  fprintf(stderr, "Unhandled option\n");
	  /* Error case. */
	  return 1;
	}
    }

  /* set default values */
  if (!pc.basepath) 
    pc.basepath=strdup("/var/cache/pbuilder/base.cow");
  if (!pc.buildplace)
    {
      mkdir("/var/cache/pbuilder/build",0777); /* create if it does not exist */
      asprintf(&(pc.buildplace), "/var/cache/pbuilder/build/cow.%i", (int)getpid());
    }

  switch(pc.operation)
    {
    case pbuilder_build:
      return cpbuilder_build(&pc, av[optind]);
      
    case pbuilder_create:
      return cpbuilder_create(&pc);
      
    case pbuilder_update:
      return cpbuilder_update(&pc);
      
    case pbuilder_login:
      return cpbuilder_login(&pc);

    case pbuilder_execute:
      return cpbuilder_execute(&pc, &av[optind]);

    case pbuilder_help:
      return cpbuilder_help();

    default:			
      return 1;
    }
  
  return 0;
}
