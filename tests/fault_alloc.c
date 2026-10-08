#include "rbtree.h"
#include "../tests/fault_alloc.h"
#include <stdlib.h>

static long countdown;
static long total;

void *rb_malloc(size_t t) 
{
    total += 1;
    if(countdown > 0) //fault_alloc is armed
    {
        countdown -= 1;
        if(countdown == 0) //Call #n
        {
            return NULL; //fail; don't call malloc
        }
    }
    return malloc(t);
}
void rb_free(void *p) {free(p);}

void fault_alloc_arm(long n)
{
    countdown = n;
}
void fault_alloc_disarm()
{
    countdown = 0;
}

long fault_alloc_total() 
{
    return total;
}

