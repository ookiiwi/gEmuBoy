#ifndef GB_FIFO_H_
#define GB_FIFO_H_

#include <stddef.h> /* size_t */

#define FIFO_DEFINE_STRUCT(fifo_struct_name, element_type, capacity)                            \
typedef struct {                                                                                \
    element_type arr[capacity];                                                                 \
    size_t cap##acity;                                                                          \
    size_t start;                                                                               \
    size_t size;                                                                                \
} fifo_struct_name

/**
 * Intializes the fifo pointed by the specified pointer
 *
 * @param fifo pointer to a fifo
 * @param capacity the capacity for this fifo
 */
#define fifo_init(fifo, capacity) do {                                                          \
    (fifo)->start       = 0;                                                                    \
    (fifo)->size        = 0;                                                                    \
    (fifo)->ca##pacity  = capacity;                                                             \
} while (0)

#define fifo_free(fifo) do {} while (0)

#define fifo_push(fifo, element) do {                                                           \
    if ( (fifo)->size >= (fifo)->capacity ) return;                                             \
    (fifo)->arr[ ( (fifo)->start + (fifo)->size ) % (fifo)->capacity ] = element;               \
    (fifo)->size++;                                                                             \
} while (0)

#define fifo_pop(fifo) do {                                                                     \
    if ((fifo)->size) {                                                                         \
        (fifo)->start = ((fifo)->start + 1 ) % (fifo)->capacity;                                \
        (fifo)->size--;                                                                         \
    }                                                                                           \
} while (0)

#define fifo_peek(fifo, res) do {                                                               \
    if ((fifo)->size) {                                                                         \
        (res) = (fifo)->arr + (fifo)->start;                                                    \
    }                                                                                           \
} while (0)

#endif
