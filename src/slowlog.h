//
// Created by fly on 9/4/23.
//

#ifndef FLY_SLOWLOG_H
#define FLY_SLOWLOG_H

typedef struct slowlogEntry {
  cobj **argv;
  int argc;
  long long id;
  long long duration;
  time_t time;
} slowlogEntry;

void slowlogInit(void);
void slowlogPushEntryIfNeeded(cobj **argv, int argc, long long duration);
void slowlogCommand(cacheClient *c);


#endif