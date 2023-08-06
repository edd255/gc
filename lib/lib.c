#include "lib.h"

virtual_machine_t* new_virtual_machine(void) {
    virtual_machine_t* virtual_machine =
        (virtual_machine_t*)malloc(sizeof(virtual_machine_t));
    virtual_machine->stack_size = 0;
    virtual_machine->num_objects = 0;
    virtual_machine->first = NULL;
    virtual_machine->max_objects = INITIAL_GC_THRESHOLD;
    return virtual_machine;
}

int push(virtual_machine_t* virtual_machine, object_t* value) {
    if (virtual_machine->stack_size >= MAX_STACK_SIZE) {
        return 0;
    }
    virtual_machine->stack[virtual_machine->stack_size++] = value;
    return 1;
}

object_t* pop(virtual_machine_t* virtual_machine) {
    if (virtual_machine->stack_size <= 0) {
        return NULL;
    }
    return virtual_machine->stack[--virtual_machine->stack_size];
}

object_t* new_object(virtual_machine_t* virtual_machine, object_type type) {
    object_t* object = (object_t*)malloc(sizeof(object_t));
    object->type = type;
    object->marked = false;
    object->next = virtual_machine->first;
    virtual_machine->first = object;
    virtual_machine->num_objects++;
    return object;
}

void push_int(virtual_machine_t* virtual_machine, int value) {
    object_t* object = new_object(virtual_machine, OBJ_INT);
    object->value = value;
    push(virtual_machine, object);
}

object_t* push_pair(virtual_machine_t* virtual_machine) {
    object_t* object = new_object(virtual_machine, OBJ_PAIR);
    object->tail = pop(virtual_machine);
    object->head = pop(virtual_machine);
    push(virtual_machine, object);
    return object;
}

void mark_all(virtual_machine_t* virtual_machine) {
    for (int i = 0; i < virtual_machine->stack_size; i++) {
        mark(virtual_machine->stack[i]);
    }
}

void mark(object_t* object) {
    if (object->marked) {
        return;
    }
    object->marked = true;
    if (object->type == OBJ_PAIR) {
        mark(object->head);
        mark(object->tail);
    }
}

void sweep(virtual_machine_t* virtual_machine) {
    object_t** object = &virtual_machine->first;
    while (*object != NULL) {
        if (!(*object)->marked) {
            object_t* unreached = *object;
            *object = unreached->next;
            free(unreached);
            virtual_machine->num_objects--;
        } else {
            (*object)->marked = false;
            object = &(*object)->next;
        }
    }
}

void gc(virtual_machine_t* virtual_machine) {
    int num_objects = virtual_machine->num_objects;
    mark_all(virtual_machine);
    sweep(virtual_machine);
    virtual_machine->max_objects = 2 * num_objects;
    virtual_machine->max_objects = virtual_machine->num_objects == 0
        ? INITIAL_GC_THRESHOLD
        : virtual_machine->num_objects * 2;
}

void free_vm(virtual_machine_t* virtual_machine) {
    virtual_machine->stack_size = 0;
    gc(virtual_machine);
    free(virtual_machine);
}

void object_print(object_t* object) {
    switch (object->type) {
        case OBJ_INT:
            printf("%d\n", object->value);
            break;

        case OBJ_PAIR:
            printf("(");
            object_print(object->head);
            printf(", ");
            object_print(object->tail);
            printf(")");
            break;
    }
}
