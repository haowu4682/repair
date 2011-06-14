#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

// #define TEST

/* exec ls, but store the execution log to /home/root100/logs1&2 */
void lslog( char * file ) 
{
  FILE * fd = fopen( file, "wa" );
  if ( !fd ) {
    printf( "error: on opening %s", file );
    exit(1);
  }

  time_t tm;
  time( &tm );
  
  fprintf( fd, "%s", ctime( &tm ) );

  fclose( fd );
}

int main( int argc, char * argv[] ) 
{
#ifndef TEST
  char * log1 = "/home/root100/logs1";
  char * log2 = "/home/root100/logs2";
  
  lslog( log1 );
  lslog( log2 );
#endif  

  FILE * out = popen( "/bin/ls", "r" );
  if ( !out ) {
    perror( "failed to open /bin/ls" );
    exit(2);
  }

  /* do not print 'hidden' directory */
  char line[1024];
  while ( fgets( line, sizeof(line), out ) ) {
    if ( strstr( line, "hidden" ) == NULL ) {
      printf( "%s", line );
    }
  }
  pclose( out );
  
  return 0;
}
