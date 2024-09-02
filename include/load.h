#ifndef LOAD_H_
#define LOAD_H_

#include <stdio.h>
#include <stdlib.h>
#include "cputype.h"

#define LOADROM_FAIL_OPEN   (-1)
#define LOADROM_FAIL_ALLOC  (-2)

/**
 * Load [dst] with the content from the specified source file
 * 
 * @param src_filename Source file
 * @param dst Destination buffer
 * @param dst_size Allocated size for dst 
 * 
 * @return Number of bytes read. This should be the same as dst_size
*/
int loadrom(const char *src_filename, BYTE **dst, size_t *dst_size);

#endif