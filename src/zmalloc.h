#ifndef ZMALLOC_H
#define ZMALLOC_H

#include <jemalloc/jemalloc.h>

#define HAVE_MALLOC_SIZE 1

void *zmalloc(size_t size);

void *zcalloc(size_t size);

void *zre_alloc(void *ptr, size_t size);

void zfree(void *ptr);

char *z_str_dup(const char *s);

size_t zmalloc_used_memory(void);

void zmalloc_enable_thread_safeness(void);

void zmalloc_set_oom_handler(void (*oom_handler)(size_t));

float zmalloc_get_fragmentation_ratio(size_t rss);

size_t zmalloc_get_rss(void);

size_t zmalloc_get_private_dirty(void);

size_t zmalloc_get_smap_bytes_by_field(char *field);

void zlibc_free(void *ptr);

#define zmalloc_size(p) je_malloc_usable_size(p)

#endif
