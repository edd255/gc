#ifndef GC_H
#define GC_H

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

/* We allocate blocks in page-sized chunks */
#define MIN_ALLOC_SIZE 4096

#define UNTAG(p) (((uintptr_t)(p)) & 0xfffffffc)

typedef struct header_t header_t;
struct header_t {
    unsigned int size;
    header_t* next;
};

void* gc_malloc(size_t alloc_size);
void gc_init(void);
void gc_collect(void);

#endif
