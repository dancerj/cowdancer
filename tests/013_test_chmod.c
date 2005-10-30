/*BINFMTC:
 chmod test */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

/* chmod to 0400 */
int main(int argc, char** argv)
{
  int fid;
  
  /* test that the two args work. */
  if(argc < 3)
    exit(1);
  
  chmod(argv[1], 0400);
  fid=open(argv[2], O_RDONLY);
  if (fchmod(fid, 0400)!=-1)
      return 1;			/* this func will fail */
  
  close(fid);
	
  return 0;
}
