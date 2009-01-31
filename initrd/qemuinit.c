/**

  An init for qemubuilder.
  Used within qemu.

 */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/mount.h>

#include "../parameter.h"

#define PBUILDER_MOUNTPOINT "/var/cache/pbuilder/pbuilder-mnt"
#define ROOT_MOUNTPOINT "/root"

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

  forkexeclp("/bin/insmod", "/bin/insmod", "lib/modules/ext3.ko", NULL);
  forkexeclp("/bin/cp", "/bin/cp", "/proc/mounts", "/etc/mtab", NULL);
  mount("/dev/rootdevice", ROOT_MOUNTPOINT, "ext3", 
	MS_MGC_VAL, NULL);
  mount("/dev/commanddevice", 
	ROOT_MOUNTPOINT PBUILDER_MOUNTPOINT, 
	"ext3", MS_MGC_VAL, NULL);
  
  /* invoke debug shell for the fun of it -- why doesn't it accept any input?? */
  forkexeclp("/bin/sh", "/bin/sh");
  
  chroot("root/");
  chdir("/");
  forkexeclp("bin/sh", "bin/sh", PBUILDER_MOUNTPOINT "/pbuilder-run");
  return 0;
}
