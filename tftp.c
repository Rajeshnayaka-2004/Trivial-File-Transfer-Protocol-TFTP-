
#include "tftp.h"

#include <string.h>
#include <arpa/inet.h>

int tftp_form_request(unsigned char *packet, size_t capacity,
                      uint16_t opcode, const char *filename)
{
    const char *mode = "octet";
    size_t name_len, mode_len, total;
    uint16_t net_opcode;

    if (!packet || !filename ||
        (opcode != TFTP_RRQ && opcode != TFTP_WRQ))
        return -1;

    name_len = strlen(filename);
    mode_len = strlen(mode);
    total = 2 + name_len + 1 + mode_len + 1;

    if (name_len == 0 || total > capacity)
        return -1;

    net_opcode = htons(opcode);
    memcpy(packet, &net_opcode, 2);
    memcpy(packet + 2, filename, name_len + 1);
    memcpy(packet + 2 + name_len + 1, mode, mode_len + 1);

    return (int)total;
}

int tftp_form_data(unsigned char *packet, uint16_t block,
                   const unsigned char *data, size_t data_len)
{
    uint16_t opcode = htons(TFTP_DATA);
    uint16_t net_block = htons(block);

    if (!packet || (!data && data_len != 0) ||
        data_len > BLIMIT)
        return -1;

    memcpy(packet, &opcode, 2);
    memcpy(packet + 2, &net_block, 2);

    if (data_len > 0)
        memcpy(packet + 4, data, data_len);

    return (int)(4 + data_len);
}

int tftp_form_ack(unsigned char *packet, uint16_t block)
{
    uint16_t opcode = htons(TFTP_ACK);
    uint16_t net_block = htons(block);

    if (!packet)
        return -1;

    memcpy(packet, &opcode, 2);
    memcpy(packet + 2, &net_block, 2);

    return 4;
}

int tftp_parse_opcode(const unsigned char *packet, size_t len,
                      uint16_t *opcode)
{
    uint16_t value;

    if (!packet || !opcode || len < 2)
        return -1;

    memcpy(&value, packet, 2);
    *opcode = ntohs(value);

    if (*opcode < TFTP_RRQ || *opcode > TFTP_ERROR)
        return -1;

    return 0;
}

int tftp_parse_block(const unsigned char *packet, size_t len,
                     uint16_t *block)
{
    uint16_t value, opcode;

    if (!packet || !block || len < 4 ||
        tftp_parse_opcode(packet, len, &opcode) != 0)
        return -1;

    if (opcode != TFTP_DATA && opcode != TFTP_ACK)
        return -1;

    memcpy(&value, packet + 2, 2);
    *block = ntohs(value);

    return 0;
}
