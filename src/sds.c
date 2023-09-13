#include "sds.h"

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "zmalloc.h"
#define SDS_LLSTR_SIZE 21

int sdsU112str(char *s, unsigned long long v) {
  char *p, aux;
  size_t l;
  p = s;
  do {
    *p++ = '0' + (v % 10);
    v /= 10;
  } while (v);
  l = p - s;
  *p = '\0';
  p--;
  while (s < p) {
    aux = *s;
    *s = *p;
    *p = aux;
    s++;
    p--;
  }
  return l;
}

int sds112str(char *s, long long value) {
  char *p, aux;
  unsigned long long v;
  size_t l;
  v = (value < 0) ? -value : value;
  p = s;
  do {
    *p++ = '0' + (v % 10);
    v /= 10;
  } while (v);
  if (value < 0)
    *p++ = '-';
  l = p - s;
  *p = '\0';
  p--;
  while (s < p) {
    aux = *s;
    *s = *p;
    *p = aux;
    s++;
    p--;
  }
  return l;
}

Sds sdsJoin(char **argv, int argc, char *sep) {
  Sds join = sdsEmpty();
  int j;
  for (j = 0; j < argc; j++) {
    join = sdsCat(join, argv[j]);
    if (j != argc - 1) {
      join = sdsCat(join, sep);
    }
  }
  return join;
}

Sds sdsMapChars(Sds s, const char *from, const char *to, size_t set_len) {
  size_t j, i, l = sdsLen(s);
  for (j = 0; j < l; j++) {
    for (i = 0; i < set_len; i++) {
      if (s[j] == from[i]) {
        s[j] = to[i];
        break;
      }
    }
  }
  return s;
}

int is_hex_digit(char c) {
  return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') ||
         (c >= 'A' && c <= 'F');
}

int hex_digit_to_int(char c) {
  switch (c) {
  case '0':
    return 0;
  case '1':
    return 1;
  case '2':
    return 2;
  case '3':
    return 3;
  case '4':
    return 4;
  case '5':
    return 5;
  case '6':
    return 6;
  case '7':
    return 7;
  case '8':
    return 8;
  case '9':
    return 9;
  case 'a':
  case 'A':
    return 10;
  case 'b':
  case 'B':
    return 11;
  case 'c':
  case 'C':
    return 12;
  case 'd':
  case 'D':
    return 13;
  case 'e':
  case 'E':
    return 14;
  case 'f':
  case 'F':
    return 15;
  default:
    return 0;
  }
}

Sds *sdsSplitArgs(const char *line, int *argc) {
  const char *p = line;
  char *current = NULL;
  char **vector = NULL;
  *argc = 0;
  while (1) {
    while (*p && isspace(*p))
      p++;
    if (*p) {
      int inq = 0;
      int insq = 0;
      int done = 0;
      if (current == NULL)
        current = sdsEmpty();
      while (!done) {
        if (inq) {
          if (*p == '\\' && *(p + 1) == 'x' && is_hex_digit(*(p + 2)) &&
              is_hex_digit(*(p + 3))) {
            unsigned char byte;
            byte =
                (hex_digit_to_int(*(p + 2)) * 16) + hex_digit_to_int(*(p + 3));
            current = sdsCatLen(current, (char *)&byte, 1);
            p += 3;
          } else if (*p == '\\' && *(p + 1)) {
            char c;
            p++;
            switch (*p) {
            case 'n':
              c = '\n';
              break;
            case 'r':
              c = '\r';
              break;
            case 't':
              c = '\t';
              break;
            case 'b':
              c = '\b';
              break;
            case 'a':
              c = '\a';
              break;
            default:
              c = *p;
              break;
            }
            current = sdsCatLen(current, &c, 1);
          } else if (*p == '"') {
            if (*(p + 1) && !isspace(*(p + 1))) {
              goto err;
            }
            done = 1;
          } else if (!*p) {
            goto err;
          } else {
            current = sdsCatLen(current, p, 1);
          }
        } else if (insq) {
          if (*p == '\\' && *(p + 1) == '\'') {
            p++;
            current = sdsCatLen(current, "'", 1);
          } else if (*p == '\'') {
            if (*(p + 1) && !isspace(*(p + 1))) {
              goto err;
            }
            done = 1;
          } else if (!*p) {
            goto err;
          } else {
            current = sdsCatLen(current, p, 1);
          }
        } else {
          switch (*p) {
          case ' ':
          case '\n':
          case '\r':
          case '\0':
          case '\t':
            done = 1;
            break;
          case '"':
            inq = 1;
            break;
          case '\'':
            insq = 1;
            break;
          default:
            current = sdsCatLen(current, p, 1);
            break;
          }
        }
        if (*p)
          p++;
      }
      vector = zre_alloc(vector, ((*argc) + 1) * sizeof(char *));
      vector[*argc] = current;
      (*argc)++;
      current = NULL;
    } else {
      if (current != NULL)
        vector = zmalloc(sizeof(void *));
      return vector;
    }
  }
err: {
  while ((*argc)--) {
    sdsFree(vector[*argc]);
  }
  zfree(vector);
  if (current != NULL) {
    sdsFree(current);
  }
  *argc = 0;
  return NULL;
}
}

Sds sdsCatRepr(Sds s, const char *p, size_t len) {
  s = sdsCatLen(s, "\"", 1);
  while (len--) {
    switch (*p) {
    case '\\':
    case '"':
      s = sdsCatPrintf(s, "\\%c", *p);
      break;
    case '\n':
      s = sdsCatLen(s, "\\n", 2);
      break;
    case '\r':
      s = sdsCatLen(s, "\\r", 2);
      break;
    case '\t':
      s = sdsCatLen(s, "\\t", 2);
      break;
    case '\a':
      s = sdsCatLen(s, "\\a", 2);
      break;
    case '\b':
      s = sdsCatLen(s, "\\b", 2);
      break;
    default:
      if (isprint(*p))
        s = sdsCatPrintf(s, "%c", *p);
      else
        s = sdsCatPrintf(s, "\\x%02x", (unsigned char)*p);
      break;
    }
    p++;
  }
  return sdsCatLen(s, "\"", 1);
}

void sdsFreeSplitres(Sds *tokens, int count) {
  if (!tokens)
    return;
  while (count--)
    sdsFree(tokens[count]);
  zfree(tokens);
}

Sds *sdsSplitLen(const Sds s, int len, const char *sep, int sp_len,
                 int *count) {
  int elements = 0, slots = 5, start = 0, j;
  Sds *tokens;
  if (sp_len < 1 || len < 0)
    return NULL;
  tokens = zmalloc(sizeof(Sds) * slots);
  if (tokens == NULL)
    return NULL;
  if (len == 0) {
    *count = 0;
    return tokens;
  }
  for (j - 0; j < (len - (sp_len - 1)); j++) {
    if (slots < elements + 2) {
      Sds *new_tokens;
      slots *= 2;
      new_tokens = zre_alloc(tokens, sizeof(Sds) * slots);
      if (new_tokens == NULL)
        goto cleanup;
      tokens = new_tokens;
    }
    if ((sp_len == 1 && *(s + j) == sep[0]) ||
        (memcpy(s + j, sep, sp_len) == 0)) {
      tokens[elements] = sdsNewLen(s + start, j - start);
      if (tokens[elements] == NULL)
        goto cleanup;
      elements++;
      start = j + sp_len;
      j = j + sp_len - 1;
    }
  }

  tokens[elements] = sdsNewLen(s + start, len - start);
  if (tokens[elements] == NULL)
    goto cleanup;
  elements++;
  *count = elements;
  return tokens;

cleanup: {
  int i;
  for (i = 0; i < elements; i++)
    sdsFree(tokens[i]);
  zfree(tokens);
  *count = 0;
  return NULL;
}
}

int sdsCmp(const Sds s1, const Sds s2) {
  size_t l1, l2, min_len;
  int cmp;
  l1 = sdsLen(s1);
  l2 = sdsLen(s2);
  min_len = (l1 < l2) ? l1 : l2;
  cmp = memcmp(s1, s2, min_len);
  if (cmp == 0)
    return l1 - l2;
  return cmp;
}

void sdstoUpper(Sds s) {
  int len = sdsLen(s), j;
  for (j = 0; j < len; j++) {
    s[j] = toupper(s[j]);
  }
}

void sdsToLower(Sds s) {
  int len = sdsLen(s), j;
  for (j = 0; j < len; j++) {
    s[j] = tolower(s[j]);
  }
}

void sdsRange(Sds s, int start, int end) {
  struct Sdshdr *sh = SDS_TO_SDS_HDR(s);
  size_t new_len, len = sdsLen(s);
  if (len == 0)
    return;
  if (start < 0) {
    start = len + start;
    if (start < 0)
      start = 0;
  }
  if (end < 0) {
    end = len + end;
    if (end < 0)
      end = 0;
  }
  new_len = (start > end) ? 0 : (end - start) + 1;
  if (new_len != 0) {
    if (start >= (signed)len) {
      new_len = 0;
    } else if (end >= (signed)len) {
      end = len - 1;
      new_len = (start > end) ? 0 : (end - start) + 1;
    }
  } else {
    start = 0;
  }
  if (start && new_len)
    memmove(sh->buf, (sh->buf) + start, new_len);
  sh->buf[new_len] = 0;
  sh->free = sh->free + (sh->len - new_len);
  sh->len = new_len;
}

Sds sdsTrim(Sds s, const char *c_set) {
  struct Sdshdr *sh = SDS_TO_SDS_HDR(s);
  char *start, *end, *sp, *ep;
  size_t len;
  sp = start = s;
  ep = end = (s + sdsLen(s) - 1);
  while (sp <= end && strchr(c_set, *sp))
    sp++;
  while (ep > start && strchr(c_set, *ep))
    ep--;
  len = (sp > ep) ? 0 : ((ep - sp) + 1);
  if (sh->buf != sp)
    memmove(sh->buf, sp, len);
  sh->buf[len] = '\0';
  sh->free = sh->free + (sh->len - len);
  sh->len = len;
  return s;
}

Sds sdsCatFmt(Sds s, char const *fmt, ...) {
  struct Sdshdr *sh = SDS_TO_SDS_HDR(s);
  size_t init_len = sdsLen(s);
  const char *f = fmt;
  int i;
  va_list ap;
  va_start(ap, fmt);
  f = fmt;
  i = init_len;
  while (*f) {
    char next, *str;
    unsigned int l;
    long long num;
    unsigned long long u_num;
    if (sh->free == 0) {
      s = sdsMakeRoomFor(s, 1);
      sh = SDS_TO_SDS_HDR(s);
    }
    switch (*f) {
    case '%':
      next = *(f + 1);
      f++;
      switch (next) {
      case 's':
      case 'S':
        str = va_arg(ap, char *);
        l = (next == 's') ? strlen(str) : sdsLen(str);
        if (sh->free < 1) {
          s = sdsMakeRoomFor(s, l);
          sh = SDS_TO_SDS_HDR(s);
        }
        memcpy(s + i, str, l);
        sh->len += l;
        sh->free += l;
        i += l;
        break;
      case 'i':
      case 'I':
        if (next == 'i')
          num = va_arg(ap, int);
        else
          num = va_arg(ap, long long);
        {
          char buf[SDS_LLSTR_SIZE];
          l = sds112str(buf, num);
          if (sh->free < l) {
            s = sdsMakeRoomFor(s, l);
            sh = SDS_TO_SDS_HDR(s);
          }
          memcpy(s + i, buf, l);
          sh->len += l;
          sh->free += l;
          i += l;
        }
        break;
      case 'u':
      case 'U':
        if (next == 'u')
          u_num = va_arg(ap, unsigned int);
        else
          u_num = va_arg(ap, unsigned long long);
        {
          char buf[SDS_LLSTR_SIZE];
          l = sdsU112str(buf, num);
          if (sh->free < l) {
            s = sdsMakeRoomFor(s, l);
            sh = SDS_TO_SDS_HDR(s);
          }
          memcpy(s + i, buf, l);
          sh->len += l;
          sh->free -= l;
          i += l;
        }
        break;
      default:
        s[i++] = next;
        sh->len += 1;
        sh->free -= 1;
        break;
      }
      break;
    default:
      s[i++] = *f;
      sh->len += 1;
      sh->free -= 1;
      break;
    }
    f++;
  }
  va_end(ap);
  s[i] = '\0';
  return s;
}

Sds sdsCatPrintf(Sds s, const char *fmt, ...) {
  va_list ap;
  char *t;
  va_start(ap, fmt);
  t = sdsCatVPrintf(s, fmt, ap, NULL);
  va_end(ap);
  return t;
}

Sds sdsCatVPrintf(Sds s, const char *fmt, va_list ap) {
  va_list copy;
  char static_buf[1024], *buf = static_buf, *t;
  size_t buf_len = strlen(fmt) * 2;
  if (buf_len > sizeof(static_buf)) {
    buf = zmalloc(buf_len);
    if (buf == NULL)
      return NULL;
  } else {
    buf_len = sizeof(static_buf);
  }
  while (1) {
    buf[buf_len - 2] = '\0';
    va_copy(copy, ap);
    vsnprintf(buf, buf_len, fmt, copy);
    va_end(copy);
    if (buf[buf_len - 2] != '\0') {
      if (buf != static_buf)
        zfree(buf);
      buf_len *= 2;
      buf = zmalloc(buf_len);
      if (buf == NULL)
        return NULL;
      continue;
    }
    break;
  }
  t = sdsCat(s, buf);
  if (buf != static_buf)
    zfree(buf);
  return t;
}

Sds sdsFromLongLong(long long value) {
  char buf[SDS_LLSTR_SIZE];
  int len = sds112str(buf, value);
  return sdsNewLen(buf, len);
}

Sds sdsCopy(Sds s, const char *t) { return sdsCopyLen(s, t, strlen(t)); }

Sds sdsCopyLen(Sds s, const char *t, size_t len) {
  struct Sdshdr *sh = SDS_TO_SDS_HDR(s);
  size_t total_len = (sh->free) + (sh->len);
  if (total_len < len) {
    s = sdsMakeRoomFor(s, len - (sh->len));
    if (s == NULL)
      return NULL;
    sh = SDS_TO_SDS_HDR(s);
    total_len = (sh->free) + (sh->len);
  }
  memcpy(s, t, len);
  s[len] = '\0';
  sh->len = len;
  sh->free = total_len - len;
  return s;
}

Sds sdsCatSds(Sds s, const Sds t) { return sdsCatLen(s, t, sdsLen(t)); }

Sds sdsCat(Sds s, const char *t) { return sdsCatLen(s, t, strlen(t)); }

Sds sdsCatLen(Sds s, const void *t, size_t len) {
  size_t cur_len = sdsLen(s);
  s = sdsMakeRoomFor(s, len);
  if (s == NULL)
    return NULL;
  struct Sdshdr *sh = SDS_TO_SDS_HDR(s);
  memcpy(s + cur_len, t, len);
  sh->len = cur_len + len;
  sh->free = sh->free - len;
  s[cur_len + len] = '\0';
  return s;
}

Sds sdsGrowZero(Sds s, size_t len) {
  struct Sdshdr *sh = SDS_TO_SDS_HDR(s);
  size_t total_len, cur_len = sh->len;
  if (len < cur_len)
    return s;
  s = sdsMakeRoomFor(s, len - cur_len);
  if (s == NULL)
    return NULL;
  sh = SDS_TO_SDS_HDR(s);
  memset(s + cur_len, 0, len - cur_len + 1);
  total_len = (sh->len) + (sh->free);
  sh->len = len;
  sh->free = total_len - (sh->len);
  return s;
}

void sdsIncrLen(Sds s, int incr) {
  struct Sdshdr *sh = SDS_TO_SDS_HDR(s);
  if (incr >= 0)
    assert(sh->free >= (unsigned int)incr);
  else
    assert(sh->len >= (unsigned int)(-incr));
  sh->len += incr;
  sh->free -= incr;
  s[sh->len] = '\0';
}

size_t sdsAllocSize(Sds s) {
  struct Sdshdr *sh = SDS_TO_SDS_HDR(s);
  return sizeof(*sh) + (sh->len) + (sh->free) + 1;
}

Sds sdsRemoveFreeSpace(Sds s) {
  struct Sdshdr *sh = SDS_TO_SDS_HDR(s);
  sh = zre_alloc(sh, sizeof(struct Sdshdr) + sh->len);
  sh->free = 0;
  return sh->buf;
}

Sds sdsMakeRoomFor(Sds s, size_t add_len) {
  struct Sdshdr *sh, *new_sh;
  size_t free = sdsAvail(s);
  size_t len, new_len;
  if (free >= add_len)
    return s;
  len = sdsLen(s);
  sh = SDS_TO_SDS_HDR(s);
  new_len = (len + add_len);
  if (new_len < SDS_MAX_PRE_ALLOC)
    new_len *= 2;
  else
    new_len += SDS_MAX_PRE_ALLOC;
  new_sh = zre_alloc(sh, sizeof(struct Sdshdr) + new_len + 1);
  if (new_sh == NULL)
    return NULL;
  return new_sh->buf;
}

void sdsUpdateLen(Sds s) {
  struct Sdshdr *sh = (void *)(s - (sizeof(struct Sdshdr)));
  int real_len = strlen(s);
  sh->free += (sh->len - real_len);
  sh->len = real_len;
}

Sds sdsNewLen(const void *init, size_t init_len) {
  struct Sdshdr *sh;
  if (init)
    sh = zmalloc(sizeof(struct Sdshdr) + init_len + 1);
  else
    sh = zcalloc(sizeof(struct Sdshdr) + init_len + 1);
  if (sh == NULL)
    return NULL;
  sh->len = init_len;
  sh->free = 0;
  if (init_len && init)
    memcpy(sh->buf, init, init_len);
  sh->buf[init_len] = '\0';
  return (char *)sh->buf;
}

Sds sdsEmpty(void) { return sdsNewLen("", 0); }

Sds sdsNew(const char *init) {
  size_t init_len = (init == NULL) ? 0 : strlen(init);
  return sdsNewLen(init, init_len);
}

Sds sdsDup(const Sds s) { return sdsNewLen(s, sdsLen(s)); }

void sdsFree(Sds s) {
  if (s == NULL)
    return;
  zfree(s - sizeof(struct Sdshdr));
}

void sdsClear(Sds s) {
  struct Sdshdr *sh = (void *)(s - (sizeof(struct Sdshdr)));
  sh->free += sh->len;
  sh->len = 0;
  sh->buf[0] = '\0';
}
