#ifndef SEAMLESS_USER_H
#define SEAMLESS_USER_H

#include <string>
#include <iostream>
#include <sys/socket.h>	
#include <unistd.h>
#include <string.h>

#define M_CLOUD_REGISTER_PORT		9898

class m_seamless_system;

int seamless_socket(int domain, int type, int protocol);
int seamless_sendto(int sockfd, const void * buffer, size_t len, int flags, const struct sockaddr * dest_addr, socklen_t addrlen);
int seamless_send(int sockfd, const void * buffer, size_t len, int flags);
int seamless_recvfrom(int sockd, void * buf, size_t len, int flags, struct sockaddr * src_addr, socklen_t * addrlen);
int seamless_recv(int sockfd, void * buf, size_t len, int flags);
int seamless_bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
int seamless_recvall(int sockfd, bool condition);
int seamless_cloud_notify(int sockfd, bool condition);
int seamless_listen(int sockfd, int backlog);
int seamless_accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
int seamless_connect(int sockfd, struct sockaddr *addr, socklen_t addrlen);
int seamless_close(int sockfd);

#endif