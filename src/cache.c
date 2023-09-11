//
// Created by fly on 9/1/23.
//

#include "cache.h"

#include <arpa/inet.h>
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <float.h>
#include <limits.h>
#include <locale.h>
#include <math.h>
#include <signal.h>
#include <stdarg.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/uio.h>
#include <sys/utsname.h>
#include <sys/wait.h>
#include <time.h>

#include "bio.h"
#include "cluster.h"
#include "latency.h"
#include "slowlog.h"

struct sharedObjectStruct shared;
double R_Zero, R_PosInf, R_NegInf, R_Nan;
struct cacheServer server;
struct cacheCommand cacheCommandTable[] = {
    {"get", getCommand, 2, "rF", 0, NULL, 1, 1, 1, 0, 0},
    {"set", setCommand, -3, "wm", 0, NULL, 1, 1, 1, 0, 0},
    {"setnx", setnxCommand, 3, "wmF", 0, NULL, 1, 1, 1, 0, 0},
    {"setex", setexCommand, 4, "wm", 0, NULL, 1, 1, 1, 0, 0},
    {"psetex", psetexCommand, 4, "wm", 0, NULL, 1, 1, 1, 0, 0},
    {"append", appendCommand, 3, "wm", 0, NULL, 1, 1, 1, 0, 0},
    {"strlen", strlenCommand, 2, "rF", 0, NULL, 1, 1, 1, 0, 0},
    {"del", delCommand, -2, "w", 0, NULL, 1, -1, 1, 0, 0},
    {"exists", existsCommand, 2, "rF", 0, NULL, 1, 1, 1, 0, 0},
    {"setbit", setbitCommand, 4, "wm", 0, NULL, 1, 1, 1, 0, 0},
    {"getbit", getbitCommand, 3, "rF", 0, NULL, 1, 1, 1, 0, 0},
    {"setrange", setrangeCommand, 4, "wm", 0, NULL, 1, 1, 1, 0, 0},
    {"getrange", getrangeCommand, 4, "r", 0, NULL, 1, 1, 1, 0, 0},
    {"substr", getrangeCommand, 4, "r", 0, NULL, 1, 1, 1, 0, 0},
    {"incr", incrCommand, 2, "wmF", 0, NULL, 1, 1, 1, 0, 0},
    {"decr", decrCommand, 2, "wmF", 0, NULL, 1, 1, 1, 0, 0},
    {"mget", mgetCommand, -2, "r", 0, NULL, 1, -1, 1, 0, 0},
    {"rpush", rpushCommand, -3, "wmF", 0, NULL, 1, 1, 1, 0, 0},
    {"lpush", lpushCommand, -3, "wmF", 0, NULL, 1, 1, 1, 0, 0},
    {"rpushx", rpushxCommand, 3, "wmF", 0, NULL, 1, 1, 1, 0, 0},
    {"lpushx", lpushxCommand, 3, "wmF", 0, NULL, 1, 1, 1, 0, 0},
    {"linsert", linsertCommand, 5, "wm", 0, NULL, 1, 1, 1, 0, 0},
    {"rpop", rpopCommand, 2, "wF", 0, NULL, 1, 1, 1, 0, 0},
    {"lpop", lpopCommand, 2, "wF", 0, NULL, 1, 1, 1, 0, 0},
    {"brpop", brpopCommand, -3, "ws", 0, NULL, 1, 1, 1, 0, 0},
    {"brpoplpush", brpoplpushCommand, 4, "wms", 0, NULL, 1, 2, 1, 0, 0},
    {"blpop", blpopCommand, -3, "ws", 0, NULL, 1, -2, 1, 0, 0},
    {"llen", llenCommand, 2, "rF", 0, NULL, 1, 1, 1, 0, 0},
    {"lindex", lindexCommand, 3, "r", 0, NULL, 1, 1, 1, 0, 0},
    {"lset", lsetCommand, 4, "wm", 0, NULL, 1, 1, 1, 0, 0},
    {"lrange", lrangeCommand, 4, "r", 0, NULL, 1, 1, 1, 0, 0},
    {"ltrim", ltrimCommand, 4, "w", 0, NULL, 1, 1, 1, 0, 0},
    {"lrem", lremCommand, 4, "w", 0, NULL, 1, 1, 1, 0, 0},
    {"rpoplpush", rpoplpushCommand, 3, "wm", 0, NULL, 1, 2, 1, 0, 0},
    {"sadd", saddCommand, -3, "wmF", 0, NULL, 1, 1, 1, 0, 0},
    {"srem", sremCommand, -3, "wF", 0, NULL, 1, 1, 1, 0, 0},
    {"smove", smoveCommand, 4, "wF", 0, NULL, 1, 2, 1, 0, 0},
    {"sismember", sismemberCommand, 3, "rF", 0, NULL, 1, 1, 1, 0, 0},
    {"scard", scardCommand, 2, "rF", 0, NULL, 1, 1, 1, 0, 0},
    {"spop", spopCommand, 2, "wRsF", 0, NULL, 1, 1, 1, 0, 0},
    {"srandmember", srandmemberCommand, -2, "rR", 0, NULL, 1, 1, 1, 0, 0},
    {"sinter", sinterCommand, -2, "rS", 0, NULL, 1, -1, 1, 0, 0},
    {"sinterstore", sinterstoreCommand, -3, "wm", 0, NULL, 1, -1, 1, 0, 0},
    {"sunion", sunionCommand, -2, "rS", 0, NULL, 1, -1, 1, 0, 0},
    {"sunionstore", sunionstoreCommand, -3, "wm", 0, NULL, 1, -1, 1, 0, 0},
    {"sdiff", sdiffCommand, -2, "rS", 0, NULL, 1, -1, 1, 0, 0},
    {"sdiffstore", sdiffstoreCommand, -3, "wm", 0, NULL, 1, -1, 1, 0, 0},
    {"smembers", sinterCommand, 2, "rS", 0, NULL, 1, 1, 1, 0, 0},
    {"sscan", sscanCommand, -3, "rR", 0, NULL, 1, 1, 1, 0, 0},
    {"zadd", zaddCommand, -4, "wmF", 0, NULL, 1, 1, 1, 0, 0},
    {"zincrby", zincrbyCommand, 4, "wmF", 0, NULL, 1, 1, 1, 0, 0},
    {"zrem", zremCommand, -3, "wF", 0, NULL, 1, 1, 1, 0, 0},
    {"zremrangebyscore", zremrangebyscoreCommand, 4, "w", 0, NULL, 1, 1, 1, 0,
     0},
    {"zremrangebyrank", zremrangebyrankCommand, 4, "w", 0, NULL, 1, 1, 1, 0, 0},
    {"zremrangebylex", zremrangebylexCommand, 4, "w", 0, NULL, 1, 1, 1, 0, 0},
    {"zunionstore", zunionstoreCommand, -4, "wm", 0, zunionInterGetKeys, 0, 0,
     0, 0, 0},
    {"zinterstore", zinterstoreCommand, -4, "wm", 0, zunionInterGetKeys, 0, 0,
     0, 0, 0},
    {"zrange", zrangeCommand, -4, "r", 0, NULL, 1, 1, 1, 0, 0},
    {"zangebyscore", zrangebyscoreCommand, -4, "r", 0, NULL, 1, 1, 1, 0, 0},
    {"zrevrangebyscore", zrevrangebyscoreCommand, -4, "r", 0, NULL, 1, 1, 1, 0,
     0},
    {"zrangebylex", zrangebylexCommand, -4, "r", 0, NULL, 1, 1, 1, 0, 0},
    {"zrevrangebylex", zrevrangebylexCommand, -4, "r", 0, NULL, 1, 1, 1, 0, 0},
    {"zcount", zcountCommand, 4, "rF", 0, NULL, 1, 1, 1, 0, 0},
    {"zlexcount", zlexcountCommand, 4, "rF", 0, NULL, 1, 1, 1, 0, 0},
    {"zrevrange", zrevrangeCommand, -4, "r", 0, NULL, 1, 1, 1, 0, 0},
    {"zcard", zcardCommand, 2, "rF", 0, NULL, 1, 1, 1, 0, 0},
    {"zscore", zscoreCommand, 3, "rF", 0, NULL, 1, 1, 1, 0, 0},
    {"zrank", zrankCommand, 3, "rF", 0, NULL, 1, 1, 1, 0, 0},
    {"zrevrank", zrevrankCommand, 3, "rF", 0, NULL, 1, 1, 1, 0, 0},
    {"zscan", zscanCommand, 3, "rR", 0, NULL, 1, 1, 1, 0, 0},
    {"hset", hsetCommand, 4, "wmF", 0, NULL, 1, 1, 1, 0, 0},
    {"hmget", hmgetCommand, -3, "r", 0, NULL, 1, 1, 1, 0, 0},
    {"hmset", hmsetCommand, -4, "wm", 0, NULL, 1, 1, 1, 0, 0},
    {"hincrby", hincrbyCommand, 4, "wmF", 0, NULL, 1, 1, 1, 0, 0},
    {"hincrbyfloat", hincrbyfloatCommand, 4, "wmF", 0, NULL, 1, 1, 1, 0, 0},
    {"hdel", hdelCommand, -3, "wF", 0, NULL, 1, 1, 1, 0, 0},
    {"hlen", hlenCommand, 2, "rF", 0, NULL, 1, 1, 1, 0, 0},
    {"hkeys", hkeysCommand, 2, "rS", 0, NULL, 1, 1, 1, 0, 0},
    {"hvals", hvalsCommand, 2, "rS", 0, NULL, 1, 1, 1, 0, 0},
    {"hegttall", hgetallCommand, 2, "r", 0, NULL, 1, 1, 1, 0, 0},
    {"hexists", hexistsCommand, 3, "rF", 0, NULL, 1, 1, 1, 0, 0},
    {"hscan", hscanCommand, -3, "rR", -3, NULL, 1, 1, 1, 0, 0},
    {"incrby", incrbyCommand, 3, "wmF", 0, NULL, 1, 1, 1, 0, 0},
    {"decrby", decrbyCommand, 3, "wmF", 0, NULL, 1, 1, 1, 0, 0},
    {"incrbyfloat", inctbyfloatCommand, 3, "wmF", 0, NULL, 1, 1, 1, 0, 0},
    {"getset", getsetCommand, 3, "wm", 0, NULL, 1, 1, 1, 0, 0},
    {"mset", msetCommand, -3, "wm", 0, NULL, 1, -1, 2, 0, 0},
    {"msetnx", msetnxCommand, -3, "wm", 0, NULL, 1, -1, 2, 0, 0},
    {"randomkey", randomkeyCommand, 1, "rR", 0, NULL, 0, 0, 0, 0, 0},
    {"select", selectCommand, 2, "rlF", 0, NULL, 0, 0, 0, 0, 0},
    {"move", moveCommand, 3, "wF", 0, NULL, 1, 1, 1, 0, 0},
    {"rename", renameCommand, 3, "w", 0, NULL, 1, 2, 1, 0, 0},
    {"renamenx", renamenxCommand, 3, "wF", 0, NULL, 1, 2, 1, 0, 0},
    {"expire", expireCommand, 3, "wF", 0, NULL, 1, 1, 1, 0, 0},
    {"expireat", expireatCommand, 3, "wF", 0, NULL, 1, 1, 1, 0, 0},
    {"pexpire", pexpireCommand, 3, "wF", 0, NULL, 1, 1, 1, 0, 0},
    {"pexpireat", pexpireatCommand, 3, "wF", 0, NULL, 1, 1, 1, 0, 0},
    {"keys", keysCommand, 2, "rS", 0, NULL, 0, 0, 0, 0, 0},
    {"scan", scanCommand, -2, "rR", 0, NULL, 0, 0, 0, 0, 0},
    {"dbsize", dbsizeCommand, 1, "rF", 0, NULL, 0, 0, 0, 0, 0},
    {"auth", authCommand, 2, "rsltF", 0, NULL, 0, 0, 0, 0, 0},
    {"ping", pingCommand, -1, "rtF", 0, NULL, 0, 0, 0, 0, 0},
    {"echo", echoCommand, 2, "rF", 0, NULL, 0, 0, 0, 0, 0},
    {"save", saveCommand, 1, "ars", 0, NULL, 0, 0, 0, 0, 0},
    {"bgsave", bgsaveCommand, 1, "ar", 0, NULL, 0, 0, 0, 0, 0},
    {"bgrewriteaof", bgrewriteaofCommand, 1, "ar", 0, NULL, 0, 0, 0, 0, 0},
    {"shutdown", shutdownCommand, -1, "arlt", 0, NULL, 0, 0, 0, 0, 0},
    {"lastsave", lastsaveCommand, 1, "rRF", 0, NULL, 0, 0, 0, 0, 0},
    {"type", typeCommand, 2, "rF", 0, NULL, 1, 1, 1, 0, 0},
    {"multi", multiCommand, 1, "rsF", 0, NULL, 0, 0, 0, 0, 0},
    {"exec", execCommand, 1, "sM", 0, NULL, 0, 0, 0, 0, 0},
    {"discard", discardCommand, 1, "rsF", 0, NULL, 0, 0, 0, 0, 0},
    {"sync", syncCommand, 1, "ars", 0, NULL, 0, 0, 0, 0, 0},
    {"psync", syncCommand, 3, "ars", 0, NULL, 0, 0, 0, 0, 0},
    {"replconf", replconfCommand, -1, "arslt", 0, NULL, 0, 0, 0, 0, 0},
    {"flushdb", flushdbCommand, 1, "w", 0, NULL, 0, 0, 0, 0, 0},
    {"flushall", flushallCommand, 1, "w", 0, NULL, 0, 0, 0, 0, 0},
    {"sort", sortCommand, -2, "wm", 0, sortGetKeys, 1, 1, 1, 0, 0},
    {"info", infoCommand, -1, "rlt", 0, NULL, 0, 0, 0, 0, 0},
    {"monitor", monitorCommand, 1, "ars", 0, NULL, 0, 0, 0, 0, 0},
    {"ttl", ttlCommand, 2, "rF", 0, NULL, 1, 1, 1, 0, 0},
    {"pttl", pttlCommand, 2, "rF", 0, NULL, 1, 1, 1, 0, 0},
    {"persist", persistCommand, 2, "wF", 0, NULL, 1, 1, 1, 0, 0},
    {"slaveof", slaveofCommand, 3, "ast", NULL, 0, 0, 0, 0, 0},
    {"role", roleCommand, 1, "lst", 0, NULL, 0, 0, 0, 0, 0},
    {"debug", debugCommand, -2, "as", 0, NULL, 0, 0, 0, 0, 0},
    {"config", configCommand, -2, "art", NULL, 0, 0, 0, 0, 0},
    {"subscribe", subscribeCommand, -2, "rpslt", 0, NULL, 0, 0, 0, 0, 0},
    {"unsubscribe", unsubscribeCommand, -1, "rpslt", 0, NULL, 0, 0, 0, 0, 0},
    {"psubscribe", psubscribeCommand, -2, "rpslt", 0, NULL, 0, 0, 0, 0, 0},
    {"punsubscribe", punsubscribeCommand, -1, "rpslt", 0, NULL, 0, 0, 0, 0, 0},
    {"publish", publishCommand, 3, "pltrF", 0, NULL, 0, 0, 0, 0, 0},
    {"pubsub", pubsubCommand, -2, "pltrR", 0, NULL, 0, 0, 0, 0, 0},
    {"watch", watchCommand, -2, "rsF", 0, NULL, 1, -1, 1, 0, 0},
    {"unwatch", unwatchCommand, 1, "rsF", 0, NULL, 0, 0, 0, 0, 0},
    {"cluster", clusterCommand, -2, "ar", 0, NULL, 0, 0, 0, 0, 0},
    {"restore", restoreCommand, -4, "wm", 0, NULL, 1, 1, 1, 0, 0},
    {"restore-asking", restoreCommand, -4, "wmk", 0, NULL, 1, 1, 1, 0, 0},
    {"migrate", migrateCommand, -6, "w", 0, NULL, 0, 0, 0, 0, 0},
    {"asking", askingCommand, 1, "r", 0, NULL, 0, 0, 0, 0, 0},
    {"readonly", readonlyCommand, 1, "rF", 0, NULL, 0, 0, 0, 0, 0},
    {"readwrite", readwriteCommand, 1, "rF", 0, NULL, 0, 0, 0, 0, 0},
    {"dump", dumpCommand, 2, "r", 0, NULL, 1, 1, 1, 0, 0},
    {"object", objectCommand, 3, "r", 0, NULL, 2, 2, 2, 0, 0},
    {"client", clientCommand, -2, "rs", 0, NULL, 0, 0, 0, 0, 0},
    {"eval", evalCommand, -3, "s", 0, evalGetKeys, 0, 0, 0, 0, 0},
    {"evalsha", evalShaCommand, -3, "s", 0, evalGetKeys, 0, 0, 0, 0, 0},
    {"slowlog", slowlogCommand, -2, "r", 0, NULL, 0, 0, 0, 0, 0},
    {"script", scriptCommand, -2, "rs", 0, NULL, 0, 0, 0, 0, 0},
    {"time", timeCommand, 1, "rRF", 0, NULL, 0, 0, 0, 0, 0},
    {"bitop", bitopCommand, -4, "wm", 0, NULL, 0, 0, 0, 0, 0},
    {"bitcount", bitcountCommand, -2, "r", 0, NULL, 1, 1, 1, 0, 0},
    {"bitops", bitposCommand, -3, "r", 0, NULL, 1, 1, 1, 0, 0},
    {"wait", waitCommand, 3, "rs", 0, NULL, 0, 0, 0, 0, 0},
    {"command", commandCommand, 0, "rlt", 0, NULL, 0, 0, 0, 0, 0},
    {"pfselftest", pfselftestCommand, 1, "r", 0, NULL, 0, 0, 0, 0, 0},
    {"pfadd", pfaddCommand, -2, "wmF", 0, NULL, 1, 1, 1, 0, 0},
    {"pfcount", pfcountCommand, -2, "r", 0, NULL, 1, 1, 1, 0, 0},
    {"pfmerge", pfmergeCommand, -2, "wm", 0, NULL, 1, -1, 1, 0, 0},
    {"pfdebug", pfdebugCommand, -3, "w", 0, NULL, 0, 0, 0, 0, 0},
    {"latency", latencyCommand, -2, "arslt", 0, NULL, 0, 0, 0, 0, 0},
};

struct evictionPoolEntry *evictionPoolAlloc(void);

void cacheLogRaw(int level, const char *msg) {
  const int syslogLevelMap[] = {LOG_DEBUG, LOG_INFO, LOG_NOTICE, LOG_WARNING};
  const char *c = ".-*#";
  FILE *fp;
  char buf[64];
  int rawmode = (level & CACHE_LOG_RAW);
  int log_to_stdout = server.logfile[0] == '\0';
  level &= 0xff;
  if (level < server.verbosity) return;
  fp = log_to_stdout ? stdout : fopen(server.logfile, "a");
  if (!fp) return;
  if (rawmode) {
    fprintf(fp, "%s", msg);
  } else {
    int off;
    struct timeval tv;
    int role_char;
    pid_t pid = getpid();
    gettimeofday(&tv, NULL);
    off = (int)strftime(buf, sizeof(buf), "%d %b %H:%M:%S",
                        localtime(&tv.tv_sec));
    snprintf(buf + off, sizeof(buf) - off, "%03d", (int)tv.tv_usec / 1000);
    if (server.sentinel_mode) {
      role_char = 'X';
    } else if (pid != server.pid) {
      role_char = 'C';
    } else {
      role_char = (server.masterhost ? 'S' : 'M');
    }
    fprintf(fp, "%d:%c %s %c %s\n", (int)getpid(), role_char, buf, c[level],
            msg);
  }
  fflush(fp);
  if (!log_to_stdout) fclose(fp);
  if (server.syslog_enabled) syslog(syslogLevelMap[level], "%s", msg);
}

void cacheLog(int level, const char *fmt, ...) {
  va_list ap;
  char msg[CACHE_MAX_LOGMSG_LEN];
  if ((level & 0xff) < server.verbosity) return;
  va_start(ap, fmt);
  vsnprintf(msg, sizeof(msg), fmt, ap);
  va_end(ap);
  cacheLogRaw(level, msg);
}

void cacheLogFromHandler(int level, const char *msg) {
  int fd;
  int log_to_stdout = server.logfile[0] == '\0';
  char buf[64];
  if ((level & 0xff) < server.verbosity || (log_to_stdout && server.daemonize))
    return;
  fd = log_to_stdout
           ? STDOUT_FILENO
           : open(server.logfile, O_APPEND | O_CREAT | O_WRONLY, 0644);
  if (fd == -1) return;
  ll2string(buf, sizeof(buf), getpid());
  if (write(fd, buf, strlen(buf)) == -1) goto err;
  if (write(fd, ":signal-handler (", 17) == -1) goto err;
  ll2string(buf, sizeof(buf), time(NULL));
  if (write(fd, buf, strlen(buf)) == -1) goto err;
  if (write(fd, ") ", 2) == -1) goto err;
  if (write(fd, msg, strlen(msg)) == -1) goto err;
  if (write(fd, "\n", 1) == -1) goto err;
err:
  if (!log_to_stdout) close(fd);
}

long long ustime(void) {
  struct timeval tv;
  long long ust;
  gettimeofday(&tv, NULL);
  ust = ((long long)tv.tv_sec) * 1000000;
  ust += tv.tv_usec;
  return ust;
}

long long mstime(void) { return ustime() / 1000; }

void exitFromChild(int retcode) {
#ifdef COVERAGE_TEST
  exit(retcode);
#else
  _exit(retcode);
#endif
}

void dictVanillaFree(void *private, void *val) {
  DICT_NOTUSED(private);
  zfree(val);
}

void dictListDestructor(void *private, void *val) {
  DICT_NOTUSED(private);
  listRelease((List *)val);
}

int dictSdsKeyCompare(void *private, const void *key1, const void *key2) {
  int l1, l2;
  DICT_NOTUSED(private);
  l1 = (int)sdsLen((Sds)key1);
  l2 = (int)sdsLen((Sds)key2);
  if (l1 != l2) return 0;
  return memcmp(key1, key2, l1) == 0;
}

int dictSdsKeyCaseCompare(void *private, const void *key1, const void *key2) {
  DICT_NOTUSED(private);
  return strcasecmp(key1, key2) == 0;
}

void dictCacheObjectDestructor(void *private, void *val) {
  DICT_NOTUSED(private);
  if (val == NULL) return;
  decrRefCount(val);
}

void dictSdsDestructor(void *private, void *val) {
  DICT_NOTUSED(private);
  sdsFree(val);
}

int dictObjKeyCompare(void *private, const void *key1, const void *key2) {
  const cobj *o1 = key1, *o2 = key2;
  return dictSdsKeyCompare(private, o1->ptr, o2->ptr);
}

unsigned int dictObjHash(const void *key) {
  const cobj *o = key;
  return dictGenHashFunction(o->ptr, (int)sdsLen((Sds)o->ptr));
}

unsigned int dictSdsHash(const void *key) {
  return dictGenHashFunction((unsigned char *)key, sdsLen((char *)key));
}

unsigned int dictSdsCaseHash(const void *key) {
  return dictGenCaseHashFunction((unsigned char *)key, sdsLen((char *)key));
}

int dictEncObjKeyCompare(void *privdata, const void *key1, const void *key2) {
  cobj *o1 = (cobj *)key1, *o2 = (cobj *)key2;
  int cmp;
  if (o1->encoding == CACHE_ENCODING_INT &&
      o2->encoding == CACHE_ENCODING_INT) {
    return o1->ptr == o2->ptr;
  }
  o1 = getDecodedObject(o1);
  o2 = getDecodedObject(o2);
  cmp = dictSdsKeyCompare(privdata, o1->ptr, o2->ptr);
  decrRefCount(o1);
  decrRefCount(o2);
  return cmp;
}

unsigned int dictEncObjHash(const void *key) {
  cobj *o = (cobj *)key;
  if (sdsEncodedObject(o)) {
    return dictGenHashFunction(o->ptr, sdsLen((Sds)o->ptr));
  } else {
    if (o->encoding == CACHE_ENCODING_INT) {
      char buf[32];
      int len;
      len = ll2string(buf, 32, (long)o->ptr);
      return dictGenHashFunction((unsigned char *)buf, len);
    } else {
      unsigned int hash;
      o = getDecodedObject(0);
      hash = dictGenHashFunction(o->ptr, sdsLen((Sds)o->ptr));
      decrRefCount(o);
      return hash;
    }
  }
}

DictType setDictType = {dictEncObjHash,
                        NULL,
                        NULL,
                        dictEncObjKeyCompare,
                        dictCacheObjectDestructor,
                        NULL};
DictType zsetDictType = {dictEncObjHash,
                         NULL,
                         NULL,
                         dictEncObjKeyCompare,
                         dictCacheObjectDestructor,
                         NULL};
DictType dbDictType = {dictSdsHash,
                       NULL,
                       NULL,
                       dictSdsKeyCompare,
                       dictSdsDestructor,
                       dictCacheObjectDestructor};
DictType shaScriptObjectDictType = {dictSdsCaseHash,
                                    NULL,
                                    NULL,
                                    dictSdsKeyCaseCompare,
                                    dictSdsDestructor,
                                    dictCacheObjectDestructor};
DictType ketptrDictType = {dictSdsHash,       NULL, NULL,
                           dictSdsKeyCompare, NULL, NULL};

DictType commandRableDictType = {
    dictSdsCaseHash,   NULL, NULL, dictSdsKeyCaseCompare,
    dictSdsDestructor, NULL};

DictType hashDictType = {dictEncObjHash,
                         NULL,
                         NULL,
                         dictEncObjKeyCompare,
                         dictCacheObjectDestructor,
                         dictCacheObjectDestructor};
DictType keylistDictType = {
    dictObjHash,       NULL, NULL, dictObjKeyCompare, dictCacheObjectDestructor,
    dictListDestructor};

DictType clusterNodesDictType = {
    dictSdsHash, NULL, NULL, dictSdsKeyCompare, dictSdsDestructor, NULL,
};

DictType migrateCacheDictType = {
    dictSdsHash, NULL, NULL, dictSdsKeyCompare, dictSdsDestructor, NULL};

DictType replScriptCacheDictType = {
    dictSdsCaseHash,   NULL, NULL, dictSdsKeyCaseCompare,
    dictSdsDestructor, NULL};

int htNeedsResize(Dict *dict) {
  long long size, used;
  size = dictSlots(dict);
  used = dictSize(dict);
  return (size && used && size > DICT_HT_INITIAL_SIZE &&
          (used * 100 / size < CACHE_HT_MINFILL));
}

void tryResizeHashTables(int dbid) {
  if (htNeedsResize(server.db[dbid].dict)) dictResize(server.db[dbid].dict);
  if (htNeedsResize(server.db[dbid].expires))
    dictResize(server.db[dbid].expires);
}

int incrmentallyRehash(int dbid) {
  if (dictIsRehashing(server.db[dbid].dict)) {
    dictRehashMilliseconds(server.db[dbid].dict, 1);
    return 1;
  }
  if (dictIsRehashing(server.db[dbid].expires)) {
    dictRehashMilliseconds(server.db[dbid].expires, 1);
    return 1;
  }
  return 0;
}

void updateDictResizePolicy(void) {
  if (server.rdb_chiled_pid == -1 && server.aof_child_pid == -1) {
    dictEnableResize();
  } else {
    dictDisableResize();
  }
}

int activeExpireCycleTryExpire(cacheDB *db, DictEntry *de, long long now) {
  long long t = dictGetSignedIntegerVal(de);
  if (now > t) {
    Sds key = dictGetKey(de);
    cobj *keyobj = createStringObject(key, SdsLen(key));
    propagateExpire(db, keyobj);
    dbDelete(db, keyobj);
    notifyKeyspaceEvent(CACHE_NOTIFY_EXPIRED, "expired", keyobj, db->id);
    decrRefCount(keyobj);
    server.stat_expiredkeys++;
    return 1;
  } else {
    return 0;
  }
}

void activeExpireCycle(int type) {
  static unsigned int current_db = 0;
  static int time_limit_exit = 0;
  static long long last_fast_cycle = 0;
  int j, iteration = 0;
  int dbs_per_call = CACHE_DBCRON_DBS_PER_CALL;
  long long start = ustime(), time_limit;

  if (type == ACTIVE_EXPIRE_CYCLE_FAST) {
    if (!time_limit_exit) return;
    if (start < last_fast_cycle + ACTIVE_EXPIRE_CYCLE_FAST_DURATION * 2) return;
    last_fast_cycle = start;
  }
  if (dbs_per_call > server.dbnum || time_limit_exit)
    dbs_per_call = server.dbnum;

  time_limit = 1000000 * ACTIVE_EXPIRE_CYCLE_SLOW_TIME_PERC / server.hz / 100;
  time_limit_exit = 0;
  if (time_limit <= 0) time_limit = 1;
  if (type == ACTIVE_EXPIRE_CYCLE_FAST)
    time_limit = ACTIVE_EXPIRE_CYCLE_FAST_DURATION;
  for (j = 0; j < dbs_per_call; j++) {
    int expired;
    cacheDB *db = server.db + (current_db % server.dbnum);
    current_db++;
    do {
      unsigned long num, slots;
      long long now, ttl_sum;
      int ttl_samples;
      if ((num = dictSize(db->expires)) == 0) {
        db->avg_ttl = 0;
        break;
      }
      slots = dictSlots(db->expires);
      now = mstime();
      if (num && slots > DICT_HT_INITIAL_SIZE && (num * 100 / slots < 1)) break;
      expired = 0;
      ttl_sum = 0;
      ttl_samples = 0;
      if (num > ACTIVE_EXPIRE_CYCLE_LOOKUPS_PER_LOOP)
        num = ACTIVE_EXPIRE_CYCLE_LOOKUPS_PER_LOOP;
      while (num--) {
        DictEntry *de;
        long long ttl;
        if ((de = dictGetRandomKey(db->expires)) == NULL) break;
        ttl = dictGetSignedIntegerVal(de) - now;
        if (activeExpireCycleTryExpire(db, de, now)) expired++;
        if (ttl < 0) ttl = 0;
        ttl_sum += ttl;
        ttl_samples++;
      }
      if (ttl_samples) {
        long long avg_ttl = ttl_sum / ttl_samples;
        if (db->avg_ttl == 0) db->avg_ttl = avg_ttl;
        db->avg_ttl = (db->avg_ttl + avg_ttl) / 2;
      }
      iteration++;
      if ((iteration & 0xf) == 0) {
        long long elapsed = ustime() - start;
        latencyAddSampleIfNeeded("expire-cycle", elapsed / 1000);
        if (elapsed > time_limit) time_limit_exit = 1;
      }
      if (time_limit_exit) return;
    } while (expired > ACTIVE_EXPIRE_CYCLE_LOOKUPS_PER_LOOP);
  }
}