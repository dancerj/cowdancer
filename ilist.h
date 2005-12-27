/* 
   define ilist common struct, shared between cowdancer, and cow-shell.
 */
struct ilist_struct
{
  dev_t dev;
  ino_t inode;
};

/* comparison function for qsort/bsearch of ilist
 */
static int
compare_ilist (const void *a, const void *b)
{
  const struct ilist_struct * ilista = (const struct ilist_struct*) a;
  const struct ilist_struct * ilistb = (const struct ilist_struct*) b;
  int ret;
  
  ret = ilista->inode - ilistb->inode;
  if (!ret)
    {
      ret = ilista->dev - ilistb->dev;
    }
  return ret;
}

