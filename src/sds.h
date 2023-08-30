#ifndef __Sds_H

#define __Sds_H

#include <stdarg.h>
#include <sys/types.h>

#define SDS_MAX_PRE_ALLOC (1024 * 1024)
#define SDS_TO_SDS_HDR(s) (void *)(s - (sizeof(struct Sdshdr)))

typedef char *Sds;

struct Sdshdr {
  unsigned int len;
  unsigned int free;
  char buf[];
};

static inline size_t sdsLen(const Sds s) {
  struct Sdshdr *sh = (Sdshdr *)SDS_TO_SDS_HDR(s);
  return sh->len;
}

static inline size_t sdsAvail(const Sds s) {
  struct Sdshdr *sh = (Sdshdr *)SDS_TO_SDS_HDR(s);
  return sh->free;
}

Sds sdsNewLen(const void *init, size_t init_len);

Sds sdsNew(const char *init);

Sds sdsEmpty(void);

size_t sdsLen(const Sds s);

Sds sdsDup(const Sds s);

void sdsFree(Sds s);

size_t sdsAvail(const Sds s);

Sds sdsGrowZero(Sds s, size_t len);

Sds sdsCatLen(Sds s, const void *t, size_t len);

Sds sdsCat(Sds s, const char *t);

Sds sdsCatSds(Sds s, const Sds t);

Sds sdsCopy(Sds s, const char *t);

Sds sdsCatVPrintf(Sds s, const char *fmt, va_list ap);

Sds sdsCatPrintf(Sds s, const char *fmt, ...);

Sds sdsCatFmt(Sds s, char const *fmt, ...);

Sds sdsTrim(Sds s, const char *c_set);

void sdsRange(Sds s, int start, int end);

void sdsUpdateLen(Sds s);

void sdsClear(Sds s);

int sdsCmp(const Sds s1, const Sds s2);

Sds *sdsSplitLen(const Sds s, int len, const char *sep, int sp_len, int *count);

void sdsFreeSplitres(Sds *tokens, int count);

void sdsToLower(Sds s);

void sdstoUpper(Sds s);

Sds sdsFromLongLong(long long value);

Sds sdsCatRepr(Sds s, const char *p, size_t len);

Sds *sdsSplitArgs(const char *line, int *argc);

Sds sdsMapChars(Sds s, const char *from, const char *to, size_t set_len);

Sds sdsJoin(char **argv, int argc, char *sep);

Sds sdsMakeRoomFor(Sds s, size_t add_len);

void sdsIncrLen(Sds s, int incr);

Sds sdsRemoveFreeSpace(Sds s);

size_t sdsAllocSize(Sds s);

#endif
