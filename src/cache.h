//
// Created by fly on 8/2/23.
//

#ifndef CACHE_1_0_0_CACHE_H
#define CACHE_1_0_0_CACHE_H

#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <lua.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>

#include "config.h"
#include "macros.h"
#include "solarisfixes.h"

typedef long long ms_time_t;

#include "adlist.h"
#include "ae.h"
#include "anet.h"
#include "dict.h"
#include "intset.h"
#include "latency.h"
#include "sds.h"
#include "sparkline.h"
#include "util.h"
#include "version.h"
#include "ziplist.h"
#include "zmalloc.h"

#define CACHE_OK 0
#define CACHE_ERR -1

#define CACHE_DEFAULT_HZ 10
#define CACHE_MIN_HZ 1
#define CACHE_MAX_HZ 500
#define CACHE_SERVER_PORT 6379
#define CACHE_TCP_BACKLOG 511
#define CACHE_MAXIDLETIME 0
#define CACHE_DEFAULT_DBNUM 16
#define CACHE_CONFIGLINE_MAX 1024
#define CACHE_DBCRON_DBS_PER_CALL 16
#define CACHE_MAX_WRITE_PER_EVENT (1024 * 64)
#define CACHE_SHARED_SELECT_CMDS 10
#define CACHE_SHARED_INTEGERS 10000
#define CACHE_SHARED_BULKHDR_LEN 32
#define CACHE_MAX_LOGMSG_LEN 1024
#define CACHE_AOF_REWRITE_PERC 100
#define CACHE_AOF_REWRITE_MIN_SIZE (64 * 1024 * 1024)
#define CACHE_AOF_REWRITE_ITEMS_PER_CMD 64
#define CACHE_SLOWLOG_LOG_SLOWER_THAN 10000
#define CACHE_SLOWLOG_MAX_LEN 128
#define CACHE_MAX_CLIENTS 10000
#define CACHE_AUTHPASS_MAX_LEN 512
#define CACHE_DEFAULT_SLAVE_PRIORITY 100
#define CACHE_REPL_TIMEOUT 60
#define CACHE_REPL_PING_SLAVE_PERIOD 10
#define CACHE_RUN_ID_SIZE 40
#define CACHE_EOF_MARK_SIZE 40
#define CACHE_DEFAULT_REPL_BACKLOG_SIZE (1024 * 1024)
#define CACHE_DEFAULT_REPL_BACKLOG_TIME_LIMIT (60 * 60)
#define CACHE_REPL_BACKLOG_MIN_SIZE (1024 * 16)
#define CACHE_BGSAVE_RETRY_DELAY 5
#define CACHE_DEFAULT_PID_FILE "/var/run/cache.pid"
#define CACHE_DEFAULT_SYSLOG_IDENT "cache"
#define CACHE_DEFAULT_CLUSTER_CONFIG_FILE "nodes.conf"
#define CACHE_DEFAULT_DAEMONIZE 0
#define CACHE_DEFAULT_UNIX_SOCKET_PERM 0
#define CACHE_DEFAULT_TCP_KEEPALIVE 0
#define CACHE_DEFAULT_LOGFILE ""
#define CACHE_DEFAULT_SYSLOG_ENABLED 0
#define CACHE_DEFAULT_STOP_WRITES_ON_BGSAVE_ERROR 1
#define CACHE_DEFAULT_RDB_COMPRESSION 1
#define CACHE_DEFAULT_RDB_CHECKSUM 1
#define CACHE_DEFAULT_RDB_FILENAME "dump.rdb"
#define CACHE_DEFAULT_REPL_DISKLESS_SYNC 0
#define CACHE_DEFAULT_REPL_DISKLESS_SYNC_DELAY 5
#define CACHE_DEFAULT_SLAVE_SERVER_STALE_DATA 1
#define CACHE_DEFAULT_SLAVE_READ_ONLY 1
#define CACHE_DEFAULT_REPL_DISABLE_TCP_NODELAY 0
#define CACHE_DEFAULT_MAXMEMORY 0
#define CACHE_DEFAULT_MAXMEMORY_SAMPLES 5
#define CACHE_DEFAULT_AOF_FILENAME "appendonly.aof"
#define CACHE_DEFAULT_AOF_NO_FSYNC_ON_REWRITE 0
#define CACHE_DEFAULT_AOF_LOAD_TRUNCATED 1
#define CACHE_DEFAULT_ACTIVE_REHASHING 1
#define CACHE_DEFAULT_AOF_REWRITE_INCREMENTAL_FSYNC 1
#define CACHE_DEFAULT_MIN_SLAVES_TO_WRITE 0
#define CACHE_DEFAULT_MIN_SLAVES_MAX_LAG 10
#define CACHE_IP_STR_LEN 46
#define CACHE_PEER_ID_LEN (CACHE_IP_STR_LEN + 32)
#define CACHE_BINDADDR_MAX 16
#define CACHE_MIN_RESERVED_FDS 32
#define CACHE_DEFAULT_LATENCY_MONITOR_THRESHOLD 0
#define ACTIVE_EXPIRE_CYCLE_LOOKUPS_PER_LOOP 20
#define ACTIVE_EXPIRE_CYCLE_FAST_DURATION 1000
#define ACTIVE_EXPIRE_CYCLE_SLOW_TIME_PERC 25
#define ACTIVE_EXPIRE_CYCLE_SLOW 0
#define ACTIVE_EXPIRE_CYCLE_FAST 1
#define CACHE_METRIC_SAMPLES 16
#define CACHE_METRIC_COMMAND 0
#define CACHE_METRIC_NET_INPUT 1
#define CACHE_METRIC_NET_OUTPUT 2
#define CACHE_METRIC_COUNT 3
#define CACHE_MAX_QUERYBUF_LEN (1024 * 1024 * 1024)
#define CACHE_IOBUF_LEN (1024 * 16)
#define CACHE_REPLY_CHUNK_BYTES (16 * 1024)
#define CACHE_INLINE_MAX_SIZE (1024 * 16)
#define CACHE_MBULK_BIG_ARG (1024 * 32)
#define CACHE_LONGSTR_SIZE 21
#define CACHE_AOF_AUTOSYNC_BYTES (1024 * 1024 * 32)
#define CACHE_EVENTLOOP_FDSET_INCR (CACHE_MIN_RESERVED_FDS + 96)
#define CACHE_HT_MINFILL 10
#define CACHE_CMD_WRITE 1
#define CACHE_CMD_READONLY 2
#define CACHE_CMD_DENYOOM 4
#define CACHE_CMD_NOT_USED_1 8
#define CACHE_CMD_ADMIN 16
#define CACHE_CMD_PUBSUB 32
#define CACHE_CMD_NOSCRIPT 64
#define CACHE_CMD_RANDOM 128
#define CACHE_CMD_SORT_FOR_SCRIPT 256
#define CACHE_CMD_SORT_LOADING 512
#define CACHE_CMD_STALE 1024
#define CACHE_CMD_SKIP_MONITOR 2048
#define CACHE_CMD_ASKING 4096
#define CACHE_CMD_FAST 9192
#define CACHE_STRING 0
#define CACHE_LIST 1
#define CACHE_SET 2
#define CACHE_ZSET 3
#define CACHE_HASH 4
#define CACHE_ENCODING_RAW 0
#define CACHE_ENCODING_INT 1
#define CACHE_ENCODING_HT 2
#define CACHE_ENCODING_ZIPMAP 3
#define CACHE_ENCODING_LINKEDLIST 4
#define CACHE_ENCODING_ZIPLIST 5
#define CACHE_ENCODING_INTSET 6
#define CACHE_ENCODING_SKIPLIST 7
#define CACHE_ENCODING_EMBSTR 8
#define CACHE_RDB_6BITLEN 0
#define CACHE_RDB_14BITLEN 1
#define CACHE_RDB_32BITLEN 2
#define CACHE_RDB_ENCVAL 3
#define CACHE_RDB_LENERR UINT_MAX
#define CACHE_RDB_ENC_INT8 0
#define CACHE_RDB_ENC_INT16 1
#define CACHE_RDB_ENC_INT32 2
#define CACHE_RDB_ENC_LZF 3
#define CACHE_AOF_OFF 0
#define CACHE_AOF_ON 1
#define CACHE_AOF_WAIT_REWRITE 2

#define CACHE_SLAVE (1 << 0)
#define CACHE_MASTER (1 << 1)
#define CACHE_MONITOR (1 << 2)
#define CACHE_MULTI (1 << 3)
#define CAHCE_BLOCKED (1 << 4)
#define CACHE_DIRTY_CAS (1 << 5)
#define CACHE_CLOSE_AFTER_REPLY (1 << 6)
#define CACHE_UNBLOCKED (1 << 7)

#define CAHCE_LUA_CLIENT (1 << 8)
#define CACHE_ASKING (1 << 9)
#define CACHE_CLOSE_ASAP (1 << 10)
#define CACHE_UNIX_SOCKET (1 << 11)
#define CACHE_DIRTY_EXEC (1 << 12)
#define CACHE_MASTER_FORCE_REPLY (1 << 13)
#define CACHE_FORCE_AOF (1 << 14)
#define CACHE_FORCE_REPL (1 << 15)
#define CACHE_PRE_PSYNC (1 << 16)
#define CACHE_READONLY (1 << 17)
#define CACHE_PUBSUB (1 << 18)

#define CACKE_BLOCKED_NONE 0
#define CACHE_BLOCKED_LIST 1
#define CACHE_BLOCKED_WAIT 2

#define CACHE_REQ_INLINE 1
#define CACHE_REQ_MULTIBULK 2

#define CACHE_CLIENT_TYPE_NORMAL 0
#define CACHE_CLIENT_TYPE_SLAVE 1
#define CACHE_CLIENT_TYPE_PUBSUB 2
#define CACHE_CLIENT_TYPE_COUNT 4

#define CACHE_REPL_NONE 0
#define CACHE_REPL_CONNECT 1
#define CACHE_REPL_CONNECTING 2
#define CACHE_REPL_RECEIVE_PONG 3
#define CACHE_REPL_TRANSFER 4
#define CACHE_REPL_CONNECTED 5
#define CACHE_REPL_WAIT_BGSAVE_START 6
#define CACHE_REPL_WAIT_BGSAVE_END 7
#define CACHE_REPL_SEND_BULK 8
#define CACHE_REPL_ONLINE 9
#define CACHE_REPL_SYNCIO_TIMEOUT 5
#define CACHE_HEAD 0
#define CACHE_TAIL 1

#define CACHE_SORT_GET 0
#define CACHE_SORT_ASC 1
#define CACHE_SORT_DESC 2
#define CACHE_SORTKEY_MAX 1024

#define CACHE_DEBUG 0
#define CACHE_VERBOSE 1
#define CACHE_NOTICE 2
#define CACHE_WARNING 3
#define CACHE_LOG_RAW (1 << 10)
#define CACHE_DEFAULT_VERBOSITY CACHE_NOTICE

#define CACHE_NOTUSED(V) ((void)V)
#define ZSKIPLIST_MAXLEVEL 32
#define ZSKIPLIST_P 0.25
#define AOF_FSYNC_NO 0
#define AOF_FSYNC_ALWAYS 1
#define AOF_FSYNC_EVERYSEC 2
#define CACHE_DEFAULT_AOF_FSYNC AOF_FSYNC_EVERYSEC

#define CACHE_HASH_MAX_ZIPLIST_ENTRIES 512
#define CACHE_HASH_MAX_ZIPLIST_VALUE 64
#define CACHE_LIST_MAX_ZIPLIST_ENTRIES 512
#define CACHE_LIST_MAX_ZIPLIST_VALUE 64
#define CACHE_SET_MAX_INTSET_ENTRIES 512
#define CACHE_ZSET_MAX_ZIPLIST_ENTRIES 128
#define CACHE_ZSET_MAX_ZIPLIST_VALUE 64
#define CACHE_DEFAULT_HLL_SPARSE_MAX_BYTES 3000
#define CACHE_OP_UNION 0
#define CACHE_OP_DIFF 1
#define CACHE_OP_INTER 2

#define CACHE_MAXMEMORY_VOLATILE_LRU 0
#define CACHE_MAXMEMORY_VOLATILE_TTL 1
#define CACHE_MAXMEMORY_VOLATILE_RANDOM 2
#define CACHE_MAXMEMORY_ALLKEYS_LRU 3
#define CACHE_MAXMEMORY_ALLKEYS_RANDOM 4
#define CACHE_MAXMEMORY_NO_EVICTION 5
#define CACHE_DEFAULT_MAXMEMORY_POLICY CACHE_MAXMEMORY_NO_EVICTION
#define CACHE_LUA_TIME_LIMIT 5000
#define UNIT_SECONDS 0
#define UNIT_MILLISECONDS 1
#define CHACHE_SHUTDOWN_SAVE 1
#define CHACHE_SHUTDOWN_NOSAVE 2
#define CACHE_CALL_NONE 0
#define CACHE_CALL_SLOWLOG 1
#define CACHE_CALL_STATS 2
#define CACHE_CALL_PROPAGATE 4
#define CACHE_CALL_FULL \
  (CACHE_CALL_SLOWLOG | CACHE_CALL_STATS | CACHE_CALL_PROPAGATE)

#define CACHE_PROPAGATE_NONE 0
#define CACHE_PROPAGATE_AOF 1
#define CACHE_PROPAGATE_REPL 2

#define CACHE_RDB_CHILD_TYPE_NONE 0
#define CACHE_RDB_CHILD_TYPE_DISK 1
#define CACHE_RDB_CHILD_TYPE_SOCKET 2
#define CACHE_NOTIFY_KEYSPACE (1 << 0)
#define CACHE_NOTIFY_KEYEVENT (1 << 1)
#define CACHE_NOTIFY_GENERIC (1 << 2)
#define CACHE_NOTIFY_STRING (1 << 3)
#define CACHE_NOTIFY_LIST (1 << 4)
#define CACHE_NOTIFY_SET (1 << 5)
#define CACHE_NOTIFY_HASH (1 << 6)
#define CACHE_NOTIFY_ZSET (1 << 7)
#define CACHE_NOTIFY_EXPIRED (1 << 8)
#define CACHE_NOTIFY_EVICTED (1 << 9)
#define CACHE_NOTIFY_ALL                                            \
  (CACHE_NOTIFY_GENERIC | CACHE_NOTIFY_STRING | CACHE_NOTIFY_LIST | \
   CACHE_NOTIFY_SET | CACHE_NOTIFY_HASH | CACHE_NOTIFY_ZSET |       \
   CACHE_NOTIFY_EXPIRED | CACHE_NOTIFY_EVICTED)
#define CACHE_BIND_ADDR (server.bindaddr_count ? server.bindaddr[0] : NULL)
#define run_with_period(_ms_)       \
  if ((_ms_ <= 1000 / server.hz) || \
      !(server.cronloops % ((_ms_) / (1000 / server.hz))))
#define cacheAssertWithInfo(_c, _o, _e) \
  ((_e) ? (void)0                       \
        : (_cacheAssertWithInfo(_c, _o, #_e, __FILE__, __LINE__), _exit(1)))
#define cacheAssert(_e) \
  ((_e) ? (void)0 : (_cacheAssert(#_e, __FILE__, __LINE__), _exit(1)))
#define cachePanic(_e) _cachePanic(#_e, __FILE__, __LINE__), _exit(1)
#define CACHE_LRU_BITS 24
#define CACHE_LRU_CLOCK_MAX ((1 << CACHE_LRU_BITS) - 1)
#define CACHE_LRU_CLOCK_RESOLUTION 1000
typedef struct cacheObject {
  unsigned type : 4;
  unsigned encoding : 4;
  unsigned lru : CACHE_LRU_BITS;
  int refcount;
  void *ptr;
} cobj;

#define LRU_CLOCK()                                                   \
  ((1000 / server.hz <= CACHE_LRU_CLOCK_RESOLUTION) ? server.lruclock \
                                                    : getLRUClock())

#define initStaticStringObject(_var, _ptr) \
  do {                                     \
    _var.refcount = 1;                     \
    _var.type = CACHE_STRING;              \
    _var.encoding = CACHE_ENCODING_RAW;    \
    _var.ptr = _ptr;                       \
  } while (0);

#define CACHE_EVICTION_POOL_SIZE 16
struct evictionPoolEntry {
  unsigned long long idle;
  Sds *key;
};

typedef struct cacheDB {
  Dict *dict;
  Dict *expires;
  Dict *blocking_keys;
  Dict *ready_keys;
  Dict *watched_keys;
  int id;
  long long avg_ttl;
  struct evictionPoolEntry *eviction_pool;
} cacheDB;

typedef struct multiCmd {
  cobj **argv;
  int argc;
  struct cacheCommand *cmd;
} multiCmd;

typedef struct multiState {
  multiCmd *commands;
  int count;
  int minreplicas;
  time_t minreplicas_timeout;
} multiState;

typedef struct blockingState {
  ms_time_t timeout;
  Dict *keys;
  cobj *target;
  int numreplicas;
  long long reploffset;
} blockingState;

typedef struct readyList {
  cacheDB *db;
  cobj *key;
} readyList;

typedef struct cacheClient {
  uint64_t id;
  int fd;
  cacheDB *db;
  int dictid;
  cobj *name;
  Sds querybuf;
  size_t querybuf_peak;
  int argc;
  cobj **argv;
  struct cacheCommand *cmd, *lastcmd;
  int reqtype;
  int multibulklen;
  long bulklen;
  List *reply;
  unsigned long reply_bytes;
  int sentlen;
  time_t ctime;
  time_t lastinteraction;
  time_t obuf_soft_limit_reached_time;
  int flags;
  int authenticated;
  int replstate;
  int repl_put_online_on_ack;
  int repldbfd;
  off_t repldboff;
  off_t repldbsize;
  Sds replpreamble;
  long long reploff;
  long long repl_ack_off;
  long long repl_ack_time;
  char replrunid[CACHE_RUN_ID_SIZE + 1];
  int slave_listening_port;
  multiState mstate;
  int btype;
  blockingState bpop;
  long long woff;
  List *watched_keys;
  Dict *pubsub_channels;
  List *pubsub_patterns;
  Sds peerid;
  int bufpos;
  char buf[CACHE_REPLY_CHUNK_BYTES];
} cacheClient;

struct saveparam {
  time_t seconds;
  int changes;
};

struct sharedObjectStruct {
  cobj *crlf, *ok, *err, *emptybulk, *czero, *cone, *cnegone, *pong, *space,
      *colon, *nullbulk, *nullmultibulk, *queued, *emptymultibulk,
      *wrongtypeerr, *nokeyerr, *syntaxerr, *sameobjecterr, *outofrangeerr,
      *noscripterr, *loadingerr, *slowscripterr, *bgsaveerr, *masterdownerr,
      *roslaveerr, *execaborterr, *noautherr, *noreplicaserr, *busykeyerr,
      *oomerr, *plus, *messagebulk, *pmessagebulk, *subscribebulk,
      *unsubscribebulk, *psubscribebulk, *punsubscribebulk, *del, *rpop, *lpop,
      *lpush, *emptyscan, *minstring, *maxstring,
      *select[CACHE_SHARED_SELECT_CMDS], *integers[CACHE_SHARED_INTEGERS],
      *mbulkhdr[CACHE_SHARED_BULKHDR_LEN], *bulkhdr[CACHE_SHARED_BULKHDR_LEN];
};

typedef struct zskiplistNode {
  cobj *obj;
  double score;
  struct zskiplistNode *backward;
  struct zskiplistLevel {
    struct zskiplistNode *forward;
    unsigned int span;
  } level[];

} zskiplistNode;

typedef struct zskiplist {
  struct zskiplistNode *header, *tail;
  unsigned long length;
  int level;
} zskiplist;

typedef struct zset {
  Dict *dict;
  zskiplist *zsl;
} zset;

typedef struct clientBufferLimitsConfig {
  unsigned long long hard_limit_bytes;
  unsigned long long soft_limit_bytes;
  time_t soft_limit_seconds;
} clientBufferLimitsConfig;

extern clientBufferLimitsConfig
    clientBufferLimitsDefaults[CACHE_CLIENT_TYPE_COUNT];

typedef struct cacheOP {
  cobj **argv;
  int argc, db_id, target;
  struct cacheCommand *cmd;
} cacheOP;

typedef struct cacheOPArray {
  cacheOP *ops;
  int num_ops;
} cacheOPArray;

struct clusterState;
#ifdef _AIX
#undef hz
#endif

struct cacheServer {
  pid_t pid;
  char *configfile;
  int hz;
  cacheDB *db;
  Dict *commands;
  Dict *orig_commands;
  aeEventLoop *el;
  unsigned lruclock : CACHE_LRU_BITS;
  int shutdown_asap;
  int activerehashing;
  char *requirepass;
  char *pidfile;
  int arch_bits;
  int cronloops;
  char runid[CACHE_RUN_ID_SIZE + 1];
  int sentinel_mode;
  int port;
  int tcp_backlog;
  char *bindaddr[CACHE_BINDADDR_MAX];
  int bindaddr_count;
  char *unixsocket;
  mode_t unixsocketperm;
  int ipfd[CACHE_BINDADDR_MAX];
  int ipfd_count;
  int sofd;
  int cfd[CACHE_BINDADDR_MAX];
  int cfd_count;
  List *clients;
  List *client_to_close;
  List *slaves, *monitors;
  cacheClient *current_client;
  int clients_paused;
  ms_time_t clients_pause_end_time;
  char neterr[ANET_ERR_LEN];
  Dict *migrate_cached_sockets;
  uint64_t next_client_id;
  int loading;
  off_t loading_total_bytes;
  off_t loading_loaded_bytes;
  time_t loading_start_time;
  off_t loading_process_events_interval_bytes;
  struct cacheCommand *delCommand, *multiCommand, *lpushCommand, *lpopCommand,
      *rpopCommand;
  time_t stat_starttime;
  long long stat_numcommands;
  long long stat_numconnections;
  long long stat_expiredkeys;
  long long stat_evictedkeys;
  long long stat_keyspace_hits;
  long long stat_keyspace_misses;
  size_t stat_peak_memory;
  long long stat_fork_time;
  double start_fork_rate;
  long long start_rejected_connections;
  long long stat_sync_full;
  long long stat_sync_partial_ok;
  long long stat_sync_partial_err;
  List *slowlog;
  long long slowlog_entry_id;
  long long slowlog_log_slower_than;
  unsigned long slowlog_max_len;
  size_t cacheident_set_size;
  long long stat_net_input_bytes;
  long long stat_net_output_bytes;
  struct {
    long long last_sample_time;
    long long last_sample_count;
    long long samples[CACHE_METRIC_SAMPLES];
    int idx;
  } inst_metric[CACHE_METRIC_COUNT];
  int verbosity;
  int maxidletime;
  int tcpkeepalive;
  int active_expire_enabled;
  size_t client_max_querybuf_len;
  int dbnum;
  int daemonize;
  clientBufferLimitsConfig client_obuf_limits[CACHE_CLIENT_TYPE_COUNT];
  int aof_state;
  int aof_fsync;
  char *aof_filename;
  int aof_no_fsync_on_rewrite;
  int aof_rewrite_perc;
  off_t aof_rewrite_min_size;
  off_t aof_rewrite_base_size;
  off_t aof_current_size;
  int aof_rewrite_scheduled;
  pid_t aof_child_pid;
  List *aof_rewrite_buf_blocks;
  Sds aof_buf;
  int aof_fd;
  int aof_selected_db;
  time_t aof_flush_postponed_start;
  int aof_last_fsync;
  time_t aof_rewrite_time_last;
  time_t aof_rewrite_time_start;
  int aof_lastbgrewrite_status;
  unsigned long apf_delayed_fsync;
  int aof_rewrite_incremental_fsync;
  int aof_last_write_status;
  int aof_last_write_errno;
  int aof_load_truncated;
  int aof_pipe_write_data_to_child;
  int aof_pipe_read_data_from_parent;
  int aof_pipe_write_ack_to_parent;
  int aof_pipe_write_ack_to_child;
  int aof_pipe_read_ack_from_child;
  int aof_pipe_read_ack_from_parent;
  int aof_stop_sending_diff;
  Sds aof_child_diff;
  long long dirty;
  long long dirty_before_bgsave;
  pid_t rdb_chiled_pid;
  struct saveparam *saveparams;
  int saveparamslen;
  char *rdb_filename;
  int rdb_compression;
  int rdb_checksum;
  time_t lastsave;
  time_t lastbgsave_try;
  time_t rdb_save_time_last;
  time_t rdb_save_time_start;
  int rdb_child_type;
  int lastbgsave_status;
  int stop_writes_on_bgsave_err;
  int rdb_pipe_write_result_to_parent;
  int rdb_pipe_read_result_from_child;
  cacheOPArray also_propagate;
  char *logfile;
  int syslog_enabled;
  char *syslog_ident;
  int syslog_facility;
  int slaveseldb;
  long long master_repl_offset;
  int repl_ping_slave_period;
  char *repl_backlog;
  long long repl_backlog_size;
  long long repl_backlog_histlen;
  long long repl_backlog_idx;
  long long repl_backlog_off;
  time_t repl_backlog_time_limit;
  time_t repl_no_slaves_since;
  int repl_min_slaves_to_write;
  int repl_min_slaves_max_lag;
  int repl_good_slaves_count;
  int repl_diskless_sync;
  int repl_diskless_sync_delay;
  char *masterauth;
  char *masterhost;
  int masterport;
  int repl_timeout;
  cacheClient *master;
  cacheClient *cached_master;
  int repl_syncio_timeout;
  int repl_state;
  off_t repl_transfer_size;
  off_t repl_transfer_read;
  off_t repl_transfer_last_fsync_off;
  int repl_transfer_s;
  char *repl_transfer_fd;
  char *repl_transfer_tmpfile;
  time_t repl_transfer_lastio;
  int repl_serve_stale_data;
  int repl_slave_ro;
  time_t repl_down_since;
  int repl_disable_tcp_nodelay;
  int slave_priority;
  char repl_master_runid[CACHE_RUN_ID_SIZE + 1];
  long long repl_master_initial_offset;
  Dict *repl_scriptcache_dict;
  List *repl_scriptcache_fifo;
  unsigned int repl_scriptcache_size;
  List *clients_waiting_acks;
  int get_ack_from_slaves;
  unsigned int maxclients;
  unsigned long long maxmemory;
  int maxmemory_policy;
  int maxmemory_samples;
  unsigned int bpop_blocked_clients;
  List *unblocked_clients;
  List *ready_keys;
  int sort_desc;
  int sort_alpha;
  int sort_bypattern;
  int sort_store;

  size_t hash_max_ziplist_entries;
  size_t hash_max_ziplist_value;
  size_t list_max_ziplist_entries;
  size_t list_max_ziplist_value;
  size_t set_max_intset_entries;
  size_t zset_max_ziplist_entries;
  size_t zset_max_ziplist_value;
  size_t hll_sparse_max_bytes;
  time_t unixtime;
  long long mstime;
  Dict *pubsub_channels;
  List *pubsub_patterns;
  int notify_keyspace_events;
  int cluster_enabled;
  ms_time_t cluster_node_timeout;
  char *cluster_configfile;
  struct clusterState *cluster;
  int cluster_migration_barrier;
  int cluster_slave_validity_factor;
  int cluster_require_full_coverage;
  lua_State *lua;
  cacheClient *lua_client;
  cacheClient *lua_caller;
  Dict *lua_scripts;
  ms_time_t lua_time_limit;
  ms_time_t lua_time_start;
  int lua_write_dirty;
  int lua_random_dirty;
  int lua_timeout;
  int lua_kill;
  long long latency_monitor_threshold;
  Dict *latency_events;
  char *assert_failed;
  char *assert_file;
  int assert_line;
  int bug_report_start;
  int watchdog_period;
};

typedef struct pubsubPattern {
  cacheClient *client;
  cobj *pattern;
} pubsubPattern;

typedef void cacheCommandProc(cacheClient *c);

typedef int *cacheGetKeysProc(struct cacheCommand *cmd, cobj **argv, int argc,
                              int *num_keys);

struct cacheCommand {
  char *name;
  cacheCommandProc *proc;
  int arity;
  char *sflags;
  int flags;
  cacheGetKeysProc *getkeys_proc;
  int firstkey;
  int lastkey;
  int keystep;
  long long microseconds, calls;
};

struct cacheFunctionSym {
  char *name;
  unsigned long pointer;
};

typedef struct _cacheSortObject {
  cobj *obj;
  union {
    double score;
    cobj *cmp_obj;
  } u;
} cacheSortObject;

typedef struct _redisSortOperation {
  int type;
  cobj *pattern;
} redisSortOperation;

typedef struct {
  cobj *subject;
  unsigned char encoding;
  unsigned char direction;
  unsigned char *zi;
  ListNode *ln;
} listTypeIterator;

typedef struct {
  listTypeIterator *li;
  unsigned char *zi;
  ListNode *ln;
} listTypeEntry;

typedef struct {
  cobj *subject;
  int encoding;
  int ii;
  DictIterator *di;
} setTypeIterator;

typedef struct {
  cobj *subject;
  int encoding;
  unsigned char *fptr, *vptr;
  DictIterator *di;
  DictEntry *de;
} hashTypeIterator;

#define CACHE_HASH_KEY 1
#define CACHE_HASH_VALUE 2

extern struct cacheServer server;
extern struct sharedObjectStruct shared;
extern DictType setDictType;
extern DictType zsetDictType;
extern DictType clusterNodesDictType;
extern DictType clusterNodesBlackListDictType;
extern DictType dbDictType;
extern DictType shaScriptObjectDictType;
extern double R_Zero, R_PosInf, R_NegInf, R_Nan;
extern DictType hashDictType;
extern DictType replScriptCacheDictType;

long long ustime(void);

long long mstime(void);

void getRandomHexChars(char *p, unsigned int len);

uint64_t crc64(uint64_t crc, const unsigned char *s, uint64_t l);

void exitFromChild(int retcode);

size_t cachePopcount(void *s, long count);

void cacheSetProcTitle(char *title);

cacheClient *createClient(int fd);

void closeTimedoutClients(void);

void freeClient(cacheClient *c);

void freeClientAsync(cacheClient *c);

void resetClient(cacheClient *c);

void sendReplyToClient(aeEventLoop *el, int fd, void *privdata, int mask);

void *addDeferredMultiBulkLength(cacheClient *c);

void setDeferredMultiBulkLength(cacheClient *c, void *node, long length);

void processInputBuffer(cacheClient *c);

void acceptHandler(aeEventLoop *el, int fd, void *privdata, int mask);

void acceptTcpHandler(aeEventLoop *el, int fd, void *privdata, int mask);

void acceptUnixHandler(aeEventLoop *el, int fd, void *privdata, int mask);

void readQueryFromClient(aeEventLoop *el, int fd, void *privdata, int mask);

void addReplyBulk(cacheClient *c, cobj *obj);

void addReplyBulkCString(cacheClient *c, char *s);

void addReplyBulkCBuffer(cacheClient *c, void *p, size_t len);

void addReplyBulkLongLong(cacheClient *c, long long ll);

void addReply(cacheClient *c, cobj *obj);

void addReplySds(cacheClient *c, Sds s);

void addReplyError(cacheClient *c, char *err);

void addReplyStatus(cacheClient *c, char *status);

void addReplyDouble(cacheClient *c, double d);

void addReplyLongLong(cacheClient *c, long long ll);

void addReplyMultiBulkLen(cacheClient *c, long length);

void copyClientOutputBuffer(cacheClient *dst, cacheClient *src);

void *dupClientReplyValue(void *o);

void getClientsMaxBuffers(unsigned long *longest_output_list,
                          unsigned long *biggest_input_buffer);

void formatPeerId(char *peerid, size_t peerid_len, char *ip, int port);

char *getClientPeerId(cacheClient *client);

Sds catClientInfoString(Sds s, cacheClient *client);

Sds getAllClientsInfoString(void);

void rewriteClientCommandVector(cacheClient *c, int argc, ...);

void rewriteClientCommandArgument(cacheClient *c, int i, cobj *newval);

unsigned long getClientOutputBufferMemoryUsage(cacheClient *c);

void freeClientsInAsyncFreeQueue(void);

void asyncCloseClientOnOutputBufferLimitReached(cacheClient *c);

int getClientType(cacheClient *c);

int getClientTypeByName(char *name);

char *getClientTypeName(int class);

void flushSlavesOutputBuffers(void);

void disconnectSlaves(void);

int listenToPort(int port, int *fds, int *count);

void pauseClients(ms_time_t end);

int clientsArePaused(void);

int processEventsWhileBlocked(void);

#ifdef __GUNC__
void addReplyErrorFormat(cacheClient *c, const char *fmt, ...)
    __attribute__((format(printf, 2, 3)));
void addReplyStatusFormat(cacheClient *c, const char *fmt, ...)
    __attribute__((format(printf, 2, 3)));
#else

void addReplyErrorFormat(cacheClient *c, const char *fmt, ...);

void addReplyStatusFormat(cacheClient *c, const char *fmt, ...);

#endif

void listTypeTryConversion(cobj *subject, cobj *value);

void listTypePush(cobj *subject, cobj *value, int where);

cobj *listTypePop(cobj *subject, int where);

unsigned long listTypeLength(cobj *subject);

listTypeIterator *listTypeInitIterator(cobj *subject, long index,
                                       unsigned char direction);

void listTypeReleaseIterator(listTypeIterator *li);

int listTypeNext(listTypeIterator *li, listTypeEntry *entry);

cobj *listTypeGet(listTypeEntry *entry);

void listTypeInsert(listTypeEntry *entry, cobj *value, int where);

void listTypeDelete(listTypeEntry *entry);

int listTypeEqual(listTypeEntry *entry, cobj *o);

void listTypeConvert(cobj *subject, int enc);

void unblockClientWaitingData(cacheClient *c);

void handleClientsBlockedOnLists(void);

void popGenericCommand(cacheClient *c, int where);

void signalListAsReady(cacheDB *db, cobj *key);

void unwatchAllKeys(cacheClient *c);

void initClientMultiState(cacheClient *c);

void freeClientMultiState(cacheClient *c);

void queueMultiCommand(cacheClient *c);

void touchWatchedKey(cacheDB *db, cobj *key);

void touchWatchedKeysOnFlush(int dbid);

void discardTransaction(cacheClient *c);

void flagTransaction(cacheClient *c);

void decrRefCount(cobj *o);

void decrRefCountVoid(void *o);

void incrRefCount(cobj *o);

cobj *resetRefCount(cobj *o);

void freeStringObject(cobj *o);

void freeListObject(cobj *o);

void freeSetObject(cobj *o);

void freeZsetObject(cobj *o);

void freeHashObject(cobj *o);

cobj *createObject(int type, void *ptr);

cobj *createStringObject(char *ptr, size_t len);

cobj *createRawStringObject(char *ptr, size_t len);

cobj *createEmbeddedStringObject(char *ptr, size_t len);

cobj *dupStringObject(cobj *o);

int isObjectRepresentableAsLongLong(cobj *o, long long *llval);

cobj *tryObjectEncoding(cobj *o);

cobj *getDecodedObject(cobj *o);

size_t stringObjectLen(cobj *o);

cobj *createStringObjectFromLongLong(long long value);

cobj *createStringObjectFromLongDouble(long double value, int humanfriendly);

cobj *createListObject(void);

cobj *createZipListObject(void);

cobj *createSetObject(void);

cobj *createIntsetObject(void);

cobj *createHashObject(void);

cobj *createZsetObject(void);

cobj *createZsetZiplistObject(void);

int getLongFromObjectOrReply(cacheClient *c, cobj *o, long *target,
                             const char *msg);

int checkType(cacheClient *c, cobj *o, int type);

int getLongLongFromObjectOrReply(cacheClient *c, cobj *o, long long *target,
                                 const char *msg);

int getDoubleFromObjectOrReply(cacheClient *c, cobj *o, double *target,
                               const char *msg);

int getLongLongFromObject(cobj *o, long long *target);

int getLongDoubleFromObject(cobj *o, long double *target);

int getLongDoubleFromObjectOrReply(cacheClient *c, cobj *o, long double *target,
                                   const char *msg);

char *strEncoding(int encoding);

int compareStringObjects(cobj *a, cobj *b);

int collateStringObjects(cobj *a, cobj *b);

int equalStringObjects(cobj *a, cobj *b);

unsigned long long estimateObjectIdleTime(cobj *o);

#define sdsEncodedObject(objptr)               \
  ((objptr)->encoding == CACHE_ENCODING_RAW || \
   (objptr)->encoding == CACHE_ENCODING_EMBSTR)

ssize_t syncWrite(int fd, char *ptr, ssize_t size, ms_time_t timeout);

ssize_t syncRead(int fd, char *ptr, ssize_t size, ms_time_t timeout);

ssize_t syncReadLine(int fd, char *ptr, ssize_t size, ms_time_t timeout);

void replicationFeedSlaves(ListNode *slaves, int dictid, cobj **argv, int argc);

void replicationFeedMonitors(cacheClient *c, List *monitors, int dictid,
                             cobj **argv, int argc);

void updateSlavesWaitingBgsave(int bgsaveerr, int type);

void replicationCron(void);

void replicationHandleMasterDisconnection(void);

void replicationCacheMaster(cacheClient *c);

void resizeReplicationBacklog(long long newsize);

void replicationSetMaster(char *ip, int port);

void replicationUnsetMaster(void);

void refreshGoodSlavesCount(void);

void replicationScriptCacheInit(void);

void replicationScriptCacheFlush(void);

void replicationScriptCacheAdd(Sds sha1);

int replicationScriptCacheExists(Sds sha1);

void processClientsWaitingReplicas(void);

void unblockClientWaitingReplicas(cacheClient *c);

int replicationCountAcksByOffset(long long offset);

void replicationSendNewLineToMaster(void);

long long replicationGetSlaveOffset(void);

char *replicationGetSlaveName(cacheClient *c);

void startLoading(FILE *fp);

void loadingProgress(off_t pos);

void stopLoading(void);

#include "rdb.h"

void flushAppendOnlyFile(int force);

void feedAooendOnlyFile(struct cacheCommand *cmd, int dictid, cobj *argv,
                        int argc);

void aofRemoveTempFile(pid_t child_pid);

int rewriteAppendOnlyFileBackground(void);

int loadAppendOnlyFile(char *filename);

void stopAppendOnly(void);

int startAppendOnly(void);

void backgroundRewriteDoneHandler(int exitcode, int bysignal);

void aofRewriteBufferReset(void);

unsigned long aofRewriteBufferSize(void);

typedef struct {
  double min, max;
  int minex, maxex;
} zrangespec;

typedef struct {
  cobj *min, *max;
  int minex, maxex;
} zlexrangespec;

zskiplist *zslCreate(void);

void zslFree(zskiplist *zsl);

zskiplistNode *zslInsert(zskiplist *zsl, double score, cobj *obj);

unsigned char *zzlInsert(unsigned char *zl, cobj *ele, double score);

int zslDelete(zskiplist *zsl, double score, cobj *obj);

zskiplistNode *zslFirstInRange(zskiplist *zsl, zrangespec *range);

zskiplistNode *zslLastInRange(zskiplist *zsl, zrangespec *range);

double zzlGetScore(unsigned char *sptr);

void zzlNext(unsigned char *zl, unsigned char **eptr, unsigned char **sptr);

void zzlPrev(unsigned char *zl, unsigned char **eptr, unsigned char **sptr);

unsigned int zsetLength(cobj *zobj);

void zsetConvert(cobj *zobj, int encoding);

unsigned long zslGetRank(zskiplist *zsl, double score, cobj *o);

int freeMemoryIfNeeded(void);

int processCommand(cacheClient *c);

void setupSignalHandlers(void);

struct cacheCommand *lookupCommand(Sds name);

struct cacheCommand *lookupCommandByCString(char *s);

struct cacheCommand *lookupCmmandOrOriginal(Sds name);

void call(cacheClient *c, int flags);

void propagate(struct cacheCommand *cmd, int dbid, cobj **argv, int argc,
               int target);

void alsoPropagate(struct cacheCommand *cmd, int dbid, cobj **argv, int argc,
                   int target);

void forceCommandPropagation(cacheClient *c, int flags);

int prepareForShutdown(void);

#ifdef __GUNC__
void redislog(int level, const char *fmt, ...)
    __attribute__((format(printf, 2, 3)));
#else

void redislog(int level, const char *fmt, ...);

#endif

void cacheLogRaw(int level, const char *msg);

void cacheLogFromHandler(int level, const char *msg);

void usage(void);

void updateDictResizePolicy(void);

int htNeedsResize(Dict *dict);

void oom(const char *msg);

void populateCommandTable(void);

void resetCommandTableStats(void);

void adjustOpenFilesLimit(void);

void closeListeningSockets(int unlink_unix_socket);

void updateCachedTime(void);

void resetServerStats(void);

unsigned int getLRUClock(void);

cobj *setTypeCreate(cobj *value);

int setTypeAdd(cobj *subject, cobj *value);

int setTypeRemove(cobj *subject, cobj *value);

int setTypeIsMember(cobj *subject, cobj *value);

setTypeIterator *setTypeInitIterator(cobj *subject);

void setTypeReleaseIterator(setTypeIterator *si);

int setTypeNext(setTypeIterator *si, cobj **objele, int64_t *llele);

int setTypeRandomElement(cobj *setobj, cobj **objele, int64_t *llele);

unsigned long setTypeSize(cobj *subject);

void setTypeConvert(cobj *subject, int enc);

void hashTypeConvert(cobj *o, int enc);

void hashTypeTryCnversion(cobj *subject, cobj **argv, int start, int end);

void hashTypeTryObjectEncoding(cobj *subject, cobj **o1, cobj **o2);

cobj *hashTypeGetOptions(cobj *o, cobj *key);

int hashTypeExists(cobj *o, cobj *key);

int hashTypeSet(cobj *o, cobj *key, cobj *value);

int hashTypeDelete(cobj *o, cobj *key);

unsigned long hashTypeLength(cobj *o);

hashTypeIterator *hashTypeInitIterator(cobj *subject);

void hashTypeReleaseIterator(hashTypeIterator *hi);

int hashTypeNext(hashTypeIterator *hi);

void hashTypeCurrentFromZiplist(hashTypeIterator *hi, int what,
                                unsigned char **vstr, unsigned int *vlen,
                                long long *vll);

void hashTypeCurrentFromHashTable(hashTypeIterator *hi, int what, cobj **dst);

cobj *hashTypeCurrentObject(hashTypeIterator *hi, int what);

cobj *hashTypeLookupWriteOrCreate(cacheClient *c, cobj *key);

int pubsubUnsubscribeAllChannels(cacheClient *c, int notify);

int oubsubUnsubscribeAllPatterns(cacheClient *c, int notify);

void freePubsubPattern(void *p);

int listMatchPubsubPattern(void *a, void *b);

int pubsubPublishMessage(cobj *channel, cobj *message);

void notifyKeyspaceEvent(int type, char *event, cobj *key, int dbid);

int keyspaceEventsStringToFlags(char *classes);

Sds keyspaceEventsFlagsToString(int flags);

void loadServerConfig(char *filename, char *options);

void appendServerSaveParams(time_t seconds, int changes);

void resetServerSaveParams(void);

struct rewriteConfigState;

void rewriteConfigRewriteLine(struct rewriteConfigState *state, char *option,
                              Sds line, int force);

int rewriteConfig(char *path);

int removeExpire(cacheDB *db, cobj *key);

void propagateExpire(cacheDB *db, cobj *key);

int expireIfNeeded(cacheDB *db, cobj *key);

long long getExpire(cacheDB *db, cobj *key);

void setExpire(cacheDB *db, cobj *key, long long when);

cobj *lookupKey(cacheDB *db, cobj *key);

cobj *loohupKeyRead(cacheDB *db, cobj *key);

cobj *lookupKeyWrite(cacheDB *db, cobj *key);

cobj *lookupKeyReadOrReply(cacheClient *c, cobj *key, cobj *reply);

cobj *lookupKeyWriteOrReply(cacheClient *c, cobj *key, cobj *reply);

void dbAdd(cacheDB *db, cobj *key, cobj *val);

void dbOverwrite(cacheDB *db, cobj *key, cobj *val);

void setKey(cacheDB *db, cobj *key, cobj *val);

int dbExists(cacheDB *db, cobj *key);

cobj *dbRandomKey(cacheDB *db);

int dbDelete(cacheDB *db, cobj *key);

cobj *dbUnshareStringValue(cacheDB *db, cobj *key, cobj *o);

long long emptyDb(void(callback)(void *));

int selectDb(cacheClient *c, int id);

void signalModifiedKey(cacheDB *db, cobj *key);

void signalFlushedDb(cacheDB *db);

unsigned int getKeysInSlot(unsigned int hashslot, cobj **keys,
                           unsigned int count);

unsigned int countKeysInSlot(unsigned int hashslot);

unsigned int delkeysInSlot(unsigned int hashslot);

int verifyClusterConfigWithData(void);

void scanGenericCommand(cacheClient *c, cobj *o, unsigned long cursor);

int parseScanCursorOrReply(cacheClient *c, cobj *o, unsigned long *cursor);

int *getKeysFromCommand(struct cacheCommand *cmd, cobj **argv, int argc,
                        int *numkeys);

void getkeysFreeResult(int *result);

int *zunionInterGetKeys(struct cacheCommand *cmd, cobj **argv, int argc,
                        int *numkeys);

int *evalGetKeys(struct cacheCommand *cmd, cobj **argv, int argc, int *numkeys);

int *sortGetKeys(struct cacheCommand *cmd, cobj **argv, int argc, int *numkeys);

void clusterInit(void);

unsigned short crc16(const char *buf, int len);

unsigned int keyhashSlot(char *key, int keylen);

void clusterCron(void);

void clusterPropagatePublish(cobj *channel, cobj *message);

void migrateCloseTimedoutSockets(void);

void clusterBeforeSleep(void);

void initSentinelConfig(void);

void initSentinel(void);

void sentinelTimer(void);

char *sentinelHandleConfiguration(char **argv, int argc);

void sentinelIsRunning(void);

void scriptingInit(void);

void processUnblockedClients(void);

void blockClient(cacheClient *c, int btype);

void unblockClient(cacheClient *c);

void replyToBlockedClientTimedOut(cacheClient *c);

int getTimeoutFromObjectOrReply(cacheClient *c, cobj *object,
                                ms_time_t *timeout, int unit);

void disconnectAllBlockedClients(void);

char *cacheGitSHA1(void);

char *cacheGitDirty(void);

uint64_t cacheBuildId(void);

void authCommand(cacheClient *c);

void pingCommand(cacheClient *c);

void echoCommand(cacheClient *c);

void commandCommand(cacheClient *c);

void setCommand(cacheClient *c);

void setnxCommand(cacheClient *c);

void setexCommand(cacheClient *c);

void psetexCommand(cacheClient *c);

void getCommand(cacheClient *c);

void delCommand(cacheClient *c);

void existsCommand(cacheClient *c);

void setbitCommand(cacheClient *c);

void getbitCommand(cacheClient *c);

void setrangeCommand(cacheClient *c);

void getrangeCommand(cacheClient *c);

void incrCommand(cacheClient *c);

void decrCommand(cacheClient *c);

void incrbyCommand(cacheClient *c);

void decrbyCommand(cacheClient *c);

void inctbyfloatCommand(cacheClient *c);

void selectCommand(cacheClient *c);

void randomkeyCommand(cacheClient *c);

void keysCommand(cacheClient *c);

void scanCommand(cacheClient *c);

void dbsizeCommand(cacheClient *c);

void lastsaveCommand(cacheClient *c);

void saveCommand(cacheClient *c);

void bgsaveCommand(cacheClient *c);

void bgrewriteaofCommand(cacheClient *c);

void shutdownCommand(cacheClient *c);

void moveCommand(cacheClient *c);

void renameCommand(cacheClient *c);

void renamenxCommand(cacheClient *c);

void lpushCommand(cacheClient *c);

void rpushCommand(cacheClient *c);

void lpushxCommand(cacheClient *c);

void rpushxCommand(cacheClient *c);

void linsertCommand(cacheClient *c);

void lpopCommand(cacheClient *c);

void rpopCommand(cacheClient *c);

void llenCommand(cacheClient *c);

void lindexCommand(cacheClient *c);

void lrangeCommand(cacheClient *c);

void ltrimCommand(cacheClient *c);

void typeCommand(cacheClient *c);

void lsetCommand(cacheClient *c);

void saddCommand(cacheClient *c);

void sremCommand(cacheClient *c);

void smoveCommand(cacheClient *c);

void sismemberCommand(cacheClient *c);

void scardCommand(cacheClient *c);

void spopCommand(cacheClient *c);

void srandmemberCommand(cacheClient *c);

void sinterCommand(cacheClient *c);

void sinterstoreCommand(cacheClient *c);

void sunionCommand(cacheClient *c);

void sunionstoreCommand(cacheClient *c);

void sdiffCommand(cacheClient *c);

void sdiffstoreCommand(cacheClient *c);

void sscanCommand(cacheClient *c);

void syncCommand(cacheClient *c);

void flushdbCommand(cacheClient *c);

void flushallCommand(cacheClient *c);

void sortCommand(cacheClient *c);

void lremCommand(cacheClient *c);

void rpoplpushCommand(cacheClient *c);

void infoCommand(cacheClient *c);

void mgetCommand(cacheClient *c);

void monitorCommand(cacheClient *c);

void expireCommand(cacheClient *c);

void expireatCommand(cacheClient *c);

void pexpireCommand(cacheClient *c);

void pexpireatCommand(cacheClient *c);

void getsetCommand(cacheClient *c);

void ttlCommand(cacheClient *c);

void pttlCommand(cacheClient *c);

void persistCommand(cacheClient *c);

void slaveofCommand(cacheClient *c);

void roleCommand(cacheClient *c);

void debugCommand(cacheClient *c);

void msetCommand(cacheClient *c);

void msetnxCommand(cacheClient *c);

void zaddCommand(cacheClient *c);

void zincrbyCommand(cacheClient *c);

void zrangeCommand(cacheClient *c);

void zrangebyscoreCommand(cacheClient *c);

void zrevrangebyscoreCommand(cacheClient *c);

void zrangebylexCommand(cacheClient *c);

void zremrangebylexCommand(cacheClient *c);

void zrevrangebylexCommand(cacheClient *c);

void zcountCommand(cacheClient *c);

void zlexcountCommand(cacheClient *c);

void zrevrangeCommand(cacheClient *c);

void zcardCommand(cacheClient *c);

void zremCommand(cacheClient *c);

void zscoreCommand(cacheClient *c);

void zremrangebyscoreCommand(cacheClient *c);

void zremrangebyrankCommand(cacheClient *c);

void multiCommand(cacheClient *c);

void execCommand(cacheClient *c);

void discardCommand(cacheClient *c);

void blpopCommand(cacheClient *c);

void brpopCommand(cacheClient *c);

void brpoplpushCommand(cacheClient *c);

void appendCommand(cacheClient *c);

void strlenCommand(cacheClient *c);

void zrankCommand(cacheClient *c);

void zrevrankCommand(cacheClient *c);

void hsetCommand(cacheClient *c);

void hsetnxCommand(cacheClient *c);

void hgetCommand(cacheClient *c);

void hmsetCommand(cacheClient *c);

void hmgetCommand(cacheClient *c);

void hdelCommand(cacheClient *c);

void hlenCommand(cacheClient *c);

void zremrangebyrankCommand(cacheClient *c);

void zunionstoreCommand(cacheClient *c);

void zinterstoreCommand(cacheClient *c);

void zscanCommand(cacheClient *c);

void hkeysCommand(cacheClient *c);

void hvalsCommand(cacheClient *c);

void hgetallCommand(cacheClient *c);

void hexistsCommand(cacheClient *c);

void hscanCommand(cacheClient *c);

void configCommand(cacheClient *c);

void hincrbyCommand(cacheClient *c);

void hincrbyfloatCommand(cacheClient *c);

void subscribeCommand(cacheClient *c);

void unsubscribeCommand(cacheClient *c);

void psubscribeCommand(cacheClient *c);

void punsubscribeCommand(cacheClient *c);

void publishCommand(cacheClient *c);

void pubsubCommand(cacheClient *c);

void watchCommand(cacheClient *c);

void unwatchCommand(cacheClient *c);

void clusterCommand(cacheClient *c);

void restoreCommand(cacheClient *c);

void migrateCommand(cacheClient *c);

void askingCommand(cacheClient *c);

void readonlyCommand(cacheClient *c);

void readwriteCommand(cacheClient *c);

void dumpCommand(cacheClient *c);

void objectCommand(cacheClient *c);

void clientCommand(cacheClient *c);

void evalCommand(cacheClient *c);

void evalShaCommand(cacheClient *c);

void scriptCommand(cacheClient *c);

void timeCommand(cacheClient *c);

void bitopCommand(cacheClient *c);

void bitcountCommand(cacheClient *c);

void bitposCommand(cacheClient *c);

void replconfCommand(cacheClient *c);

void waitCommand(cacheClient *c);

void pfselftestCommand(cacheClient *c);

void pfaddCommand(cacheClient *c);

void pfcountCommand(cacheClient *c);

void pfmergeCommand(cacheClient *c);

void pfdebugCommand(cacheClient *c);

void latencyCommand(cacheClient *c);

#if defined(__GUNC__)
void *calloc(size_t count, size_t size) __attribute__((deprecated));
void free(void *ptr) __attribute__((deprecated));
void *malloc(size_t size) __attribute__((deprecated));
void *realloc(void *ptr, size_t size) __attribute__((deprecated));
#endif

void _cacheAssertWithInfo(cacheClient *c, cobj *o, char *estr, char *file,
                          int line);

void _cacheAssert(char *estr, char *file, int line);

void _cachePanic(char *msg, char *file, int line);

void bugReportStart(void);

void cacheLogObjectDebugInfo(cobj *o);

void sigsegvHandler(int sig, siginfo_t *info, void *secret);

Sds genCacheInfoString(char *section);

void enableWatchdog(int period);

void disableWatchdog(void);

void watchdogScheduleSignal(int period);

void cacheLogHexDump(int level, char *descr, void *value, size_t len);

#define cacheDebug(fmt, ...) \
  printf("DEBUG %s:%d > " fmt "\n", __FILE__, __LINE__, __VA_ARGS__)
#define cacheDebugMark() printf("-- MARK %s:%d --\n", __FILE__, __LINE__)
#endif
