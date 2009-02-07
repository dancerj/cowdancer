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

/**
 * arch-specific routine; disk device name to use
 */
const char* qemu_arch_diskdevice(const struct pbuilderconfig* pc)
{
  if (pc->arch_diskdevice)
    return pc->arch_diskdevice;

  if (!strcmp(pc->arch, "arm") || 
      !strcmp(pc->arch, "armel"))
    {
      return "sd";
    }
  return "hd";
}

/* mknod with prepended pathname
   return -1 on failure.
 */
int mknod_inside_chroot(const char* chroot, const char* pathname, mode_t mode, dev_t dev)
{
  char* p = NULL;
  int ret;
  
  if (-1==asprintf(&p, "%s%s", chroot, pathname))
    {
      fprintf(stderr, "out of memory on asprintf\n");      
      return -1;
    }
  
  ret=mknod(p, mode, dev);

  if (ret == -1)  
    {
      /* output the error message for debug, but ignore it here. */
      perror(p);
    }
  
  free(p);
  return ret;
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
      if(!strcmp(host_arch,arch) && !(system("which kvm")))
	return "kvm";
      else if((!strcmp(host_arch, "i386")) && (!strcmp(arch, "i386")))
	return "qemu";
      else 
	return "qemu-system-x86_64";
    }
  else if (!strcmp(arch, "powerpc"))
    return "qemu-system-ppc";
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
    return "mips";
  else if (!strcmp(arch, "powerpc"))
    return "prep";
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
      return "ttyAMA0";
    }
  return "ttyS0";
}

