#ifndef POOL_H_
#define POOL_H_

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

Pool *poolcreate(size_t count, size_t blocksize);
void *poolgrab(Pool *pool, int est);
void poolfree(Pool *pool, void *mem, int est);
void pooldestroy(Pool *pool);


#endif
