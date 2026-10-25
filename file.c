
#include "file.h"

FILE *file_open_read(const char *filename)
{
    return fopen(filename, "rb");
}

FILE *file_open_write(const char *filename)
{
    return fopen(filename, "wb");
}

size_t file_buffer_from_pos(FILE *fp, unsigned char *buffer,
                            size_t capacity)
{
    return fread(buffer, 1, capacity, fp);
}

int file_append_from_buffer(FILE *fp, const unsigned char *buffer,
                            size_t len)
{
    return fwrite(buffer, 1, len, fp) == len ? 0 : -1;
}

void file_close(FILE *fp)
{
    if (fp)
        fclose(fp);
}
