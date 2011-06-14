//
// xdr in kernel space
//

typedef struct xdr_stream XDR;
typedef int bool_t;
typedef uint64_t quad_t;

#define dbgxdr( msg, ... )
/*
#define dbgxdr( msg, ... )                      \
    do {                                        \
        printk( KERN_INFO "%s(%d): " msg,       \
                __FUNCTION__,                   \
                __LINE__,                       \
                __VA_ARGS__ );                  \
    } while( 0 )
*/
#define TRUE  (1)
#define FALSE (0)

static inline
bool_t xdr_int( XDR * xdr, int * n )
{
    __be32 * p;
    __be32 raw = htonl(*n);

    dbgxdr( "n=%d", *n );

    p = xdr_reserve_space( xdr, sizeof(*n) );
    if ( unlikely( p == NULL ) ) {
        return 0;
    }

    *p = raw;

    return 1;
}

static inline
bool_t xdr_bool( XDR * xdr, int * n )
{
    __be32 * p;
    __be32 raw = htonl(*n != 0);

    dbgxdr( "n=%d", *n );

    p = xdr_reserve_space( xdr, sizeof(*n) );
    if ( unlikely( p == NULL ) ) {
        return 0;
    }

    *p = raw;

    return 1;
}

static inline
bool_t xdr_quad_t( XDR * xdr, quad_t * n )
{
    dbgxdr( "n=%ld", (unsigned long) *n );

    xdr_int( xdr, (int *)((unsigned long) n + 4) );
    xdr_int( xdr, (int *)n );

    return 1;
}

typedef bool_t (*xdrproc_t) (XDR *, void *,...);

static inline
bool_t xdr_bytes( XDR * xdr, char ** sp, unsigned int * sizep,
                  unsigned int maxsize )
{
    __be32 * p;

    dbgxdr( "size:%u, buf:%p", *sizep, *sp );

    xdr_int( xdr, sizep );

    p = xdr_reserve_space( xdr, *sizep );
    if ( unlikely( p == NULL ) ) {
        return 0;
    }

    memcpy( p, *sp, *sizep );

    return 1;
}

static inline
bool_t xdr_string( XDR * xdr, char ** sp, unsigned int maxsize )
{
    __be32 * p;
    int len = 0;

    if ( *sp != NULL ) {
        len = strnlen( *sp, maxsize );
    }

    dbgxdr( "%s(%d)", *sp, len );

    if ( unlikely( xdr_int( xdr, &len ) == 0 ) ) {
        return 0;
    }

    if ( len == 0 ) {
        return 1;
    }

    p = xdr_reserve_space( xdr, len );
    if ( unlikely( p == NULL ) ) {
        return 0;
    }

    strncpy( (char *) p, *sp, len );

    return 1;
}

static inline
bool_t xdr_array( XDR * xdr, char ** arrp, unsigned int * sizep,
                  unsigned int maxsize, unsigned int elsize,
                  xdrproc_t elproc )
{
    unsigned int i = 0;

    dbgxdr( "size=%u, array=%p, proc=%p", *sizep, *arrp, elproc );

    xdr_int( xdr, sizep );

    for ( i = 0 ; i < *sizep ; i ++ ) {
        void * next = (void *)( (unsigned long) *arrp + i * elsize );

        dbgxdr( "called#%d %p(%p)", i, elproc, next );

        if ( !elproc( xdr, next ) ) {
            return FALSE;
        }
    }

    return TRUE;
}

static inline
bool_t xdr_pointer( XDR * xdr, char ** objpp,
                    unsigned int objsize, xdrproc_t xdrobj )
{
    int n = 0;

    dbgxdr( "obj:%p(%p)", xdrobj, *objpp );

    if ( *objpp != NULL ) {
        n = 1;
        xdr_int( xdr, &n );
        return xdrobj( xdr, *objpp );
    }

	xdr_int( xdr, &n );

    return TRUE;
}

#undef dbgxdr