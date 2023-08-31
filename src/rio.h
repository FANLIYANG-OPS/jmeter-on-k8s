#ifndef __CACHE_RIO_H
#define __CACHE_RIO_H

#include <stdint.h>
#include <stdio.h>

#include "sds.h"

struct _rio {
  size_t (*read)(struct _rio *, void *buf, size_t len);
  size_t (*write)(struct _rio *, const void *buf, size_t len);
  off_t (*tell)(struct _rio *);
  int (*flush)(struct _rio *);
  void (*update_cksum)(struct _rio *, const void *buf, size_t len);
  uint64_t cksum;
  size_t processed_bytes;
  size_t max_processing_chunk;
  union {
    struct {
      Sds ptr;
      off_t pos;
    } buffer;
    struct {
      FILE *fp;
      off_t buffered;
      off_t autosync;
    } file;
    struct {
      int *fds;
      int *state;
      int numfds;
      off_t pos;
      Sds buf;
    } fdset;
  } io;
};

typedef struct _rio rio;
static inline size_t rioWrite(rio *r, const void *buf, size_t len) {
  while (len) {
    size_t bytes_to_write =
        (r->max_processing_chunk && r->max_processing_chunk < len)
            ? r->max_processing_chunk
            : len;
    if (r->update_cksum) {
      r->update_cksum(r, buf, bytes_to_write);
    }
    if (r->write(r, buf, bytes_to_write) == 0) {
      return 0;
    }
    buf = (char *)buf + bytes_to_write;
    len -= bytes_to_write;
    r->processed_bytes += bytes_to_write;
  }
  return 1;
}

static inline off_t rioTell(rio *r) { return r->tell(r); }

static inline int rioFlush(rio *r) { return r->flush(r); }

void rioInitWithFile(rio *r, FILE *fp);
void rioInitWithBuffer(rio *r, Sds s);
void rioInitWithFdset(rio *r, int *fds, int numfds);
size_t rioWriteBulkCount(rio *r, char prefix, int count);
size_t rioWriteBulkString(rio *r, const char *buf, size_t len);
size_t rioWriteBulkLongLong(rio *r, long long l);
size_t rioWriteBulkDouble(rio *r, double d);

void rioGenericUpdateChecksum(rio *r, const void *buf, size_t len);
void rioSetAutoSync(rio *r, off_t bytes);

#endif