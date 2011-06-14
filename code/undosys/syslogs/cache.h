#ifndef _CACHE_H_
#define _CACHE_H_

struct string {
    char c;
};

// big stat info trick
#define big_stat_info stat_info

//
// make sure that 2^*
//
#define __CACHE_SIZE 256

#define DEF_CACHE( name, UNIT )                                       \
                                                                      \
const int CACHE_UNIT_##name = (UNIT);                                 \
static spinlock_t cache_lock_##name;                                  \
                                                                      \
struct cache_##name                                                   \
{                                                                     \
    int busy;                                                         \
    struct name var[UNIT];                                            \
};                                                                    \
                                                                      \
struct cache_##name * __cache_##name;                                 \
                                                                      \
static                                                                \
void __cache_dump_##name( int next )                                  \
{                                                                     \
    int i;                                                            \
    for ( i = (next - 5 >= 0 ? next - 5 : 0) ;                        \
          i < (next + 5 < __CACHE_SIZE ? next + 5 : __CACHE_SIZE ) ;  \
          i ++ ) {                                                    \
        printk( KERN_ERR "[%d] %d:%p",                                \
                i,                                                    \
                __cache_##name[i].busy,                               \
                (void *) &__cache_##name[i].var );                    \
    }                                                                 \
}                                                                     \
                                                                      \
static                                                                \
int init_cache_##name( void )                                         \
{                                                                     \
    spin_lock_init( &cache_lock_##name );                             \
                                                                      \
    __cache_##name                                                    \
        = vmalloc( __CACHE_SIZE * sizeof( struct cache_##name ) );    \
                                                                      \
    if ( !__cache_##name ) {                                          \
        printk( KERN_ERR "CRITICAL ERROR:%s ON VMALLOC!!",            \
                __FUNCTION__ );                                       \
    }                                                                 \
                                                                      \
    memset( __cache_##name, 0,                                        \
            __CACHE_SIZE * sizeof( struct cache_##name ) );           \
                                                                      \
    return (__cache_##name == NULL);                                  \
}                                                                     \
                                                                      \
static                                                                \
void exit_cache_##name( void )                                        \
{                                                                     \
    vfree( __cache_##name );                                          \
}                                                                     \
                                                                      \
static                                                                \
struct name * get_cache_##name( int len )                             \
{                                                                     \
    static int next = 0;                                              \
    struct name * rtn = NULL;                                         \
                                                                      \
    spin_lock( &cache_lock_##name );                                  \
                                                                      \
    if ( len > UNIT ) {                                               \
        printk( KERN_ERR "CRITICAL ERROR:%s!!(UNIT:%d,LEN:%d,"        \
                "BUSY:%d,NEXT:%d)",                                   \
                __FUNCTION__,                                         \
                UNIT,                                                 \
                len,                                                  \
                __cache_##name[next].busy,                            \
                next );                                               \
        __cache_dump_##name( next );                                  \
        goto out;                                                     \
    }                                                                 \
                                                                      \
    if ( unlikely( __cache_##name[next].busy ) ) {                    \
        int old = next;                                               \
        do {                                                          \
            next = (next + 1) % __CACHE_SIZE;                         \
        } while ( old != next && __cache_##name[next].busy );         \
                                                                      \
        if ( old == next ) {                                          \
            printk( KERN_ERR "CRITICAL ERROR: Full of cache!!" );     \
            __cache_dump_##name( next );                              \
            goto out;                                                 \
        }                                                             \
    }                                                                 \
                                                                      \
    dbg( cache_debug, "%p(%d)",                                       \
         (void *) &__cache_##name[next].var[0], len );                \
                                                                      \
    __cache_##name[next].busy = 1;                                    \
    rtn = &__cache_##name[ next ].var[0];                             \
    next = (next + 1) % __CACHE_SIZE;                                 \
                                                                      \
out:                                                                  \
    spin_unlock( &cache_lock_##name );                                \
    return rtn;                                                       \
}                                                                     \
                                                                      \
static                                                                \
void put_cache_##name( struct name * var )                            \
{                                                                     \
    struct cache_##name * cache = (struct cache_##name *)             \
        ((unsigned long) var - offsetof( struct cache_##name, var )); \
                                                                      \
    dbg( cache_debug, "%p(%p:%d)",                                    \
         (void *) var, (void *) cache, cache->busy );                 \
    cache->busy = 0;                                                  \
}

#endif /* _CACHE_H_ */
