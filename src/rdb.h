#ifndef CACHE_RDB_H
#define CACHE_RDB_H

#include <stdio.h>

#include "cache.h"
#include "rio.h"

#define CACHE_RDB_VERSION 6
#define CACHE_RDB_6BITLEN 0
#define CACHE_RDB_14BITLEN 1
#define CACHE_RDB_32BITLEN 2
#define CACHE_RDB_ENCVAL 3
#define CACHE_RDB_LENERR UINT_MAX

#define CACHE_RDB_ENC_INT8 0
#define CACHE_RDB_ENC_INT16 1
#define CACHE_RDB_ENC_INT32 2
#define CACHE_RDB_ENC_LZF 3

#define CACHE_RDB_TYPE_STRING 0
#define CACHE_RDB_TYPE_LIST 1
#define CACHE_RDB_TYPE_SET 2
#define CACHE_RDB_TYPE_ZSET 3
#define CACHE_RDB_TYPE_HASH 4

#define CACHE_RDB_TYPE_HASH_ZIPMAP 9
#define CACHE_RDB_TYPE_LIST_ZIPLIST 10
#define CACHE_RDB_TYPE_SET_INTSET 11
#define CACHE_RDB_TYPE_ZSET_ZIPLIST 12
#define CACHE_RDB_TYPE_HASH_ZIPLIST 13

#define rdbIsObjectType(t) ((t >= 0 && t <= 4) || (t >= 9 && t <= 13))
#define CACHE_RDB_OPCODE_EXPIRETIME_MS 252
#define CACHE_RDB_OPCODE_EXPIRETIME 253
#define CACHE_RDB_OPCODE_SELECTDB 254
#define CACHE_RDB_OPCODE_EOF 255

int rdbSaveType(rio *rdb, unsigned char type);
int rdbLoadType(rio *rdb);
int rdbSaveTime(rio *rdb, time_t t);
time_t rdbLoadTime(rio *rdb);
int rdbSaveLen(rio *rdb, uint32_t len);
uint32_t rdbLoadLen(rio *rdb, int *isencoded);
int rdbSaveObjectType(rio *rdb, cobj *o);
int rdbLoadObjectType(rio *rdb);
int rdbLoad(char *filename);
int rdbSaveBackground(char *filename);
int rdbSaveToSlavesSockets(void);
void rdbRemoveTempFile(pid_t childpid);
int rdbSave(char *filename);
int rdbSaveObject(rio *rdb, cobj *o);

off_t rdbSaveObjectLen(cobj *o);
off_t rdbSavedObjectPages(cobj *o);
cobj *rdbLoadObject(int type, rio *rdb);
void backgroundSaveDoneHandler(int exitcode, int bysignal);
int rdbSaveKeyValuePair(rio *rdb, cobj *key, cobj *val, long long expiretime,
                        long long now);
cobj *rdbLoadStringObject(rio *rdb);

#endif