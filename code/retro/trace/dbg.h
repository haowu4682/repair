#pragma once

//
// comment this to depress all debuging message
//
// ex) dbg(filter, msg), err(msg),
//     dbg(unlink, "failed:%d", error)
//

#include "config.h"

#ifdef RETRO_DEBUG

 enum { dbg_welcome  = 1 };
 enum { dbg_unlink   = 0 };
 enum { dbg_iget     = 1 };
 enum { dbg_sha1     = 1 };
 enum { dbg_test     = 1 };
 enum { dbg_trace    = 0 };
 enum { dbg_det      = 0 };
 enum { dbg_hook     = 0 };
 enum { dbg_info     = 1 };

# define dbg( filter, msg, ... )                \
    do {                                        \
        if ( dbg_##filter ) {                   \
            printk( KERN_ERR "%s(%d): " msg,    \
                    __FUNCTION__,               \
                    __LINE__,                   \
                    ##__VA_ARGS__ );            \
        }                                       \
    } while( 0 )

# define ifdbg( filter, statements )            \
    do {                                        \
        if ( dbg_##filter ) {                   \
            (statements);                       \
        }                                       \
    } while ( 0 )

# define msg( filter, msg ) dbg( filter, "%s", msg )

#else

# define dbg( filter, msg, ... )                \
    do {                                        \
    } while( 0 )

# define ifdbg( filter, statements )            \
    do {                                        \
    } while ( 0 )

# define msg( filter, msg )                     \
    do {                                        \
    } while ( 0 )
    
#endif

#define err( msg, ... )                         \
    do {                                        \
        printk( KERN_ERR "[!] %s(%d): " msg,    \
                __FUNCTION__,                   \
                __LINE__,                       \
                ##__VA_ARGS__ );                \
    } while( 0 )
