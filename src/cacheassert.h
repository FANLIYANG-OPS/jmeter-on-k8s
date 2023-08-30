#ifndef __CACHEASSERT_H__
#define __CACHEASSERT_H__

#include <unistd.h>

#define assert(_e) \
  ((_e) ? (void)0 : (_cacheassert(#_e, __FILE__, __LINE__), _exit(1)))

void _cacheassert(char *estr, char *file, int line);

#endif