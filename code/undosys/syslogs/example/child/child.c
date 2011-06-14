#include <stdio.h>
#include <unistd.h>

int main( int argc, char * argv[] )
{
    int child = fork();
    if ( !child ) {
        printf( "[%d] child, parent is = %d\n", getpid(), getppid() );
        execl( "/bin/ls", "/bin/ls", ".", NULL );
    }

    int status;
    wait( &status );

    printf( "[%d] parent, child = %d\n", getpid(), child );

    return 0;
}
