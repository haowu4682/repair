#ifndef _HOOK_H_
#define _HOOK_H_

#define IF_TRACKING_BEG_XDR(statments)              \
    if ( is_tracing() ) {                           \
        statments;                                  \
        rec.pid   = _pid;                           \
        rec.enter = 1;                              \
        rec.ret   = 0;                              \
        xdr_fill_sysarg( &rec, &args[0] );          \
        rec.err   = 0;                              \
        xdr_free_sysarg( &rec, &args[0] );          \
        post_hook = 1;                              \
    } do {} while(0)

#define IF_TRACKING_END_XDR(statments)              \
    if ( post_hook ) {                              \
        rec.enter = 0;                              \
        rec.ret   = rtn;                            \
        rec.err   = 0;                              \
        xdr_fill_sysarg( &rec, &args[0] );          \
        statments;                                  \
        xdr_free_sysarg( &rec, &args[0] );          \
    }                                               \
    return rtn

#define DEF_HOOK_FN_0( name )                                                   \
    DEF_HOOK_FN( name, void ) {                                                 \
        IF_TRACKING_BEG_XDR();                                                  \
        POST_HOOK_RTN( name );                                                  \
        IF_TRACKING_END_XDR();                                                  \
    } END_DEF_HOOK

#define DEF_HOOK_FN_1( name, arg1_t, arg1 )                                     \
    DEF_HOOK_FN( name, arg1_t arg1 ) {                                          \
        IF_TRACKING_BEG_XDR( args[0] = &arg1 );                                 \
        POST_HOOK_RTN( name, arg1 );                                            \
        IF_TRACKING_END_XDR();                                                  \
    } END_DEF_HOOK

#define DEF_HOOK_FN_2( name, arg1_t, arg1, arg2_t, arg2 )                       \
    DEF_HOOK_FN( name, arg1_t arg1, arg2_t arg2 ) {                             \
        IF_TRACKING_BEG_XDR( args[0] = &arg1; args[1] = &arg2 );                \
        POST_HOOK_RTN( name, arg1, arg2 );                                      \
        IF_TRACKING_END_XDR();                                                  \
    } END_DEF_HOOK

#define DEF_HOOK_FN_3( name, arg1_t, arg1, arg2_t, arg2, arg3_t, arg3 )         \
    DEF_HOOK_FN( name, arg1_t arg1, arg2_t arg2, arg3_t arg3 ) {                \
        IF_TRACKING_BEG_XDR(args[0] = &arg1; args[1] = &arg2; args[2] = &arg3); \
        POST_HOOK_RTN( name, arg1, arg2, arg3 );                                \
        IF_TRACKING_END_XDR();                                                  \
    } END_DEF_HOOK

#define DEF_HOOK_FN_4( name, arg1_t, arg1, arg2_t, arg2, arg3_t, arg3,          \
                       arg4_t, arg4 )                                           \
    DEF_HOOK_FN( name, arg1_t arg1, arg2_t arg2, arg3_t arg3, arg4_t arg4 ) {   \
        IF_TRACKING_BEG_XDR(args[0] = &arg1; args[1] = &arg2;                   \
                            args[2] = &arg3; args[3] = &arg4 );                 \
        POST_HOOK_RTN( name, arg1, arg2, arg3, arg4 );                          \
        IF_TRACKING_END_XDR();                                                  \
    } END_DEF_HOOK

#define DEF_HOOK_FN_5( name, arg1_t, arg1, arg2_t, arg2, arg3_t, arg3,          \
                       arg4_t, arg4, arg5_t, arg5 )                             \
    DEF_HOOK_FN( name, arg1_t arg1, arg2_t arg2, arg3_t arg3, arg4_t arg4,      \
                 arg5_t arg5 ) {                                                \
        IF_TRACKING_BEG_XDR(args[0] = &arg1; args[1] = &arg2;                   \
                            args[2] = &arg3; args[3] = &arg4;                   \
                            args[4] = &arg5 );                                  \
        POST_HOOK_RTN( name, arg1, arg2, arg3, arg4, arg5 );                    \
        IF_TRACKING_END_XDR();                                                  \
    } END_DEF_HOOK

#define DEF_HOOK_FN_6( name, arg1_t, arg1, arg2_t, arg2, arg3_t, arg3,          \
                       arg4_t, arg4, arg5_t, arg5, arg6_t, arg6 )               \
    DEF_HOOK_FN( name, arg1_t arg1, arg2_t arg2, arg3_t arg3, arg4_t arg4,      \
                 arg5_t arg5, arg6_t arg6 ) {                                   \
        IF_TRACKING_BEG_XDR(args[0] = &arg1; args[1] = &arg2;                   \
                            args[2] = &arg3; args[3] = &arg4;                   \
                            args[4] = &arg5; args[5] = &arg6 );                 \
        POST_HOOK_RTN( name, arg1, arg2, arg3, arg4, arg5, arg6 );              \
        IF_TRACKING_END_XDR();                                                  \
    } END_DEF_HOOK

#endif /* _HOOK_H_ */