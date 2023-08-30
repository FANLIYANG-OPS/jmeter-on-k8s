#define ZIP_LIST_HEAD 0
#define ZIP_LIST_TAIL 1

#include <sys/types.h>

unsigned char *zipListNew(void);

unsigned char *zipListPush(unsigned char *zl, unsigned char *s, unsigned char s_len, int where);

unsigned char *zipListIndex(unsigned char *zl, int index);

unsigned char *zipListNext(unsigned char *zl, unsigned char *p);

unsigned char *zipListPrev(unsigned char *zl, unsigned char *p);

unsigned int zipListGet(unsigned char *p, unsigned char **s_val, unsigned int *s_len, ll *l_val);

unsigned char *zipListInsert(unsigned char *zl, unsigned char *p, unsigned char *s, unsigned int s_len);

unsigned char *zipListDelete(unsigned char *zl, unsigned char **p);

unsigned char *zipListDeleteRange(unsigned char *zl, unsigned int index, unsigned int num);

unsigned int zipListCompare(unsigned char *p, unsigned char *s, unsigned int s_len);

unsigned char *zipListFind(unsigned char *p, unsigned char *v_str, unsigned int v_len, unsigned int skip);

unsigned int zipListLen(unsigned char *zl);

size_t zipListBlobLen(unsigned char *zl);
