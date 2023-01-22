#include "lib.h"

/* Zero sized block to get us started */
static header_t base;

/* Points to first free block of memory */
static header_t* free_ptr = &base;

/* Points to first used block of memory */
static header_t* used_ptr;

static unsigned int stack_bottom;

/*
 * Scan the free list and look for a place to put the block. We're looking for
 * any block that the to-be-freed block might have been partitioned from.
 */
static void add_to_free_list(header_t* block_ptr) {
    header_t* ptr;
    for (ptr = free_ptr; !(block_ptr > ptr && block_ptr < ptr->next);
         ptr = ptr->next) {
        if (ptr >= ptr->next && (block_ptr > ptr || block_ptr < ptr->next)) {
            break;
        }
    }
    if (block_ptr + block_ptr->size == ptr->next) {
        block_ptr->size += ptr->next->size;
        block_ptr->next = ptr->next->next;
    } else {
        block_ptr->next = ptr->next;
    }
    if (ptr + ptr->size == block_ptr) {
        ptr->size += block_ptr->size;
        ptr->next = block_ptr->next;
    } else {
        ptr->next = block_ptr;
    }
    free_ptr = ptr;
}

/* Request more memory from the kernel */
static header_t* more_memory(size_t num_units) {
    // Temporarily store result of sbrk in vp
    void* tmp_ptr;

    // Used to initialize metadata of the allocated block and manage its
    // placement in the free-list
    header_t* header_ptr;
    if (num_units > MIN_ALLOC_SIZE) {
        num_units = MIN_ALLOC_SIZE / sizeof(header_t);
    }
    if ((tmp_ptr = sbrk(num_units * sizeof(header_t))) == (void*)-1) {
        return NULL;
    }
    header_ptr = (header_t*)tmp_ptr;
    header_ptr->size = num_units;
    add_to_free_list(header_ptr);
    return free_ptr;
}

/* Find a chunk from the free-list and put it in the used list */
void* gc_malloc(size_t alloc_size) {
    // Requested memory + header overhead
    size_t num_units =
        (alloc_size + sizeof(header_t) - 1) / sizeof(header_t) + 1;

    // Traverse the free list
    header_t* prev_ptr = free_ptr;
    for (header_t* ptr = prev_ptr->next;; prev_ptr = ptr, ptr = ptr->next) {
        // Here, the allocated space is big enough
        if (ptr->size >= num_units) {
            // If is it exact, we're fine, and remove it from the free list else
            // we make one memory region smaller and the next one exact the
            // space we need
            if (ptr->size == num_units) {
                prev_ptr->next = ptr->next;
            } else {
                ptr->size -= num_units;

                // ptr will point to the started of the unused portion of the
                // memory block
                ptr += ptr->size;
                ptr->size = num_units;
            }
            free_ptr = prev_ptr;

            // Add ptr to the used list
            if (used_ptr == NULL) {
                used_ptr = ptr->next = ptr;
            } else {
                ptr->next = used_ptr->next;
                used_ptr->next = ptr;
            }
            return (void*)(ptr + 1);
        }
        // Not enough memory
        if (ptr == free_ptr) {
            ptr = more_memory(num_units);

            // Request for more memory failed
            if (ptr == NULL) {
                return NULL;
            }
        }
    }
}

/*
 * Scan a region of memory and mark any items in the used list appropriately.
 * Both arguments should be word aligned.
 */
static void scan_region(unsigned int* start_ptr, unsigned int* end_ptr) {
    header_t* block_ptr;
    for (; start_ptr < end_ptr; start_ptr++) {
        unsigned int v = *start_ptr;
        block_ptr = used_ptr;
        do {
            if (
                (uintptr_t)(block_ptr + 1) <= v
                && (uintptr_t)(block_ptr + 1 + block_ptr->size) > v
            ) {
                block_ptr->next = (header_t*)(((uintptr_t)block_ptr->next) | 1);
                break;
            }
        } while ((block_ptr = (header_t*)(UNTAG(block_ptr->next))) != used_ptr);
    }
}

/*
 * Scan the marked blocks for references to other unmarked blocks.
 */
static void scan_heap(void) {
    unsigned int* value_ptr;
    header_t* block_ptr;
    header_t* unmarked_ptr;
    for (
        block_ptr = (header_t*)(UNTAG(used_ptr->next));
        block_ptr != used_ptr;
        block_ptr = (header_t*)(UNTAG(block_ptr->next))
    ) {
        if (!((uintptr_t)block_ptr->next & 1)) {
            continue;
        }
        for (value_ptr = (unsigned int*)(block_ptr + 1);
             (header_t*)value_ptr < (block_ptr + block_ptr->size + 1);
             value_ptr++) {
            unsigned int value = *value_ptr;
            unmarked_ptr = (header_t*)(UNTAG(block_ptr->next));
            do {
                if (unmarked_ptr != block_ptr
                    && (uintptr_t)(unmarked_ptr + 1) <= value
                    && (uintptr_t)(unmarked_ptr + 1 + unmarked_ptr->size)
                        > value) {
                    unmarked_ptr->next =
                        (header_t*)(((uintptr_t)unmarked_ptr->next) | 1);
                    break;
                }
            } while ((unmarked_ptr = (header_t*)(UNTAG(unmarked_ptr->next)))
                     != block_ptr);
        }
    }
}

/* Find the absolute bottom of the stack and set stuff up. */
void gc_init(void) {
    static int initted;
    FILE* statfp;
    if (initted) {
        return;
    }
    initted = 1;
    statfp = fopen("/proc/self/stat", "r");
    assert(statfp != NULL);
    fscanf(
        statfp,
        "%*d %*s %*c %*d %*d %*d %*d %*d %*u "
        "%*lu %*lu %*lu %*lu %*lu %*lu %*ld %*ld "
        "%*ld %*ld %*ld %*ld %*llu %*lu %*ld "
        "%*lu %*lu %*lu %lu",
        &stack_bottom
    );
    fclose(statfp);
    used_ptr = NULL;
    base.next = free_ptr = &base;
    base.size = 0;
}

/* Mark blocks of memory in use and free the ones not in use */
void gc_collect(void) {
    header_t* ptr;
    header_t* prev_ptr;
    header_t* t_ptr;
    unsigned int stack_top;

    // Provided by the linker
    extern char end, etext;

    if (used_ptr == NULL) {
        return;
    }

    // Scan the BSS and initialized data segments
    scan_region(&etext, &end);

    // Scan the stack
    asm volatile("movl %%ebp, %0" : "=r"(stack_top));
    scan_region(&stack_top, &stack_bottom);

    // Mark from the heap
    scan_heap();

    // Collect
    for (prev_ptr = used_ptr, ptr = (header_t*)(UNTAG(used_ptr->next));;
         prev_ptr = ptr, ptr = (header_t*)(UNTAG(ptr->next))) {
    next_chunk:
        if (!((unsigned int)ptr->next & 1)) {
            t_ptr = ptr;
            ptr = (header_t*)(UNTAG(ptr->next));
            add_to_free_list(t_ptr);
            if (used_ptr == t_ptr) {
                used_ptr = NULL;
                break;
            }
            prev_ptr->next =
                (header_t*)((unsigned int)ptr | ((unsigned int)prev_ptr->next & 1));
            goto next_chunk;
        }
        ptr->next = (header_t*)(((unsigned int)ptr->next) & ~1);
        if (ptr == used_ptr) {
            break;
        }
    }
}
