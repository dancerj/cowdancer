/* 
   arch-specific code for qemubuilder.
 */

#ifndef __QEMUARCH_H__
#define __QEMUARCH_H__

#include "parameter.h"

const char* qemu_arch_diskdevice(const struct pbuilderconfig* pc);
const int qemu_create_arch_serialdevice(const char* basedir, const char* arch);
char* get_host_dpkg_arch();
const char* qemu_arch_qemu(const char* arch);
const char* qemu_arch_qemumachine(const char* arch);
const char* qemu_arch_tty(const char* arch);
int mknod_inside_chroot(const char* chroot, const char* pathname, mode_t mode, dev_t dev);

#endif
