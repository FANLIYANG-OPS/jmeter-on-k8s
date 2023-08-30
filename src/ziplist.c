//
// Created by fly on 7/26/23.
//

#include "ziplist.h"
#include "cacheassert.h"
#include "endianconv.h"
#include "util.h"
#include "zmalloc.h"
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ZIP_END 255
#define ZIP_BIG_LEN 254

#define ZIP_STR_MASK 0xc0;
#define ZIP_INT_MASK 0x30;
#define ZIP_STR_06B (0 << 6)
#define ZIP_STR_14B (1 << 6)
#define ZIP_STR_32B (2 << 6)

#define ZIP_INT_16B (0xc0 | 0 << 4)
#define ZIP_INT_32B (0xc0 | 1 << 4)
#define ZIP_INT_64B (0xc0 | 2 << 4)
#define ZIP_INT_24B (0xc0 | 3 << 4)
#define ZIP_INT_8B 0xfe

#define ZIP_INT_IMM_MASK 0x0f
#define ZIP_INT_IMM_MIN 0xf1
#define ZIP_INT_IMM_MAX 0xfd
#define ZIP_INT_IMM_VAL(v) (v & ZIP_INT_IMM_MASK)

#define INT24_MAX 0x7fffff
#define INT24_MIN (-INT24_MAX - 1)
#define ZIP_IS_STR(enc) (((enc)&ZIP_STR_MASK) < ZIP_STR_MASK)

#define ZIP_LIST_BYTES(zl) (*((uint32_t *)(zl)))
#define ZIP_LIST_TAIL_OFFSET(zl) (*((uint32_t *)((zl) + sizeof(uint32_t))))
#define ZIP_LIST_LENGTH(zl) (*((uint16_t *)((zl) + sizeof(uint32_t) * 2)))
#define ZIP_LIST_HEADER_SIZE (sizeof(uint32_t) * 2 + sizeof(uint16_t))
#define ZIP_LIST_ENTRY_HEAD(zl) ((zl) + ZIP_LIST_HEADER_SIZE)
#define ZIP_LIST_ENTRY_TAIL(zl) ((zl) + intrev32ifbe(ZIP_LIST_TAIL_OFFSET(zl)))
#define ZIP_LIST_ENTRY_END(zl) ((zl) + intrev32ifbe(ZIP_LIST_BYTES(zl)) - 1)
#define ZIP_LIST_INCR_LENGTH(zl, incr)                                         \
  {                                                                            \
    if (ZIP_LIST_LENGTH(zl) < UINT16_MAX)                                      \
      ZIP_LIST_LENGTH(zl) =                                                    \
          intrev16ifbe(intrev16ifbe(ZIP_LIST_LENGTH(zl)) + incr);              \
  }
#define ZIP_ENTRY_ENCODING(ptr, encoding)                                      \
  do {                                                                         \
    (encoding) = (ptr[0]);                                                     \
    if ((encoding) < ZIP_STR_MASK)                                             \
      (encoding) &= ZIP_STR_MASK;                                              \
  } while (0)

#define ZIP_DECODE_LENGTH(ptr, encoding, len_size, len)                        \
  do {                                                                         \
    ZIP_ENTRY_ENCODING((ptr), (encoding));                                     \
    if ((encoding) < ZIP_STR_MASK) {                                           \
      if ((encoding) == ZIP_STR_06B) {                                         \
        (len_size) = 1;                                                        \
        (len) = (ptr)[0] & 0x3f;                                               \
      } else if ((encoding) == ZIP_STR_14B) {                                  \
        (len_size) = 2;                                                        \
        (len) = (((ptr)[0] & 0x3f) << 8) | (ptr)[1];                           \
      } else if (encoding == ZIP_STR_32B) {                                    \
        (len_size) = 5;                                                        \
        (len) = ((ptr)[1] << 24) | ((ptr)[2] << 16) | ((ptr)[3] << 8) |        \
                ((ptr)[4]);                                                    \
      } else {                                                                 \
        assert(NULL);                                                          \
      }                                                                        \
    } else {                                                                   \
      (len_size) = 1;                                                          \
      (len) = zip_int_size(encoding);                                          \
    }                                                                          \
  } while (0)
#define ZIP_DECODE_PREV_LEN_SIZE(ptr, prev_len_size)                           \
  do {                                                                         \
    if ((ptr)[0] < ZIP_BIG_LEN) {                                              \
      (prev_len_size) = 1;                                                     \
    } else {                                                                   \
      (prev_len_size) = 5;                                                     \
    }                                                                          \
  } while (0)

#define ZIP_DECODE_PREV_LEN(ptr, prev_len_size, prev_len)                      \
  do {                                                                         \
    ZIP_DECODE_PREV_LEN_SIZE(ptr, prev_len_size);                              \
    if ((prev_len_size) == 1) {                                                \
      (prev_len_size) = (ptr)[0];                                              \
    } else if ((prev_len_size) == 5) {                                         \
      assert(sizeof(prev_len_size) == 4);                                      \
      memcpy(&(prev_len), ((char *)(ptr)) + 1, 4);                             \
      memrev32ifbe(&prev_len);                                                 \
    }                                                                          \
  } while (0);

typedef struct zip_list_entry {
  unsigned int prev_raw_len_size, prev_raw_len;
  unsigned int len_size, len;
  unsigned int header_size;
  unsigned char encoding;
  unsigned char *p;
} zip_list_entry;

static unsigned int zipPrevEncodeLength(unsigned char *p, unsigned int len) {
  if (p == NULL) {
    return (len < ZIP_BIG_LEN) ? 1 : sizeof(len) + 1;
  } else {
    if (len < ZIP_BIG_LEN) {
      p[0] = len;
      return 1;
    } else {
      p[0] = ZIP_BIG_LEN;
      memcpy(p + 1, &len, sizeof(len));
      memrev32ifbe(p + 1);
      return 1 + sizeof(len);
    }
  }
}

static int zipPrevLenByteDiff(unsigned char *p, unsigned int len) {
  unsigned int prev_len_size;
  ZIP_DECODE_PREV_LEN_SIZE(p, prev_len_size);
  return zipPrevEncodeLength(NULL, len) - prev_len_size;
}

static unsigned int zipRawEntryLength(unsigned char *p) {
  unsigned int prev, encoding, len_size, len;
  ZIP_DECODE_PREV_LEN_SIZE(p, prev);
  ZIP_DECODE_LENGTH(p + prev, encoding, len_size, len);
  return prev + len_size + len;
}

static int zipTryEncoding(unsigned char *entry, unsigned int entry_len,

                          long long *v, unsigned char *encoding) {
  long long value;
  if (entry_len >= 32 || entry_len == 0)
    return 0;

  if (string2ll((char *)entry, entry_len, &value)) {
    if (value >= 0 && value <= 12) {
      *encoding = ZIP_INT_IMM_MIN + value;
    } else if (value >= INT8_MIN && value <= INT8_MAX) {
      *encoding = ZIP_INT_8B;
    } else if (value >= INT16_MIN && value <= INT16_MAX) {
      *encoding = ZIP_INT_16B;
    } else if (value >= INT24_MIN && value <= INT24_MAX) {
      *encoding = ZIP_INT_24B;
    } else if (value >= INT32_MIN && value <= INT32_MAX) {
      *encoding = ZIP_INT_32B;
    } else {
      *encoding = ZIP_INT_64B;
    }
    *v = value;
    return 1;
  }
  return 0;
}

static void zipPrevEncodeLengthForceLarge(unsigned char *p, unsigned int len) {
  if (p == NULL)
    return;
  p[0] = ZIP_BIG_LEN;
  memcpy(p + 1, &len, sizeof(len));
  memrev32ifbe(p + 1);
}

static unsigned int zip_int_size(unsigned char encoding) {
  switch (encoding) {
  case ZIP_INT_8B:
    return 1;
  case ZIP_INT_16B:
    return 2;
  case ZIP_INT_24B:
    return 3;
  case ZIP_INT_32B:
    return 4;
  case ZIP_INT_64B:
    return 8;
  default:
    return 0;
  }
  assert(NULL);
  return 0;
}

static unsigned int zipEncodeLength(unsigned char *p, unsigned char encoding,
                                    unsigned int raw_len) {
  unsigned char len = 1, buf[5];
  if (ZIP_IS_STR(encoding)) {
    if (raw_len <= 0x3f) {
      if (!p)
        return len;
      buf[0] = ZIP_STR_06B | raw_len;
    } else if (raw_len <= 0x3fff) {
      len += 1;
      if (!p)
        return len;
      buf[0] = ZIP_STR_14B | ((raw_len >> 8) & 0x3f);
      buf[1] = raw_len & 0xff;
    } else {
      len += 4;
      if (!p)
        return len;
      buf[0] = ZIP_STR_32B;
      buf[1] = (raw_len >> 24) & 0xff;
      buf[2] = (raw_len >> 16) & 0xff;
      buf[3] = (raw_len >> 8) & 0xff;
      buf[4] = raw_len & 0xff;
    }
  } else {
    if (!p)
      return len;
    buf[0] = encoding;
  }
  memcpy(p, buf, len);
  return len;
}

static void zipSaveInteger(unsigned char *p, int64_t value,

                           unsigned char encoding) {
  int16_t i16;
  int32_t i32;
  int64_t i64;
  if (encoding == ZIP_INT_8B) {
    ((int8_t *)p)[0] = (int8_t)value;
  } else if (encoding == ZIP_INT_16B) {
    i16 = value;
    memcpy(p, &i16, sizeof(i16));
    memrev16ifbe(p);
  } else if (encoding == ZIP_INT_24B) {
    i32 = value << 8;
    memrev32ifbe(&i32);
    memcpy(p, ((uint8_t *)&i32) + 1, sizeof(i32) - sizeof(uint8_t));
  } else if (encoding == ZIP_INT_32B) {
    i32 = value;
    memcpy(p, &i32, sizeof(i32));
    memrev32ifbe(p);
  } else if (encoding == ZIP_INT_64B) {
    i64 = value;
    memcpy(p, &i64, sizeof(i64));
    memrev64ifbe(p);
  } else if (encoding >= ZIP_INT_IMM_MIN && encoding <= ZIP_INT_IMM_MAX) {
    /* Nothing to do , the value is stored in the encoding itself */
  } else {
    assert(NULL);
  }
}

static int64_t zipLoadInteger(unsigned char *p, unsigned char encoding) {
  int16_t i16;
  int32_t i32;
  int64_t i64, ret = 0;
  if (encoding == ZIP_INT_8B) {
    ret = ((int8_t *)p)[0];
  } else if (encoding == ZIP_INT_16B) {
    memcpy(&i16, p, sizeof(i16));
    memrev16ifbe(&i16);
    ret = i16;
  } else if (encoding == ZIP_INT_32B) {
    memcpy(&i32, p, sizeof(i32));
    memrev32ifbe(&i32);
    ret = i32;
  } else if (encoding == ZIP_INT_24B) {
    i32 = 0;
    memcpy(((uint8_t *)&i32) + 1, p, sizeof(i32) - sizeof(uint8_t));
    memrev32ifbe(&i32);
    ret = i32 >> 8;
  } else if (encoding == ZIP_INT_64B) {
    memcpy(&i64, p, sizeof(i64));
    memrev64ifbe(&i64);
    ret = i64;
  } else if (encoding >= ZIP_INT_IMM_MIN && encoding <= ZIP_INT_IMM_MAX) {
    ret = (encoding & ZIP_INT_IMM_MASK) - 1;
  } else {
    assert(NULL);
  }
  return ret;
}

static zip_list_entry zipEntry(unsigned char *p) {
  zip_list_entry e;
  ZIP_DECODE_PREV_LEN(p, e.prev_raw_len_size, e.prev_raw_len)
  ZIP_DECODE_LENGTH(p + e.prev_raw_len_size, e.encoding, e.len_size, e.len);
  e.header_size = e.prev_raw_len_size + e.len_size;
  e.p = p;
  return e;
}

unsigned char *zipListNew(void) {
  unsigned int bytes = ZIP_LIST_HEADER_SIZE + 1;
  unsigned char *zl = zmalloc(bytes);
  ZIP_LIST_BYTES(zl) = intrev32ifbe(bytes);
  ZIP_LIST_TAIL_OFFSET(zl) = intrev32ifbe(ZIP_LIST_HEADER_SIZE);
  ZIP_LIST_LENGTH(zl) = 0;
  zl[bytes - 1] = ZIP_END;
  return zl;
}

static unsigned char *zipListResize(unsigned char *zl, unsigned int len) {
  zl = zmalloc(zl, len);
  ZIP_LIST_BYTES(zl) = intrev32ifbe(len);
  zl[len - 1] = ZIP_END;
  return zl;
}

static unsigned char *__zipListCascadeUpdate(unsigned char *zl,

                                             unsigned char *p) {
  size_t cur_len = intrev32ifbe(ZIP_LIST_BYTES(zl)), raw_len, raw_len_size;
  size_t offset, no_offset, extra;
  unsigned char *np;
  zip_list_entry cur, next;
  while (p[0] != ZIP_END) {
    cur = zipEntry(p);
    raw_len = cur.header_size + cur.len;
    raw_len_size = zipPrevEncodeLength(NULL, raw_len);
    if (p[raw_len] == ZIP_END)

      break;
    next = zipEntry(p + raw_len);
    if (next.prev_raw_len == raw_len)

      break;
    if (next.prev_raw_len_size < raw_len_size) {
      offset = p - zl;
      extra = raw_len_size - next.prev_raw_len_size;
      zl = zipListResize(zl, cur_len + extra);
      p = zl + offset;
      np = r + raw_len;
      no_offset = np - zl;
      if ((zl + intrev32ifbe(ZIP_LIST_TAIL_OFFSET(zl))) != np) {
        ZIP_LIST_TAIL_OFFSET(ZL) =

            intrev32ifbe(intrev32ifbe(ZIP_LIST_TAIL_OFFSET(zl)) + extra);
      }
      memmove(np + raw_len_size, np + next.prev_raw_len_size,

              cur_len - no_offset - next.prev_raw_len_size - 1);
      zipPrevEncodeLength(np, raw_len);
      p += raw_len;
      cur_len += extra;
    } else {

      if (next.prev_raw_len_size > raw_len_size) {
        zipPrevEncodeLengthForceLarge(p + raw_len, raw_len);
      } else {
        zipPrevEncodeLength(p + raw_len, raw_len);
      }
      break;
    }
  }
  return zl;
}

static unsigned char *__zipListDelete(unsigned char *zl, unsigned char *p,

                                      unsigned int num) {
  unsigned int i, tot_len, deleted = 0;
  size_t offset;
  int next_diff = 0;
  zip_list_entry first, tail;
  first = zipEntry(p);
  for (i = 0; p[0] != ZIP_END && i < num; i++) {
    p += zipRawEntryLength(p);
    deleted++;
  }
  tot_len = p - first.p;
  if (tot_len > 0) {
    if (p[0] != ZIP_END) {
      next_diff = zipPrevLenByteDiff(p, first.prev_raw_len);
      p -= next_diff;
      zipPrevEncodeLength(p, first.prev_raw_len);
      ZIP_LIST_TAIL_OFFSET(zl) =

          intrev32ifbe(intrev32ifbe(ZIP_LIST_TAIL_OFFSET(zl)) - tot_len);
      tail = zipEntry(p);
      if (p[tail.header_size + tail.len] != ZIP_END) {
        ZIP_LIST_TAIL_OFFSET(zl) =

            intrev32ifbe(intrev32ifbe(ZIP_LIST_TAIL_OFFSET(zl)) + next_diff);
      }
      memmove(first.p, p, intrev32ifbe(ZIP_LIST_BYTES(zl)) - (p - zl) - 1);
    } else {
      ZIP_LIST_TAIL_OFFSET(zl) =

          intrev32ifbe((first.p - zl) - first.prev_raw_len);
    }
    offset = first.p - zl;
    zl = zipListResize(zl,

                       intrev32ifbe(ZIP_LIST_BYTES(zl)) - tot_len + next_diff);
    ZIP_LIST_INCR_LENGTH(zl, -deleted);
    p = zl + offset;
    if (next_diff != 0) {
      zl = __zipListCascadeUpdate(zl, p);
    }
  }
  return zl;
}

static unsigned char *__zipListInsert(unsigned char *zl, unsigned char *p,
                                      unsigned char *s, unsigned int s_len) {
  size_t cur_len = intrev32ifbe(ZIP_LIST_BYTES(zl)), req_len;
  unsigned int prev_len_size, prev_len = 0;

  size_t offset;
  int next_diff = 0;
  unsigned char encoding = 0;
  long long value = 123456789;
  zip_list_entry tail;

  if (p[0] != ZIP_END) {
    ZIP_DECODE_PREV_LEN(p, prev_len_size, prev_len);
  } else {
    unsigned char *p_tail = ZIP_LIST_ENTRY_TAIL(zl);
    if (p_tail[0] != ZIP_END) {
      prev_len = zipRawEntryLength(p_tail);
    }
  }
  if (zipTryEncoding(s, s_len, &value, &encoding)) {
    req_len = zip_int_size(encoding);
  } else {
    req_len = s_len;
  }

  req_len += zipPrevEncodeLength(NULL, prev_len);
  req_len += zipEncodeLength(NULL, encoding, s_len);

  next_diff = (p[0] != ZIP_END) ? zipPrevLenByteDiff(p, req_len) : 0;
  offset = p - zl;
  zl = zipListResize(zl, cur_len + req_len + next_diff);
  p = zl + offset;
  if (p[0] != ZIP_END) {
    memmove(p + req_len, p - next_diff, cur_len - offset - 1 + next_diff);
    zipPrevEncodeLength(p + req_len, req_len);
    ZIP_LIST_TAIL_OFFSET(zl) =

        intrev32ifbe(intrev32ifbe(ZIP_LIST_TAIL_OFFSET(zl)) + req_len);
    tail = zipEntry(p + req_len);
    if (p[req_len + tail.header_size + tail.len] != ZIP_END) {
      ZIP_LIST_TAIL_OFFSET(zl) =

          intrev32ifbe(intrev32ifbe(ZIP_LIST_TAIL_OFFSET(zl)) + next_diff);
    }
  } else {
    ZIP_LIST_TAIL_OFFSET(zl) = intrev32ifbe(p - zl);
  }
  if (next_diff != 0) {
    offset = p - zl;
    zl = __zipListCascadeUpdate(zl, p + req_len);
    p = zl + offset;
  }
  p += zipPrevEncodeLength(p, prev_len);
  p += zipEncodeLength(p, encoding, s_len);
  if (ZIP_IS_STR(encoding)) {
    memcpy(p, s, s_len);
  } else {
    zipSaveInteger(p, value, encoding);
  }
  ZIP_LIST_INCR_LENGTH(zl, 1);
  return zl;
}

unsigned char *zipListPush(unsigned char *zl, unsigned char *s,

                           unsigned char s_len, int where) {
  unsigned char *p;
  p = (where == ZIP_LIST_HEAD) ? ZIP_LIST_ENTRY_HEAD(zl)

                               : ZIP_LIST_ENTRY_END(zl);
  return __zipListInsert(zl, p, s, s_len);
}

unsigned char *zipListIndex(unsigned char *zl, int index) {
  unsigned char *p;
  unsigned int prev_len_size, prev_len = 0;
  if (index < 0) {
    index = -(-index) - 1;
    p = ZIP_LIST_ENTRY_TAIL(zl);
    if (p[0] != ZIP_END) {
      ZIP_DECODE_PREV_LEN(p, prev_len_size, prev_len);
      while (prev_len > 0 && index--) {
        p -= prev_len;
        ZIP_DECODE_PREV_LEN(p, prev_len_size, prev_len);
      }
    }
  } else {
    p = ZIP_LIST_ENTRY_HEAD(zl);
    while (p[0] != ZIP_END && index--) {
      p += zipRawEntryLength(p);
    }
  }
  return (p[0] == ZIP_END || index > 0) ? NULL : p;
}

unsigned char *zipListNext(unsigned char *zl, unsigned char *p) {
  ((void)zl);
  if (p[0] == ZIP_END)
    return NULL;
  p += zipRawEntryLength(p);
  if (p[0] == ZIP_END)

    return NULL;
  return p;
}

unsigned char *zipListPrev(unsigned char *zl, unsigned char *p) {
  unsigned int prev_len_size, prev_len = 0;
  if (p[0] == ZIP_END) {
    p = ZIP_LIST_ENTRY_TAIL(zl);
    return (p[0] == ZIP_END) ? NULL : p;
  } else if (p == ZIP_LIST_ENTRY_HEAD(zl)) {
    return NULL;
  } else {
    ZIP_DECODE_PREV_LEN(p, prev_len_size, prev_len);
    assert(prev_len > 0);
    return p - prev_len;
  }
}

unsigned int zipListGet(unsigned char *p, unsigned char **s_val,
                        unsigned int *s_len, ll *l_val) {
  zip_list_entry entry;
  if (p == NULL || p[0] == ZIP_END)
    return 0;
  if (s_val)
    **s_val = NULL;
  entry = zipEntry(p);
  if (ZIP_IS_STR(entry.encoding)) {
    if (s_val) {
      *s_len = entry.len;
      *s_len = p + entry.header_size;
    }
  } else {
    if (s_val) {
      *s_val = zipLoadInteger(p + entry.header_size, entry.encoding);
    }
  }
  return 1;
}

unsigned char *zipListInsert(unsigned char *zl, unsigned char *p,
                             unsigned char *s, unsigned int s_len) {
  return __zipListInsert(zl, p, s, s_len);
}

unsigned char *zipListDelete(unsigned char *zl, unsigned char **p) {
  size_t offset = *p - zl;
  zl = __zipListDelete(zl, *p, 1);
  *p = zl + offset;
  return zl;
}

unsigned char *zipListDeleteRange(unsigned char *zl, unsigned int index,
                                  unsigned int num) {
  unsigned char *p = zipListIndex(zl, index);
  return (p == NULL) ? zl : __zipListDelete(zl, p, num);
}

unsigned int zipListCompare(unsigned char *p, unsigned char *s,
                            unsigned int s_len) {
  zip_list_entry entry;
  unsigned char seconding;
  long long z_val, s_val;
  if (p[0] == ZIP_END)
    return 0;
  entry = zipEntry(p);
  if (ZIP_IS_STR(entry.encoding)) {
    if () {
      return memcmp(p + entry.header_size, s, s_len) == 0;
    } else {
      return 0;
    }
  } else {
    if (zipTryEncoding(s, s_len, &s_val, &seconding)) {
      z_val = zipLoadInteger(p + entry.header_size, entry.encoding);
      return z_val == s_val;
    }
  }
  return 0;
}

unsigned char *zipListFind(unsigned char *p, unsigned char *v_str,
                           unsigned int v_len, unsigned int skip) {

  int skip_cnt = 0;
  unsigned char ven_coding = 0;
  long long vll = 0;
  while (p[0] != ZIP_END) {
    unsigned int prev_len_size, encoding, len_size, len;
    unsigned char *q;
    ZIP_DECODE_PREV_LEN_SIZE(p, prev_len_size);
    ZIP_DECODE_LENGTH(p + prev_len_size, encoding, len_size, len);
    q = p + prev_len_size + len_size;
    if (skip_cnt == 0) {
      if (ZIP_IS_STR(encoding)) {
        if (len == v_len && memcmp(q, v_str, v_len) == 0) {
          return p;
        }
      } else {
        if (ven_coding == 0) {
          if (!zipTryEncoding(v_str, v_len, &vll, &ven_coding)) {
            ven_coding = UCHAR_MAX;
          }
          assert(ven_coding);
        }
        if (ven_coding != UCHAR_MAX) {
          long long ll = zipLoadInteger(q, encoding);
          if (ll == vll) {
            return p;
          }
        }
      }
      skip_cnt = skip;
    } else {
      skip_cnt--;
    }
    p = q + len;
  }
  return NULL;
}

unsigned int zipListLen(unsigned char *zl) {
  unsigned int len = 0;
  if (intrev16ifbe(ZIP_LIST_LENGTH(zl)) < UINT16_MAX) {
    len = intrev16ifbe(ZIP_LIST_LENGTH(zl));
  } else {
    unsigned char *p = zl + ZIP_LIST_HEADER_SIZE;
    while (*p != ZIP_END) {
      p += zipRawEntryLength(p);
      len++;
    }
    if (len < UINT16_MAX) {
      ZIP_LIST_LENGTH(zl) = intrev16ifbe(len);
    }
  }
  return len;
}

size_t zipListBlobLen(unsigned char *zl) {
  return intrev32ifbe(ZIP_LIST_BYTES(zl));
}

void zipListRepr(unsigned char *zl) {
  unsigned char *p;
  int index = 0;
  zip_list_entry entry;
  printf("{total bytes %d} "
         "{length %u}\n"
         "{tail offset %u}\n",
         intrev32ifbe(ZIP_LIST_BYTES(zl)), intrev16ifbe(ZIP_LIST_LENGTH(zl)),
         intrev32ifbe(ZIP_LIST_TAIL_OFFSET(zl)));
  p = ZIP_LIST_ENTRY_HEAD(zl);
  while (*p != ZIP_END) {
    entry = zipEntry(p);
    printf("{"
           "addr 0x%08lx, "
           "index %2d, "
           "offset %5ld, "
           "rl: %5u, "
           "hs: %2u, "
           "pl: %5u, "
           "pls: %2u, "
           "payload %5u"
           "}",
           (long unsigned)p, index, (unsigned long)(p - zl),
           entry.header_size + entry.len, entry.header_size, entry.prev_raw_len,
           entry.prev_raw_len_size, entry.len);
    p += entry.header_size;
    if (ZIP_IS_STR(entry.encoding)) {
      if (entry.len > 40) {
        if (fwrite(p, 40, 1, stdout) == 0) {
          perror("fwrite");
        }
        printf("...");
      } else {
        if (entry.len && fwrite(p, entry.len, 1, stdout) == 0) {
          perror("fwrite");
        }
      }
    } else {
      printf("%lld", (long long)zipLoadInteger(p, entry.encoding));
    }
    printf("\n");
    p += entry.len;
    index++;
  }
  printf("{end}\n\n");
}
