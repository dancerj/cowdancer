/*BINFMTC:

  test 
  close(0);
  open(XXX,XXX);
  will return '0' as file ID.

 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int main(int ac, const char** av)
{
  int fd1, fd2;
  close(0);
  fd1=open(av[1], O_RDONLY);
  close(0);
  fd2=open(av[1], O_RDONLY);

  printf("close(0)open(): try1: %i, try2: %i\n", fd1, fd2);
  
  return fd1;
}
