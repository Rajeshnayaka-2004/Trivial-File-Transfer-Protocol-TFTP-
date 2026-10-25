
#ifndef TFTP_H
#define TFTP_H

#include <stddef.h>
#include <stdint.h>

#define BLIMIT 512
#define PACKETSIZE 516

#define TFTP_RRQ   1
#define TFTP_WRQ   2
#define TFTP_DATA  3
#define TFTP_ACK   4
#define TFTP_ERROR 5

int tftp_form_request(unsigned char *packet, size_t capacity,
                      uint16_t opcode, const char *filename);

int tftp_form_data(unsigned char *packet, uint16_t block,
                   const unsigned char *data, size_t data_len);

int tftp_form_ack(unsigned char *packet, uint16_t block);

int tftp_parse_opcode(const unsigned char *packet, size_t len,
                      uint16_t *opcode);

int tftp_parse_block(const unsigned char *packet, size_t len,
                     uint16_t *block);

#endif
