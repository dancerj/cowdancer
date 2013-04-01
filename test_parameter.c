/*BINFMTC: parameter.c

 */
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "parameter.h"

/*
   Mock functions
 */
int cpbuilder_build(const struct pbuilderconfig* pc, const char* dscfile)
{
  return 0;
}

int cpbuilder_login(const struct pbuilderconfig* pc)
{
  return 0;
}

int cpbuilder_execute(const struct pbuilderconfig* pc, char** av)
{
  return 0;
}

int cpbuilder_update(const struct pbuilderconfig* pc)
{
  return 0;
}

int cpbuilder_help(void)
{
  return 0;
}

int cpbuilder_create(const struct pbuilderconfig* pc)
{
  return 0;
}
/*
   end of mock functions
 */

/*
  Test size of Null Terminated String Array handling.
 */
void test_size_of_ntarray()
{
  char* test[32];
  test[0]="test1";
  test[1]="test2";
  test[2]=NULL;
  assert(size_of_ntarray(test)==2);
}

/*
  Test handling of quoted parameter string.
 */
void test_load_quoted_config()
{
  pbuilderconfig pc;
  assert(0 == load_config_file("tests/102_test_cowbuilder_debbuildopts.config", &pc));
  assert(!strcmp("-j2 -I", pc.debbuildopts));
  assert(!strcmp("/boot/vmlinuz-x.y.z", pc.kernel_image));
}

int main()
{
  test_size_of_ntarray();
  test_load_quoted_config();
  return 0;
}
