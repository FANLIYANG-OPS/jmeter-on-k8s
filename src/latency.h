//
// Created by fly on 8/3/23.
//

#ifndef CACHE_1_0_0_LATENCY_H
#define CACHE_1_0_0_LATENCY_H

#include <stdint.h>
#include <time.h>

#define LATENCY_TS_LEN 160

struct latencySample {
  int32_t time;
  uint32_t latency;
};

struct latencyTimeSeries {
  int idx;
  uint32_t max;
  struct latencySample samples[LATENCY_TS_LEN];
};

struct latencyStats {
  uint32_t all_time_high;
  uint32_t avg;
  uint32_t min;
  uint32_t max;
  uint32_t mad;
  uint32_t samples;
  time_t period;
};

void latencyMonitorInit(void);

void latencyAddSample(char *event, ms_time_t latency);

int THPIsEnable(void);

#define latencyStartMonitor(var)          \
  if (server.latency_monitor_threshold) { \
    var = mstime();                       \
  } else {                                \
    var = 0;                              \
  }

#define latencyEndMonitor(var)            \
  if (server.latency_monitor_threshold) { \
    var = mstime() - var;                 \
  }

#define latencyAddSampleIfNeeded(event, var)       \
  if (server.latency_monitor_threshold &&          \
      (var) >= server.latency_monitor_threshold) { \
    latencyAddSample((event), (var));              \
  }

#define latencyRemoveNestedEvent(event_var, nested_var) event_var += nested_var;

#endif  // CACHE_1_0_0_LATENCY_H
