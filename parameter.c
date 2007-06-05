/*BINFMTC:
 *  parameter handling for cpbuilder.
 *  Copyright (C) 2007 Junichi Uekawa
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
#include <sys/types.h>
#include <sys/socket.h>
#include <assert.h>
#include "parameter.h"


/* 
   
The pbuilder command-line to pass

0: pbuilder
1: build/create/login etc.
offset: the next command

The last-command will be
PBUILDER_ADD_PARAM(NULL);

 */
char* pbuildercommandline[MAXPBUILDERCOMMANDLINE];
int offset=2;

/*
  execvp that does fork.
  
  @return < 0 for failure, exit code for other cases.
 */
int forkexecvp (char *const argv[])
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
int
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
      perror("pbuilder: execlp");
      exit(EXIT_FAILURE);
    case -1:
      /* error condition in fork(); something is really wrong */
      perror("pbuilder: fork");
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

/**
 * load configuration.
 */
int load_config_file(const char* config, pbuilderconfig* pc)
{
  char *s;
  FILE* f;

  char* buf=NULL;
  size_t bufsiz=0;
  char* delim;

  asprintf(&s, "env - bash -c 'set -e ; . %s; set '", config);
  f=popen(s, "r");
  while (getline(&buf,&bufsiz,f)>0)
    {
      if (strrchr(buf,'\n'))
	{
	  *(strrchr(buf,'\n'))=0;
	}
      
      if ((delim=strchr(buf,'=')))
	{
	  /* assuming config entry */
	  *(delim++)=0;
	  if (!strcmp(buf, "MIRRORSITE"))
	    {
	      pc->mirror=strdup(delim);
	      //printf("DEBUG: %s, %s\n", buf, delim);
	    }
	  else if (!strcmp(buf, "BUILDRESULT"))
	    {
	      pc->buildresult=strdup(delim);
	    }
	  else if (!strcmp(buf, "DISTRIBUTION"))
	    {
	      pc->distribution=strdup(delim);
	      //printf("DEBUG: %s, %s\n", buf, delim);
	    }
	  else if (!strcmp(buf, "KERNEL_IMAGE"))
	    {
	      pc->kernel_image=strdup(delim);
	      //printf("DEBUG: %s, %s\n", buf, delim);
	    }
	  else if (!strcmp(buf, "INITRD"))
	    {
	      pc->initrd=strdup(delim);
	      //printf("DEBUG: %s, %s\n", buf, delim);
	    }
	  else if (!strcmp(buf, "MEMORY_MEGS"))
	    {
	      pc->memory_megs=atoi(delim);
	      //printf("DEBUG: %s, %s\n", buf, delim);
	    }
	  else if (!strcmp(buf, "ARCH"))
	    {
	      pc->arch=strdup(delim);
	      //printf("DEBUG: %s, %s\n", buf, delim);
	    }
	  else if (!strcmp(buf, "BASEPATH"))
	    {
	      pc->basepath=strdup(delim);
	      //printf("DEBUG: %s, %s\n", buf, delim);
	    }
	}
    }
  if(buf) free(buf);
  if(s) free(s);
  pclose(f);
  return 0;
}


int parse_parameter(int ac, char** av, 
		    const char* keyword)
{
  int c;			/* option */
  int index_point;
  char * cmdstr=NULL;
  static pbuilderconfig pc;
  
  static struct option long_options[]=
  {
    {"basepath", required_argument, 0, 'b'},
    {"buildplace", required_argument, 0, 'B'},
    {"mountproc", no_argument, &(pc.mountproc), 1},
    {"mountdev", no_argument, &(pc.mountdev), 1},
    {"mountdevpts", no_argument, &(pc.mountdevpts), 1},
    {"nomountproc", no_argument, &(pc.mountproc), 0},
    {"nomountdev", no_argument, &(pc.mountdev), 0},
    {"nomountdevpts", no_argument, &(pc.mountdevpts), 0},
    {"save-after-login", no_argument, &(pc.save_after_login), 1},
    {"save-after-exec", no_argument, &(pc.save_after_login), 1},
    {"build", no_argument, (int*)&(pc.operation), pbuilder_build},
    {"create", no_argument, (int*)&(pc.operation), pbuilder_create},
    {"update", no_argument, (int*)&(pc.operation), pbuilder_update},
    {"login", no_argument, (int*)&(pc.operation), pbuilder_login},
    {"execute", no_argument, (int*)&(pc.operation), pbuilder_execute},
    {"help", no_argument, (int*)&(pc.operation), pbuilder_help},
    {"version", no_argument, 0, 'v'},
    {"configfile", required_argument, 0, 'c'},
    {"mirror", required_argument, 0, 0},
    {"buildresult", required_argument, 0, 0},

    /* verbatim options, synced as of pbuilder 0.153 */
    {"othermirror", required_argument, 0, 'M'},
    {"http-proxy", required_argument, 0, 'M'},
    {"aptcache", required_argument, 0, 'M'},
    {"extrapackages", required_argument, 0, 'M'},
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
  
  load_config_file("/usr/share/pbuilder/pbuilderrc", &pc);
  load_config_file("/etc/pbuilderrc", &pc);
  load_config_file("~/.pbuilderrc", &pc);

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
	case 'c':		/* --config */
	  load_config_file(optarg, &pc);
	  if (0>asprintf(&cmdstr, "--%s", long_options[index_point].name))
	    {
	      /* error */
	      fprintf(stderr, "out of memory constructing command-line options\n");
	      exit (1);
	    }
	  PBUILDER_ADD_PARAM(cmdstr);
	  PBUILDER_ADD_PARAM(strdup(optarg));
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
	  /* other cases with long option with flags, this is expected
	     behavior, so ignore it, for most of the time.
	  */
	  
	  /* handle specific options which also give 0. */
	  if (!strcmp(long_options[index_point].name,"mirror"))
	    {
	      pc.mirror=strdup(optarg);
	    }
	  else if (!strcmp(long_options[index_point].name,"buildresult"))
	    {
	      pc.buildresult=strdup(optarg);
	    }
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
    asprintf(&(pc.basepath), "/var/cache/pbuilder/base.%s", keyword);
  if (!pc.buildplace)
    {
      mkdir("/var/cache/pbuilder/build",0777); /* create if it does not exist */
      asprintf(&(pc.buildplace), "/var/cache/pbuilder/build/%s.%i",
	       keyword, (int)getpid());
    }
  if (!pc.distribution)
    pc.distribution=strdup("sid");
  if (!pc.memory_megs)
    pc.memory_megs=128;

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
      fprintf (stderr, "E: No operation specified\n");
      return 1;
    }
  
  return 0;
}
