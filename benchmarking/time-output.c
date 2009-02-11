/*BINFMTC:
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
