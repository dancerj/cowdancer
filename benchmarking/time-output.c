/*BINFMTC:

  Pipe through this program to get time information on each line of text.

    Use it like:
    sudo cowbuilder --update  | ./time-output.c

 */
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

int main()
{
  time_t start = time(NULL);
  char b[4096];

  while(fgets(b,sizeof(b),stdin)!=0)
    {
      printf("[%i] %s", 
	     (int)difftime(time(NULL), start), 
	     b);
    }
  return 0;
}
