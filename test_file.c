/*BINFMTC: file.c forkexec.c
 */

#define _GNU_SOURCE
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "file.h"
#include "parameter.h"

void test_copy_file()
{
  assert(copy_file("/proc/mounts", "test_file.c.test~")==0);
  assert(forkexeclp("diff", "diff", "-u", "/proc/mounts", 
		    "test_file.c.test~", NULL)==0);
  assert(copy_file("/proc/mounts", "/dev/path/does/not/exist")==-1);
}

void test_create_sparse_file()
{
  char* temp=strdupa("/tmp/sparseXXXXXX");
  mkstemp(temp);
  assert(create_sparse_file(temp,18UL*1UL<<30UL)==0);
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

int main() 
{
  test_mknod_inside_chroot();
  test_copy_file();
  test_create_sparse_file();
  return 0;
}
