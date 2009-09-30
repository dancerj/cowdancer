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

/*
   insert module and display error message when insmod returns an
   error.
 */
int insmod(const char* module)
{
  int ret=forkexeclp("/bin/insmod", "/bin/insmod", module, NULL);
  if (ret)
    {
      fprintf(stderr, "W: insmod of %s returned %i\n",
	      module,
	      ret);
    }
  return ret;

}

int main(int argc, char** argv)
{
  printf(" -> qemu-pbuilder first-stage(initrd version)\n");

  umask(S_IWGRP | S_IWOTH);

  chdir("/");
  setenv("PATH", "/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin", 1);
  setenv("PBUILDER_MOUNTPOINT", PBUILDER_MOUNTPOINT, 1);

  mkdir("/root", 0777);		/* mountpoint */
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

  /* ext3 driver */
  insmod("lib/modules/linux-live/kernel/drivers/ide/ide-core.ko");
  insmod("lib/modules/linux-live/kernel/drivers/ide/ide-disk.ko");
  insmod("lib/modules/linux-live/kernel/drivers/ide/ide-generic.ko");
  insmod("lib/modules/linux-live/kernel/fs/mbcache.ko");
  insmod("lib/modules/linux-live/kernel/fs/jbd/jbd.ko");
  insmod("lib/modules/linux-live/kernel/fs/ext2/ext2.ko");
  insmod("lib/modules/linux-live/kernel/fs/ext3/ext3.ko");
  sleep(1);

  /* ne2k driver */
  insmod("lib/modules/linux-live/kernel/drivers/net/8390.ko");
  insmod("lib/modules/linux-live/kernel/drivers/net/ne2k-pci.ko");
  if (copy_file("/proc/mounts", "/etc/mtab"))
    {
      fprintf(stderr, "Cannot copy file /proc/mounts to /etc/mtab.\n");
    }

  mknod_inside_chroot("", "dev/sda", S_IFBLK | 0660, makedev(8, 0));
  mknod_inside_chroot("", "dev/sdb", S_IFBLK | 0660, makedev(8, 16));
  mknod_inside_chroot("", "dev/hda", S_IFBLK | 0660, makedev(3, 0));
  mknod_inside_chroot("", "dev/hdb", S_IFBLK | 0660, makedev(3, 64));

  if (mount("/dev/sda", ROOT_MOUNTPOINT, "ext3",
	     MS_MGC_VAL, NULL) == -1)
    {
      perror("mount root device try sda");
      if (mount("/dev/hda", ROOT_MOUNTPOINT, "ext3",
		MS_MGC_VAL, NULL) == -1)
	{
	  perror("mount root device try hda");
	}
    }

  if (mount("/dev/sdb",
	    ROOT_MOUNTPOINT PBUILDER_MOUNTPOINT,
	    "ext3", MS_MGC_VAL, NULL) == -1)
    {
      perror("mount command device try sdb");
      if (mount("/dev/hdb",
		ROOT_MOUNTPOINT PBUILDER_MOUNTPOINT,
		"ext3", MS_MGC_VAL, NULL) == -1)
	{
	   perror("mount command device try hdb");
	}
    }

  forkexeclp("/bin/sh", "/bin/sh", NULL);

  if (chroot("root/"))
    {
      perror("chroot");
    }

  if (chdir("/"))
    {
      perror("chdir");
    }

  forkexeclp("bin/sh", "bin/sh", PBUILDER_MOUNTPOINT "/pbuilder-run", NULL);
  /* debug shell inside chroot. */
  forkexeclp("bin/sh", "bin/sh", NULL);

  /* turn the machine off */
  sync();
  sync();
  reboot(RB_POWER_OFF);
  return 0;
}
