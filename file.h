
#ifndef FILE_H
#define FILE_H

#include <stdio.h>
#include <stddef.h>

FILE *file_open_read(const char *filename);

FILE *file_open_write(const char *filename);

size_t file_buffer_from_pos(FILE *fp, unsigned char *buffer,
                            size_t capacity);

int file_append_from_buffer(FILE *fp, const unsigned char *buffer,
                            size_t len);

void file_close(FILE *fp);

#endif
