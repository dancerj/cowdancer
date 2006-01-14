/*BINFMTC:
open test */
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

void writeandclose(int a)
{
  write(a,"abc",3);
  close(a);
}

int main(int argc, char** argv)
{
  /* test that the three args work. */
  int a,b,c;
  
  if(argc < 4)
    exit(1);
  
  a=open(argv[1], O_RDONLY);
  b=open(argv[2], O_WRONLY);
  c=open(argv[3], O_RDWR);
  writeandclose(a);
  writeandclose(b);
  writeandclose(c);

  return 0;
}
