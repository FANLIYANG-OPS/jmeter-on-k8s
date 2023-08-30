#ifndef CACHE_UTIL_H
#define CACHE_UTIL_H

#include "sds.h"

int stringMatchLen(const char *p, int p_len, const char *s, int s_len, int no_case);

int stringMatch(const char *p, const char *s, int no_case);

long long mem_toll(const char *p, int *err);

int ll2string(char *s, size_t len, long long value);

int string2ll(const char *s, size_t s_len, long long *value);

int string2l(const char *s, size_t s_len, long *value);

int d2string(char *buf, size_t len, double value);

Sds getAbsolutePath(char *file_name);

int pathIsBaseName(char *path);


#endif