/*BINFMTC:
chown test
Used by 012 and 020 tests.
 */

/* for fchown */
/*#define _BSD_SOURCE*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
//#include <sys/types.h>
//#include <sys/stat.h>
//#include <fcntl.h>

int main(int argc, char** argv)
{
  uid_t uid=getuid();
  gid_t gid;
  //int fid;
  
  /* test that four args are given. */
  if(argc < 5)
    exit(1);
  
  gid=atoi(argv[1]);		/* get the gid to change to */
  
  printf("chown %s\n", argv[2]);
  chown(argv[2], uid, gid);
  
  //fid=open(argv[3], O_RDONLY);
  //if (fchown(fid, uid, gid)!=-1)	/* this func will fail */
  //  return 1;
  //close(fid);

  printf("lchown %s\n", argv[4]);
  lchown(argv[4], uid, gid);

  return 0;
}
