//
// minimal #hooks for testing
// 
#ifndef _SYSCALL_HOOKS_H_
#define _SYSCALL_HOOKS_H_

#define HOOKS( FN )                  \
    do {                             \
        FN( unlinkat              ); \
        FN( close                 ); \
        FN( open                  ); \
        FN( read                  ); \
        FN( write                 ); \
        FN( writev                ); \
        FN( ftruncate             ); \
        FN( mmap                  ); \
        FN( creat                 ); \
        FN( truncate              ); \
        FN( rename                ); \
        FN( openat                ); \
        FN( exit                  ); \
        FN( exit_group            ); \
                                     \
        FN##_S( execve            ); \
        FN##_S( clone             ); \
        FN##_S( fork              ); \
        FN##_S( vfork             ); \
    } while ( 0 )

#endif /* _SYSCALL_HOOKS_H_ */