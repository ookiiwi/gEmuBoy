#include "load.h"

int loadrom(const char *src_filename, BYTE **dst, size_t *dst_size) {
    FILE *p = fopen(src_filename, "rb");
    if (p == NULL) {
        return LOADROM_FAIL_OPEN;
    }

    fseek(p, 0, SEEK_END);
    long fsize = ftell(p);
    fseek(p, 0, SEEK_SET);

    *dst_size   = (size_t)fsize;
    *dst        = (BYTE*)( malloc(*dst_size * sizeof(BYTE)) );

    if (*dst == NULL) return LOADROM_FAIL_ALLOC;

    int rv = fread((void*)(*dst), sizeof(BYTE), fsize, p);
    fclose(p);

    return rv;
}