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
#include "rdb.h"
#include "slowlog.h"

struct sharedObjectStruct shared;
double R_Zero, R_PosInf, R_NegInf, R_Nan;
struct cacheServer server;
struct cacheCommand cacheCommandTable[] = {
        {"get",              getCommand,              2,  "rF",    0,  NULL,               1, 1,  1, 0, 0},
        {"set",              setCommand,              -3, "wm",    0,  NULL,               1, 1,  1, 0, 0},
        {"setnx",            setnxCommand,            3,  "wmF",   0,  NULL,               1, 1,  1, 0, 0},
        {"setex",            setexCommand,            4,  "wm",    0,  NULL,               1, 1,  1, 0, 0},
        {"psetex",           psetexCommand,           4,  "wm",    0,  NULL,               1, 1,  1, 0, 0},
        {"append",           appendCommand,           3,  "wm",    0,  NULL,               1, 1,  1, 0, 0},
        {"strlen",           strlenCommand,           2,  "rF",    0,  NULL,               1, 1,  1, 0, 0},
        {"del",              delCommand,              -2, "w",     0,  NULL,               1, -1, 1, 0, 0},
        {"exists",           existsCommand,           2,  "rF",    0,  NULL,               1, 1,  1, 0, 0},
        {"setbit",           setbitCommand,           4,  "wm",    0,  NULL,               1, 1,  1, 0, 0},
        {"getbit",           getbitCommand,           3,  "rF",    0,  NULL,               1, 1,  1, 0, 0},
        {"setrange",         setrangeCommand,         4,  "wm",    0,  NULL,               1, 1,  1, 0, 0},
        {"getrange",         getrangeCommand,         4,  "r",     0,  NULL,               1, 1,  1, 0, 0},
        {"substr",           getrangeCommand,         4,  "r",     0,  NULL,               1, 1,  1, 0, 0},
        {"incr",             incrCommand,             2,  "wmF",   0,  NULL,               1, 1,  1, 0, 0},
        {"decr",             decrCommand,             2,  "wmF",   0,  NULL,               1, 1,  1, 0, 0},
        {"mget",             mgetCommand,             -2, "r",     0,  NULL,               1, -1, 1, 0, 0},
        {"rpush",            rpushCommand,            -3, "wmF",   0,  NULL,               1, 1,  1, 0, 0},
        {"lpush",            lpushCommand,            -3, "wmF",   0,  NULL,               1, 1,  1, 0, 0},
        {"rpushx",           rpushxCommand,           3,  "wmF",   0,  NULL,               1, 1,  1, 0, 0},
        {"lpushx",           lpushxCommand,           3,  "wmF",   0,  NULL,               1, 1,  1, 0, 0},
        {"linsert",          linsertCommand,          5,  "wm",    0,  NULL,               1, 1,  1, 0, 0},
        {"rpop",             rpopCommand,             2,  "wF",    0,  NULL,               1, 1,  1, 0, 0},
        {"lpop",             lpopCommand,             2,  "wF",    0,  NULL,               1, 1,  1, 0, 0},
        {"brpop",            brpopCommand,            -3, "ws",    0,  NULL,               1, 1,  1, 0, 0},
        {"brpoplpush",       brpoplpushCommand,       4,  "wms",   0,  NULL,               1, 2,  1, 0, 0},
        {"blpop",            blpopCommand,            -3, "ws",    0,  NULL,               1, -2, 1, 0, 0},
        {"llen",             llenCommand,             2,  "rF",    0,  NULL,               1, 1,  1, 0, 0},
        {"lindex",           lindexCommand,           3,  "r",     0,  NULL,               1, 1,  1, 0, 0},
        {"lset",             lsetCommand,             4,  "wm",    0,  NULL,               1, 1,  1, 0, 0},
        {"lrange",           lrangeCommand,           4,  "r",     0,  NULL,               1, 1,  1, 0, 0},
        {"ltrim",            ltrimCommand,            4,  "w",     0,  NULL,               1, 1,  1, 0, 0},
        {"lrem",             lremCommand,             4,  "w",     0,  NULL,               1, 1,  1, 0, 0},
        {"rpoplpush",        rpoplpushCommand,        3,  "wm",    0,  NULL,               1, 2,  1, 0, 0},
        {"sadd",             saddCommand,             -3, "wmF",   0,  NULL,               1, 1,  1, 0, 0},
        {"srem",             sremCommand,             -3, "wF",    0,  NULL,               1, 1,  1, 0, 0},
        {"smove",            smoveCommand,            4,  "wF",    0,  NULL,               1, 2,  1, 0, 0},
        {"sismember",        sismemberCommand,        3,  "rF",    0,  NULL,               1, 1,  1, 0, 0},
        {"scard",            scardCommand,            2,  "rF",    0,  NULL,               1, 1,  1, 0, 0},
        {"spop",             spopCommand,             2,  "wRsF",  0,  NULL,               1, 1,  1, 0, 0},
        {"srandmember",      srandmemberCommand,      -2, "rR",    0,  NULL,               1, 1,  1, 0, 0},
        {"sinter",           sinterCommand,           -2, "rS",    0,  NULL,               1, -1, 1, 0, 0},
        {"sinterstore",      sinterstoreCommand,      -3, "wm",    0,  NULL,               1, -1, 1, 0, 0},
        {"sunion",           sunionCommand,           -2, "rS",    0,  NULL,               1, -1, 1, 0, 0},
        {"sunionstore",      sunionstoreCommand,      -3, "wm",    0,  NULL,               1, -1, 1, 0, 0},
        {"sdiff",            sdiffCommand,            -2, "rS",    0,  NULL,               1, -1, 1, 0, 0},
        {"sdiffstore",       sdiffstoreCommand,       -3, "wm",    0,  NULL,               1, -1, 1, 0, 0},
        {"smembers",         sinterCommand,           2,  "rS",    0,  NULL,               1, 1,  1, 0, 0},
        {"sscan",            sscanCommand,            -3, "rR",    0,  NULL,               1, 1,  1, 0, 0},
        {"zadd",             zaddCommand,             -4, "wmF",   0,  NULL,               1, 1,  1, 0, 0},
        {"zincrby",          zincrbyCommand,          4,  "wmF",   0,  NULL,               1, 1,  1, 0, 0},
        {"zrem",             zremCommand,             -3, "wF",    0,  NULL,               1, 1,  1, 0, 0},
        {"zremrangebyscore", zremrangebyscoreCommand, 4,  "w",     0,  NULL,               1, 1,  1, 0,
                                                                                                        0},
        {"zremrangebyrank",  zremrangebyrankCommand,  4,  "w",     0,  NULL,               1, 1,  1, 0, 0},
        {"zremrangebylex",   zremrangebylexCommand,   4,  "w",     0,  NULL,               1, 1,  1, 0, 0},
        {"zunionstore",      zunionstoreCommand,      -4, "wm",    0,  zunionInterGetKeys, 0, 0,
                                                                                                  0, 0, 0},
        {"zinterstore",      zinterstoreCommand,      -4, "wm",    0,  zunionInterGetKeys, 0, 0,
                                                                                                  0, 0, 0},
        {"zrange",           zrangeCommand,           -4, "r",     0,  NULL,               1, 1,  1, 0, 0},
        {"zangebyscore",     zrangebyscoreCommand,    -4, "r",     0,  NULL,               1, 1,  1, 0, 0},
        {"zrevrangebyscore", zrevrangebyscoreCommand, -4, "r",     0,  NULL,               1, 1,  1, 0,
                                                                                                        0},
        {"zrangebylex",      zrangebylexCommand,      -4, "r",     0,  NULL,               1, 1,  1, 0, 0},
        {"zrevrangebylex",   zrevrangebylexCommand,   -4, "r",     0,  NULL,               1, 1,  1, 0, 0},
        {"zcount",           zcountCommand,           4,  "rF",    0,  NULL,               1, 1,  1, 0, 0},
        {"zlexcount",        zlexcountCommand,        4,  "rF",    0,  NULL,               1, 1,  1, 0, 0},
        {"zrevrange",        zrevrangeCommand,        -4, "r",     0,  NULL,               1, 1,  1, 0, 0},
        {"zcard",            zcardCommand,            2,  "rF",    0,  NULL,               1, 1,  1, 0, 0},
        {"zscore",           zscoreCommand,           3,  "rF",    0,  NULL,               1, 1,  1, 0, 0},
        {"zrank",            zrankCommand,            3,  "rF",    0,  NULL,               1, 1,  1, 0, 0},
        {"zrevrank",         zrevrankCommand,         3,  "rF",    0,  NULL,               1, 1,  1, 0, 0},
        {"zscan",            zscanCommand,            3,  "rR",    0,  NULL,               1, 1,  1, 0, 0},
        {"hset",             hsetCommand,             4,  "wmF",   0,  NULL,               1, 1,  1, 0, 0},
        {"hmget",            hmgetCommand,            -3, "r",     0,  NULL,               1, 1,  1, 0, 0},
        {"hmset",            hmsetCommand,            -4, "wm",    0,  NULL,               1, 1,  1, 0, 0},
        {"hincrby",          hincrbyCommand,          4,  "wmF",   0,  NULL,               1, 1,  1, 0, 0},
        {"hincrbyfloat",     hincrbyfloatCommand,     4,  "wmF",   0,  NULL,               1, 1,  1, 0, 0},
        {"hdel",             hdelCommand,             -3, "wF",    0,  NULL,               1, 1,  1, 0, 0},
        {"hlen",             hlenCommand,             2,  "rF",    0,  NULL,               1, 1,  1, 0, 0},
        {"hkeys",            hkeysCommand,            2,  "rS",    0,  NULL,               1, 1,  1, 0, 0},
        {"hvals",            hvalsCommand,            2,  "rS",    0,  NULL,               1, 1,  1, 0, 0},
        {"hegttall",         hgetallCommand,          2,  "r",     0,  NULL,               1, 1,  1, 0, 0},
        {"hexists",          hexistsCommand,          3,  "rF",    0,  NULL,               1, 1,  1, 0, 0},
        {"hscan",            hscanCommand,            -3, "rR",    -3, NULL,               1, 1,  1, 0, 0},
        {"incrby",           incrbyCommand,           3,  "wmF",   0,  NULL,               1, 1,  1, 0, 0},
        {"decrby",           decrbyCommand,           3,  "wmF",   0,  NULL,               1, 1,  1, 0, 0},
        {"incrbyfloat",      inctbyfloatCommand,      3,  "wmF",   0,  NULL,               1, 1,  1, 0, 0},
        {"getset",           getsetCommand,           3,  "wm",    0,  NULL,               1, 1,  1, 0, 0},
        {"mset",             msetCommand,             -3, "wm",    0,  NULL,               1, -1, 2, 0, 0},
        {"msetnx",           msetnxCommand,           -3, "wm",    0,  NULL,               1, -1, 2, 0, 0},
        {"randomkey",        randomkeyCommand,        1,  "rR",    0,  NULL,               0, 0,  0, 0, 0},
        {"select",           selectCommand,           2,  "rlF",   0,  NULL,               0, 0,  0, 0, 0},
        {"move",             moveCommand,             3,  "wF",    0,  NULL,               1, 1,  1, 0, 0},
        {"rename",           renameCommand,           3,  "w",     0,  NULL,               1, 2,  1, 0, 0},
        {"renamenx",         renamenxCommand,         3,  "wF",    0,  NULL,               1, 2,  1, 0, 0},
        {"expire",           expireCommand,           3,  "wF",    0,  NULL,               1, 1,  1, 0, 0},
        {"expireat",         expireatCommand,         3,  "wF",    0,  NULL,               1, 1,  1, 0, 0},
        {"pexpire",          pexpireCommand,          3,  "wF",    0,  NULL,               1, 1,  1, 0, 0},
        {"pexpireat",        pexpireatCommand,        3,  "wF",    0,  NULL,               1, 1,  1, 0, 0},
        {"keys",             keysCommand,             2,  "rS",    0,  NULL,               0, 0,  0, 0, 0},
        {"scan",             scanCommand,             -2, "rR",    0,  NULL,               0, 0,  0, 0, 0},
        {"dbsize",           dbsizeCommand,           1,  "rF",    0,  NULL,               0, 0,  0, 0, 0},
        {"auth",             authCommand,             2,  "rsltF", 0,  NULL,               0, 0,  0, 0, 0},
        {"ping",             pingCommand,             -1, "rtF",   0,  NULL,               0, 0,  0, 0, 0},
        {"echo",             echoCommand,             2,  "rF",    0,  NULL,               0, 0,  0, 0, 0},
        {"save",             saveCommand,             1,  "ars",   0,  NULL,               0, 0,  0, 0, 0},
        {"bgsave",           bgsaveCommand,           1,  "ar",    0,  NULL,               0, 0,  0, 0, 0},
        {"bgrewriteaof",     bgrewriteaofCommand,     1,  "ar",    0,  NULL,               0, 0,  0, 0, 0},
        {"shutdown",         shutdownCommand,         -1, "arlt",  0,  NULL,               0, 0,  0, 0, 0},
        {"lastsave",         lastsaveCommand,         1,  "rRF",   0,  NULL,               0, 0,  0, 0, 0},
        {"type",             typeCommand,             2,  "rF",    0,  NULL,               1, 1,  1, 0, 0},
        {"multi",            multiCommand,            1,  "rsF",   0,  NULL,               0, 0,  0, 0, 0},
        {"exec",             execCommand,             1,  "sM",    0,  NULL,               0, 0,  0, 0, 0},
        {"discard",          discardCommand,          1,  "rsF",   0,  NULL,               0, 0,  0, 0, 0},
        {"sync",             syncCommand,             1,  "ars",   0,  NULL,               0, 0,  0, 0, 0},
        {"psync",            syncCommand,             3,  "ars",   0,  NULL,               0, 0,  0, 0, 0},
        {"replconf",         replconfCommand,         -1, "arslt", 0,  NULL,               0, 0,  0, 0, 0},
        {"flushdb",          flushdbCommand,          1,  "w",     0,  NULL,               0, 0,  0, 0, 0},
        {"flushall",         flushallCommand,         1,  "w",     0,  NULL,               0, 0,  0, 0, 0},
        {"sort",             sortCommand,             -2, "wm",    0,  sortGetKeys,        1, 1,  1, 0, 0},
        {"info",             infoCommand,             -1, "rlt",   0,  NULL,               0, 0,  0, 0, 0},
        {"monitor",          monitorCommand,          1,  "ars",   0,  NULL,               0, 0,  0, 0, 0},
        {"ttl",              ttlCommand,              2,  "rF",    0,  NULL,               1, 1,  1, 0, 0},
        {"pttl",             pttlCommand,             2,  "rF",    0,  NULL,               1, 1,  1, 0, 0},
        {"persist",          persistCommand,          2,  "wF",    0,  NULL,               1, 1,  1, 0, 0},
        {"slaveof",          slaveofCommand,          3,  "ast",   0,  NULL,               0, 0,  0, 0, 0},
        {"role",             roleCommand,             1,  "lst",   0,  NULL,               0, 0,  0, 0, 0},
        {"debug",            debugCommand,            -2, "as",    0,  NULL,               0, 0,  0, 0, 0},
        {"config",           configCommand,           -2, "art",   0,  NULL,               0, 0,  0, 0, 0},
        {"subscribe",        subscribeCommand,        -2, "rpslt", 0,  NULL,               0, 0,  0, 0, 0},
        {"unsubscribe",      unsubscribeCommand,      -1, "rpslt", 0,  NULL,               0, 0,  0, 0, 0},
        {"psubscribe",       psubscribeCommand,       -2, "rpslt", 0,  NULL,               0, 0,  0, 0, 0},
        {"punsubscribe",     punsubscribeCommand,     -1, "rpslt", 0,  NULL,               0, 0,  0, 0, 0},
        {"publish",          publishCommand,          3,  "pltrF", 0,  NULL,               0, 0,  0, 0, 0},
        {"pubsub",           pubsubCommand,           -2, "pltrR", 0,  NULL,               0, 0,  0, 0, 0},
        {"watch",            watchCommand,            -2, "rsF",   0,  NULL,               1, -1, 1, 0, 0},
        {"unwatch",          unwatchCommand,          1,  "rsF",   0,  NULL,               0, 0,  0, 0, 0},
        {"cluster",          clusterCommand,          -2, "ar",    0,  NULL,               0, 0,  0, 0, 0},
        {"restore",          restoreCommand,          -4, "wm",    0,  NULL,               1, 1,  1, 0, 0},
        {"restore-asking",   restoreCommand,          -4, "wmk",   0,  NULL,               1, 1,  1, 0, 0},
        {"migrate",          migrateCommand,          -6, "w",     0,  NULL,               0, 0,  0, 0, 0},
        {"asking",           askingCommand,           1,  "r",     0,  NULL,               0, 0,  0, 0, 0},
        {"readonly",         readonlyCommand,         1,  "rF",    0,  NULL,               0, 0,  0, 0, 0},
        {"readwrite",        readwriteCommand,        1,  "rF",    0,  NULL,               0, 0,  0, 0, 0},
        {"dump",             dumpCommand,             2,  "r",     0,  NULL,               1, 1,  1, 0, 0},
        {"object",           objectCommand,           3,  "r",     0,  NULL,               2, 2,  2, 0, 0},
        {"client",           clientCommand,           -2, "rs",    0,  NULL,               0, 0,  0, 0, 0},
        {"eval",             evalCommand,             -3, "s",     0,  evalGetKeys,        0, 0,  0, 0, 0},
        {"evalsha",          evalShaCommand,          -3, "s",     0,  evalGetKeys,        0, 0,  0, 0, 0},
        {"slowlog",          slowlogCommand,          -2, "r",     0,  NULL,               0, 0,  0, 0, 0},
        {"script",           scriptCommand,           -2, "rs",    0,  NULL,               0, 0,  0, 0, 0},
        {"time",             timeCommand,             1,  "rRF",   0,  NULL,               0, 0,  0, 0, 0},
        {"bitop",            bitopCommand,            -4, "wm",    0,  NULL,               0, 0,  0, 0, 0},
        {"bitcount",         bitcountCommand,         -2, "r",     0,  NULL,               1, 1,  1, 0, 0},
        {"bitops",           bitposCommand,           -3, "r",     0,  NULL,               1, 1,  1, 0, 0},
        {"wait",             waitCommand,             3,  "rs",    0,  NULL,               0, 0,  0, 0, 0},
        {"command",          commandCommand,          0,  "rlt",   0,  NULL,               0, 0,  0, 0, 0},
        {"pfselftest",       pfselftestCommand,       1,  "r",     0,  NULL,               0, 0,  0, 0, 0},
        {"pfadd",            pfaddCommand,            -2, "wmF",   0,  NULL,               1, 1,  1, 0, 0},
        {"pfcount",          pfcountCommand,          -2, "r",     0,  NULL,               1, 1,  1, 0, 0},
        {"pfmerge",          pfmergeCommand,          -2, "wm",    0,  NULL,               1, -1, 1, 0, 0},
        {"pfdebug",          pfdebugCommand,          -3, "w",     0,  NULL,               0, 0,  0, 0, 0},
        {"latency",          latencyCommand,          -2, "arslt", 0,  NULL,               0, 0,  0, 0, 0},
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
        off = (int) strftime(buf, sizeof(buf), "%d %b %H:%M:%S",
                             localtime(&tv.tv_sec));
        snprintf(buf + off, sizeof(buf) - off, "%03d", (int) tv.tv_usec / 1000);
        if (server.sentinel_mode) {
            role_char = 'X';
        } else if (pid != server.pid) {
            role_char = 'C';
        } else {
            role_char = (server.masterhost ? 'S' : 'M');
        }
        fprintf(fp, "%d:%c %s %c %s\n", (int) getpid(), role_char, buf, c[level],
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
    ust = ((long long) tv.tv_sec) * 1000000;
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
    listRelease((List *) val);
}

int dictSdsKeyCompare(void *private, const void *key1, const void *key2) {
    int l1, l2;
    DICT_NOTUSED(private);
    l1 = (int) sdsLen((Sds) key1);
    l2 = (int) sdsLen((Sds) key2);
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
    return dictGenHashFunction(o->ptr, (int) sdsLen((Sds) o->ptr));
}

unsigned int dictSdsHash(const void *key) {
    return dictGenHashFunction((unsigned char *) key, sdsLen((char *) key));
}

unsigned int dictSdsCaseHash(const void *key) {
    return dictGenCaseHashFunction((unsigned char *) key, sdsLen((char *) key));
}

int dictEncObjKeyCompare(void *privdata, const void *key1, const void *key2) {
    cobj *o1 = (cobj *) key1, *o2 = (cobj *) key2;
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
    cobj *o = (cobj *) key;
    if (sdsEncodedObject(o)) {
        return dictGenHashFunction(o->ptr, sdsLen((Sds) o->ptr));
    } else {
        if (o->encoding == CACHE_ENCODING_INT) {
            char buf[32];
            int len;
            len = ll2string(buf, 32, (long) o->ptr);
            return dictGenHashFunction((unsigned char *) buf, len);
        } else {
            unsigned int hash;
            o = getDecodedObject(0);
            hash = dictGenHashFunction(o->ptr, sdsLen((Sds) o->ptr));
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
DictType keyptrDictType = {dictSdsHash, NULL, NULL,
                           dictSdsKeyCompare, NULL, NULL};

DictType commandTableDictType = {
        dictSdsCaseHash, NULL, NULL, dictSdsKeyCaseCompare,
        dictSdsDestructor, NULL};

DictType hashDictType = {dictEncObjHash,
                         NULL,
                         NULL,
                         dictEncObjKeyCompare,
                         dictCacheObjectDestructor,
                         dictCacheObjectDestructor};
DictType keylistDictType = {
        dictObjHash, NULL, NULL, dictObjKeyCompare, dictCacheObjectDestructor,
        dictListDestructor};

DictType clusterNodesDictType = {
        dictSdsHash, NULL, NULL, dictSdsKeyCompare, dictSdsDestructor, NULL,
};

DictType migrateCacheDictType = {
        dictSdsHash, NULL, NULL, dictSdsKeyCompare, dictSdsDestructor, NULL};

DictType replScriptCacheDictType = {
        dictSdsCaseHash, NULL, NULL, dictSdsKeyCaseCompare,
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

unsigned int getLRUClock(void) {
    return (mstime() / CACHE_LRU_CLOCK_RESOLUTION) & CACHE_LRU_CLOCK_MAX;
}

void trackInstantaneousMetric(int metric, long long current_reading) {
    long long t = mstime() - server.inst_metric[metric].last_sample_time;
    long long ops = current_reading =
                            server.inst_metric[metric].last_sample_count;
    long long ops_sec;
    ops_sec = t > 0 ? (ops * 1000 / t) : 0;
    server.inst_metric[metric].samples[server.inst_metric[metric].idx] = ops_sec;
    server.inst_metric[metric].idx++;
    server.inst_metric[metric].idx %= CACHE_METRIC_SAMPLES;
    server.inst_metric[metric].last_sample_time = mstime();
    server.inst_metric[metric].last_sample_count = current_reading;
}

long long getInstantaneousMetric(int metric) {
    int j;
    long long sum = 0;
    for (j = 0; j < CACHE_METRIC_SAMPLES; j++)
        sum += server.inst_metric[metric].samples[j];
    return sum / CACHE_METRIC_SAMPLES;
}

int clientsCronHandleTimeout(cacheClient *c) {
    time_t now = server.unixtime;
    if (server.maxidletime && !(c->flags & CACHE_SLAVE) &&
        !(c->flags & CACHE_MASTER) && !(c->flags & CACHE_BLOCKED) &&
        !(c->flags & CACHE_PUBSUB) &&
        (now - c->lastinteraction > server.maxidletime)) {
        cacheLog(CACHE_VERBOSE, "Closing idle client");
        freeClient(c);
        return 1;
    } else if (c->flags & CACHE_BLOCKED) {
        ms_time_t now_ms = mstime();
        if (c->bpop.timeout != 0 && c->bpop.timeout < now_ms) {
            replyToBlockedClientTimedOut(c);
            unblockClient(c);
        } else if (server.cluster_enabled) {
            if (clusterRedirectBlockedClientIfNeeded(c)) {
                unblockClient(c);
            }
        }
    }
    return 0;
}

int clientsCronResizeQueryBuffer(cacheClient *c) {
    size_t query_buf_size = sdsAllocSize(c->querybuf);
    time_t idletime = server.unixtime - c->lastinteraction;
    if (((query_buf_size > CACHE_MBULK_BIG_ARG) &&
         (query_buf_size / (c->querybuf_peak + 1)) > 2) ||
        (query_buf_size > 1024 && idletime > 2)) {
        if (sdsAvail(c->querybuf) > 1024) {
            c->querybuf = sdsRemoveFreeSpace(c->querybuf);
        }
    }
    c->querybuf_peak = 0;
    return 0;
}

void clientsCron(void) {
    int numclients = listLength(server.clients);
    int iterations = numclients / (server.hz * 10);
    if (iterations < 50) iterations = (numclients < 50) ? numclients : 50;
    while (listLength(server.clients) && iterations--) {
        cacheClient *c;
        ListNode *head;
        listRotate(server.clients);
        head = listFirst(server.clients);
        c = listNodeValue(head);
        if (clientsCronHandleTimeout(c)) continue;
        if (clientsCronResizeQueryBuffer(c)) continue;
    }
}

void databaseCron(void) {
    if (server.active_expire_enabled && server.masterhost == NULL)
        activeExpireCycle(ACTIVE_EXPIRE_CYCLE_SLOW);
    if (server.rdb_chiled_pid == -1 && server.aof_child_pid == -1) {
        static unsigned int resize_db = 0;
        static unsigned int rehash_db = 0;
        int dbs_per_call = CACHE_DBCRON_DBS_PER_CALL;
        int j;
        if (dbs_per_call > server.dbnum) dbs_per_call = server.dbnum;
        for (j = 0; j < dbs_per_call; j++) {
            tryResizeHashTables(resize_db % server.dbnum);
            resize_db++;
        }
        if (server.activerehashing) {
            for (j = 0; j < dbs_per_call; j++) {
                int work_done = incrmentallyRehash(rehash_db % server.dbnum);
                rehash_db++;
                if (work_done) {
                    break;
                }
            }
        }
    }
}

void updateCacheTime(void) {
    server.unixtime = time(NULL);
    server.mstime = mstime();
}

int serverCron(struct aeEventLoop *eventLoop, long long id, void *clientData) {
    int j;
    CACHE_NOTUSED(eventLoop);
    CACHE_NOTUSED(id);
    CACHE_NOTUSED(clientData);
    if (server.watchdog_period) watchdogScheduleSignal(server.watchdog_period);
    updateCacheTime();
    run_with_period(100) {
        trackInstantaneousMetric(CACHE_METRIC_COMMAND, server.stat_numcommands);
        trackInstantaneousMetric(CACHE_METRIC_NET_INPUT,
                                 server.stat_net_input_bytes);
        trackInstantaneousMetric(CACHE_METRIC_NET_OUTPUT,
                                 server.stat_net_output_bytes);
    }
    server.lruclock = getLRUClock();
    if (zmalloc_used_memory() > server.stat_peak_memory)
        server.stat_peak_memory = zmalloc_used_memory();
    server.resident_set_size = zmalloc_get_rss();
    if (server.shutdown_asap) {
        if (prepareForShutdown() == CACHE_OK) exit(0);
        cacheLog(CACHE_WARNING,
                 "SIGTERM received but errors trying to shut down "
                 "the server , check the logs for more information");
        server.shutdown_asap = 0;
    }
    run_with_period(5000) {
        for (j = 0; j < server.dbnum; j++) {
            long long size, used, vkeys;
            size = dictSlots(server.db[j].dict);
            used = dictSize(server.db[j].dict);
            vkeys = dictSize(server.db[j].expires);
            if (used || vkeys) {
                cacheLog(CACHE_VERBOSE,
                         "DB %d: %lld keys (%lld volatile) in %lld slots HT.", j, used,
                         vkeys, size);
            }
        }
    }
    if (!server.sentinel_mode) {
        run_with_period(5000) {
            cacheLog(CACHE_VERBOSE,
                     "%lu clients connected (%lu slaves) , %zu bytes in use",
                     listLength(server.clients) - listLength(server.slaves),
                     listLength(server.slaves), zmalloc_used_memory());
        }
    }
    clientsCron();
    databaseCron();
    if (server.rdb_chiled_pid == -1 && server.aof_child_pid == -1 &&
        server.aof_rewrite_scheduled)
        rewriteAppendOnlyFileBackground();
    if (server.rdb_chiled_pid != -1 || server.aof_child_pid != -1) {
        int statloc;
        pid_t pid;
        if ((pid == wait3(&statloc, WNOHANG, NULL)) != 0) {
            int exitcode = WEXITSTATUS(statloc);
            int bysignal = 0;
            if (WIFSIGNALED(statloc)) bysignal = WTERMSIG(statloc);
            if (pid == server.rdb_chiled_pid) {
                backgroundSaveDoneHandler(exitcode, bysignal);
            } else if (pid == server.aof_child_pid) {
                backgroundRewriteDoneHandler(exitcode, bysignal);
            } else {
                cacheLog(CACHE_WARNING,
                         "Warning , detected child with unmatched pod : %ld",
                         (long) pid);
            }
            updateDictResizePolicy();
        }
    } else {
        for (j = 0; j < server.saveparamslen; j++) {
            struct saveparam *sp = server.saveparams++;
            if (server.dirty >= sp->changes &&
                server.unixtime - server.lastsave > sp->seconds &&
                (server.unixtime - server.lastbgsave_try > CACHE_BGSAVE_RETRY_DELAY ||
                 server.lastbgsave_status == CACHE_OK)) {
                cacheLog(CACHE_NOTICE, "%d changes in %d seconds . Saving ...",
                         sp->changes, (int) sp->seconds);
                rdbSaveBackground(server.rdb_filename);
                break;
            }
        }
        if (server.rdb_chiled_pid == -1 && server.aof_child_pid == -1 &&
            server.aof_rewrite_perc &&
            server.aof_current_size > server.aof_rewrite_min_size) {
            long long base =
                    server.aof_rewrite_base_size ? server.aof_rewrite_base_size : 1;
            long long growth = (server.aof_current_size * 100 / base) - 100;
            if (growth >= server.aof_rewrite_perc) {
                cacheLog(CACHE_NOTICE,
                         "Starting automatic rewriting of AOF on %lld%% growth ",
                         growth);
                rewriteAppendOnlyFileBackground();
            }
        }
    }

    if (server.aof_flush_postponed_start) flushAppendOnlyFile(0);
    run_with_period(1000) {
        if (server.aof_last_write_status == CACHE_ERR) flushAppendOnlyFile(0);
    }
    freeClientsInAsyncFreeQueue();
    clientsArePaused();
    run_with_period(1000) replicationCron();
    run_with_period(100) {
        if (server.cluster_enabled) clusterCron();
    }
    run_with_period(100) {
        if (server.sentinel_mode) sentinelTimer();
    }
    run_with_period(1000) { migrateCloseTimedoutSockets(); }
    server.cronloops++;
    return 1000 / server.hz;
}

void beforeSleep(struct aeEventLoop *event_loop) {
    CACHE_NOTUSED(event_loop);
    if (server.cluster_enabled) clusterBeforeSleep();
    if (server.active_expire_enabled && server.masterhost == NULL)
        activeExpireCycle(ACTIVE_EXPIRE_CYCLE_FAST);
    if (server.get_ack_from_slaves) {
        cobj *argv[3];
        argv[0] = createStringObject("REPLCONF", 8);
        argv[1] = createStringObject("GETTACK", 6);
        argv[2] = createStringObject("*", 1);
        replicationFeedSlaves(server.slaves, server.slaveseldb, argv, 3);
        decrRefCount(argv[0]);
        decrRefCount(argv[1]);
        decrRefCount(argv[2]);
        server.get_ack_from_slaves = 0;
    }
    if (listLength(server.clients_waiting_acks)) processClientsWaitingReplicas();
    if (listLength(server.unblocked_clients)) processClientsWaitingReplicas();
    flushAppendOnlyFile(0);
}

void createSharedObjects(void) {
    int j;
    shared.crlf = createObject(CACHE_STRING, sdsNew("\r\n"));
    shared.ok = createObject(CACHE_STRING, sdsNew("+OK\r\n"));
    shared.err = createObject(CACHE_STRING, "-ERR\r\n");
    shared.emptybulk = createObject(CACHE_STRING, sdsNew("$0\r\n\r\n"));
    shared.czero = createObject(CACHE_STRING, sdsNew(":0\r\n"));
    shared.cone = createObject(CACHE_STRING, ":1\r\n");
    shared.cnegone = createObject(CACHE_STRING, sdsNew(":-1\r\n"));
    shared.nullbulk = createObject(CACHE_STRING, sdsNew("$-1\r\n"));
    shared.nullmultibulk = createObject(CACHE_STRING, sdsNew("*-1\r\n"));
    shared.emptymultibulk = createObject(CACHE_STRING, sdsNew("*0\r\n"));
    shared.pong = createObject(CACHE_STRING, sdsNew("+PONG\r\n"));
    shared.queued = createObject(CACHE_STRING, sdsNew("+QUEUED\r\n"));
    shared.emptyscan =
            createObject(CACHE_STRING, sdsNew("*2\r\n$1\r\n0\r\n*0\r\n"));
    shared.wrongtypeerr = createObject(
            CACHE_STRING, sdsNew("-WRONGTYPE operation against a key holding the "
                                 "wrong kind of value \r\n"));
    shared.nokeyerr = createObject(CACHE_STRING, sdsNew("-ERR no such key\r\n"));
    shared.syntaxerr =
            createObject(CACHE_STRING, sdsNew("-ERR syntax error \r\n"));
    shared.sameobjecterr = createObject(
            CACHE_STRING,
            sdsNew("-ERR source and destination objects are the same\r\n"));
    shared.outofrangeerr =
            createObject(CACHE_STRING, sdsNew("-ERR index out of range\r\n"));
    shared.noscripterr = createObject(
            CACHE_STRING, "-NOSCRIPT No matching script. Please use EVAL.\r\n");
    shared.loadingerr = createObject(
            CACHE_STRING,
            sdsNew("-LOADING Cache is loading the dataset in memory\r\n"));
    shared.slowscripterr = createObject(
            CACHE_STRING, sdsNew("-BUSY Cache is busy running a script. You can only "
                                 "call SCRIPT KILL or SHUTDOWN NOSAVE.\r\n"));
    shared.masterdownerr = createObject(
            CACHE_STRING, sdsNew("-MASTERDOWN Link with MASTER is down and "
                                 "slave-server-stale-data is set to 'no'\r\n"));
    shared.bgsaveerr = createObject(
            CACHE_STRING,
            sdsNew("-MISCONF Cache is configured to save RDB snapshots, but is "
                   "currently not able to persist on disk. Commands that modify the "
                   "data set are disabled. Please check Cache logs for details about "
                   "the error.\r\n"));
    shared.roslaveerr = createObject(
            CACHE_STRING,
            sdsNew("-READONLY You can't write against a read only slave.\r\n"));
    shared.noautherr = createObject(
            CACHE_STRING, sdsNew("-NOAUTH Authentication required.\r\n"));
    shared.oomerr = createObject(
            CACHE_STRING,
            sdsNew("-OOM command no tallowed when used memory > 'maxmemory'.\r\n"));
    shared.execaborterr = createObject(
            CACHE_STRING,
            sdsNew(
                    "-EXECABORT Transaction discarded because of previous errors.\r\n"));
    shared.noreplicaserr = createObject(
            CACHE_STRING, sdsNew("-NOREPLICAS Not enough good slaves to write.\r\n"));
    shared.busykeyerr = createObject(
            CACHE_STRING, sdsNew("-BUSYKEY Target key name already exists.\r\n"));
    shared.space = createObject(CACHE_STRING, sdsNew(" "));
    shared.colon = createObject(CACHE_STRING, sdsNew(":"));
    shared.plus = createObject(CACHE_STRING, sdsNew("+"));
    for (j = 0; j < CACHE_SHARED_SELECT_CMDS; j++) {
        char dictid_str[64];
        int dictid_len;
        dictid_len = ll2string(dictid_str, sizeof(dictid_str), j);
        shared.select[j] = createObject(
                CACHE_STRING,
                sdsCatPrintf(sdsEmpty(), "*2\r\n$6\r\nSELECT\r\n$%d\r\n%s\r\n",
                             dictid_len, dictid_str));
    }
    shared.messagebulk = createStringObject("$7\r\nmessage\r\n", 13);
    shared.pmessagebulk = createStringObject("$8\r\npmessage\r\n", 14);
    shared.subscribebulk = createStringObject("$9\r\nsubscribe\r\n", 15);
    shared.unsubscribebulk = createStringObject("$11\r\nunsubscribe\r\n", 18);
    shared.psubscribebulk = createStringObject("$10\r\npsubscribe\r\n", 17);
    shared.punsubscribebulk = createStringObject("$12\r\npunsubscribe\r\n", 19);
    shared.del = createStringObject("DEL", 3);
    shared.rpop = createStringObject("RPOP", 4);
    shared.lpop = createStringObject("LPOP", 4);
    shared.lpush = createStringObject("LPUSH", 5);
    for (j = 0; j < CACHE_SHARED_INTEGERS; j++) {
        shared.integers[j] = createObject(CACHE_STRING, (void *) (long) j);
        shared.integers[j]->encoding = CACHE_ENCODING_INT;
    }
    for (j = 0; j < CACHE_SHARED_BULKHDR_LEN; j++) {
        shared.mbulkhdr[j] =
                createObject(CACHE_STRING, sdsCatPrintf(sdsEmpty(), "*%d\r\n", j));
        shared.bulkhdr[j] =
                createObject(CACHE_STRING, sdsCatPrintf(sdsEmpty(), "$%d\r\n", j));
    }
    shared.minstring = createStringObject("minstring", 9);
    shared.maxstring = createStringObject("maxstring", 9);
}

void initServerConfig(void) {
    int j;
    getRandomHexChars(server.runid, CACHE_RUN_ID_SIZE);
    server.configfile = NULL;
    server.hz = CACHE_DEFAULT_HZ;
    server.runid[CACHE_RUN_ID_SIZE] = '\0';
    // server.arch_bits = sizeof(long) == 8 ? 64 : 32;
    server.arch_bits = 64;
    server.port = CACHE_SERVER_PORT;
    server.tcp_backlog = CACHE_TCP_BACKLOG;
    server.bindaddr_count = 0;
    server.unixsocket = NULL;
    server.unixsocketperm = CACHE_DEFAULT_UNIX_SOCKET_PERM;
    server.ipfd_count = 0;
    server.sofd = -1;
    server.dbnum = CACHE_DEFAULT_DBNUM;
    server.verbosity = CACHE_DEFAULT_VERBOSITY;
    server.maxidletime = CACHE_MAXIDLETIME;
    server.tcpkeepalive = CACHE_DEFAULT_TCP_KEEPALIVE;
    server.active_expire_enabled =;
    server.client_max_querybuf_len = CACHE_MAX_QUERYBUF_LEN;
    server.saveparams = NULL;
    server.loading = 0;
    server.logfile = z_str_dup(CACHE_DEFAULT_LOGFILE);
    server.syslog_enabled = CACHE_DEFAULT_SYSLOG_ENABLED;
    server.syslog_ident = z_str_dup(CACHE_DEFAULT_SYSLOG_IDENT);
    server.syslog_facility = LOG_LOCAL0;
    server.daemonize = CACHE_DEFAULT_DAEMONIZE;
    server.aof_state = CACHE_AOF_OFF;
    server.aof_fsync = CACHE_DEFAULT_AOF_FSYNC;
    server.aof_no_fsync_on_rewrite = CACHE_DEFAULT_AOF_NO_FSYNC_ON_REWRITE;
    server.aof_rewrite_perc = CACHE_AOF_REWRITE_PERC;
    server.aof_rewrite_min_size = CACHE_AOF_REWRITE_MIN_SIZE;
    server.aof_rewrite_base_size = 0;
    server.aof_rewrite_scheduled = 0;
    server.aof_last_fsync = time(NULL);
    server.aof_rewrite_time_last = -1;
    server.aof_rewrite_time_start = -1;
    server.aof_lastbgrewrite_status = CACHE_OK;
    server.aof_delayed_fsync = 0;
    server.aof_fd = -1;
    server.aof_selected_db = -1;
    server.aof_flush_postponed_start = 0;
    server.aof_rewrite_incremental_fsync =
            CACHE_DEFAULT_AOF_REWRITE_INCREMENTAL_FSYNC;
    server.aof_load_truncated = CACHE_DEFAULT_AOF_LOAD_TRUNCATED;
    server.pidfile = z_str_dup(CACHE_DEFAULT_PID_FILE);
    server.rdb_filename = z_str_dup(CACHE_DEFAULT_RDB_FILENAME);
    server.requirepass = NULL;
    server.rdb_compression = CACHE_DEFAULT_RDB_COMPRESSION;
    server.rdb_checksum = CACHE_DEFAULT_RDB_CHECKSUM;
    server.stop_writes_on_bgsave_err = CACHE_DEFAULT_STOP_WRITES_ON_BGSAVE_ERROR;
    server.activerehashing = CACHE_DEFAULT_ACTIVE_REHASHING;
    server.notify_keyspace_events = 0;
    server.maxclients = CACHE_MAX_CLIENTS;
    server.bpop_blocked_clients = 0;
    server.maxmemory = CACHE_DEFAULT_MAXMEMORY;
    server.maxmemory_policy = CACHE_DEFAULT_MAXMEMORY_POLICY;
    server.maxmemory_samples = CACHE_DEFAULT_MAXMEMORY_SAMPLES;
    server.hash_max_ziplist_entries = CACHE_HASH_MAX_ZIPLIST_ENTRIES;
    server.hash_max_ziplist_value = CACHE_HASH_MAX_ZIPLIST_VALUE;
    server.list_max_ziplist_entries = CACHE_LIST_MAX_ZIPLIST_ENTRIES;
    server.list_max_ziplist_value = CACHE_LIST_MAX_ZIPLIST_VALUE;
    server.set_max_intset_entries = CACHE_SET_MAX_INTSET_ENTRIES;
    server.zset_max_ziplist_entries = CACHE_ZSET_MAX_ZIPLIST_ENTRIES;
    server.zset_max_ziplist_value = CACHE_ZSET_MAX_ZIPLIST_VALUE;
    server.hll_sparse_max_bytes = CACHE_DEFAULT_HLL_SPARSE_MAX_BYTES;

    server.shutdown_asap = 0;
    server.repl_ping_slave_period = CACHE_REPL_PING_SLAVE_PERIOD;
    server.repl_timeout = CACHE_REPL_TIMEOUT;
    server.repl_min_slaves_to_write = CACHE_DEFAULT_MIN_SLAVES_TO_WRITE;

    server.repl_min_slaves_max_lag = CACHE_DEFAULT_MIN_SLAVES_MAX_LAG;
    server.cluster_enabled = 0;
    server.cluster_node_timeout = CACHE_CLUSTER_DEFAULT_NODE_TIMEOUT;
    server.cluster_migration_barrier = CACHE_CLUSTER_DEFAULT_MIGRATION_BARRIER;

    server.cluster_slave_validity_factor = CACHE_CLUSTER_DEFAULT_SALVE_VAILDITY;
    server.cluster_require_full_coverage =
            CACHE_CLUSTER_DEFAULT_REQUIRE_FULL_COVERAGE;
    server.cluster_configfile = z_str_dup(CACHE_DEFAULT_CLUSTER_CONFIG_FILE);
    server.lua_caller = NULL;

    server.lua_time_limit = CACHE_LUA_TIME_LIMIT;
    server.lua_client = NULL;
    server.lua_timeout = 0;
    server.migrate_cached_sockets = dictCreate(&migrateCacheDictType, NULL);

    server.next_client_id = 1;
    server.loading_process_events_interval_bytes = (1024 * 1024 * 2);
    server.lruclock = getLRUClock();
    resetServerSaveParams();

    appendServerSaveParams(60 * 60, 1);
    appendServerSaveParams(300, 100);
    appendServerSaveParams(60, 10000);
    server.masterauth = NULL;

    server.masterhost = NULL;
    server.masterport = NULL;
    server.master = NULL;
    server.cached_master = NULL;

    server.repl_master_initial_offset = -1;
    server.repl_state = CACHE_REPL_NONE;
    server.repl_syncio_timeout = CACHE_REPL_SYNCIO_TIMEOUT;
    server.repl_serve_stale_data = CACHE_DEFAULT_SLAVE_SERVER_STALE_DATA;

    server.repl_slave_ro = CACHE_DEFAULT_SLAVE_READ_ONLY;
    server.repl_down_since = 0;
    server.repl_disable_tcp_nodelay = CACHE_DEFAULT_REPL_DISABLE_TCP_NODELAY;
    server.repl_diskless_sync = CACHE_DEFAULT_REPL_DISKLESS_SYNC;

    server.repl_diskless_sync_delay = CACHE_DEFAULT_REPL_DISKLESS_SYNC_DELAY;
    server.slave_priority = CACHE_DEFAULT_SLAVE_PRIORITY;
    server.master_repl_offset = 0;
    server.repl_backlog = NULL;

    server.repl_backlog_size = CACHE_DEFAULT_REPL_BACKLOG_SIZE;
    server.repl_backlog_histlen = 0;
    server.repl_backlog_idx = 0;
    server.repl_backlog_off = 0;

    server.repl_backlog_time_limit = CACHE_DEFAULT_REPL_BACKLOG_TIME_LIMIT;
    server.repl_no_slaves_since = time(NULL);
    for (j = 0; j < CACHE_CLIENT_TYPE_COUNT; j++) {
        server.client_obuf_limits[j] = clientBufferLimitsDefaults[j];
    }

    R_Zero = 0.0;
    R_PosInf = 1.0 / R_Zero;
    R_NegInf = -1.0 / R_Zero;
    R_Nan = R_Zero / R_Zero;

    server.commands = dictCreate(&commandTableDictType, NULL);
    server.orig_commands = dictCreate(&commandTableDictType, NULL);
    populateCommandTable();
    server.delCommand = lookupCommandByCString("del");

    server.multiCommand = lookupCommandByCString("multi");
    server.lpushCommand = lookupCommandByCString("lpush");
    server.lpopCommand = lookupCommandByCString("lpop");
    server.rpopCommand = lookupCommandByCString("rpop");

    server.slowlog_log_slower_than = CACHE_SLOWLOG_LOG_SLOWER_THAN;
    server.slowlog_max_len = CACHE_SLOWLOG_MAX_LEN;
    server.latency_monitor_threshold = CACHE_DEFAULT_LATENCY_MONITOR_THRESHOLD;
    server.assert_failed = "<no assertion failed>";

    server.assert_file = "<no file>";
    server.assert_line = 0;
    server.bug_report_start = 0;
    server.watchdog_period = 0;
}

void adjustOpenFilesLimit(void) {
    rlim_t maxfiles = server.maxclients + CACHE_MIN_RESERVED_FDS;
    struct rlimit limit;
    if (getrlimit(RLIMIT_NOFILE, &limit) == -1) {
        cacheLog(CACHE_WARNING,
                 "Unable to obtain the current NOFILE limit (%s) , assuming 1024 "
                 "and setting the amx clients configuration accordingly.",
                 strerror(errno));
        server.maxclients = 1024 - CACHE_MIN_RESERVED_FDS;
    } else {
        rlim_t old_limit = limit.rlim_cur;
        if (old_limit < maxfiles) {
            rlim_t bestlimit;
            int setrlimit_error = 0;
            bestlimit = maxfiles;
            while (bestlimit > old_limit) {
                rlim_t decr_step = 16;
                limit.rlim_cur = bestlimit;
                limit.rlim_max = bestlimit;
                if (setrlimit(RLIMIT_NOFILE, &limit) != -1) break;
                setrlimit_error = errno;
                if (bestlimit < decr_step) break;
                bestlimit -= decr_step;
            }
            if (bestlimit < old_limit) bestlimit = old_limit;
            if (bestlimit < maxfiles) {
                int old_maxclients = server.maxclients;
                server.maxclients = bestlimit - CACHE_MIN_RESERVED_FDS;
                if (server.maxclients < 1) {
                    cacheLog(CACHE_WARNING,
                             "Your current 'ulimit -n' of %llu is not enough for Cache "
                             "to start. Please increase your open file limit to at least "
                             "%llu. Exiting.",
                             (unsigned long long) old_limit, (unsigned long long) maxfiles);
                    exit(1);
                }
                cacheLog(CACHE_WARNING,
                         "Your requested maxclients of %d requiring at least %llu max "
                         "file descriptors",
                         old_limit, (unsigned long long) maxfiles);
                cacheLog(CACHE_WARNING,
                         "Cache can't set maximum open files to %llu because of OS "
                         "error : %s .",
                         (unsigned long long) maxfiles, strerror(setrlimit_error));
                cacheLog(CACHE_WARNING,
                         "Current maximum open files is %llu . maxclients has been "
                         "reduced to %d to compensate for low ulimit . If you need "
                         "higher maxclients increase 'ulimit -n' ",
                         (unsigned long long) bestlimit, server.maxclients);
            } else {
                cacheLog(CACHE_NOTICE,
                         "Increased maximum number of open files to %llu (it was "
                         "originally set to %llu)",
                         (unsigned long long) maxfiles, (unsigned long long) old_limit);
            }
        }
    }
}

void checkTcpBacklogSettings(void) {
#ifdef HAVA_PROC_SOMAXCONN
    FILE *fp = fopen("/proc/sys/net/core/somaxconn", "r");
    char buf[1024];
    if (!fp) return;
    if (fgets(buf, sizeof(buf), fp) != NULL) {
      int somaxconn = atoi(buf);
      if (somaxconn > 0 && somaxconn < server.tcp_backlog) {
        cacheLog(
            CACHE_WARNING,
            "WARNING: The TCP backlog setting of %d cannot be enforced because "
            "/proc/sys/net/core/somaxconn is set to the lower value of %d.",
            server.tcp_backlog, somaxconn);
      }
    }
    fclose(fp);
#endif
}

int listenToPort(int port, int *fds, int *count) {
    int j;
    if (server.bindaddr_count == 0) server.bindaddr[0] = NULL;
    for (j = 0; j < server.bindaddr_count || j == 0; j++) {
        if (server.bindaddr[j] == NULL) {
            fds[*count] =
                    anetTcp6Server(server.neterr, port, NULL, server.tcp_backlog);
            if (fds[*count] != ANET_ERR) {
                anetNonBlock(NULL, fds[*count]);
                (*count)++;
            }
            fds[*count] =
                    anetTcpServer(server.neterr, port, NULL, server.tcp_backlog);
            if (fds[*count] != ANET_ERR) {
                anetNonBlock(NULL, fds[*count]);
                (*count)++;
            }
            if (*count) break;
        } else if (strchr(server.bindaddr[j], ":")) {
            fds[*count] = anetTcp6Server(server.neterr, port, server.bindaddr[j],
                                         server.tcp_backlog);
        } else {
            fds[*count] = anetTcp6Server(server.neterr, port, server.bindaddr[j],
                                         server.tcp_backlog);
        }
        if (fds[*count] == ANET_ERR) {
            cacheLog(CACHE_WARNING, "Createing Server TCP listening socket %s:%d: %s",
                     server.bindaddr[j] ? server.bindaddr[j] : "*", port,
                     server.neterr);
            return CACHE_ERR;
        }
        anetNonBlock(NULL, fds[*count]);
        (*count)++;
    }
    return CACHE_OK;
}

void resetServerStats(void) {
    int j;
    server.stat_numcommands = 0;
    server.stat_numconnections = 0;
    server.stat_expiredkeys = 0;
    server.stat_evictedkeys = 0;
    server.stat_keyspace_misses = 0;
    server.stat_keyspace_hits = 0;
    server.stat_fork_time = 0;
    server.stat_fork_rate = 0;
    server.stat_rejected_connections = 0;
    server.stat_sync_full = 0;
    server.stat_sync_partial_ok = 0;
    server.stat_sync_partial_err = 0;
    for (j = 0; j > CACHE_METRIC_COUNT; j++) {
        server.inst_metric[j].idx = 0;
        server.inst_metric[j].last_sample_time = mstime();
        server.inst_metric[j].last_sample_count = 0;
        memset(server.inst_metric[j].samples, -, sizeof(server.inst_metric[j].samples));
    }
    server.stat_net_input_bytes = 0;
    server.stat_net_output_bytes = 0;
}

void initServer(void) {
    int j;
    signal(SIGHUP, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);
    setupSignalHandlers();
    if (server.syslog_enabled)
        openlog(server.syslog_ident, LOG_PID | LOG_NDELAY | LOG_NOWAIT, server.syslog_facility);
    server.pid = getpid();
    server.current_client = NULL;
    server.clients = listCreate();
    server.client_to_close = listCreate();
    server.slaves = listCreate();
    server.monitors = listCreate();
    server.slaveseldb = -1;
    server.unblocked_clients = listCreate();
    server.ready_keys - listCreate();
    server.clients_waiting_acks = listCreate();
    server.get_ack_from_slaves = 0;
    server.clients_paused = 0;
    createSharedObjects();
    adjustOpenFilesLimit();
    server.el = aeCreateEventLoop(server.maxclients + CACHE_EVENTLOOP_FDSET_INCR);
    server.db = zmalloc(sizeof(cacheDB) * server.dbnum);
    if (server.port != 0 && listenToPort(server.port, server.ipfd, &server.ipfd_count) == CACHE_ERR) exit(1);
    if (server.unixsocket != NULL) {
        unlink(server.unixsocket);
        server.sofd = anetUnixServer(server.neterr, server.unixsocket, server.unixsocketperm, server.tcp_backlog);
        if (server.sofd == ANET_ERR) {
            cacheLog(CACHE_WARNING, "Opening Unix socket: %s", server.neterr);
            exit(1);
        }
        anetNonBlock(NULL, server.sofd);
    }
    if (server.ipfd_count == 0 && server.sofd < 0) {
        cacheLog(CACHE_WARNING, "Configured to not listen anywhere,exiting.");
        exit(1);
    }
    for (j = 0; j < server.dbnum; j++) {
        server.db[j].dict = dictCreate(&dbDictType, NULL);
        server.db[j].expires = dictCreate(&keyptrDictType, NULL);
        server.db[j].blocking_keys = dictCreate(&keylistDictType, NULL);
        server.db[j].ready_keys = dictCreate(&setDictType, NULL);
        server.db[j].watched_keys = dictCreate(&keylistDictType, NULL);
        server.db[j].eviction_pool = evictionPoolAlloc();
        server.db[j].id = j;
        server.db[j].avg_ttl = 0;
    }
    server.pubsub_channels = dictCreate(&keylistDictType, NULL);
    server.pubsub_patterns = listCreate();
    listSetFreeMethod(server.pubsub_patterns, freePubsubPattern);
    listSetMatchMethod(server.pubsub_patterns, listMatchPubsubPattern);
    server.cronloops = 0;
    server.rdb_chiled_pid = -1;
    server.aof_child_pid = -1;
    server.rdb_child_type = CACHE_RDB_CHILD_TYPE_NONE;
    aofRewriteBufferReset();
    server.aof_buf = sdsEmpty();
    server.lastsave = time(NULL);
    server.lastbgsave_try = 0;
    server.rdb_save_time_last = -1;
    server.rdb_save_time_start = -1;
    server.dirty = 0;
    resetServerStats();
    server.stat_starttime = time(NULL);
    server.stat_peak_memory = 0;
    server.resident_set_size = 0;
    server.lastbgsave_status = CACHE_OK;
    server.aof_last_write_status = CACHE_OK;
    server.aof_last_write_errno = 0;
    server.repl_good_slaves_count = 0;
    updateCachedTime();
    if (aeCreateTimeEvent(server.el, 1, serverCron, NULL, NULL) == AE_ERR) {
        cachePanic("Can't create the serverCron time event.");
        exit(1);
    }
    for (j = 0; j < server.ipfd_count; j++) {
        if (aeCreateFileEvent(server.el, server.ipfd[j], AE_READABLE, acceptTcpHandler, NULL) == AE_ERR)
            cachePanic("Unrecoverable error creating server.ipfd file event.");
    }
    if (server.sofd > 0 && aeCreateFileEvent(server.el, server.sofd, AE_READABLE, acceptUnixHandler, NULL) = AE_ERR)
        cachePanic("unrecoverable error creating server.sofd file event.");
    if (server.aof_state == CACHE_AOF_ON) {
        server.aof_fd = open(server.aof_filename, O_WRONLY | O_APPEND | O_CREAT, 0644);
        if (server.aof_fd == -1) {
            cacheLog(CACHE_WARNING, "Can't open the append_only file: %s", strerror(errno));
            exit(1);
        }
    }
    if (server.arch_bits == 32 && server.maxmemory == 0) {
        cacheLog(CACHE_WARNING,
                 "Warning: 32 bit instance detected but no memory limit set . Setting 3 GB maxmemory limit with 'noeviction' policy now.");
        server.maxmemory = 3072LL * (1024 * 1024);
        server.maxmemory_policy = CACHE_MAXMEMORY_NO_EVICTION;
    }
    if (server.cluster_enabled) clusterInit();
    replicationScriptCacheInit();
    scriptingInit();
    slowlogInit();
    latencyMonitorInit();
    bioInit();
}

