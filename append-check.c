#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <libgen.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <getopt.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

#define RECORD_SIZE 100l
#define BASE_SIZE (20*1024)
#define WORKER_MAX 16

#define MIN(a,b) ((a) < (b) ? (a) : (b))

static void usage( const char *progname, const char *msg )
{
    fprintf( stderr, "%s: %s\n"
                     "usage:\n\n"
                     "\t%s <filename>\n", progname, msg, progname );
    exit( -1 );
}

typedef struct rle_s
{
    char data;
    size_t length;
} rle_t;

static void runlength_scan( const char *data, const size_t max, rle_t *rle )
{
    rle->data = data[0];
    rle->length = 0;

    while( rle->length < max )
    {
	if( data[rle->length] != rle->data )
	    break;

	rle->length++;
    }
}

int main( int argc, char *argv[] )
{
    static unsigned record_count[WORKER_MAX];
    const char *progname = basename(argv[0]);
    size_t record_size = RECORD_SIZE; 
    size_t base_size = BASE_SIZE;

    char ch;
    while ((ch = getopt(argc, argv, "b:s:")) != -1)
    {
         switch (ch)
         {
         case 'b':
             base_size = atoi( optarg );
	     break;

         case 's':
             record_size = atoi( optarg );
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

    /* Open test file */
    struct stat st;
    const int stat_result = stat( proc_file, &st );
    if( stat_result < 0 )
    {
        usage( progname, strerror(errno) );
    }

    int proc_fd = open( proc_file, O_RDWR );
    if( proc_fd < 0 )
    {
        usage( progname, strerror(errno) );
    }

    char *data = mmap( NULL, st.st_size, PROT_READ, MAP_PRIVATE, proc_fd, 0 );
    if( data == NULL )
    {
	usage( progname, strerror(errno) );
    }

    /* Scan the base section of the file (should be all zeroes) */
    unsigned corruptions = 0;
    size_t bytes_scanned = 0, bytes_remain = base_size;
    while( bytes_remain )
    {
	rle_t rl;
	runlength_scan( data + bytes_scanned, bytes_remain, &rl );
  	if( rl.data != '\0' )
	{
	    fprintf( stderr, "%s: data corruption at %zd: %zd bytes by worker %c\n",
			progname, bytes_scanned, rl.length, rl.data );
            corruptions++;
	}
        bytes_scanned += rl.length, bytes_remain -= rl.length;
    }

    /* Scan the appended section of the file */
    bytes_remain = st.st_size - bytes_scanned;
    while( bytes_remain )
    {
	rle_t rl;
        runlength_scan( data + bytes_scanned, MIN(record_size, bytes_remain), &rl );
        const unsigned proc_id = rl.data - '0';
        if( rl.length != record_size )
	{
	    fprintf( stderr, "%s: data corruption at %zd: %zd bytes by worker %c, expected %zd\n",
			progname, bytes_scanned, rl.length, rl.data, record_size );
	    corruptions++;
	}
        record_count[ proc_id ]++;
        bytes_scanned += rl.length, bytes_remain -= rl.length;
    }

    printf( "%s: %u corruptions detected\n", progname, corruptions );
    for( unsigned i=0; i < WORKER_MAX; i++ )
    {
	if( record_count[i] )	printf( "%s: worker %u: %u records written\n", progname, i, record_count[i] );
    }

    close( proc_fd );
    return 0;
}
