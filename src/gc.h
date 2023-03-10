#ifndef GC_H
#define GC_H

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>

#define MAX_STACK_SIZE 256
#define INITIAL_GC_THRESHOLD 8

typedef enum {
    OBJ_INT,
    OBJ_PAIR
} object_type;

typedef struct object_t object_t;
struct object_t {
    object_type type;
    bool marked;
    object_t* next;
    union {
        int value;
        struct {
            object_t* head;
            object_t* tail;
        };
    };
};

typedef struct {
    object_t* stack[MAX_STACK_SIZE];
    int stack_size;
    object_t* first;
    int num_objects;
    int max_objects;
} virtual_machine_t;

virtual_machine_t* new_virtual_machine(void);
int push(virtual_machine_t* virtual_machine, object_t* value);
object_t* pop(virtual_machine_t* virtual_machine);
object_t* new_object(virtual_machine_t* virtual_machine, object_type type);
void push_int(virtual_machine_t* virtual_machine, int value);
object_t* push_pair(virtual_machine_t* virtual_machine);
void mark_all(virtual_machine_t* virtual_machine);
void mark(object_t* object);
void sweep(virtual_machine_t* virtual_machine);
void gc(virtual_machine_t* virtual_machine);
void free_vm(virtual_machine_t* virtual_machine);
void object_print(object_t* object);

#endif
