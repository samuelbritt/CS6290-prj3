#include <assert.h>
#include <stdio.h>
#include <string>
#include <unistd.h>

#include <cstdlib>
#include <cstdio>

#include "sim.h"

typedef enum {
    thread_begin = 0,
    thread_end,load,
    store,
    nop
} opcode_t;

typedef struct buffer {
    int thread_id;
    opcode_t opcode;
    paddr addr;
} buffer_t;

extern Simulator Sim;

int main(int argc, char *argv[])
{
    int c;
    extern char *optarg;

    char buf[50];
    FILE * data_file = NULL;
    buffer_t buffer;
    counter_t count = 0;
    counter_t limit = 750 * (1<<20);

    while ((c = getopt(argc,argv,"f:")) != EOF) {
        switch(c) {
        case 'f':
            sprintf( buf, "zcat %s", optarg );
	    fprintf( stderr, "buf %s\n", buf );
            data_file = popen( buf, "r" );
            break;
        default:
            fprintf (stderr, "Unknown option!\n");
            exit (-1);
        }
    }

    int consumed = 0;
    fprintf (stderr, "limit %lld\n", limit );
    while( count <= limit ) {

        size_t n_read = fread( (void*)&buffer, sizeof(buffer), 1, data_file );
        if( n_read == 0 ) {
            break; 
        }

        buffer.thread_id = buffer.thread_id % NUM_CORES;

        if( buffer.opcode == load || buffer.opcode == store ) {

            //fprintf (stderr, "%d %d 0x%llx\n", buffer.thread_id, buffer.opcode, buffer.addr);
            Sim.new_request( buffer.thread_id, 
                             buffer.addr,
                             buffer.opcode == store ? STORE: LOAD );
            count++;

        }
        else if( buffer.opcode == nop) {
            Sim.new_request( buffer.thread_id, 
                             buffer.addr,
                             NOP );
            count++;
        }
        else if( buffer.opcode == thread_begin ) {
            fprintf (stderr, "Thread %d begins\n", buffer.thread_id);
            count++;
            
        } else if( buffer.opcode == thread_end ) {
            fprintf (stderr, "Thread %d ends\n", buffer.thread_id);
            count++;
        }           

        if (consumed++ > 1024){
            consumed = 0;
            while (Sim.adv_clock());
        }
        
    }
    fprintf (stderr, "processed %llu trace elements\n", count );
    
    Sim.flush_requests ();
    fprintf (stderr, "\n");
    Sim.fini ();
}
