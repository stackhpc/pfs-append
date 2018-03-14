#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <libgen.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <getopt.h>

#include <sys/types.h>
#include <sys/select.h>

#define RECORD_SIZE 100l
#define RECORD_ITERS 100u
#define RECORD_DELAY 10u

static void usage( const char *progname, const char *msg )
{
    fprintf( stderr, "%s: %s\n"
                     "usage:\n\n"
                     "\t%s <filename>\n", progname, msg, progname );
    exit( -1 );
}

static fd_set zero_fd;

static void delay( const unsigned proc_ms )
{
    struct timeval tv;
    tv.tv_sec = proc_ms / 1000;
    tv.tv_usec = (proc_ms % 1000) * 1000;
    select( 1, &zero_fd, &zero_fd, &zero_fd, &tv );
}

int main( int argc, char *argv[] )
{
    static const char proc_char[] = "0123456789ABCDEF";
    const char *progname = basename(argv[0]);
    size_t record_size = RECORD_SIZE; 
    char *record_buf;
    unsigned proc_id = 0;
    unsigned proc_iters = RECORD_ITERS;
    unsigned proc_ms = RECORD_DELAY;

    FD_ZERO( &zero_fd );

    char ch;
    while ((ch = getopt(argc, argv, "s:n:c:")) != -1)
    {
         switch (ch)
         {
         case 's':
             record_size = atoi( optarg );
             break;

         case 'n':
             proc_id = atoi( optarg );
             break;

	 case 'c':
	     proc_iters = atoi( optarg );
	     break;

         default:
             usage( progname, "Incorrect arguments" );
         }
    }
    argc -= optind;
    argv += optind; 
    if( argc <= 0 )
    {
        usage( progname, "Expected a test file name" );
    }
    const char *proc_file = argv[argc-1];

    /* Generate test data */
    record_buf = malloc( record_size );         /* NB: not null-terminating */
    if( record_buf == NULL )
    {
        fprintf( stderr, "%s: could not alloc buffer for %zd byte records\n", progname, record_size );
    }
    memset( record_buf, proc_char[proc_id % sizeof(proc_char)], record_size );

    /* Open test file */
    int proc_fd = open( proc_file, O_RDWR | O_APPEND );
    if( proc_fd < 0 )
    {
        usage( progname, strerror(errno) );
    }

    for( unsigned i=0; i < proc_iters; i++ )
    {
        delay( proc_ms );
        const int write_result = write( proc_fd, record_buf, record_size );
        if( write_result != record_size )
        {
            if( write_result < 0 )
            {
                fprintf( stderr, "%s[%u]: write error on iteration %u: %s\n",
                         progname, proc_id, i, strerror(errno) );
                exit( -1 );
            }
            else if( write_result == 0 )
            {
                fprintf( stderr, "%s[%u]: end-of-file on iteration %u: %s\n",
                         progname, proc_id, i, strerror(errno) );
                exit( -1 );
            }
            else
            {
                fprintf( stderr, "%s[%u]: short write on iteration %u: %d wanted %zd\n",
                         progname, proc_id, i, write_result, record_size );
                /* Bravely soldier on */
            }
        }
    }
    close( proc_fd );
    return 0;
}
