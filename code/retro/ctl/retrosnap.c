#include <stdio.h>
#include <unistd.h>

#define __user 
#include "../trace/btrfs.h"

int retro_snapshot(const char * trunk,
                   const char * parent,
                   const char * name)
{
    return syscall(NR_snapshot, trunk, parent, name);
}

int main( int argc, char * argv[] )
{
    if ( argc != 4 ) {
        fprintf(stderr, "Usage: %s trunk-dir snap-parent snap-name\n", argv[0]);
        fprintf(stderr, "Create snapshot of trunk-dir in snap-parent/name\n");
    }
        
    return retro_snapshot(argv[1], argv[2], argv[3]) >= 0 ? 0 : -1;
}
