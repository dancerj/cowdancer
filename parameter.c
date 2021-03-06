/*BINFMTC:
 *  parameter handling for cpbuilder.
 *  Copyright (C) 2007-2009 Junichi Uekawa
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
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include "parameter.h"
#include <assert.h>

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
   get size of Null Terminated array of strings
 */
int size_of_ntarray(char ** buf)
{
  int i = 0;
  while(buf[i]) {
    ++i;
    assert(i < MAX_CUSTOM_FILES);
  }
  return i;
}

/**
 * @returns strdup of string with optionally removing the starting and
 * trailing "'". bash 'set' command will quote.
 *
 ... for ', ''"'"
 */
char* strdup_strip_quote(const char* p) {
  if (*p == 0) {
    return strdup("");
  }

  size_t len = strlen(p);
  if (*p == '\'' && p[len-1] == '\'') {
    char* ret = strdup(p+1);
    assert(strlen(ret) == len - 1);
    ret[len-2] = 0;
    return ret;
  }
  return strdup(p);
}

/**
 * load configuration.
 *
 * Returns bash return codes or -1 on popen error.
 *
 * Most interesting codes:
 *  -1 = popen failed
 *   0 = ok
 *   1 = file not found
 *   2 = syntax error
 */
int load_config_file(const char* config, pbuilderconfig* pc)
{
  char *s;
  FILE* f;

  char* buf=NULL;
  size_t bufsiz=0;
  char* delim;
  int result;

  asprintf(&s, "env bash -c 'set -e ; . %s; set ' 2>&1", config);
  f=popen(s, "r");
  if( NULL == f )
    return -1;
  while ( (0 == feof(f)) && (getline(&buf,&bufsiz,f)>0) )
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
	      pc->mirror=strdup_strip_quote(delim);
	    }
	  else if (!strcmp(buf, "BUILDRESULT"))
	    {
	      pc->buildresult=strdup_strip_quote(delim);
	    }
	  else if (!strcmp(buf, "DISTRIBUTION"))
	    {
	      pc->distribution=strdup_strip_quote(delim);
	    }
	  else if (!strcmp(buf, "KERNEL_IMAGE"))
	    {
	      pc->kernel_image=strdup_strip_quote(delim);
	    }
	  else if (!strcmp(buf, "INITRD"))
	    {
	      pc->initrd=strdup_strip_quote(delim);
	    }
	  else if (!strcmp(buf, "MEMORY_MEGS"))
	    {
	      pc->memory_megs=atoi(delim);
	    }
	  else if (!strcmp(buf, "ARCHITECTURE"))
	    {
	      pc->arch=strdup_strip_quote(delim);
	    }
	  else if (!strcmp(buf, "ARCH"))
	    {
	      pc->arch=strdup_strip_quote(delim);
	    }
	  else if (!strcmp(buf, "BASEPATH"))
	    {
	      pc->basepath=strdup_strip_quote(delim);
	    }
	  else if (!strcmp(buf, "BUILDPLACE"))
	    {
	      pc->buildplace=strdup_strip_quote(delim);
	    }
	  else if (!strcmp(buf, "COMPONENTS"))
	    {
	      pc->components=strdup_strip_quote(delim);
	    }
	  else if (!strcmp(buf, "OTHERMIRROR"))
	    {
	      pc->othermirror=strdup_strip_quote(delim);
	    }
	  else if (!strcmp(buf, "SMP"))
	    {
	      pc->smp=strdup_strip_quote(delim);
	    }
	  else if (!strcmp(buf, "DEBBUILDOPTS"))
	    {
	      pc->debbuildopts=strdup_strip_quote(delim);
	    }
	  else if (!strcmp(buf, "BINARY_ARCH"))
	    {
	      pc->binary_arch=1;
	    }
	  else if (!strcmp(buf, "NO_COWDANCER_UPDATE"))
	    {
	      pc->no_cowdancer_update=1;
	    }
	  else if (!strcmp(buf, "DEBIAN_ETCH_WORKAROUND"))
	    {
	      pc->debian_etch_workaround=1;
	    }
	  else if (!strcmp(buf, "ARCH_DISKDEVICE"))
	    {
	      pc->arch_diskdevice=strdup_strip_quote(delim);
	    }
	  else if (!strcmp(buf, "HTTP_PROXY"))
	    {
	      pc->http_proxy=strdup_strip_quote(delim);
	    }
	}
    }

  result = WEXITSTATUS( pclose(f) );
  if(buf) {
    // Don't warn of missing config files
    if( result > 1 )
      printf( "(exit %i) -> %s\n", result, buf );
    free(buf);
  }
  if(s) free(s);
  return result;
}


int cpbuilder_dumpconfig(pbuilderconfig* pc)
{
  /* dump configuration */
  int i;

  printf("dump config\n");
#define DUMPINT(S) printf("  "#S": %i\n", pc->S);
#define DUMPSTR(S) printf("  "#S": %s\n", pc->S);
#define DUMPSTRARRAY(S) i=0; \
  while (pc->S[i]) \
    { \
      printf("  "#S"[%i]: %s\n", i, pc->S[i]); \
      i++; \
    }

  DUMPINT(mountproc);
  DUMPINT(mountdev);
  DUMPINT(mountdevpts);
  DUMPINT(save_after_login);
  DUMPINT(debug);

  DUMPSTR(buildplace);
  DUMPSTR(buildresult);
  DUMPSTR(basepath);
  DUMPSTR(mirror);
  DUMPSTR(distribution);
  DUMPSTR(components);
  DUMPSTR(othermirror);
  DUMPSTR(smp);
  DUMPSTR(debbuildopts);
  DUMPINT(binary_arch);
  DUMPSTR(http_proxy);
  DUMPSTRARRAY(inputfile);
  DUMPSTRARRAY(outputfile);

  DUMPINT(no_cowdancer_update);

  DUMPSTR(kernel_image);
  DUMPSTR(initrd);
  DUMPINT(memory_megs);
  DUMPSTR(arch);
  return 0;
}

int parse_parameter(int ac, char** av,
		    const char* keyword)
{
  int c;			/* option */
  int index_point;
  int config_ok = -1, load_ok;
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
    {"dumpconfig", no_argument, (int*)&(pc.operation), pbuilder_dumpconfig},
    {"version", no_argument, 0, 'v'},
    {"debug", no_argument, 0, 0},
    {"configfile", required_argument, 0, 'c'},
    {"mirror", required_argument, 0, 0},
    {"buildresult", required_argument, 0, 0},
    {"distribution", required_argument, 0, 0},
    {"components", required_argument, 0, 0},
    {"othermirror", required_argument, 0, 0},
    {"smp", required_argument, 0, 0},
    {"debbuildopts", required_argument, 0, 0},
    {"binary-arch", no_argument, 0, 0},
    {"inputfile", required_argument, 0, 0},
    {"outputfile", required_argument, 0, 0},
    {"architecture", required_argument, 0, 0},
    {"http-proxy", required_argument, 0, 0},

    /* cowbuilder specific options */
    {"no-cowdancer-update", no_argument, 0, 0},
    {"debian-etch-workaround", no_argument, 0, 0},

    /* qemubuilder specific options */
    {"arch-diskdevice", no_argument, 0, 0},

    /* verbatim options with argument, synced as of pbuilder 0.153 */
    {"aptcache", required_argument, 0, 'M'},
    {"extrapackages", required_argument, 0, 'M'},
    {"hookdir", required_argument, 0, 'M'},
    {"debemail", required_argument, 0, 'M'},
    {"logfile", required_argument, 0, 'M'},
    {"aptconfdir", required_argument, 0, 'M'},
    {"keyring", required_argument, 0, 'M'},
    {"timeout", required_argument, 0, 'M'},
    {"bindmounts", required_argument, 0, 'M'},
    {"debootstrapopts", required_argument, 0, 'M'},
    {"debootstrap", required_argument, 0, 'M'},

    /* verbatim options without argument, synced as of pbuilder 0.153 */
    {"allow-untrusted", no_argument, 0, 'm'},
    {"removepackages", no_argument, 0, 'm'},
    {"override-config", no_argument, 0, 'm'},
    {"pkgname-logfile", no_argument, 0, 'm'},
    {"preserve-buildplace", no_argument, 0, 'm'},
    {"autocleanaptcache", no_argument, 0, 'm'},
    {"twice", no_argument, 0, 'm'},

    {0,0,0,0}
  };

  /* define pc to be clear. */
  memset (&pc, 0, sizeof(pbuilderconfig));
  /* default command-line component */
  pbuildercommandline[0]="pbuilder";

  /**
   * Try to load all standard config files.
   * Skip non existing, but exit on broken ones.
   * config_ok is 0, if any load was successfull
   **/
  load_ok = load_config_file("/usr/share/pbuilder/pbuilderrc", &pc);
  if( load_ok > 1 ) exit( 2 );
  if( config_ok != 0 ) config_ok = load_ok;

  load_ok = load_config_file("/etc/pbuilderrc", &pc);
  if( load_ok > 1 ) exit( 3 );
  if( config_ok != 0 ) config_ok = load_ok;

  load_ok = load_config_file("~/.pbuilderrc", &pc);
  if( load_ok > 1 ) exit( 4 );
  if( config_ok != 0 ) config_ok = load_ok;

#define PASS_TO_PBUILDER_WITH_PARAM PBUILDER_ADD_PARAM(cmdstr); \
	      PBUILDER_ADD_PARAM(strdup(optarg));

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
	  load_ok = load_config_file(optarg, &pc);
	  if( load_ok > 1 ) exit( 5 );
	  if( config_ok != 0 ) config_ok = load_ok;

	  if (0>asprintf(&cmdstr, "--%s", long_options[index_point].name))
	    {
	      /* error */
	      fprintf(stderr, "out of memory constructing command-line options\n");
	      exit (1);
	    }
	  PASS_TO_PBUILDER_WITH_PARAM
	  break;
	case 'M':		/* pass through to pbuilder: duplicate with param */
	  if (0>asprintf(&cmdstr, "--%s", long_options[index_point].name))
	    {
	      /* error */
	      fprintf(stderr, "out of memory constructing command-line options\n");
	      exit (1);
	    }
	  PASS_TO_PBUILDER_WITH_PARAM
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

	  /* first, generate 'cmdstr' which is useful anyway */
	  if (0>asprintf(&cmdstr, "--%s", long_options[index_point].name))
	    {
	      /* error */
	      fprintf(stderr, "out of memory constructing command-line options\n");
	      exit (1);
	    }

	  if (!strcmp(long_options[index_point].name,"mirror"))
	    {
	      pc.mirror=strdup(optarg);
	    }
	  else if (!strcmp(long_options[index_point].name,"buildresult"))
	    {
	      pc.buildresult=strdup(optarg);
	    }
	  else if (!strcmp(long_options[index_point].name,"distribution"))
	    {
	      pc.distribution=strdup(optarg);
	    }
	  else if (!strcmp(long_options[index_point].name,"no-cowdancer-update"))
	    {
	      pc.no_cowdancer_update=1;
	    }
	  else if (!strcmp(long_options[index_point].name,"debian-etch-workaround"))
	    {
	      pc.debian_etch_workaround=1;
	    }
	  else if (!strcmp(long_options[index_point].name,"architecture"))
	    {
	      pc.arch=strdup(optarg);
	    }
	  else if (!strcmp(long_options[index_point].name,"arch-diskdevice"))
	    {
	      pc.arch_diskdevice=strdup(optarg);
	    }
	  else if (!strcmp(long_options[index_point].name,"debug"))
	    {
	      pc.debug=1;
	      PBUILDER_ADD_PARAM(cmdstr);
	    }
	  else if (!strcmp(long_options[index_point].name,"inputfile"))
	    {
	      pc.inputfile[size_of_ntarray(pc.inputfile)]=strdup(optarg);
	      if (size_of_ntarray(pc.inputfile) >= MAX_CUSTOM_FILES)
		{
		  fprintf(stderr, "too many inputfile options\n");
		  exit (1);
		}
	      PASS_TO_PBUILDER_WITH_PARAM
	    }
	  else if (!strcmp(long_options[index_point].name,"outputfile"))
	    {
	      pc.inputfile[size_of_ntarray(pc.outputfile)]=strdup(optarg);
	      if (size_of_ntarray(pc.outputfile) >= MAX_CUSTOM_FILES)
		{
		  fprintf(stderr, "too many outputfile options\n");
		  exit (1);
		}
	    }
	  else if (!strcmp(long_options[index_point].name,"components"))
	    {
	      /* this is for qemubuilder */
	      pc.components=strdup(optarg);

	      /* pass it for cowbuilder */
	      PASS_TO_PBUILDER_WITH_PARAM
	    }
	  else if (!strcmp(long_options[index_point].name,"othermirror"))
	    {
	      /* this is for qemubuilder */
	      pc.othermirror=strdup(optarg);

	      /* pass it for cowbuilder */
	      PASS_TO_PBUILDER_WITH_PARAM
	    }
	  else if (!strcmp(long_options[index_point].name,"smp"))
	    {
	      /* this is for qemubuilder */
	      pc.smp=strdup(optarg);

	      /* pass it for cowbuilder */
	      PASS_TO_PBUILDER_WITH_PARAM
	    }
	  else if (!strcmp(long_options[index_point].name,"http-proxy"))
	    {
	      /* this is for qemubuilder */
	      pc.http_proxy=strdup(optarg);

	      /* pass it for cowbuilder */
	      PASS_TO_PBUILDER_WITH_PARAM
	    }
	  else if (!strcmp(long_options[index_point].name,"debbuildopts"))
	    {
	      /* this is for qemubuilder */
	      pc.debbuildopts=strdup(optarg);

	      /* pass it for cowbuilder */
	      PASS_TO_PBUILDER_WITH_PARAM
	    }
	  else if (!strcmp(long_options[index_point].name,"binary-arch"))
	    {
	      pc.binary_arch=1;
	      PBUILDER_ADD_PARAM(cmdstr);
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

  if( 0 != config_ok ) {
    printf( "Couldn't load any valid config file.\n" );
    exit( 6 );
  }

  /* set default values */
  if (!pc.basepath)
    asprintf(&(pc.basepath), "/var/cache/pbuilder/base.%s", keyword);
  if (!pc.buildplace)
    {
      fprintf(stderr, "E: BUILDPLACE is not set\n");
      return 1;
    }
  else
    {
      char* buildplace_ = pc.buildplace;
      mkdir(buildplace_,0777); /* create if it does not exist */
      /* Bug: 573126 This adds '//' if buildplace already ends with / */
      asprintf(&(pc.buildplace), "%s/%s.%i",
	       buildplace_, keyword, (int)getpid());
      free(buildplace_);
    }

  if (!pc.distribution)
    pc.distribution=strdup("sid");

  if (!pc.memory_megs)
    pc.memory_megs=128;

  switch(pc.operation)
    {
    case pbuilder_build:
      if (!av[optind])
	{
	  /* parameter missing */
	  fprintf(stderr, "E: parameter missing for build operation\n");
	  return 1;
	}

      return cpbuilder_build(&pc, av[optind]);

    case pbuilder_create:
      if (av[optind])
	{
	  /* extra parameter */
	  fprintf(stderr, "E: too many parameters for create\n");
	  return 1;
	}
      return cpbuilder_create(&pc);

    case pbuilder_update:
      if (av[optind])
	{
	  /* extra parameter */
	  fprintf(stderr, "E: too many parameters for update\n");
	  return 1;
	}
      return cpbuilder_update(&pc);

    case pbuilder_login:
      return cpbuilder_login(&pc);

    case pbuilder_execute:
      if (!av[optind])
	{
	  /* parameter missing */
	  fprintf(stderr, "E: parameter missing for execute operation\n");
	  return 1;
	}
      return cpbuilder_execute(&pc, &av[optind]);

    case pbuilder_help:
      return cpbuilder_help();

    case pbuilder_dumpconfig:
      return cpbuilder_dumpconfig(&pc);

    default:
      fprintf (stderr, "E: No operation specified\n");
      return 1;
    }

  return 0;
}
