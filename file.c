/*
 *  qemubuilder: pbuilder with qemu
 *  Basic file operations code.
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

/**
   copy file contents to temporary filesystem, so that it can be used
   inside qemu.

   return -1
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <assert.h>
#include <alloca.h>
#include "file.h"

/**
 * Copy file from orig to dest.
 *
 * returns 0 on success, -1 on failure.
 */
int copy_file(const char*orig, const char*dest)
{
  const int buffer_size=4096;
  char *buf=malloc(buffer_size);
  int ret=-1;
  int fin=-1;
  int fout=-1;
  size_t count;

  if (!buf)
    {
      fprintf(stderr, "Out of memory\n");
      goto out;
    }

  if (-1==(fin=open(orig,O_RDONLY)))
    {
      perror("file copy: open for read");
      goto out;
    }
  if (-1==(fout=creat(dest,0666)))
    {
      perror("file copy: open for write");
      goto out;
    }
  while((count=read(fin, buf, buffer_size))>0)
    {
      if (-1==write(fout, buf, count))
	{			/* error */
	  perror("file copy: write");
	  fprintf(stderr, "%i\n", fin);
	  fprintf(stderr, "%i %i\n", fout, (int)count);
	  goto out;
	}
    }
  if(count==-1)
    {
      perror("file copy: read ");
      goto out;
    }
  if (-1==close(fin) ||
      -1==close(fout))
    {
      perror("file copy: close ");
      goto out;
    }
  ret=0;
 out:
  free(buf);
  return ret;
}

/**
   Create sparse file of specified size.

   returns 0 on success, 1 on fail.
 */
int create_sparse_file(const char* filename, unsigned long int size)
{
  int fd=creat(filename, 0660);
  const off_t seeksize = 1 << 30;	/* try with 30-bit seeks (1GB / seek) */
  assert(size > 1);
  size--;
  if (-1==lseek(fd, 0, SEEK_SET))
    {
	perror("initial lseek");
	return 1;
    }

  while(size > seeksize) {
    if (-1==lseek(fd, seeksize, SEEK_CUR))
      {
	perror("intermediate lseek");
	return 1;
      }
    size -= seeksize;
  }
  if (-1==lseek(fd, size - 1, SEEK_CUR))
    {
      perror("final lseek");
      return 1;
    }

  if (-1==write(fd, "", 1))		/* A string consists of \0, write 0 to end of file */
    {
      perror("write");
      return 1;
    }

  if (-1==close(fd))
    {
      perror("close");
      return 1;
    }
  return 0;
}

/* mknod with prepended pathname
   return -1 on failure.
 */
int mknod_inside_chroot(const char* chroot, const char* pathname, mode_t mode, dev_t dev)
{
  char* p = alloca(strlen(chroot)+strlen(pathname)+2);
  int ret;

  if (!p)
    {
      fprintf(stderr, "error on alloca\n");
      return -1;
    }

  if (-1==sprintf(p, "%s/%s", chroot, pathname))
    {
      fprintf(stderr, "error on sprintf\n");
      return -1;
    }

  ret=mknod(p, mode, dev);

  if (ret == -1)
    {
      /* output the error message for debug, but ignore it here. */
      perror(p);
    }

  return ret;
}

