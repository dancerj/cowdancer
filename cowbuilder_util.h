#ifndef COWBUILDER_UTIL_H
#define COWBUILDER_UTIL_H
int check_mountpoint(const char* mountpoint);

void canonicalize_doubleslash(const char* buildplace,
			      char* dest);

#endif
