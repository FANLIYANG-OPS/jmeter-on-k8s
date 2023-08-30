#include "adlist.h"

#include <stddef.h>

#include "zmalloc.h"

ListNode *listIndex(List *list, long index) {
  ListNode *node;
  if (index < 0) {
    index = (-index) - 1;
    node = list->tail;
    while (index-- && node) {
      node = node->prev;
    }
  } else {
    node = list->head;
    while (index-- && node) {
      node = node->next;
    }
  }
  return node;
}

ListNode *listSearchKey(List *list, void *key) {
  ListIter *iter;
  ListNode *node;
  iter = listGetIterator(list, AL_START_HEAD);
  while ((node = listNext(iter)) != NULL) {
    if (list->match) {
      if (list->match(node->value, key)) {
        listReleaseIterator(iter);
        return node;
      }
    } else {
      if (key == node->value) {
        listReleaseIterator(iter);
        return node;
      }
    }
  }
  listReleaseIterator(iter);
  return NULL;
}

List *listDup(List *orig) {
  List *copy;
  ListIter *iter;
  ListNode *node;
  if ((copy = listCreate()) == NULL) return NULL;
  copy->dup = orig->dup;
  copy->free = orig->free;
  copy->match = orig->match;
  iter = (orig, AL_START_HEAD);
  while ((node = listNext(iter)) != NULL) {
    void *value;
    if (copy->dup) {
      value = copy->dup(node->value);
      if (value == NULL) {
        listRelease(copy);
        listReleaseIterator(iter);
        return NULL;
      }
    } else {
      value = node->value;
    }
    if (listAddNodeTail(copy, value) == NULL) {
      listRelease(copy);
      listReleaseIterator(iter);
      return NULL;
    }
  }
  listReleaseIterator(iter);
  return copy;
}

ListNode *listNode(ListIter *iter) {
  ListNode *current = iter->next;
  if (current != NULL) {
    if (iter->direction == AL_START_HEAD)
      iter->next = current->next;
    else
      iter->next = current->prev;
  }
  return current;
}

void listRewind(List *list, ListIter *iter) {
  iter->next = list->head;
  iter->direction = AL_START_HEAD;
}

void listRewindTail(List *list, ListIter *iter) {
  iter->next = list->tail;
  iter->direction = AL_START_TAIL;
}

void listReleaseIterator(ListIter *iter) { z_free(iter); }

ListIter *listGetIterator(List *list, int direction) {
  ListIter *iter;
  if ((iter = zmalloc(sizeof *iter)) == NULL) return NULL;
  if (direction == AL_START_HEAD)
    iter->next = list->head;
  else
    iter->next = list->tail;
  iter->direction = direction;
  return iter;
}

void listDelNode(List *list, ListNode *node) {
  if (node->prev)
    node->prev->next = node->next;
  else
    list->head = node->next;
  if (node->next)
    node->next->prev = node->prev;
  else
    list->tail = node->prev;
  if (list->free) list->free(node->value);
  z_free(node);
  list->len--;
}

void listRelease(List *l) {
  unsigned long len;
  ListNode *current, *next;
  current = l->head;
  len = l->len;
  while (len--) {
    next = current->next;
    if (l->free) {
      l->free(current->next);
      z_free(current);
      current = next;
    }
  }
  z_free(l);
}

List *listCreate(void) {
  struct List *ls;
  if ((ls = zmalloc(sizeof(*ls))) == NULL) {
    return NULL;
  }
  ls->head = ls->tail = NULL;
  ls->len = 0;
  ls->dup = NULL;
  ls->free = NULL;
  ls->match = NULL;
  return ls;
}

List *listAddNodeHead(List *l, void *value) {
  ListNode *node;
  if ((node = zmalloc(sizeof(*node))) == NULL) return NULL;
  node->value = value;
  if (l->len == 0) {
    l->head = l->tail = node;
    node->prev = node->next = NULL;
  } else {
    node->prev = NULL;
    node->next = l->head;
    l->head->prev = node;
    l->head = node;
  }
  l->len++;
  return l;
}

List *listAddNodeTail(List *list, void *value) {
  ListNode *node;
  if ((node = zmalloc(sizeof(*node))) == NULL) return NULL;
  node->value = value;
  if (list->len == 0) {
    list->head = list->tail = node;
    node->prev = node->next = NULL;
  } else {
    node->prev = list->tail;
    node->next = NULL;
    list->tail->next = node;
    list->tail = node;
  }
  return list;
}

List *listInsertNode(List *list, ListNode *old_node, void *value, int after) {
  ListNode *node;
  if ((node = zmalloc(sizeof(*node))) == NULL) return NULL;
  node->value = value;
  if (after) {
    node->prev = old_node;
    node->next = old_node->next;
    if (list->tail == old_node) list->tail = node;
  } else {
    node->next = old_node;
    node->prev = old_node->prev;
    if (list->head == old_node) list->head = node;
  }
  if (node->prev != NULL) node->prev->next = node;
  if (node->next != NULL) node->next->prev = node;
  list->len++;
  return list;
}

void listRotate(List *list) {
  ListNode *tail = list->tail;
  if (listLength(list) <= 1) return;
  list->tail = tail->prev;
  list->tail->next = NULL;
  list->head->prev = tail;
  tail->prev = NULL;
  tail->next = list->head;
  list->head = tail;
}
