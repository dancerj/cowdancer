/*BINFMTC:
chown test
Used by 012 and 020 tests.
 */

/* for fchown */
#define _BSD_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int main(int argc, char** argv)
{
  uid_t uid=getuid();
  gid_t gid;
  int fid;
  
  /* test that the four args work. */
  if(argc < 5)
    exit(1);
  
  gid=atoi(argv[1]);		/* get the gid to change to */
  
  chown(argv[2], uid, gid);
  fid=open(argv[3], O_RDONLY);
  //if (fchown(fid, uid, gid)!=-1)	/* this func will fail */
  //  return 1;
  close(fid);
  lchown(argv[4], uid, gid);

  return 0;
}
