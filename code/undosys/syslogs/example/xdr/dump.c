#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>

#include "record.h"
#include "shared_xdr.h"

int main( int argc, char * argv[] )
{
    static FILE * record_f;
    static XDR xdrs;

    if (!record_f) {
        int record_fd = open( "record.log", O_WRONLY | O_CREAT | O_EXCL, 0666 );
        if (record_fd < 0) {
            perror("opening record.log");
            exit(-1);
        }

        record_f = fdopen(record_fd, "w");
        if (!record_f) {
            perror("fdopen");
            exit(-1);
        }

        setbuf(record_f, 0);
        xdrstdio_create( &xdrs, record_f, XDR_ENCODE );
    }

    /* syscall_record rec; */
    /* memset(&rec, 0, sizeof(rec)); */

    /* rec.pid = 111; */
    /* rec.scno = 222; */
    /* rec.scname = "goodluck"; */
    /* rec.enter = 3; */
    /* rec.ret = 444; */
    /* rec.err = 1; */

    /* rec.args.args_len = 0; */
    /* rec.args.args_val = NULL; */

    /* xdr_syscall_record(&xdrs, &rec); */

    printf( "sizeof( quad_t )         : %ld\n", sizeof( quad_t         ) );
    printf( "sizeof( stat_info )      : %ld\n", sizeof( stat_info      ) );
    printf( "sizeof( namei_info )     : %ld\n", sizeof( namei_info     ) );
    printf( "sizeof( namei_infos )    : %ld\n", sizeof( namei_infos    ) );
    printf( "sizeof( argv_str )       : %ld\n", sizeof( argv_str       ) );
    printf( "sizeof( syscall_arg )    : %ld\n", sizeof( syscall_arg    ) );
    printf( "sizeof( syscall_record ) : %ld\n", sizeof( syscall_record ) );
    
    test_xdr( &xdrs );

    return 0;
}
