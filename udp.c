
#include "udp.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

int udp_bind_server(unsigned short port)
{
    int fd;
    struct sockaddr_in address;

    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
        perror("socket");
        return -1;
    }

    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    address.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(fd, (struct sockaddr *)&address,
             sizeof(address)) < 0) {
        perror("bind");
        close(fd);
        return -1;
    }

    return fd;
}

int udp_bind_client(void)
{
    return socket(AF_INET, SOCK_DGRAM, 0);
}

int udp_send_packet(int sockfd, const void *buffer, size_t len,
                    const struct sockaddr_in *destination)
{
    ssize_t sent;

    sent = sendto(sockfd, buffer, len, 0,
                  (const struct sockaddr *)destination,
                  sizeof(*destination));

    if (sent < 0) {
        perror("sendto");
        return -1;
    }

    if ((size_t)sent != len)
        return -1;

    return (int)sent;
}

ssize_t udp_receive_packet(int sockfd, void *buffer, size_t capacity,
                           struct sockaddr_in *source)
{
    socklen_t source_len = sizeof(*source);

    return recvfrom(sockfd, buffer, capacity, 0,
                    (struct sockaddr *)source, &source_len);
}
