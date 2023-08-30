#ifndef CONFIG_H
#define CONFIG_H

#include <features.h>
#include <linux/version.h>



#define redis_fstat fstat
#define redis_stat stat
#define HAVE_PROC_STAT 1
#define HAVE_PROC_MAPS 1
#define HAVA_PROC_SMAPS 1
#define HAVA_PROC_SO_MAX_CONN 1

#define HAVE_BACKTRACE 1
#define HAVE_EPOLL 1

#define aof_fsync fdatasync
#define HAVE_SYNC_FILE_RANGE 1

#define redis_fsync_range(fd, off, size)                                       \
  sync_file_range(fd, off, size,                                               \
                  SYNC_FILE_RANGE_WAIT_BEFORE | SYNC_FILE_RANGE_WRITE)

#define USE_SET_PROC_TITLE
#define INIT_SET_PROC_TITLE_REPLACEMENT

#include <endian.h>
#include <sys/types.h>

#define BYTE_ORDER__ LITTLE_ENDIAN

void spt_init(int argc, char *argv[]);

void setProcTitle(const char *fmt, ...);

#endif