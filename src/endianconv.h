#ifndef ENDIANCONV_H
#define ENDIANCONV_H

#include <stdint.h>

#include "config.h"

void memrev16(void *p);
void memrev32(void *p);
void memrev64(void *p);

uint16_t intrev16(uint16_t v);
uint32_t intrev32(uint32_t v);
uint64_t intrev64(uint64_t v);

#define memrev16ifbe(p)
#define memrev32ifbe(p)
#define memrev64ifbe(p)
#define intrev16ifbe(v) (v)
#define intrev32ifbe(v) (v)
#define intrev64ifbe(v) (v)

#define htonu64(v) intrev64(v)
#define ntonu64(v) intrev64(v)

#endif
