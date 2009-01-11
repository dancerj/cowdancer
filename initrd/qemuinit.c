/**

  An init for qemubuilder.
  Used within qemu.

 */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/mount.h>

#include "../parameter.h"

#define PBUILDER_MOUNTPOINT "/var/cache/pbuilder/pbuilder-mnt"
#define ROOT_MOUNTPOINT "/root"

int main(int argc, char** argv)
{
  printf(" -> qemu-pbuilder first-stage(initrd version)\n");
  mount("/proc", "/proc", 
	"proc", MS_MGC_VAL, NULL);
  forkexeclp("insmod", "insmod", "lib/modules/ext3.ko", NULL);
  forkexeclp("cp", "cp", "/proc/mounts", "/etc/mtab", NULL);
  setenv("PATH", "/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin", 1);
  setenv("PBUILDER_MOUNTPOINT", PBUILDER_MOUNTPOINT, 1);
  mount("/dev/rootdevice", ROOT_MOUNTPOINT, "ext3", 
	MS_MGC_VAL, NULL);
  mount("/dev/commanddevice", 
	ROOT_MOUNTPOINT PBUILDER_MOUNTPOINT, 
	"ext3", MS_MGC_VAL, NULL);
  chroot("root/");
  chdir("/");
  forkexeclp("bin/sh", "bin/sh", PBUILDER_MOUNTPOINT "/pbuilder-run");
  return 0;
}
