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
   TODO: add support for --configfile etc options.
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
  char* buildplace;		/* /var/cache/pbuilder/buildplace/XXX.$$ */
  char* basepath;		/* /var/cache/pbuilder/buildplace/XXX */
  char* distribution;
  enum {
    pbuilder_do_nothing=0,
    pbuilder_build,
    pbuilder_create,
    pbuilder_update,
    pbuilder_login
  } operation;
}pbuilderconfig;

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
      //printf("debug: %i %s\n", i, argv[i]);
      
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
  printf(" -> Invoking pbuilder\n");
  ret=forkexeclp("pbuilder", "pbuilder", "build", "--buildplace", pc->buildplace, "--no-targz",
		 "--internal-chrootexec",
		 buf_chroot, 
		 dscfile,
		 NULL);
  free(buf_chroot);
  cpbuilder_internal_cleancow(pc);
  return ret;
}

int cpbuilder_create(const struct pbuilderconfig* pc)
{
  int ret;
  
  mkdir(pc->basepath,0777);
  printf(" -> Invoking pbuilder\n");
  ret=forkexeclp("pbuilder","pbuilder","create","--distribution",pc->distribution,
	     "--buildplace",pc->basepath,"--no-targz","--extrapackages",
	     "cowdancer", NULL);
  return ret;
}

int cpbuilder_login(const struct pbuilderconfig* pc)
{
  char *buf_chroot;
  int ret;
  
  cpbuilder_internal_cowcopy(pc);

  
  if (0>asprintf(&buf_chroot, "chroot %s cow-shell", pc->buildplace))
    {
      /* outofmemory */
      return -1;
    }
  printf(" -> Invoking pbuilder\n");
  ret=forkexeclp("pbuilder","pbuilder","login","--buildplace",
		 pc->buildplace,"--no-targz",
		 "--internal-chrootexec", 
		 buf_chroot, NULL);
  free(buf_chroot);
  cpbuilder_internal_cleancow(pc);
  return ret;
}


/**
   implement pbuilder update

   @return 0 on success, other values on failure.
 */
int cpbuilder_update(const struct pbuilderconfig* pc)
{
  char * buf_chroot;
  char * buf_tmpfile;
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
  if (0>asprintf(&buf_tmpfile, "%s-tmp", pc->buildplace))
    {
      /* outofmemory */
      return -1;
    }

  cpbuilder_internal_cowcopy(pc);
  printf(" -> Invoking pbuilder\n");
  ret=forkexeclp("pbuilder","pbuilder","update","--buildplace",
		 pc->buildplace,"--no-targz",
		 "--internal-chrootexec", 
		 buf_chroot, NULL);
  if (ret)
    {
      printf("pbuilder update failed\n");
      if (0!=forkexeclp("rm", "rm", "-rf", pc->buildplace, NULL))
	{
	  printf("Could not remove original tree\n");
	  ret=-1;
	}
      goto out;
    }

  printf(" -> removing cowbuilder working copy\n");
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
  free(buf_chroot);
  
  return ret;
}

int main(int ac, char** av)
{
  int c;			/* option */
  int index_point;
  static pbuilderconfig pc;

  static struct option long_options[]=
  {
    {"basepath", required_argument, 0, 'b'},
    {"distribution", required_argument, 0, 'd'},
    {"mountproc", no_argument, &pc.mountproc, 1},
    {"mountdev", no_argument, &pc.mountdev, 1},
    {"mountdevpts", no_argument, &pc.mountdevpts, 1},
    {"nomountproc", no_argument, &pc.mountproc, 0},
    {"nomountdev", no_argument, &pc.mountdev, 0},
    {"nomountdevpts", no_argument, &pc.mountdevpts, 0},
    {"build", no_argument, (int*)&pc.operation, pbuilder_build},
    {"create", no_argument, (int*)&pc.operation, pbuilder_create},
    {"update", no_argument, (int*)&pc.operation, pbuilder_update},
    {"login", no_argument, (int*)&pc.operation, pbuilder_login},
    {"help", no_argument, 0, 'h'},
    {"version", no_argument, 0, 'v'},
    {0,0,0,0}
  };

  /* define pc to be clear. */
  memset (&pc, 0, sizeof(pbuilderconfig));

  /* load config files here. */
  while((c = getopt_long (ac, av, "b:d:hv", long_options, &index_point)) != -1)
    {
      switch (c)
	{
	case 'b':
	  pc.basepath = canonicalize_file_name (optarg);
	  break;
	case 'd':
	  pc.distribution = strdup(optarg);
	  break;
	case 'h':
	  break;
	case 'v':
	  break;
	case 0:
	  /* other cases with long option with flags,
	     this is expected behavior, so ignore it.
	  */
	  break;	  
	default:
	  fprintf(stderr, "Unhandled option\n");
	  /* Error case. */
	  return 1;
	}
    }

  /* set default values */
  if (!pc.basepath) 
    pc.basepath=strdup("/var/cache/pbuilder/build/cow");
  if (!pc.distribution) 
    pc.distribution=strdup("sid");
  asprintf(&(pc.buildplace), "%s%i", pc.basepath, (int)getpid());

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
            
    default:
      fprintf(stderr, "No operation selected\n");
      return 1;
    }
  
  return 0;
}
