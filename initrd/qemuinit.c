/**

  An init for qemubuilder.
  Used within qemu.

 */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/mount.h>
#include <sys/reboot.h>

#include "../parameter.h"
#include "../file.h"

#define PBUILDER_MOUNTPOINT "/var/cache/pbuilder/pbuilder-mnt"
#define ROOT_MOUNTPOINT "/root"

/* global variable to hold /proc/cmdline */
char* proc_cmdline = NULL;

void read_proc_cmdline()
{
  int fd = open("/proc/cmdline", O_RDONLY);
  char buf[BUFSIZ];
  int len=read(fd, buf, sizeof(buf)-1);
  buf[len]=0;			/* make sure it's NULL-terminated */
  proc_cmdline=strdup(buf);
  close(fd);
}

/* parse the command-line option, and return a pointer to
space-delimited option string */
const char* parse_option(const char* option)
{
  const char* s = proc_cmdline;
  do {
    if (!strncmp(option, s, strlen(option)) &&
	s[strlen(option)] == '=')
      return s;
  } while((s=strchr(s, ' ')));
  return NULL;
}

/* 
   Return a strdup string, optionally NULL-terminating if it's
   space-delimited.
 */
char* strdup_spacedelimit(const char* str)
{
  char* s = strdup(str);
  char* space = strchr(str, ' ');
  if (space) *space = 0;
  return s;
}

int main(int argc, char** argv)
{
  printf(" -> qemu-pbuilder first-stage(initrd version)\n");

  umask(S_IWGRP | S_IWOTH);

  chdir("/");
  setenv("PATH", "/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin", 1);
  setenv("PBUILDER_MOUNTPOINT", PBUILDER_MOUNTPOINT, 1);

  mkdir("/proc", 0777);
  mkdir("/dev", 0777);
  mkdir("/dev/pts", 0777);
  
  if (mount("/proc", "/proc", 
	    "proc", MS_MGC_VAL, NULL) == -1) 
    {
      perror("mount proc");
    }

  if (mount("/dev/pts", "/dev/pts", 
	    "devpts", MS_MGC_VAL, NULL) == -1) 
    {
      perror("mount devpts");
    }

  read_proc_cmdline();
  
  forkexeclp("/bin/insmod", "/bin/insmod", "lib/modules/ext3.ko", NULL);
  if (copy_file("/proc/mounts", "/etc/mtab")) 
    {
      fprintf(stderr, "Cannot copy file /proc/mounts to /etc/mtab.\n");
    }
  if (mount("/dev/rootdevice", ROOT_MOUNTPOINT, "ext3", 
	    MS_MGC_VAL, NULL) == -1) 
    {
      perror("mount root device");
    }
  
  if (mount("/dev/commanddevice", 
	    ROOT_MOUNTPOINT PBUILDER_MOUNTPOINT, 
	    "ext3", MS_MGC_VAL, NULL) == -1)
    {
      perror("mount command device");
    }
  
  
  forkexeclp("/bin/sh", "/bin/sh", NULL);
  
  chroot("root/");
  chdir("/");
  forkexeclp("bin/sh", "bin/sh", PBUILDER_MOUNTPOINT "/pbuilder-run", NULL);

  /* turn the machine off */
  sync();
  sync();
  reboot(RB_POWER_OFF);
  return 0;
}
