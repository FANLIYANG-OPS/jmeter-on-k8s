#ifndef INTSET_H

#define INTSET_H

#include <stdint.h>
#include <sys/types.h>

typedef struct intset {
    uint32_t encodeing;
    uint32_t length;
    int8_t contents[];
} intset;

intset *intsetNew(void);

intset *intsetAdd(intset *is, int64_t value, uint8_t *success);

intset *intsetRemove(intset *is, int64_t value, int *success);

uint8_t insertFind(intset *is, int64_t value);

int64_t insertRandom(intset *is);

uint8_t intsetGet(intset *is, uint32_t pos, int64_t *value);

uint32_t intsetLen(intset *is);

size_t insertBlobLen(intset *is);

#endif