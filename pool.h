#ifndef POOL_H_
#define POOL_H_
#include <stddef.h>

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

extern Pool *poolcreate(size_t count, size_t blocksize);
extern void *poolgrab(Pool *pool, int est);
extern void poolfree(Pool *pool, void *mem, int est);
extern void pooldestroy(Pool *pool);

#endif
