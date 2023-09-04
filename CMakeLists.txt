cmake_minimum_required(VERSION 3.22.1)
project(cache_1_0_0 C)

set(CMAKE_C_STANDARD 11)

include_directories(src)

add_executable(cache_1_0_0
        src/adlist.c
        src/adlist.h
        src/ae.h
        src/anet.h
        src/cache.h
        src/cacheassert.h
        src/cluster.h
        src/config.h
        src/const.h
        src/dict.c
        src/dict.h
        src/endianconv.c
        src/endianconv.h
        src/intset.c
        src/intset.h
        src/latency.h
        src/macros.h
        src/sds.c
        src/sds.h
        src/solarisfixes.h
        src/sparkline.h
        src/util.h
        src/version.h
        src/ziplist.c
        src/ziplist.h
        src/zmalloc.c
        src/zmalloc.h
        src/rio.h
        src/rdb.h src/cache.c)
