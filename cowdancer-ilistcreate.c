/*BINFMTC: ilistcreate.c
 *
 *  ilist creation command-line interface
 *  Copyright (C) 2007-2008 Junichi Uekawa
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 *
 * ./cowdancer-ilistcreate.c .ilist 'find . -xdev -path ./home -prune -o \( \( -type l -o -type f \) -a -links +1 -print0 \) | xargs -0 stat --format "%d %i "'
 */

#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "ilist.h"
const char* ilist_PRGNAME="cowdancer-ilistcreate";

int main(int argc, char** argv)
{
  /* 
   */
  if (argc != 3)
    {
      fprintf (stderr,
	       "%s ilist-path find-option\n\n\tcreate .ilist file for use with cow-shell\n\n",
	       argv[0]);
      return 1;
    }
  
  return ilistcreate(argv[1], argv[2]);
}
