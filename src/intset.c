#include "intset.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "const.h"
#include "endianconv.h"
#include "zmalloc.h"

#define INTSET_ENC_INT16 (sizeof(int16_t))
#define INTSET_ENC_INT32 (sizeof(int32_t))
#define INTSET_ENC_INT64 (sizeof(int64_t))

static uint8_t _intsetValueEncoding(int64_t v) {
  if (v < INT32_MIN || v > INT32_MAX)
    return INTSET_ENC_INT64;
  else if (v < INT16_MIN || v > INT16_MAX)
    return INTSET_ENC_INT32;
  else
    return INTSET_ENC_INT16;
}

static int64_t _intsetGetEncoded(intset *is, int pos, uint8_t enc) {

  int64_t v64;
  int32_t v32;
  int16_t v16;

  if (enc == INTSET_ENC_INT64) {
    memcpy(&v64, ((int64_t *)(is->contents)) + pos, sizeof(v64));
    memrev64ifbe(&v64);
    return v64;
  } else if (enc == INTSET_ENC_INT32) {
    memcpy(&v32, ((int32_t *)(is->contents)) + pos, sizeof(v32));
    memrev32ifbe(&v32);
    return v32;
  } else {
    memcpy(&v16, ((int16_t *)(is->contents)) + pos, sizeof(v16));
    memrev16ifbe(&v32);
    return v16;
  }
}

static int64_t _intsetGet(intset *is, int pos) {
  return _intsetGetEncoded(is, pos, intrev32ifbe(is->encodeing));
}

static void _intsetSet(intset *is, int pos, int64_t value) {
  uint32_t encoding = intrev32ifbe(is->encodeing);
  if (encoding == INTSET_ENC_INT64) {
    ((i64 *)is->contents)[pos] = value;
    memrev64ifbe(((i64 *)is->contents) + pos);
  } else if (encoding == INTSET_ENC_INT32) {
    ((i32 *)is->contents)[pos] = value;
    memrev32ifbe((i32 *)is->contents + pos);
  } else {
    ((i16 *)is->contents)[pos] = value;
    memrev16ifbe(((i16 *)is->contents) + pos);
  }
}

intset *intsetNew(void) {
  intset *is = zmalloc(sizeof(intset));
  is->encodeing = intrev32ifbe(INTSET_ENC_INT16);
  is->length = 0;
  return is;
}

static intset *intsetResize(intset *is, u32 len) {
  u32 size = len * intrev32ifbe(is->encodeing);
  is = zre_alloc(is, sizeof(intset) + size);
  return is;
}

static u8 intsetSearch(intset *is, i64 value, u32 *pos) {
  int min = 0, max = intrev32ifbe(is->length) - 1, mid = -1;
  i64 cur = -1;
  if (intrev32ifbe(is->length) == 0) {
    if (pos)
      *pos = 0;
    return 0;
  } else {

    if (value > _intsetGet(is, intrev32ifbe(is->length) - 1)) {
      if (pos)
        *pos = intrev32ifbe(is->length);
      return 0;
    } else if (value < _intsetGet(is, 0)) {
      if (pos)
        *pos = 0;
      return 0;
    }
  }

  while (max >= min) {
    mid = ((unsigned int)min + (unsigned int)max) >> 1;
    cur = _intsetGet(is, mid);
    if (value > cur) {
      min = mid + 1;
    } else if (value < cur) {
      max = mid - 1;
    } else {
      break;
    }
  }

  if (value == cur) {
    if (pos)
      *pos = mid;
    return 1;
  } else {
    if (pos)
      *pos = min;
    return 0;
  }
}

static intset *intsetUpgradeAndAdd(intset *is, i64 value) {
  u8 curenc = intrev32ifbe(is->encodeing);
  u8 newenc = _intsetValueEncoding(value);
  int length = intrev32ifbe(is->length);
  int prepend = value < 0 ? 1 : 0;

  is->encodeing = intrev32ifbe(newenc);
  is = intsetResize(is, intrev32ifbe(is->length) + 1);
  while (length--) {
    _intsetSet(is, length + prepend, _intsetGetEncoded(is, length, curenc));
  }
  if (prepend)
    _intsetSet(is, 0, value);
  else
    _intsetSet(is, intrev32ifbe(is->length), value);
  is->length = intrev32ifbe(intrev32ifbe(is->length) + 1);
  return is;
}

static void intsetMoveTail(intset *is, u32 from, u32 to) {
  void *src, *dst;
  u32 bytes = intrev32ifbe(is->length) - from;
  u32 encoding = intrev32ifbe(is->encodeing);

  if (encoding == INTSET_ENC_INT64) {
    src = (i64 *)is->contents + from;
    dst = (i64 *)is->contents + to;
    bytes *= sizeof(i64);
  } else if (encoding == INTSET_ENC_INT32) {
    src = (i32 *)is->contents + from;
    dst = (i32 *)is->contents + to;
    bytes *= sizeof(i32);
  } else {
    src = (i16 *)is->contents + from;
    dst = (i16 *)is->contents + to;
    bytes *= sizeof(i16);
  }
  memmove(dst, src, bytes);
}

intset *intsetAdd(intset *is, int64_t value, uint8_t *success) {
  u8 va = _intsetValueEncoding(value);
  u32 pos;
  if (success)
    *success = 1;
  if (va > intrev32ifbe(is->encodeing)) {
    return intsetUpgradeAndAdd(is, va);
  } else {
    if (intsetSearch(is, va, &pos)) {
      if (success)
        *success = 0;
      return is;
    }
    is = intsetResize(is, intrev32ifbe(is->length) + 1);
    if (pos < intrev32ifbe(is->length))
      intsetMoveTail(is, pos, pos + 1);
  }
  _intsetSet(is, pos, value);
  is->length = intrev32ifbe(intrev32ifbe(is->length) + 1);
  return is;
}

intset *intsetRemove(intset *is, int64_t value, int *success) {
  u8 va = _intsetValueEncoding(value);
  u32 pos;
  if (success)
    *success = 0;
  if (va <= intrev32ifbe(is->encodeing) && intsetSearch(is, value, &pos)) {
    u32 len = intrev32ifbe(is->length);
    if (success)
      *success = 1;
    if (pos < (len - 1))
      intsetMoveTail(is, pos + 1, pos);
    is = intsetResize(is, len - 1);
    is->length = intrev32ifbe(len - 1);
  }
  return is;
}

uint8_t insertFind(intset *is, int64_t value) {
  u8 va = _intsetValueEncoding(value);
  return va <= intrev32ifbe(is->encodeing) && intsetSearch(is, value, NULL);
}

int64_t insertRandom(intset *is) {
  return _intsetGet(is, rand() % intrev32ifbe(is->length));
}

uint8_t intsetGet(intset *is, uint32_t pos, int64_t *value) {
  if (pos < intrev32ifbe(is->length)) {
    *value = _intsetGet(is, pos);
    return 1;
  }
  return 0;
}

uint32_t intsetLen(intset *is) { return intrev32ifbe(is->length); }

size_t insertBlobLen(intset *is) {
  return sizeof(intset) +
         intrev32ifbe(is->length) * intrev32ifbe(is->encodeing);
}
