
#include "tftp.h"
#include <stdlib.h>
#include "udp.h"
#include "file.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

static int send_error(int fd, struct sockaddr_in *peer,
                      const char *message)
{
    unsigned char packet[PACKETSIZE];
    uint16_t opcode = htons(TFTP_ERROR);
    uint16_t code = htons(1);
    size_t len = strlen(message);

    if (len > sizeof(packet) - 5)
        return -1;

    memcpy(packet, &opcode, 2);
    memcpy(packet + 2, &code, 2);
    memcpy(packet + 4, message, len + 1);

    return udp_send_packet(fd, packet, len + 5, peer) < 0
           ? -1 : 0;
}

static int serve_rrq(int fd, struct sockaddr_in *peer,
                     const char *filename)
{
    FILE *fp = file_open_read(filename);
    unsigned char packet[PACKETSIZE];
    uint16_t block = 0;

    if (!fp) {
        send_error(fd, peer, "File not found");
        return -1;
    }

    while (1) {
        size_t bytes = file_buffer_from_pos(fp, packet + 4, BLIMIT);
        uint16_t opcode = htons(TFTP_DATA);
        uint16_t net_block = htons(++block);

        memcpy(packet, &opcode, 2);
        memcpy(packet + 2, &net_block, 2);

        if (udp_send_packet(fd, packet, bytes + 4, peer) < 0) {
            file_close(fp);
            return -1;
        }

        unsigned char ack[PACKETSIZE];
        struct sockaddr_in sender;
        ssize_t n = udp_receive_packet(fd, ack, sizeof(ack), &sender);
        uint16_t received_opcode, received_block;

        if (n < 4 ||
            tftp_parse_opcode(ack, (size_t)n, &received_opcode) < 0 ||
            received_opcode != TFTP_ACK ||
            tftp_parse_block(ack, (size_t)n, &received_block) < 0 ||
            received_block != block) {
            file_close(fp);
            return -1;
        }

        if (bytes < BLIMIT)
            break;
    }

    file_close(fp);
    return 0;
}

static int serve_wrq(int fd, struct sockaddr_in *peer,
                     const char *filename)
{
    FILE *fp = file_open_write(filename);
    unsigned char packet[PACKETSIZE], ack[4];
    uint16_t block = 0;
    ssize_t n;

    if (!fp) {
        send_error(fd, peer, "Cannot open destination");
        return -1;
    }

    tftp_form_ack(ack, 0);
    udp_send_packet(fd, ack, 4, peer);

    while (1) {
        struct sockaddr_in sender;
        n = udp_receive_packet(fd, packet, sizeof(packet), &sender);

        if (n < 4) {
            file_close(fp);
            return -1;
        }

        uint16_t opcode, received_block;

        if (tftp_parse_opcode(packet, (size_t)n, &opcode) < 0 ||
            opcode != TFTP_DATA ||
            tftp_parse_block(packet, (size_t)n, &received_block) < 0 ||
            received_block != (uint16_t)(block + 1)) {
            file_close(fp);
            return -1;
        }

        if (file_append_from_buffer(fp, packet + 4,
                                    (size_t)n - 4) < 0) {
            file_close(fp);
            return -1;
        }

        block = received_block;
        tftp_form_ack(ack, block);
        udp_send_packet(fd, ack, 4, peer);

        if (n < PACKETSIZE)
            break;
    }

    file_close(fp);
    return 0;
}

int main(int argc, char *argv[])
{
    int fd;
    unsigned short port;
    unsigned char packet[PACKETSIZE];
    struct sockaddr_in peer;
    ssize_t n;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        return 1;
    }

    port = (unsigned short)atoi(argv[1]);
    fd = udp_bind_server(port);

    if (fd < 0)
        return 1;

    printf("TFTP server listening on UDP port %u\n", port);

    while (1) {
        socklen_t peer_len = sizeof(peer);
        n = recvfrom(fd, packet, sizeof(packet), 0,
                     (struct sockaddr *)&peer, &peer_len);

        if (n < 2)
            continue;

        uint16_t opcode;

        if (tftp_parse_opcode(packet, (size_t)n, &opcode) < 0)
            continue;

        char filename[256];
        size_t remain = (size_t)n - 2;
        const unsigned char *start = packet + 2;
        const unsigned char *end = memchr(start, '\0', remain);

        if (!end || end == start ||
            (size_t)(end - start) >= sizeof(filename)) {
            send_error(fd, &peer, "Invalid filename");
            continue;
        }

        memcpy(filename, start, (size_t)(end - start));
        filename[end - start] = '\0';

        printf("Request for %s\n", filename);

        if (opcode == TFTP_RRQ)
            serve_rrq(fd, &peer, filename);
        else if (opcode == TFTP_WRQ)
            serve_wrq(fd, &peer, filename);
        else
            send_error(fd, &peer, "Unsupported request");
    }

    close(fd);
    return 0;
}
