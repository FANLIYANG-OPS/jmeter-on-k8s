#ifndef DICT_H
#define DICT_H

#include <stdint.h>

#define DICT_OK 0
#define DCIT_ERR 1

#define DICT_NOTUSED(V) ((void)V)

typedef struct DictEntry {
    void *key;
    union {
        void *val;
        uint64_t u64;
        int64_t s64;
        double d;
    } v;
    struct DictEntry *next;
} DictEntry;

typedef struct DictType {
    unsigned int (*hashFunction)(const void *key);

    void *(*keyDup)(void *privdata, const void *key);

    void *(*valDup)(void *privdata, const void *obj);

    int (*keyCompare)(void *privdata, const void *key1, const void *key2);

    void (*keyDestructor)(void *privdata, void *key);

    void (*valDestructor)(void *privdata, void *obj);
} DictType;

typedef struct DictHT {
    DictEntry **table;
    unsigned long size;
    unsigned long size_mask;
    unsigned long used;
} DictHT;

typedef struct Dict {
    DictType *type;
    void *privdata;
    DictHT ht[2];
    long rehashidx;
    int iterators;
} Dict;

typedef struct DictIterator {
    Dict *d;
    long index;
    int table, safe;
    DictEntry *entry, *nextEntry;
    long long fingerPrint;
} DictIterator;

typedef void(dictScanFunction)(void *privdata, const DictEntry *de);

#define DICT_HT_INITIAL_SIZE 4

#define dictFreeVal(d, entry)                                \
  if ((d)->type->valDestructor) {                            \
    (d)->type->valDestructor((d)->privdata, (entry)->v.val); \
  }

#define distSetVal(d, entry, _val_)                           \
  do {                                                        \
    if ((d)->type->valDup) {                                  \
      entry->v.val = (d)->type->valDup((d)->privdata, _val_); \
    } else {                                                  \
      entry->v.val = (_val_);                                 \
    }                                                         \
  } while (0)

#define dictSetSignedIntegerVal(entry, _val_) \
  do {                                        \
    entry->v.s64 = _val_;                     \
  } while (0)

#define dictSetUnsignedIntergerVal(entry, _val_) \
  do {                                           \
    entry->v.u64 = _val_                         \
  } while (0)

#define dictSetDoubleVal(entry, _val_) \
  do {                                 \
    entry->v.d = _val_;                \
  } while (0)

#define dictFreeKey(d, entry)                              \
  if ((d)->type->keyDestructor) {                          \
    (d)->type->keyDestructor((d)->privdata, (entry)->key); \
  }

#define dictSetKey(d, entry, _key_)                         \
  do {                                                      \
    if ((d)->type->keyDup) {                                \
      entry->key = (d)->type->keyDup((d)->privdata, _key_); \
    } else {                                                \
      entry->key = (_key_);                                 \
    }                                                       \
  } while (0)

#define dictCompareKeys(d, key1, key2)                                        \
  (((d)->type->keyCompare) ? (d)->type->keyCompare((d)->privdata, key1, key2) \
                           : (key1) == (key2))

#define dictHashKey(d, key) (d)->type->hashFunction(key)
#define dictGetKey(he) ((he)->key)
#define dictGetVal(he) ((he)->v.val)
#define dictGetSignedIntegerVal(he) ((he)->v.s64)
#define dictGetUnsugnedIntergerVal(he) ((he)->v.u64)
#define dictGetDoubleVal(he) ((he)->v.d)
#define dictSlots(args) ((args)->ht[0].size + (args)->ht[1].size)
#define dictSize(args) ((args)->ht[0].used + (args)->ht[1].used)
#define dictIsRehashing(args) ((args)->rehashidx != -1)

extern DictType dictTypeHeapStringCopyKey;
extern DictType dictTypeHeapStrings;
extern DictType dictTypeHeapStringCopyKeyValue;

Dict *dictCreate(DictType *type, void *privDataPtr);

int dictExpand(Dict *d, unsigned long size);

int dictAdd(Dict *d, void *key, void *val);

DictEntry *dictAddRaw(Dict *d, void *key);

int dictReplace(Dict *d, void *key, void *val);

DictEntry *dictReplaceRaw(Dict *d, void *key);

int dictDelete(Dict *d, const void *key);

int dictDeleteNoFree(Dict *d, const void *key);

void dictRelease(Dict *d);

DictEntry *dictFind(Dict *d, const void *key);

void *dictFetchValue(Dict *d, const void *key);

int dictResize(Dict *d);

DictIterator *dictGetIterator(Dict *d);

DictIterator *dictGetSafeIterator(Dict *d);

void dictReleaseIterator(DictIterator *iter);

DictEntry *dictNext(DictIterator *iter);

DictEntry *dictGetRandomKey(Dict *d);

unsigned int dictGetSomeKeys(Dict *d, DictEntry **des, unsigned int count);

void dictPrintStats(Dict *d);

unsigned int dictGenHashFunction(const void *key, int len);

unsigned int dictGenCaseHashFunction(const unsigned char *buf, int len);

void dictEmpty(Dict *d, void(callback)(void *));

void dictEnableResize(void);

void dictDisableResize(void);

int dictRehash(Dict *d, int n);

long long timeInMilliseconds();

int dictRehashMilliseconds(Dict *d, int ms);

void dictSetHashFunctionSeed(uint32_t init_val);

unsigned int dictGetHashFunctionSeed(void);

unsigned long dictScan(Dict *d, unsigned long v, dictScanFunction *fn,
                       void *priv_data);

#endif