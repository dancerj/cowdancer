/*BINFMTC: qemuarch.c
 * test qemubuilder code
 */

#define _GNU_SOURCE
#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "qemuarch.h"

int test_get_host_dpkg_arch()
{
  /* just try to test x86_64 case */
#ifdef __x86_64__
  assert (!strcmp(get_host_dpkg_arch(), "amd64"));
#else
  printf("warning: no check for x86_64 stuff, other arch\n");
#endif
  return 0;
}

int test_mknod_inside_chroot()
{
  /* if you are running this in normal user, or running through 
     fakeroot, you would (probably) get this */
  if (getuid()!=0 || 
      (getenv("FAKEROOTKEY") && strcmp(getenv("FAKEROOTKEY"),"")))
    {
      assert(mknod_inside_chroot("/root", "/dev", 
				 S_IFCHR, makedev(204, 64))
	     == -1);
    }
  else 
    {
      /* if you are running this as root, 
	 this would be the tested codepath.
      */
      struct stat s;
      umask(S_IWOTH);
      unlink("/root/dev5");
      assert(mknod_inside_chroot("/root", 
				 "/dev5", 
				 S_IFCHR | 0660, 
				 makedev(204, 64))
	     == 0);
      assert(stat("/root/dev5", &s)==0);
      assert(S_ISCHR(s.st_mode));
    }
  return 0;
}

int test_qemu_create_arch_devices()
{
  char* temp=strdupa("/tmp/dancerXXXXXX");
  temp=mkdtemp(temp);
  printf("%s\n", temp);
  
  mkdir("./tmp.work111", 0777);
  /* if you are running this in normal user, or running through 
     fakeroot, you would (probably) get this */
  if (getuid()!=0 || 
      (getenv("FAKEROOTKEY") && strcmp(getenv("FAKEROOTKEY"),"")))
    {
      assert(qemu_create_arch_devices(temp, "x86_64")
	     < 0);
    }
  else 
    {
      /* if you are running this as root, 
	 this would be the tested codepath.
      */
      struct stat s;
      umask(S_IWOTH);
      assert(qemu_create_arch_devices(temp, "x86_64")
	     == 0);
      chdir(temp);
      assert(stat("./dev/ttyAMA0", &s)==0);
      assert(S_ISCHR(s.st_mode));
    }
  return 0;
}

int main()
{
  int val=0;
  val+=test_get_host_dpkg_arch();
  val+=test_mknod_inside_chroot();
  val+=test_qemu_create_arch_devices();
  return val;
}
