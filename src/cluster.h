#ifndef CACHE_CLUSTER_H
#define CACHE_CLUSTER_H

#define CACHE_CLUSTER_SLOTS 16384
#define CACHE_CLUSTER_OK 0
#define CACHE_CLUSTER_FAIL 1
#define CACHE_CLUSTER_NAMELEN 40
#define CACHE_CLUSTER_PORT_INCR 10000

#define CACHE_CLUSTER_DEFAULT_NODE_TIMEOUT 15000
#define CACHE_CLUSTER_DEFAULT_SALVE_VAILDITY 10
#define CACHE_CLUSTER_DEFAULT_REQUIRE_FULL_COVERAGE 1
#define CACHE_CLUSTER_FAIL_REPORT_VALIDITY_MULT 2

#define CACHE_CLUSTER_FAIL_UNDO_TIME_MULT 2
#define CACHE_CLUSTER_FAIL_UNDO_TIME_ADD 10
#define CACHE_CLUSTER_FAILOVER_DELAY 5
#define CACHE_CLUSTER_DEFAULT_MIGRATION_BARRIER 1
#define CACHE_CLUSTER_MF_TIMEOUT 5000
#define CACHE_CLUSTER_MF_PAUSE_MULT 2

#define CACHE_CLUSTER_REDIR_NONE 0
#define CACHE_CLUSTER_REDIR_CROSS_SLOT 1
#define CACHE_CLUSTER_REDIR_UNSTABLE 2
#define CACHE_CLUSTER_REDIR_ASK 3
#define CACHE_CLUSTER_REDIR_MOVED 4
#define CACHE_CLUSTER_REDIR_STATE 5
#define CACHE_CLUSTER_REDIR_DOWN_UNBOUND 6

struct clusterNode;

typedef struct clusterLink {
  ms_time_t ctime;
  int fd;
  Sds sndbuf;
  Sds rcvbuf;
  struct clusterNode *node;
} clusterLink;

#define CACHE_NODE_MASTER 1
#define CACHE_NODE_SLAVE 2
#define CACHE_NODE_PFAIL 4
#define CACHE_NODE_FAIL 8
#define CACHE_NODE_MYSELF 16
#define CACHE_NODE_HANDSHAKE 32
#define CACHE_NODE_NOADDR 64
#define CAHCE_NODE_MEET 128
#define CACHE_NODE_PROMOTED 256
#define CACHE_NODE_NULL_NAME                                                   \
  = "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000" \
    "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000" \
    "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000" \
    "\000\000\000\000\000\000\000\000\000\000"

#define nodeIsMaster(n) ((n)->flags & CACHE_NODE_MASTER)
#define nodeIsSlave(n) ((n)->flags & CACHE_NODE_SLAVE)
#define nodeInHandshake(n) ((n)->flags & CACHE_NODE_HANDSHAKE)
#define nodeHasAddr(n) (!((n)->flags & CACHE_NODE_NOADDR))
#define nodeWithoutAddr(n) ((n)->flags & CACHE_NODE_NOADDR)
#define nodeTimedOut(n) ((n)->flags & CACHE_NODE_PFAIL)
#define nodeFailed(n) ((n)->flags & CACHE_NODE_FAIL)

#define CACHE_CLUSTER_CANT_FAILOVER_NONE 0
#define CACHE_CLUSTER_CANT_FAILOVER_DATA_AGE 1
#define CACHE_CLUSTER_CANT_FAILOVER_WAITING_DELAY 2
#define CACHE_CLUSTER_CANT_FAILOVER_EXPIRED 3
#define CACHE_CLUSTER_CANT_FAILOVER_WAITING_VOTES 4
#define CACHE_CLUSTER_CANT_FAILOVER_RELOG_PERIOD (60 * 5)

typedef struct clusterNodeFailReport {
  struct clusterNode *node;
  ms_time_t time;

} clusterNodeFailReport;

typedef struct clusterNode {
  ms_time_t ctime;
  char name[CACHE_CLUSTER_NAMELEN];
  int flags;
  uint64_t configEpoch;
  unsigned char slots[CACHE_CLUSTER_SLOTS / 8];
  int numslots;
  int numslaves;
  struct clusterNode **slaves;
  struct clusterNode **slaveof;
  ms_time_t ping_sent;
  ms_time_t pong_received;
  ms_time_t fail_time;
  ms_time_t voted_time;
  ms_time_t repl_offset_time;
  long long repl_offset;
  char ip[CACHE_IP_STR_LEN];
  int port;
  clusterLink *link;
  List *fail_reports;
} clusterNode;

typedef struct clusterState {
  clusterNode *myself;
  uint64_t currentEpoch;
  int state;
  int size;
  Dict *nodes;
  Dict *nodes_black_list;
  clusterNode *migrating_slots_to[CACHE_CLUSTER_SLOTS];
  clusterNode *importing_slots_from[CACHE_CLUSTER_SLOTS];
  clusterNode *slots[CACHE_CLUSTER_SLOTS];
  zskiplist *slots_to_keys;
  ms_time_t failover_auth_time;
  int failover_auth_count;
  int failover_auth_sent;
  int failover_auth_rank;
  uint64_t failover_auth_epoch;
  int cant_failover_reason;
  ms_time_t mf_end;
  clusterNode *mf_slave;
  long long mf_master_offset;
  int mf_can_start;
  uint64_t lastVoteEpoch;
  int todo_before_sleep;
  long long stats_bus_messages_sent;
  long long stats_bus_messages_received;
} clusterState;

#define CLUSTER_TODO_HANDLE_FAILOVER (1 << 0)
#define CLUSTER_TODO_UPDATE_STATE (1 << 1)
#define CLUSTER_TODO_SAVE_CONFIG (1 << 2)
#define CLUSTER_TODO_FSYNC_CONFIG (1 << 3)

#define CLUSTERMSG_TYPE_PING 0
#define CLUSTERMSG_TYPE_PONG 1
#define CLUSTERMSG_TYPE_MEET 2
#define CLUSTERMSG_TYPE_FAIL 3
#define CLUSTERMSG_TYPE_PUBLISH 4
#define CLUSTERMSG_TYPE_FAILOVER_AUTH_REQUEST 5
#define CLUSTERMSG_TYPE_FAILOVER_AUTH_ACK 6
#define CLUSTERMSG_TYPE_UPDATE 7
#define CLUSTERMSG_TYPE_MFSTART 8

typedef struct clusterMsgDataGossip {
  char nodename[CACHE_CLUSTER_NAMELEN];
  uint32_t ping_sent;
  uint32_t pong_received;
  char ip[CACHE_IP_STR_LEN];
  uint16_t port;
  uint16_t flags;
  uint16_t notused1;
  uint32_t notused2;
} clusterMsgDataGossip;

typedef struct clusterMsgDataFail {
  char nodename[CACHE_CLUSTER_NAMELEN];
} clusterMsgDataFail;

typedef struct clusterMsgDataPublish {
  uint32_t channel_len;
  uint32_t message_len;
  unsigned char bulk_data[8];
} clusterMsgDataPublish;

typedef struct clusterMsgDataUpdate {
  uint64_t configEpoch;
  char nodename[CACHE_CLUSTER_NAMELEN];
  unsigned char slots[CACHE_CLUSTER_SLOTS / 8];
} clusterMsgDataUpdate;

union clusterMsgData {
  struct {
    clusterMsgDataGossip gossip[1];
  } ping;
  struct {
    clusterMsgDataFail about;
  } fail;
  struct {
    clusterMsgDataPublish msg;
  } publish;
  struct {
    clusterMsgDataUpdate nodecfg;
  } update;
};

#define CLUSTER_PROTO_VER 0

typedef struct {
  char sig[4];
  uint32_t totlen;
  uint16_t ver;
  uint16_t notused0;
  uint16_t type;
  uint16_t count;
  uint64_t curentEpoch;
  uint64_t configEpoch;
  uint64_t offset;
  char sender[CACHE_CLUSTER_NAMELEN];
  unsigned char myslots[CACHE_CLUSTER_SLOTS / 8];
  char slaveof[CACHE_CLUSTER_NAMELEN];
  char notused1[32];
  uint16_t port;
  uint16_t flags;
  unsigned char state;
  unsigned char mflags[3];
  union clusterMsgData data;
} clusterMsg;

#define CLUSTERMSG_MIN_LEN (sizeof(clusterMsg) - sizeof(union clusterMsgData))
#define CLUSTERMSG_FLAG0_PAUSED (1 << 0)
#define CLUSTERMSG_FLAG0_FORCEACK (1 << 1)
clusterNode *getNodeByQuery(cacheClient *c, struct cacheCommand *cmd,
                            cobj **argv, int argc, int *hashsolt, int *ask);
int clusterRedirectBlockedClientIfNeeded(cacheClient *c);
void clusterRedirectClient(cacheClient *c, clusterNode *n, int hashsolt,
                           int error_code);

#endif