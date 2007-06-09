/* 
   define ilist common struct, shared between cowdancer, and cow-shell.
 */
struct ilist_struct
{
  dev_t dev;
  ino_t inode;
};

void outofmemory(const char* msg);
int ilistcreate(const char* ilistpath);
int compare_ilist (const void *a, const void *b);


extern const char* PRGNAME;
