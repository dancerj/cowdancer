/*BINFMTC:
 *  parameter handling for cpbuilder.
 *  Copyright (C) 2007-2008 Junichi Uekawa
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

typedef struct pbuilderconfig 
{
  /* if you edit here, please add to parameter.c: dumpconfig */
  int mountproc;
  int mountdev;
  int mountdevpts;
  int save_after_login;
  char* buildplace;		/* /var/cache/pbuilder/build/XXX.$$ */
  char* buildresult;		/* /var/cache/pbuilder/result/ */
  char* basepath;		/* /var/cache/pbuilder/cow */
  char* mirror;
  char* distribution;
  char* components;
  char* debbuildopts;
  
  /* cow-specific options */
  int no_cowdancer_update;		/* --no-cowdancer-update */
  int debian_etch_workaround;		/* --debian-etch-workaround */

  /* more qemu-isque options */
  char* kernel_image;
  char* initrd;
  int memory_megs;		/* megabytes of memory */
  char* arch;
  char* arch_diskdevice;

  enum {
    pbuilder_do_nothing=0,
    pbuilder_help,
    pbuilder_build,
    pbuilder_create,
    pbuilder_update,
    pbuilder_execute,
    pbuilder_login,
    pbuilder_dumpconfig
  } operation;
}pbuilderconfig;

int load_config_file(const char* config, pbuilderconfig* pc);

int forkexeclp (const char *path, const char *arg0, ...);
int forkexecvp (char *const argv[]);
int parse_parameter(int ac, char** av, const char* keyword);
int cpbuilder_build(const struct pbuilderconfig* pc, const char* dscfile);
int cpbuilder_login(const struct pbuilderconfig* pc);
int cpbuilder_execute(const struct pbuilderconfig* pc, char** av);
int cpbuilder_update(const struct pbuilderconfig* pc);
int cpbuilder_help(void);
int cpbuilder_create(const struct pbuilderconfig* pc);

/* 
   
The pbuilder command-line to pass

0: pbuilder
1: build/create/login etc.
offset: the next command

The last-command will be
PBUILDER_ADD_PARAM(NULL);

 */
#define MAXPBUILDERCOMMANDLINE 256
#define PBUILDER_ADD_PARAM(a) \
 if(offset<(MAXPBUILDERCOMMANDLINE-1)) \
 {pbuildercommandline[offset++]=a;} \
 else \
 {pbuildercommandline[offset]=NULL; fprintf(stderr, "pbuilder-command-line: Max command-line exceeded\n");}
extern char* pbuildercommandline[MAXPBUILDERCOMMANDLINE];
extern int offset;
