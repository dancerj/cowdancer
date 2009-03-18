/*BINFMTC: parameter.c forkexec.c ilistcreate.c
 *  cowbuilder / pbuilder with cowdancer
 *  Copyright (C) 2005-2009 Junichi Uekawa
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

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include "parameter.h"
#include "ilist.h"

const char* ilist_PRGNAME="cowbuilder";


/**
   remove a directory.

   be careful about bind-mounted directories.

   @return 0 on success
 */
static int rmrf(const char* path)
{
  return forkexeclp("rm", "rm", "-rf", path, NULL);
}

/**
 * @return .ilist path as malloc()ed string, or NULL on error.
 */
static char* get_ilistfile_path(const struct pbuilderconfig* pc)
{
  char *ilistfile;
  if (0>asprintf(&ilistfile, "%s/.ilist", pc->buildplace))
    {
      /* outofmemory */
      fprintf(stderr, "cowdancer: out of memory.\n");
      return NULL;
    }
  return ilistfile;
}

/* 
   @return 0 on success, 1 on failure.
 */
static int cpbuilder_internal_cowcopy(const struct pbuilderconfig* pc)
{
  char *ilistfile;

  printf(" -> Copying COW directory\n");
  if (0!=rmrf(pc->buildplace))
    return 1;
  if (0!=forkexeclp("cp", "cp", "-al", pc->basepath, pc->buildplace, NULL))
    return 1;

  /* delete existing ilist file if it exists, because I use COWDANCER_REUSE */
  if(!(ilistfile=get_ilistfile_path(pc)))
    {
      /* outofmemory */
      fprintf(stderr, "cowdancer: out of memory.\n");
      return 1;
    }
  if (unlink(ilistfile))
    {
      /* if there was no ilist file in the beginning, that's not a
	 problem.
       */
    }
  free(ilistfile);
  
  return 0;
}

/* 
   @return 0 on success, 1 on failure.
 */
static int cpbuilder_internal_cleancow(const struct pbuilderconfig* pc)
{
  printf(" -> Cleaning COW directory\n");
  if (0!=rmrf(pc->buildplace))
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
  char * prevdir;

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
  
  if(!(ilistfile=get_ilistfile_path(pc)))
    {
      /* outofmemory */
      fprintf(stderr, "cowdancer: out of memory.\n");
      return 1;
    }

  prevdir=get_current_dir_name();
  chdir(pc->buildplace);


  if (forkexeclp("chroot", 
		 "chroot",
		 pc->buildplace,
		 "cowdancer-ilistcreate", 
		 "/.ilist", 
		 "find . -xdev -path ./home -prune -o \\( \\( -type l -o -type f \\) -a -links +1 -print0 \\) | xargs -0 stat --format '%d %i '", 
		 NULL))
    {
      /* if there was an error, back off to manual form */

      /* but first, try etch-workaround */
      if (pc->debian_etch_workaround)
	{
	  fprintf(stderr, "W: Trying my backup method for working with Debian Etch chroots.\n");
	  if (forkexeclp("chroot", 
			 "chroot",
			 pc->buildplace,
			 "cow-shell", 
			 "/bin/true", 
			 NULL))
	    {
	      /* I failed, what can I do? noooo */
	      fprintf(stderr, "E: failed running Debian Etch compatibility backup plan, I'm in panic, eek.\n");
	      return 1;
	    }
	}
      else
	{
	  /* 
	     try the normal backup method

	     It probably doesn't work but try it anyway.
	   */
	  fprintf(stderr, "W: cowdancer-ilistcreate failed to run within chroot, falling back to old method\n");
	  ilistcreate(ilistfile,
		      "find . -xdev -path ./home -prune -o \\( \\( -type l -o -type f \\) -a -links +1 -print0 \\) | xargs -0 stat --format '%d %i '");
	}
    }
  chdir(prevdir);
  free(prevdir);

  printf(" -> Invoking pbuilder\n");
  pbuildercommandline[1]="build";
  PBUILDER_ADD_PARAM("--buildplace");
  PBUILDER_ADD_PARAM(pc->buildplace);
  if (pc->buildresult)
    {
      PBUILDER_ADD_PARAM("--buildresult");
      PBUILDER_ADD_PARAM(pc->buildresult);
    }
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

  free(ilistfile);
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
  if (pc->mirror)
    {
      PBUILDER_ADD_PARAM("--mirror");
      PBUILDER_ADD_PARAM(pc->mirror);
    }
  PBUILDER_ADD_PARAM("--distribution");
  PBUILDER_ADD_PARAM(pc->distribution);
  PBUILDER_ADD_PARAM("--no-targz");
  PBUILDER_ADD_PARAM("--extrapackages");
  PBUILDER_ADD_PARAM("cowdancer");
  PBUILDER_ADD_PARAM(NULL);
  ret=forkexecvp(pbuildercommandline);

  if (ret)
    {	  
      printf("pbuilder create failed\n");
      if (0!=rmrf(pc->basepath))
	{
	  fprintf(stderr, "Could not remove original tree\n");
	}
    }
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
  if (0!=rmrf(buf_tmpfile))
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
  
  /* this is the option passed to internal-chrootexec */
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


/* 
   find matching parameter, and check if it's set.
   @returns found=1, not found=0.
 */
int find_matching_param(const char* option)
{
  int i;
  int found=0;
  for (i=0; i<offset; ++i) 
    {
      if (!strcmp(pbuildercommandline[i], option))
	{
	  found=1;
	}
    }
  return found;
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
  
  if (0>asprintf(&buf_chroot, 
		 (pc->no_cowdancer_update)?"chroot %s":	/* try to not use cow-shell, when
							   no-cowdancer-update option is used */
		 "chroot %s cow-shell",
		 pc->buildplace))
    {
      /* if outofmemory, die. */
      fprintf(stderr, "Out of memory.\n");
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
  if (find_matching_param("--override-config"))
    {
      if (pc->mirror) 
	{
	  PBUILDER_ADD_PARAM("--mirror");
	  PBUILDER_ADD_PARAM(pc->mirror);
	}
      PBUILDER_ADD_PARAM("--distribution");
      PBUILDER_ADD_PARAM(pc->distribution);
    }
  PBUILDER_ADD_PARAM("--no-targz");
  PBUILDER_ADD_PARAM("--internal-chrootexec");
  PBUILDER_ADD_PARAM(buf_chroot);
  PBUILDER_ADD_PARAM(NULL);

  ret=forkexecvp(pbuildercommandline);
  if (ret)
    {
      if (ret < 128)
	{
	  fprintf(stderr, "pbuilder update failed\n");

	  if (!pc->no_cowdancer_update)
	    {
	      /* advise the user to try this option first */
	      fprintf(stderr, "E: could not update with cowdancer, try --no-cowdancer-update option\n");
	    }

	  if (0!=rmrf(pc->buildplace))
	    {
	      fprintf(stderr, "Could not remove original tree\n");
	    }
	}
      else
	printf("pbuilder update aborted, not cleaning\n");
      
      /* either way, I don't want to touch the original tree */
      goto out;
      
    }
  printf(" -> removing %s working copy\n", ilist_PRGNAME);
  cpbuilder_internal_saveupdate(pc);
 out:
  free(buf_chroot);
  return ret;
}

int cpbuilder_help(void)
{
  printf("%s [operation] [options]\n"
	 "operation:\n"
	 " --build\n"
	 " --create\n"
	 " --update\n"
	 " --login\n"
	 " --execute\n"
	 " --help\n"
	 " --dumpconfig\n"
	 "options:\n"
	 " --basepath:\n"
	 " --buildplace:\n"
	 " --distribution:\n"
	 " ... and other pbuilder options \n", ilist_PRGNAME
	 );
  return 0;
}

int main(int ac, char** av)
{
  setenv("COWDANCER_REUSE","yes",1);
  return parse_parameter(ac, av, "cow");
}
