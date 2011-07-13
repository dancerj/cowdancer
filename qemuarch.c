/*
 *  qemubuilder: pbuilder with qemu
 *  arch-specific code.
 *
 *  Copyright (C) 2009 Junichi Uekawa
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
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <stdarg.h>
#include <assert.h>
#include <termios.h>
#include <time.h>
#include <locale.h>
#include "parameter.h"
#include "qemuarch.h"
#include "file.h"

/**
 * arch-specific routine; disk device name to use
 */
const char* qemu_arch_diskdevice(const struct pbuilderconfig* pc)
{
  if (pc->arch_diskdevice)
    return pc->arch_diskdevice;
  return "sd";
}

/**
 * arch-specific routine; the console device to make
 */
 const int qemu_create_arch_serialdevice(const char* basedir, const char* arch)
{
  dev_t consoledev;

  if (!strcmp(arch, "arm") ||
      !strcmp(arch, "armel"))
    {
      consoledev = makedev(204, 64);
    }
  else
    {
      consoledev = makedev(4, 64);
    }
  return mknod_inside_chroot(basedir, "dev/console", S_IFCHR | 0660, consoledev);
}

/**
 * arch-specific routine; make device files inside chroot
 */
const int qemu_create_arch_devices(const char* basedir, const char* arch)
{
  int ret=0;
  char* s=0;

  asprintf(&s, "%s/%s", basedir, "dev");
  if (-1==mkdir(s, 0777))
    {
      perror("mkdir chroot-/dev");
      ret=1;
    }
  free(s);

  ret+=qemu_create_arch_serialdevice(basedir, arch);
  ret+=mknod_inside_chroot(basedir, "dev/ttyS0", S_IFCHR | 0660, makedev(4, 64));
  ret+=mknod_inside_chroot(basedir, "dev/ttyAMA0", S_IFCHR | 0660, makedev(204, 64));
  ret+=mknod_inside_chroot(basedir, "dev/sda", S_IFBLK | 0660, makedev(8, 0));
  ret+=mknod_inside_chroot(basedir, "dev/sdb", S_IFBLK | 0660, makedev(8, 16));
  ret+=mknod_inside_chroot(basedir, "dev/hda", S_IFBLK | 0660, makedev(3, 0));
  ret+=mknod_inside_chroot(basedir, "dev/hdb", S_IFBLK | 0660, makedev(3, 64));
  return ret;
}

/**
 * get output of dpkg --print-architecture
 * returns a malloc'd string, you need to free it.
 */
char* get_host_dpkg_arch()
{
  FILE*f=popen("dpkg --print-architecture", "r");
  char*host_arch;
  fscanf(f, "%as", &host_arch);
  pclose(f);
  return host_arch;
}

/**
 * arch-specific routine; qemu command to use.
 */
const char* qemu_arch_qemu(const char* arch)
{
  if (!strcmp(arch, "arm") ||
      !strcmp(arch, "armel"))
    {
      return "qemu-system-arm";
    }
  if (!strcmp(arch, "mips"))
    return "qemu-system-mips";
  else if (!strcmp(arch, "mipsel"))
    return "qemu-system-mipsel";
  else if (!strcmp(arch, "i386") ||
	   !strcmp(arch, "amd64"))
    {
      /* we're leaking this memory, but don't care too much */
      char* host_arch = get_host_dpkg_arch();

      /* special-case
	 use kvm if possible
	 use qemu if i386/i386
	 use qemu-system-x86_64 otherwise
       */
      int kvm_exists = !system("which kvm");

      if(!strcmp(host_arch,arch) && kvm_exists)
	return "kvm";
      else if(!strcmp(host_arch, "amd64") && kvm_exists)
        return "kvm";
      else if((!strcmp(host_arch, "i386")) && (!strcmp(arch, "i386")))
	return "qemu";
      else
	return "qemu-system-x86_64";
    }
  else if (!strcmp(arch, "powerpc"))
    return "qemu-system-ppc";
  else if (!strcmp(arch, "sparc"))
    return "qemu-system-sparc";
  else
    return NULL;
}

/**
 * arch-specific routine; the machine spec for this arch
 */
const char* qemu_arch_qemumachine(const char* arch)
{
  if (!strcmp(arch, "arm") ||
      !strcmp(arch, "armel"))
    return "versatilepb";
  else if (!strcmp(arch, "i386") ||
	   !strcmp(arch, "amd64"))
    return "pc";
  else if (!strcmp(arch, "mips")||
	   !strcmp(arch, "mipsel"))
    return "malta";
  else if (!strcmp(arch, "powerpc"))
    return "prep";
  else if (!strcmp(arch, "sparc"))
    return "SS-5";
  return NULL;
}

/**
 * arch-specific routine; the serial device
 */
const char* qemu_arch_tty(const char* arch)
{

  if (!strcmp(arch, "arm")||
      !strcmp(arch, "armel"))
    {
      return "ttyAMA0,115200n8";
    }
  return "ttyS0,115200n8";
}

