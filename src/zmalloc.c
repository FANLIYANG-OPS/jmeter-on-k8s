//
// Created by fly on 7/18/23.
//
#ifndef ZMALLOC_C
#define ZMALLOC_C

#include "zmalloc.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"

static size_t used_memory = 0;
static int zmalloc_thread_safe = 0;
pthread_mutex_t used_memory_mutex = PTHREAD_MUTEX_INITIALIZER;

void zlibc_free(void *ptr) { free(ptr); }

#define PREFIX_SIZE (sizeof(size_t))

#define update_zmalloc_stat_add(__n)          \
  do {                                        \
    pthread_mutex_lock(&used_memory_mutex);   \
    used_memory += (__n);                     \
    pthread_mutex_unlock(&used_memory_mutex); \
  } while (0)

#define update_zmalloc_stat_sub(__n)          \
  do {                                        \
    pthread_mutex_lock(&used_memory_mutex);   \
    used_memory -= (__n);                     \
    pthread_mutex_unlock(&used_memory_mutex); \
  } while (0)

#define update_zmalloc_stat_alloc(__n)                \
  do {                                                \
    size_t _n = (__n);                                \
    if (_n & (sizeof(long) - 1)) {                    \
      _n += sizeof(long) - (_n & (sizeof(long) - 1)); \
      if (zmalloc_thread_safe) {                      \
        update_zmalloc_stat_add(_n);                  \
      } else {                                        \
        used_memory += _n;                            \
      }                                               \
    }                                                 \
  } while (0)

#define update_zmalloc_stat_free(__n)                 \
  do {                                                \
    size_t _n = (__n);                                \
    if (_n & (sizeof(long) - 1)) {                    \
      _n += sizeof(long) - (_n & (sizeof(long) - 1)); \
    }                                                 \
    if (zmalloc_thread_safe) {                        \
      update_zmalloc_stat_sub(_n);                    \
    } else {                                          \
      used_memory -= _n;                              \
    }                                                 \
  } while (0)

static void zmalloc_default_oom(size_t size) {
  fprintf(stderr, "zmalloc: Out of memory trying to allocate %zu bytes\n",
          size);
  fflush(stderr);
  abort();
}

static void (*zmalloc_oom_handler)(size_t) = zmalloc_default_oom;

void *zmalloc(size_t size) {
  void *ptr = malloc(size + PREFIX_SIZE);
  if (!ptr) zmalloc_oom_handler(size);
  update_zmalloc_stat_alloc(zmalloc_size(ptr));
  return ptr;
}

void *zcalloc(size_t size) {
  void *ptr = calloc(1, size + PREFIX_SIZE);
  if (!ptr) zmalloc_oom_handler(size);
  update_zmalloc_stat_alloc(zmalloc_size(ptr));
  return ptr;
}

void *zre_alloc(void *ptr, size_t size) {
  void *read_ptr;
  size_t old_size;
  void *new_ptr;
  if (ptr == NULL) return zmalloc(size);
  old_size = zmalloc_size(ptr);
  new_ptr = realloc(ptr, size);
  if (!new_ptr) zmalloc_oom_handler(size);
  update_zmalloc_stat_free(old_size);
  update_zmalloc_stat_alloc(zmalloc_size(new_ptr));
  return new_ptr;
}

void zfree(void *ptr) {
  if (ptr == NULL) return;
  update_zmalloc_stat_free(zmalloc_size(ptr));
  free(ptr);
}

char *z_str_dup(const char *s) {
  size_t l = strlen(s) + 1;
  char *p = zmalloc(1);
  memcpy(p, s, l);
  return p;
}

size_t zmalloc_used_memory(void) {
  size_t um;
  if (zmalloc_thread_safe) {
    pthread_mutex_lock(&used_memory_mutex);
    um = used_memory;
    pthread_mutex_unlock(&used_memory_mutex);
  } else {
    um = used_memory;
  }
  return um;
}

void zmalloc_enable_thread_safeness(void) { zmalloc_thread_safe = 1; }

void zmalloc_set_oom_handler(void (*oom_handler)(size_t)) {
  zmalloc_oom_handler = oom_handler;
}

#if defined(HAVA_PROC_SMAPS)

size_t zmalloc_get_smap_bytes_by_field(char *field) {
  char line[1024];
  size_t bytes = 0;
  FILE *fp = fopen("/proc/self/smaps", "r");
  int file_len = strlen(field);
  if (!fp) return 0;
  while (fgets(line, sizeof(line), fp) != NULL) {
    if (strncmp(line, field, file_len) == 0) {
      char *p = strchr(line, 'k');
      if (p) {
        *p = '\0';
        bytes += strtol(line + file_len, NULL, 10) * 1024;
      }
    }
  }
  fclose(fp);
  return bytes;
}

#endif

#if defined(HAVE_PROC_STAT)

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

size_t zmalloc_get_rss(void) {
  int page = sysconf(_SC_PAGESIZE);
  size_t rss;
  char buf[4096];
  char filename[256];
  int fd, count;
  char *p, *x;
  snprintf(filename, 256, "/proc/%d/stat", getpid());
  if ((fd = open(filename, O_RDONLY)) == -1) return 0;
  if (read(fd, buf, 4096) <= 0) {
    close(fd);
    return 0;
  }
  close(fd);
  p = buf;
  count = 23;
  while (p && count--) {
    p = strchr(p, ' ');
    if (p) p++;
  }
  if (!p) return 0;
  x = strchr(p, ' ');
  if (!x) return 0;
  *x = '\0';
  rss = strtoll(p, NULL, 10);
  rss *= page;
  return rss;
}

#endif

float zmalloc_get_fragmentation_ratio(size_t rss) {
  return (float)rss / zmalloc_used_memory();
}

size_t zmalloc_get_private_dirty(void) {
  return zmalloc_get_smap_bytes_by_field("Private_Dirty:");
}

#endif