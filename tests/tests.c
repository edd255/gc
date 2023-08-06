#include <assert.h>
#include "gc.h"

void test1(void)
{
        virtual_machine_t* virtual_machine = new_virtual_machine();
        push_int(virtual_machine, 1);
        push_int(virtual_machine, 2);
        gc(virtual_machine);
        assert(virtual_machine -> num_objects == 2);
        free_vm(virtual_machine);
}

void test2(void)
{
        virtual_machine_t* virtual_machine = new_virtual_machine();
        push_int(virtual_machine, 1);
        push_int(virtual_machine, 2);
        assert(virtual_machine -> num_objects == 2);
        pop(virtual_machine);
        pop(virtual_machine);
        gc(virtual_machine);
        assert(virtual_machine -> num_objects == 0);
        free_vm(virtual_machine);
}

int main(void)
{
        test1();
        test2();
        return 0;
}
