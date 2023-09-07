//
// Created by fly on 9/4/23.
//

#ifndef FLY_BIO_H
#define FLY_BIO_H

void bioInit(void);
void bioCreateBackgroundJob(int type, void *arg1, void *arg2, void *arg3);
unsigned long long bioPendingJobsOfType(int type);
void bioWaitPendingJobsLE(int type, unsigned long long num);
time_t bioOlderJobType(int type);
void bioKillThreads(void);

#define CACHE_BIO_CLOSE_FILE 0
#define CACHE_BIO_AOF_FSYNC 1
#define CACHE_BIO_NUM_OPS 2

#endif