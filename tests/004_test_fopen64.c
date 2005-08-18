/*BINFMTC: -D_FILE_OFFSET_BITS=64
fopen test */
#include <stdio.h>



void writeandclose(FILE* a)
{
  fprintf(a,"abc\n");
  fclose(a);
}

int main(int argc, char** argv)
{
  /* test that the three args work. */
  if(argc < 4)
    exit(1);
  
  writeandclose(fopen(argv[1], "r"));
  writeandclose(fopen(argv[2], "w"));
  writeandclose(fopen(argv[3], "a"));
  return 0;
}
