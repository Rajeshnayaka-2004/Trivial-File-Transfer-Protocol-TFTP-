
#include "tftp.h"
#include "udp.h"
#include "file.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <arpa/inet.h>

#define RETRIES 5

static int send_request(int fd, struct sockaddr_in *server,
                        const char *filename, uint16_t opcode)
{
    unsigned char packet[PACKETSIZE];
    int len = tftp_form_request(packet, sizeof(packet),
                                opcode, filename);

    if (len < 0)
        return -1;

    return udp_send_packet(fd, packet, (size_t)len, server) < 0
           ? -1 : 0;
}

static int receive_packet(int fd, unsigned char *packet,
                          struct sockaddr_in *peer, ssize_t *n)
{
    *n = udp_receive_packet(fd, packet, PACKETSIZE, peer);
    return *n < 0 ? -1 : 0;
}

static int download_file(int fd, struct sockaddr_in server,
                         const char *remote, const char *local)
{
    unsigned char packet[PACKETSIZE], ack[4];
    struct sockaddr_in peer;
    FILE *fp = NULL;
    uint16_t block = 0, opcode;
    ssize_t n;
    int retries = 0;

    if (send_request(fd, &server, remote, TFTP_RRQ) < 0)
        return -1;

    fp = file_open_write(local);
    if (!fp) {
        perror("open output file");
        return -1;
    }

    while (1) {
        struct timeval timeout = {3, 0};
        fd_set readfds;

        FD_ZERO(&readfds);
        FD_SET(fd, &readfds);

        if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO,
                       &timeout, sizeof(timeout)) < 0) {
            perror("setsockopt");
            file_close(fp);
            return -1;
        }

        if (receive_packet(fd, packet, &peer, &n) < 0) {
            if (++retries >= RETRIES) {
                fprintf(stderr, "Receive timeout/error\n");
                file_close(fp);
                return -1;
            }
            continue;
        }

        if (tftp_parse_opcode(packet, (size_t)n, &opcode) < 0) {
            fprintf(stderr, "Invalid packet\n");
            continue;
        }

        if (opcode == TFTP_DATA) {
            uint16_t received_block;

            if (tftp_parse_block(packet, (size_t)n,
                                 &received_block) < 0 ||
                n < 4 || received_block != (uint16_t)(block + 1))
                continue;

            if (file_append_from_buffer(fp, packet + 4,
                                        (size_t)n - 4) < 0) {
                perror("write file");
                file_close(fp);
                return -1;
            }

            block = received_block;
            retries = 0;

            int ack_len = tftp_form_ack(ack, block);
            if (udp_send_packet(fd, packet, 0, &peer) < 0) {
                /* No action; ACK is sent below. */
            }
            if (udp_send_packet(fd, ack, (size_t)ack_len, &peer) < 0) {
                file_close(fp);
                return -1;
            }

            if (n < PACKETSIZE) {
                file_close(fp);
                return 0;
            }
        } else if (opcode == TFTP_ERROR) {
            fprintf(stderr, "Server returned TFTP ERROR\n");
            file_close(fp);
            return -1;
        }
    }
}

static int upload_file(int fd, struct sockaddr_in server,
                       const char *local, const char *remote)
{
    unsigned char packet[PACKETSIZE], response[PACKETSIZE];
    struct sockaddr_in peer;
    FILE *fp = file_open_read(local);
    uint16_t block = 0;
    ssize_t n;
    int retries = 0;

    if (!fp) {
        perror("open input file");
        return -1;
    }

    if (send_request(fd, &server, remote, TFTP_WRQ) < 0) {
        file_close(fp);
        return -1;
    }

    if (receive_packet(fd, response, &peer, &n) < 0) {
        file_close(fp);
        return -1;
    }

    uint16_t opcode, received_block;
    if (tftp_parse_opcode(response, (size_t)n, &opcode) < 0 ||
        opcode != TFTP_ACK ||
        tftp_parse_block(response, (size_t)n, &received_block) < 0 ||
        received_block != 0) {
        fprintf(stderr, "Expected ACK block 0\n");
        file_close(fp);
        return -1;
    }

    while (1) {
        size_t bytes = file_buffer_from_pos(fp, packet + 4, BLIMIT);
        int final_block = bytes < BLIMIT;
        uint16_t op = htons(TFTP_DATA);
        uint16_t net_block = htons(++block);

        memcpy(packet, &op, 2);
        memcpy(packet + 2, &net_block, 2);

        int len = (int)(4 + bytes);

        if (udp_send_packet(fd, packet, (size_t)len, &peer) < 0) {
            file_close(fp);
            return -1;
        }

        if (receive_packet(fd, response, &peer, &n) < 0) {
            if (++retries >= RETRIES) {
                file_close(fp);
                return -1;
            }
            continue;
        }

        if (tftp_parse_opcode(response, (size_t)n, &opcode) < 0 ||
            opcode != TFTP_ACK ||
            tftp_parse_block(response, (size_t)n, &received_block) < 0 ||
            received_block != block) {
            fprintf(stderr, "Unexpected ACK\n");
            file_close(fp);
            return -1;
        }

        retries = 0;

        if (final_block)
            break;
    }

    file_close(fp);
    return 0;
}

int main(int argc, char *argv[])
{
    int fd, result;
    struct sockaddr_in server;

    if (argc != 6) {
        fprintf(stderr,
                "Usage: %s <server-ip> <port> <remote-file> "
                "<local-file> <get|put>\n", argv[0]);
        return 1;
    }

    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons((unsigned short)atoi(argv[2]));

    if (inet_pton(AF_INET, argv[1], &server.sin_addr) != 1) {
        fprintf(stderr, "Invalid server IP\n");
        return 1;
    }

    fd = udp_bind_client();
    if (fd < 0) {
        perror("socket");
        return 1;
    }

    if (strcmp(argv[5], "get") == 0)
        result = download_file(fd, server, argv[3], argv[4]);
    else if (strcmp(argv[5], "put") == 0)
        result = upload_file(fd, server, argv[4], argv[3]);
    else {
        fprintf(stderr, "Operation must be get or put\n");
        close(fd);
        return 1;
    }

    close(fd);
    return result == 0 ? 0 : 1;
}
