/*BINFMTC:

  Pipe through this program to get time information on each line of text.

    Use it like:
    sudo cowbuilder --update  | ./time-output.c [optional: file-to-tee]

 */
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>

void fix_terminal() 
{
  struct termios t; 

  if (isatty(1)) 
    {
      tcgetattr(1, &t); 
      t.c_lflag |= ECHO;
      tcsetattr(1, TCSANOW, &t); 
    }
}

int main(int argc, char** argv)
{
  time_t start = time(NULL);
  char b[4096];
  FILE* f = NULL;  

  if (argc == 2) 
    {
      printf("Output tee to extra file %s\n", argv[1]);
      f = fopen(argv[1], "w");
    }
  
  while(fgets(b,sizeof(b),stdin)!=0)
    {
      printf("[%i] %s", 
	     (int)difftime(time(NULL), start), 
	     b);
      fflush(stdout);
      if (f) 
	{
	  fprintf(f, "[%i] %s", 
		  (int)difftime(time(NULL), start), 
		  b);
	  fflush(f);
	}
    }

  if (f) fclose(f);
  fix_terminal();
  return 0;
}
