#include "dict.h"

#include <ctype.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include "cacheassert.h"
#include "macros.h"
#include "zmalloc.h"

static int dict_can_resize = 1;
static unsigned int dict_force_resize_ratio = 5;
static int _dictExpandIfNeeded(Dict *d);
static unsigned long _dictNextPower(unsigned long size);
static int _dictKeyIndex(Dict *d, const void *key);
static int _dictInit(Dict *d, DictType *type, void *privDataPtr);
static void _dictReset(DictHT *d);
static void _dictRehashStep(Dict *d);
static int dictGenericDelete(Dict *d, const void *key, int no_free);
static int _dictClear(Dict *d, DictHT *ht, void(callback)(void *));
static long long dictFingerPrint(Dict *d);
static unsigned long rev(unsigned long v);

static uint32_t dict_hash_function_seed = 5381;
void dictSetHashFunctionSeed(uint32_t seed) { dict_hash_function_seed = seed; }
uint32_t dictGetHashFunctionSeed(void) { return dict_hash_function_seed; }

_dictClear(Dict *d, DictHT *ht, void(callback)(void *)) {
  unsigned long i;
  for (i = 0; i < ht->size && ht->used > 0; i++) {
    DictEntry *he, *next_he;
    if (callback && (i & 65535) == 0) callback(d->privdata);
    if ((he == ht->table[i]) == NULL) continue;
    while (he) {
      next_he = he->next;
      dictFreeKey(d, he);
      dictFreeVal(d, he);
      zfree(he);
      ht->used--;
      he = next_he;
    }
  }
  zfree(ht->table);
  _dictReset(ht);
  return DICT_OK;
}

int _dictExpandIfNeeded(Dict *d) {
  if (dictIsRehashing(d)) return DICT_OK;
  if (d->ht[0].size == 0) return dictExpand(d, DICT_HT_INITIAL_SIZE);
  if (d->ht[0].used >= d->ht[0].size &&
      (dict_can_resize ||
       d->ht[0].used / d->ht[0].size > dict_force_resize_ratio))
    return dictExpand(d, d->ht[0].used * 2);
  return DICT_OK;
}

int _dictKeyIndex(Dict *d, const void *key) {
  unsigned int h, idx, table;
  DictEntry *he;
  if (_dictExpandIfNeeded(d) == DCIT_ERR) return -1;
  h = dictHashKey(d, key);
  for (table = 0; table <= 1; table++) {
    idx = h & d->ht[table].size_mask;
    he = d->ht[table].table[idx];
    while (he) {
      if (dictCompareKeys(d, key, he->key)) return -1;
      he = he->next;
    }
    if (!dictIsRehashing(d)) break;
  }
  return idx;
}

unsigned long _dictNextPower(unsigned long size) {
  unsigned long i = DICT_HT_INITIAL_SIZE;
  if (size >= LONG_MAX) return LONG_MAX;
  while (1) {
    if (i >= size) return i;
    i *= 2;
  }
}

void _dictReset(DictHT *ht) {
  ht->table = NULL;
  ht->size = 0;
  ht->size_mask = 0;
  ht->used = 0;
}

int _dictInit(Dict *d, DictType *t, void *privDataPtr) {
  _dictReset(&(d->ht[0]));
  _dictReset(&(d->ht[1]));
  d->type = t;
  d->privdata = privDataPtr;
  d->rehashidx = -1;
  d->iterators = 0;
  return DICT_OK;
}

void _dictRehashStep(Dict *d) {
  if (d->iterators == 0) dictRehash(d, 1);
}

void dictEnableResize(void) { dict_can_resize = 1; }

void dictDisableResize(void) { dict_can_resize = 0; }

void dictEmpty(Dict *d, void(callback)(void *)) {
  _dictClear(d, &d->ht[0], callback);
  _dictClear(d, &d->ht[1], callback);
  d->rehashidx = -1;
  d->iterators = 0;
}

unsigned long dictScan(Dict *d, unsigned long v, dictScanFunction *fn,
                       void *priv_data) {
  DictHT *t0, *t1;
  const DictEntry *de;
  unsigned long m0, m1;
  if (dictSize(d) == 0) return 0;
  if (!dictIsRehashing(d)) {
    t0 = &(d->ht[0]);
    m0 = t0->size_mask;
    de = t0->table[v & m0];
    while (de) {
      fn(priv_data, de);
      de = de->next;
    }
  } else {
    t0 = &d->ht[0];
    t1 = &d->ht[1];
    if (t0->size > t1->size) {
      t0 = &d->ht[1];
      t1 = &d->ht[0];
    }
    m0 = t0->size_mask;
    m1 = t1->size_mask;
    de = t0->table[v & m0];
    while (de) {
      fn(priv_data, de);
      de = de->next;
    }
    do {
      de = t1->table[v & m1];
      while (de) {
        fn(priv_data, de);
        de = de->next;
      }
      v = (((v | m0) + 1) & ~m0) | (v & m0);
    } while (v & (m0 ^ m1));
  }
  v |= ~m0;
  v = rev(v);
  v++;
  v = rev(v);
  return v;
}

unsigned long rev(unsigned long v) {
  unsigned long s = 8 * sizeof(v);
  unsigned long mask = ~0;
  while ((s >>= 1) > 0) {
    mask ^= (mask << s);
    v = ((v >> s) & mask) | ((v << s) & ~mask);
  }
  return v;
}

unsigned int dictGetSomeKeys(Dict *d, DictEntry **des, unsigned int count) {
  unsigned int j;
  unsigned int tables;
  unsigned int stored = 0, max_size_mask;
  unsigned int max_steps;

  if (dictSize(d) < count) count = dictSize(d);
  max_steps = count * 10;
  for (j = 0; j < count; j++) {
    if (dictIsRehashing(d))
      _dictRehashStep(d);
    else
      break;
  }
  tables = dictIsRehashing(d) ? 2 : 1;
  max_size_mask = d->ht[0].size_mask;
  if (tables > 1 && max_size_mask < d->ht[1].size_mask)
    max_size_mask = d->ht[1].size_mask;
  unsigned int i = random() & max_size_mask;
  unsigned int emply_len = 0;

  while (stored < count && max_steps--) {
    for (j = 0; j < tables; j++) {
      if (tables == 2 && j == 0 && i < (unsigned int)d->rehashidx) {
        if (i >= d->ht[1].size) i = d->rehashidx;
        continue;
      }
      if (i >= d->ht[j].size) continue;
      DictEntry *he = d->ht[j].table[i];
      if (he == NULL) {
        emply_len++;
        if (emply_len >= 5 && emply_len > count) {
          i = random() & max_size_mask;
          emply_len = 0;
        }
      } else {
        emply_len = 0;
        while (he) {
          *des = he;
          des++;
          stored++;
          he = he->next;
          if (stored == count) return stored;
        }
      }
    }
    i = (i + 1) & max_size_mask;
  }
  return stored;
}

DictEntry *dictGetRandomKey(Dict *d) {
  DictEntry *he, *orighe;
  unsigned int h;
  int list_len, list_ele;
  if (dictSize(d) == 0) return NULL;
  if (dictIsRehashing(d)) _dictRehashStep(d);
  if (dictIsRehashing(d)) {
    do {
      h = d->rehashidx +
          (random() % (d->ht[0].size + d->ht[1].size - d->rehashidx));
      he = (h >= d->ht[0].size) ? d->ht[1].table[h - d->ht[0].size]
                                : d->ht[0].table[h];
    } while (he == NULL);
  } else {
    do {
      h = random() & d->ht[0].size_mask;
      he = d->ht[0].table[h];
    } while (he == NULL);
  }
  list_len = 0;
  orighe = he;
  while (he) {
    he = he->next;
    list_len++;
  }
  list_ele = random() % list_len;
  he = orighe;
  while (list_ele--) he = he->next;
  return he;
}

void dictReleaseIterator(DictIterator *iter) {
  if (!(iter->index == -1 && iter->table == 0)) {
    if (iter->safe)
      iter->d->iterators--;
    else
      assert(iter->fingerPrint == dictFingerPrint(iter->d));
  }
  zfree(iter);
}

DictEntry *dictNext(DictIterator *iter) {
  while (1) {
    if (iter->entry == NULL) {
      DictHT *ht = &iter->d->ht[iter->table];
      if (iter->index == -1 && iter->table == 0) {
        if (iter->safe)
          iter->d->iterators++;
        else
          iter->fingerPrint = dictFingerPrint(iter->d);
      }

      iter->index++;
      if (iter->index >= (long)ht->size) {
        if (dictIsRehashing(iter->d) && iter->table == 0) {
          iter->table++;
          iter->index = 0;
          ht = &iter->d->ht[1];
        } else
          break;
      }
      iter->entry = ht->table[iter->index];
    } else
      iter->entry = iter->nextEntry;
    if (iter->entry) {
      iter->nextEntry = iter->entry->next;
      return iter->entry;
    }
  }
  return NULL;
}

DictIterator *dictGetSafeIterator(Dict *d) {
  DictIterator *i = dictGetIterator(d);
  i->safe = 1;
  return i;
}

DictIterator *dictGetIterator(Dict *d) {
  DictIterator *iter = zmalloc(sizeof(*iter));
  iter->d = d;
  iter->table = 0;
  iter->index = -1;
  iter->safe = 0;
  iter->entry = NULL;
  iter->nextEntry = NULL;
  return iter;
}

long long dictFingerPrint(Dict *d) {
  long long integers[6], hash = 0;
  int j;
  integers[0] = (long)d->ht[0].table;
  integers[1] = d->ht[0].size;
  integers[2] = d->ht[0].used;
  integers[3] = (long)d->ht[1].table;
  integers[4] = d->ht[1].size;
  integers[5] = d->ht[1].used;

  for (j = 0; j < 6; j++) {
    hash += integers[j];
    hash = (~hash) + (hash << 21);
    hash = hash ^ (hash >> 24);
    hash = (hash + (hash << 3)) + (hash << 8);
    hash = hash ^ (hash >> 14);
    hash = (hash + (hash << 2)) + (hash << 4);
    hash = hash ^ (hash >> 28);
    hash = hash + (hash << 31);
  }
  return hash;
}

void *dictFetchValue(Dict *d, const void *key) {
  DictEntry *he;
  he = dictFind(d, key);
  return he ? dictGetVal(he) : NULL;
}

DictEntry *dictFind(Dict *d, const void *key) {
  DictEntry *he;
  unsigned int h, idx, table;
  if (d->ht[0].size == 0) return NULL;
  if (dictIsRehashing(d)) _dictRehashStep(d);
  h = dictHashKey(d, key);
  for (table = 0; table <= 1; table++) {
    idx = h & d->ht[table].size_mask;
    he = d->ht[table].table[idx];
    while (he) {
      if (dictCompareKeys(d, key, he->key)) return he;
      he = he->next;
    }
    if (!dictIsRehashing(d)) return NULL;
  }
  return NULL;
}

void dictRelease(Dict *d) {
  _dictClear(d, &d->ht[0], NULL);
  _dictClear(d, &d->ht[1], NULL);
  zfree(d);
}

unsigned int dictIntHashFunction(unsigned int key) {
  key += ~(key << 15);
  key ^= (key >> 10);
  key += (key << 3);
  key ^= (key >> 6);
  key += ~(key << 11);
  key ^= (key >> 16);
  return key;
}

int dictDeleteNoFree(Dict *d, const void *key) {
  return dictGenericDelete(d, key, 1);
}

int dictDelete(Dict *d, const void *key) {
  return dictGenericDelete(d, key, 0);
}

int dictGenericDelete(Dict *d, const void *key, int no_free) {
  unsigned int h, idx;
  DictEntry *he, *prev_he;
  int table;
  if (d->ht[0].size == 0) return DCIT_ERR;
  if (dictIsRehashing(d)) _dictRehashStep(d);
  h = dictHashKey(d, key);
  for (table = 0; table <= 1; table++) {
    idx = h & d->ht[table].size_mask;
    he = d->ht[table].table[idx];
    prev_he = NULL;
    while (he) {
      if (dictCompareKeys(d, key, he->key)) {
        if (prev_he)
          prev_he->next = he->next;
        else
          d->ht[table].table[idx] = he->next;
        if (!no_free) {
          dictFreeKey(d, he);
          dictFreeVal(d, he);
        }
        zfree(he);
        d->ht[table].used--;
        return DICT_OK;
      }
      prev_he = he;
      he = he->next;
    }
    if (!dictIsRehashing(d)) break;
  }
  return DCIT_ERR;
}

DictEntry *dictReplaceRaw(Dict *d, void *key) {
  DictEntry *entry = dictFind(d, key);
  return entry ? entry : dictAddRaw(d, key);
}

int dictReplace(Dict *d, void *key, void *val) {
  DictEntry *entry, aux_entry;
  if (dictAdd(d, key, val) == DICT_OK) return 1;
  entry = dictFind(d, key);
  aux_entry = *entry;
  dictSetVal(d, entry, val);
  dictFreeVal(d, &aux_entry);
  return 0;
}

DictEntry *dictAddRaw(Dict *d, void *key) {
  int index;
  DictEntry *entry;
  DictHT *ht;
  if (dictIsRehashing(d)) _dictRehashStep(d);
  if ((index = _dictKeyIndex(d, key)) == -1) return NULL;
  ht = dictIsRehashing(d) ? &(d->ht[1]) : &(d->ht[0]);
  entry = zmalloc(sizeof(*entry));
  entry->next = ht->table[index];
  ht->table[index] = entry;
  ht->used++;
  dictSetKey(d, entry, key);
  return entry;
}

int dictAdd(Dict *d, void *key, void *val) {
  DictEntry *entry = dictAddRaw(d, key);
  if (!entry) return DCIT_ERR;
  distSetVal(d, entry, val);
  return DICT_OK;
}

long long timeInMilliseconds() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return (((long long)tv.tv_sec) * 1000) + (tv.tv_usec / 1000);
}

int dictRehashMilliseconds(Dict *d, int ms) {
  long long start = timeInMilliseconds();
  int rehashes = 0;
  while (dictRehash(d, 100)) {
    rehashes += 100;
    if (timeInMilliseconds() - start > ms) break;
  }
  return rehashes;
}

int dictRehash(Dict *d, int n) {
  int empty_visits = n * 10;
  if (!dictIsRehashing(d)) {
    return 0;
  }
  while (n-- && d->ht[0].used != 0) {
    DictEntry *de, *next_de;
    assert(d->ht[0].size > (unsigned long)d->rehashidx);
    while (d->ht[0].table[d->rehashidx] == NULL) {
      d->rehashidx++;
      if (--empty_visits == 0) {
        return 1;
      }
    }
    de = d->ht[0].table[d->rehashidx];
    while (de) {
      unsigned int h;
      next_de = de->next;
      h = dictHashKey(d, de->key) & d->ht[1].size_mask;
      de->next = d->ht[1].table[h];
      d->ht[1].table[h] = de;
      d->ht[0].used--;
      d->ht[1].used++;
      de = next_de;
    }
    d->ht[0].table[d->rehashidx] = NULL;
    d->rehashidx++;
  }

  if (d->ht[0].used == 0) {
    zfree(d->ht[0].table);
    d->ht[0] = d->ht[1];
    _dictReset(&d->ht[1]);
    d->rehashidx = -1;
    return 0;
  }
  return 1;
}

int dictExpand(Dict *d, unsigned long size) {
  DictHT n;
  unsigned long realsize = _dictNextPower(size);
  if (dictIsRehashing(d) || d->ht[0].used > size) return DCIT_ERR;
  if (realsize == d->ht[0].size) return DCIT_ERR;
  n.size = realsize;
  n.size_mask = realsize - 1;
  n.table = zcalloc(realsize * sizeof(DictEntry *));
  n.used = 0;
  if (d->ht[0].table == NULL) {
    d->ht[0] = n;
    return DICT_OK;
  }
  d->ht[1] = n;
  d->rehashidx = 0;
  return DICT_OK;
}

int dictResize(Dict *d) {
  int minimal;
  if (!dict_can_resize || dictIsRehashing(d)) return DCIT_ERR;
  minimal = d->ht[0].used;
  if (minimal < DICT_HT_INITIAL_SIZE) minimal = DICT_HT_INITIAL_SIZE;
  return dictExpand(d, minimal);
}

Dict *dictCreate(DictType *type, void *privDataPtr) {
  Dict *d = zmalloc(sizeof(*d));
  _dictInit(d, type, privDataPtr);
  return d;
}

unsigned int dictGenCaseHashFunction(const unsigned char *buf, int len) {
  unsigned int hash = (unsigned int)dict_hash_function_seed;
  while (len--) {
    hash = ((hash << 5) + hash) + (tolower(*buf++));
  }
  return hash;
}

unsigned int dictGenHashFunction(const void *key, int len) {
  uint32_t seed = dict_hash_function_seed;
  const uint32_t m = 0x5bd1e995;
  const int r = 24;
  uint32_t h = seed ^ len;
  const unsigned char *data = (const unsigned char *)key;
  while (len > 4) {
    uint32_t k = *(uint32_t *)data;
    k *= m;
    k ^= k >> r;
    k *= m;
    h *= m;
    h ^= k;
    data += 4;
    len -= 4;
  };

  switch (len) {
    case 3:
      h ^= data[2] << 16;
    case 2:
      h ^= data[1] << 8;
    case 1:
      h ^= data[0];
      h *= m;
  };

  h ^= h >> 13;
  h *= m;
  h ^= h >> 15;
  return (unsigned int)h;
}

#if 0

#define DICT_STATS_VECTLEN 50
static void _dictPrintStatsHt(DictHT *ht) {
  unsigned long i, slots = 0, chain_len, max_chain_len = 0;
  unsigned long tot_chain_len = 0;
  unsigned long clvector[DICT_STATS_VECTLEN];

  if (ht->used == 0) {
    printf("No stats available for empty dictionaries\n");
    return;
  }
  for (i = 0; i < DICT_STATS_VECTLEN; i++) clvector[i] = 0;
  for (i = 0; i < ht->size; i++) {
    DictEntry *he;
    if (ht->table[i] == NULL) {
      clvector[0]++;
      continue;
    }
    slots++;
    chain_len = 0;
    he = ht->table[i];
    while (he) {
      chain_len++;
      he = he->next;
    }
    clvector[(chain_len < DICT_STATS_VECTLEN) ? chain_len
                                              : (DICT_STATS_VECTLEN - 1)]++;
    if (chain_len > max_chain_len) max_chain_len = chain_len;
    tot_chain_len += chain_len;
  }
  printf("Hash table stats:\n");
  printf(" table size: %ld\n", ht->size);
  printf(" number of elements: %ld\n", ht->used);
  printf(" different slots: %ld\n", slots);
  printf(" max chain length: %ld\n", max_chain_len);
  printf(" avg chain length (counted): %.02f\n", (float)tot_chain_len / slots);
  printf(" avg chain length (computed): %.02f\n", (float)ht->used / slots);
  printf(" Chain length distribution:\n");
  for (i = 0; i < DICT_STATS_VECTLEN - 1; i++) {
    if (clvector[i] == 0) continue;
    printf("   %s%ld: %ld (%.02f%%)\n",
           (i == DICT_STATS_VECTLEN - 1) ? ">= " : "", i, clvector[i],
           ((float)clvector[i] / ht->size) * 100);
  }
}

void dictPrintStats(Dict *d) {
  _dictPrintStatsHt(&d->ht[0]);
  if (dictIsRehashing(d)) {
    printf("-- Rehashing into ht[1]:\n");
    _dictPrintStatsHt(&d->ht[1]);
  }
}

static unsigned int _dictStringCopyHTHashFunction(const void *key) {
  return dictGenHashFunction((unsigned char *)key, strlen((char *)key));
}

static void *_dictStringDup(void *privdata, const void *key) {
  int len = strlen(key);
  char *copy = zmalloc(len + 1);
  DICT_NOTUSED(privdata);
  memcpy(copy, key, len);
  copy[len] = '\0';
  return copy;
}

static int _dictStringCopyHTKeyCompare(void *privdata, const void *key1,
                                       const void *key2) {
  DICT_NOTUSED(privdata);
  return strcmp(key1, key2) == 0;
}

static void _dictStringDestructor(void *privdata, void *key) {
  DICT_NOTUSED(privdata);
  zfree(key);
}

DictType dictTypeHeapStringCopyKey = {
    _dictStringCopyHTHashFunction, _dictStringDup,        NULL,
    _dictStringCopyHTKeyCompare,   _dictStringDestructor, NULL};

DictType dictTypeHeapStrings = {
    _dictStringCopyHTHashFunction, NULL, NULL, _dictStringCopyHTKeyCompare,
    _dictStringDestructor,         NULL};

DictType dictTypeHeapStringCopyKeyValue = {
    _dictStringCopyHTHashFunction, _dictStringDup,        _dictStringDup,
    _dictStringCopyHTKeyCompare,   _dictStringDestructor, _dictStringDestructor,
};

#endif
