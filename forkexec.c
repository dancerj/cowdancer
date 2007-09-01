/*BINFMTC:
 *  fork/exec helper for pbuilder.
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
#include <sys/wait.h>
#include <unistd.h>
#include <stdarg.h>
#include "parameter.h"

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

