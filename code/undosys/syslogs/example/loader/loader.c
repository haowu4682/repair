#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main( int argc, char * argv[] )
{
    if ( argc < 2 ) {
        printf( "usage : %s exe args\n", argv[0] );
        exit(1);
    }

    if ( !fork() ) {
        FILE * fp = fopen( "/proc/undosys/tracing", "w" );
        if ( !fp ) {
            printf( "install undosys first!" );
            exit(1);
        }

        fprintf( fp, "%d", getpid() );
        fclose( fp );

        if ( execv( argv[1], &argv[1] ) == -1 ) {
            printf( "failed to execute : %s\n", argv[1] );
        }
        exit(1);
    }

    int status;
    wait( &status );
    
    return 0;
}
