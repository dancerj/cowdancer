/*BINFMTC: ilistcreate.c
 test code for ilistcreate.c
 */
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <assert.h>
#include "ilist.h"

const char* ilist_PRGNAME="testcode";

int main()
{
  ilist_outofmemory("hello world\n");
  return 0;
}
