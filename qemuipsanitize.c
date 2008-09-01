/*** IP address sanitization for qemu.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <regex.h>
#include "qemuipsanitize.h"

const char* sanitize_ipaddress(const char*addr)
{
  /* return 10.0.2.2 (qemu host OS address) if localhost */
  const char* local_host="10.0.2.2";
  struct hostent* h;
  h=gethostbyname(addr);
  if(h)
    {
      if (h->h_addr[0]==127)
	{
	  return local_host;
	}
    }
  return addr;
}

static char* r_offstr(regmatch_t m, const char*s)
{
  char*r;
  if(m.rm_so!=-1)
    {
      r=strdup(s+m.rm_so);
      r[m.rm_eo-m.rm_so]=0;
      return r;
    }
  else return strdup("");
}

char* sanitize_mirror(const char*addr)
{
  /* parse IP address string */
  regex_t r;
  int e;
  regmatch_t m[5];
  char*buf=NULL;
    
  if((e=regcomp(&r, "^([^:]*://)([^:/]*)(:[0-9]+)?(.*)$", REG_EXTENDED)))
    {
      /* error */
      fprintf(stderr, "failed compiling regexp: %i\n", e);
      return strdup(addr);
    }
  if((e=regexec(&r, addr, 5, m, 0)))
    {
      fprintf(stderr, "failed regexp match: %i\n", e);
      return strdup(addr);
    }
  asprintf(&buf,"%s%s%s%s",
	   r_offstr(m[1],addr),
	   sanitize_ipaddress(r_offstr(m[2],addr)),
	   r_offstr(m[3],addr),
	   r_offstr(m[4],addr));
  regfree(&r);
  return buf;
}
