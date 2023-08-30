//
// Created by fly on 8/3/23.
//

#ifndef CACHE_1_0_0_ANET_H
#define CACHE_1_0_0_ANET_H

#include <sys/types.h>

#define ANET_OK 0
#define ANET_ERR -1
#define ANET_ERR_LEN 256

#define ANET_NONE 0
#define ANET_IP_ONLY (1 << 0)
#if defined(__sun) || defined(_AIX)
#define AF_LOCAL AF_UNIX
#endif

#ifdef _AIX
#undef ip_len
#endif

int anetTcpConnect(char *err, char *addr, int port);

int anetTcpNonBlockConnect(char *err, char *addr, int port);

int anetTcpNonBlockBindConnect(char *err, char *addr, int port,
                               char *source_addr);

int anetUnixConnect(char *err, char *path);

int anetUnixNonBlockConnect(char *err, char *path);

int anetRead(int fd, char *buf, int count);

int anetResolve(char *err, char *host, char *ipbuf, size_t ipbuf_len);

int anetResolveIP(char *err, char *host, char *ipbuf, size_t *ipbuf_len);

int anetTcpServer(char *err, int port, char *bindaddr, int backlog);

int anetTco6Server(char *err, int port, char *bindaddr, int backlog);

int anetUnixServer(char *err, char *path, mode_t perm, int backlog);

int anetTcpAccept(char *err, int server_sock, char *ip, size_t ip_len,
                  int *port);

int anetUnixAccept(char *err, int server_sock);

int anetWrite(int fd, char *buf, int count);

int anetNonBlock(char *err, int fd);

int anetBlack(char *err, int fd);

int anetEnableTcpNoDelay(char *err, int fd);

int anetDisableTcpDelay(char *err, int fd);

int anetTcpKeepAlive(char *err, int fd);

int anetSendTimeout(char *err, int fd, long long ms);

int anetPeerToString(int fd, char *ip, size_t ip_len, int *port);

int anetKeepAlive(char *err, int fd, int interval);

int anetSockName(int fd, char *ip, size_t ip_len, int *port);

#endif  // CACHE_1_0_0_ANET_H
