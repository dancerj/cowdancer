/*BINFMTC: qemuarch.c
 * test qemubuilder code
 */

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "qemuarch.h"

int test_get_host_dpkg_arch()
{
  /* just try to test x86_64 case */
#ifdef __x86_64__
  assert (!strcmp(get_host_dpkg_arch(), "amd64"));
#else
  printf("warning: no check for x86_64 stuff, other arch\n");
#endif
  return 0;
}

int main()
{
  int val=0;
  val+=test_get_host_dpkg_arch();
  return val;
}
