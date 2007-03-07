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
  close(0);
  return open(av[1], O_RDONLY);
}
