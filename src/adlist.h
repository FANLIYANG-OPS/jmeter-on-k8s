#ifndef __ADLIST_H__
#define __ADLIST_H__

typedef struct ListNode {
    struct ListNode *prev;
    struct ListNode *next;
    void *value;
} ListNode;

typedef struct ListIter {
    ListNode *next;
    int direction;
} ListIter;

typedef struct List {
    ListNode *head;
    ListNode *tail;

    void *(*dup)(void *ptr);

    void (*free)(void *ptr);

    int (*match)(void *ptr, void *key);

    unsigned long len;
} List;

// list
#define listLength(n) ((n)->len)
#define listFirst(n) ((n)->head)
#define listLast(n) ((n)->tail)
#define listSetDupMethod(n, m) ((n)->dup = (m))
#define listSetFreeMethod(n, m) ((n)->free = (m))
#define listSetMatchMethod(n, m) ((n)->match = (m))

// iter
#define listPrevNode(n) ((n)->prev)
#define listNextNode(n) ((n)->next)
#define listNodeValue(n) ((n)->value)

#define listGetDupMethod(n) ((n)->dup)
#define listGetFree(n) ((n)->free)
#define listGetMatchMethod(n) ((n)->match)

List *listCreate(void);

void listRelease(List *l);

List *listAddNodeHead(List *l, void *value);

List *listAddNodeTail(List *l, void *value);

List *listInsertNode(List *l, ListNode *old_node, void *value, int after);

void listDelNode(List *l, ListNode *node);

ListIter *listGetIterator(List *l, int direction);

ListNode *listNext(ListIter *iter);

void listReleaseIterator(ListIter *iter);

List *listDup(List *orig);

ListNode *listSearchKey(List *list, void *key);

ListNode *listIndex(List *l, long index);

void listRewind(List *l, ListIter *iter);

void listRewindTail(List *l, ListIter *iter);

void listRotate(List *l);

#define AL_START_HEAD 0
#define AL_START_TAIL 1

#endif