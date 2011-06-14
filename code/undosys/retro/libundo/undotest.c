#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "undocall.h"

int main( int argc, char * argv[] )
{
    syscall( NR_undo_func_start, "arg_undo_mgr", "arg_funcname", 0, 0 );
    syscall( NR_undo_func_end, "arg_buf", strlen("arg_buf") );
    
    syscall( NR_undo_mask_start, 0 );
    syscall( NR_undo_mask_end, 1 );

    syscall( NR_undo_depend, 0, "name", "mgr", 1, 0 );
    syscall( NR_undo_depend, 0, "name", "mgr", 0, 1 );

    return 0;
}
