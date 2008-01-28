/* 
   define ilist common struct, shared between cowdancer, and cow-shell.
 */

#define ILISTREVISION 2
/* 'COWD' in the host-order */
#define ILISTSIG 0x4f434457

struct ilist_header
{
  int ilistsig;
  int revision;
  int ilist_struct_size;
  int dummy;
};

struct ilist_struct
{
  dev_t dev;
  ino_t inode;
};

void ilist_outofmemory(const char* msg);
int ilistcreate(const char* ilistpath, const char* findcommandline);
int compare_ilist (const void *a, const void *b);
extern const char* ilist_PRGNAME;
