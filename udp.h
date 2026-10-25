
#ifndef UDP_H
#define UDP_H

#include <stddef.h>
#include <sys/socket.h>
#include <netinet/in.h>

int udp_bind_server(unsigned short port);

int udp_bind_client(void);

int udp_send_packet(int sockfd, const void *buffer, size_t len,
                    const struct sockaddr_in *destination);

ssize_t udp_receive_packet(int sockfd, void *buffer, size_t capacity,
                           struct sockaddr_in *source);

#endif
