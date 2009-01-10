/**

  An init for qemubuilder.
  Used within qemu.

 */
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

#include "../parameter.h"

int main(int argc, char** argv)
{
  char* arg[]={"/bin/echo", "hello", "world", NULL};
  
  forkexecvp(arg);
  return 0;
}
