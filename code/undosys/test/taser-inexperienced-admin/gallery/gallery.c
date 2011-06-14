#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main( int argc, char * args[] )
{
  char cmd[1024];
  
  system( "mkdir -p mygallery" );
  
  for ( int i = 0 ; i < argc ; i ++ ) {
    
    if ( strcmp( args[i], "create" ) == 0 && i < argc - 1 ) {
      printf( "Creating new album: %s\n", args[i+1] );
      snprintf( cmd, sizeof( cmd ), "mkdir mygallery/%s\n", args[i+1] );
      system( cmd );
      exit(0);
    }

    if ( strcmp( args[i], "add" ) == 0 && i < argc - 2 ) {
      printf( "Adding new picture %s to the album %s\n", args[i+2], args[i+1] );
      snprintf( cmd, sizeof( cmd ), "echo %s > mygallery/%s/%s\n", args[i+2], args[i+1], args[i+2] );
      system( cmd );
      exit(0);
    }
  }

  exit(1);
}
