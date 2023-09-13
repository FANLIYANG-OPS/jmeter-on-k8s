cmake_minimum_required(VERSION 3.22.1)
project(cache_1.0.0)
set(CMAKE_C_STANDARD 11)
add_executable(cache_1.0.0

        adlist.c
        adlist.h
        ae.h
        anet.h
        bio.h
        cache.c
        cache.h
        cacheassert.h
        cluster.h
        config.h
        dict.c
        dict.h
        endianconv.c
        endianconv.h
        intset.c
        intset.h
        latency.h
        macros.h
        rdb.h
        rio.h
        sds.c
        sds.h
        slowlog.h
        solarisfixes.h
        sparkline.h
        util.h
        version.h
        ziplist.c
        ziplist.h
        zmalloc.c
        zmalloc.h)
