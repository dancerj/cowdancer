/*BINFMTC: -DTEST_QEMUBUILDER qemuipsanitize.c
 * test qemubuilder code
 */

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "qemuipsanitize.h"


#define assert_streq(a,b) \
  assert_streq_f(__FILE__, __LINE__, a, b)

void assert_streq_f(const char*f, int l, const char* a, const char* b)
{
  if(!strcmp(a,b))
    return;
  printf("%s:%i: [%s] and [%s] are different\n",
	 f, l, a, b);
  exit(1);
}

int test_sanitize_ipaddress()
{
  assert_streq("10.0.2.2", sanitize_ipaddress("localhost"));
  assert_streq("10.0.2.2", sanitize_ipaddress("127.0.0.1"));
  assert_streq("10.0.2.2", sanitize_ipaddress("127.0.0.2"));
  assert_streq("www.netfort.gr.jp", sanitize_ipaddress("www.netfort.gr.jp"));
  return 0;
}

int test_sanitize_mirror()
{
  assert_streq("http://www.netfort.gr.jp/debian", sanitize_mirror("http://www.netfort.gr.jp/debian"));
  assert_streq("http://www.netfort.gr.jp:9999/debian", sanitize_mirror("http://www.netfort.gr.jp:9999/debian"));
  assert_streq("http://10.0.2.2:9999/debian", sanitize_mirror("http://localhost:9999/debian"));
  assert_streq("http://10.0.2.2/debian", sanitize_mirror("http://localhost/debian"));
  return 0;
}

int main()
{
  int val=0;
  val+=test_sanitize_ipaddress();
  val+=test_sanitize_mirror();
  return val;
}

