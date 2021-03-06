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

#ifndef __FILE_H__
#define __FILE_H__
int copy_file(const char*orig, const char*dest);
int create_sparse_file(const char* filename, unsigned long int size);
int mknod_inside_chroot(const char* chroot, const char* pathname, mode_t mode, dev_t dev);
#endif
