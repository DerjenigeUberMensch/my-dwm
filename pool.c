#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdio.h>

#include "util.h"

typedef struct Block Block;
typedef struct Pool Pool;

struct Pool
{
    size_t bsize;
    size_t size;
    Block **mem;
};

struct Block
{
    void *mem;
    int free;
};

Pool *
poolcreate(size_t count, size_t blocksize)
{
    Pool *pool = ecalloc(1, sizeof(Pool));
    size_t i;
    pool->size = count * blocksize;
    pool->bsize= blocksize;
    pool->mem  = ecalloc(count, sizeof(Block *));
    for(i = 0; i < count; ++i) 
    {
        pool->mem[(int)i] = ecalloc(1, sizeof(Block ));
        pool->mem[(int)i]->mem = ecalloc(1, blocksize);
        pool->mem[(int)i]->free = 1;
    }
    return pool;
}

void *
poolgrab(Pool *pool, int est)
{
    size_t plength = pool->size / pool->bsize;
    size_t i;
    est = abs(est);
    for(i = est; i < plength; ++i)
    {
        if(pool->mem[(int)i]->free)
        {
            pool->mem[(int)i]->free = 0;
            return pool->mem[(int)i]->mem;
        }
    }
    for(i = 0; i < plength; ++i)
    {
        if(pool->mem[(int)i]->free)
        {
            pool->mem[(int)i]->free = 0;
            return pool->mem[(int)i]->mem;
        }
    }
    /* on the off chance that we cant grab any memory for some reason alloc some */
    debug("WARN: POOL IS EMPTY");
    return ecalloc(1, pool->bsize);
}

void
poolfree(Pool *pool, void *mem, int est)
{
    int plength = pool->size / pool->bsize;
    int i;
    est = abs(est);
    for(i = est; i < plength; ++i)
    {
        if(memcmp(pool->mem[i]->mem, mem, pool->bsize) == 0)
        {
            /* "free memory" */
            memset(pool->mem[i]->mem, 0, pool->bsize);
            pool->mem[i]->free = 1;
            return;
        }
    }
    for(i = 0; i < plength; ++i)
    {
        if(memcmp(pool->mem[i]->mem, mem, pool->bsize) == 0)
        {
            /* "free memory" */
            memset(pool->mem[i]->mem, 0, pool->bsize);
            pool->mem[i]->free = 1;
            return;
        }
    }
    debug("WARN: POOL CANNOT FREE SPECIFIED MEMORY");
    printf("Freed: %p\n", mem);
    /* on the off chance that we couldnt find the mem assume it was calloced */
    if(mem) free(mem);
}

void
pooldestroy(Pool *pool)
{
    int plength = pool->size / pool->bsize;
    int i;

    for(i = 0; i < plength; ++i)
    {
        free(pool->mem[i]->mem);
        free(pool->mem[i]);
    }
    free(pool->mem);
    free(pool);
}
