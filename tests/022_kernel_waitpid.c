/*BINFMTC:
   test that waitpid works expected way
 */
#include <sys/wait.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

int main()
{
  pid_t pid;
  int status;
  
  switch(pid=fork())
    {
    case 0:
      execl("/bin/cp", "/bin/cp", "-a", "/tmp/test", "/tmp/test2", NULL);
      exit (0);

    case -1:
      perror("fork()");
      exit (1);
      
    default:
      sleep(1);
      if(-1==waitpid(pid, &status, 0))
	{
	  perror("waitpid:cp");
	  exit(2);
	}
      printf("waitpid: %x\n", status);
      exit (0);
    }
  
}
