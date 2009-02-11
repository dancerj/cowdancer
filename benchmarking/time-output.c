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
  char timestr[256];
  ssize_t len;
    
  while((len=read(0,b,sizeof(b)))>0)
    {
      sprintf(timestr, "[%i] ", (int)difftime(time(NULL), start));
      write(1,timestr,strlen(timestr));
      write(1,b,len);
    }
  perror("read");
  return 0;
}

