/*BINFMTC: -DTEST_QEMUBUILDER  qemubuilder.c forkexec.c
 * test qemubuilder code
 */

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

const char* sanitize_mirror(const char*addr);

int test_sanitize_mirror()
{
  assert(!strcmp("10.0.2.2", sanitize_mirror("localhost")));
  assert(!strcmp("10.0.2.2", sanitize_mirror("127.0.0.1")));
  assert(!strcmp("10.0.2.2", sanitize_mirror("127.0.0.2")));
  assert(!strcmp("somethingelse.com", sanitize_mirror("somethingelse.com")));
  return 0;
}

int main()
{
  int val=0;
  val+=test_sanitize_mirror();
  return val;
}

