
/**********************************************************************

   File          : cmpsc473-p2.c

   Description   : Slab allocation and defenses

***********************************************************************/
/**********************************************************************
Copyright (c) 2019 The Pennsylvania State University
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
    * Neither the name of The Pennsylvania State University nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
***********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <unistd.h>
#include "cmpsc473-format-66.h"   // CHANGE: student-specific
#include "cmpsc473-mm.h"
#include "clock.h"

/* Defines */

/* Globals */

/* Project APIs */
extern void process_cmds( char *cmdfile );

/* extern data */
extern unsigned int canary;

/*****************************

Invoke:
cmpsc473-p2 <cmd-file>

Commands:
malloc <A/B/C> <id-start> <id-end>
     - allocate objects of type <A/B/C> for IDs from <id-start> to <id-end>
free <id-start> <id-end>
     - free objects for IDs from <id-start> to <id-end>
write <id> <bytes>
     - write <bytes> number of bytes to object at ID <id>
save <A/B/C> <id>
     - save a pointer of type <A/B/C> from ID <id>
     - NOTE: ID <id> may be allocated for a different type - we will check_type() to verify
use (saved reference)
     - use the last saved pointer 

******************************/

int main( int argc, char *argv[] )
{
	assert( argc == 2 );

	mm_init();
	process_cmds( argv[1] );

	exit( 0 );
}



void process_cmds( char *cmdfile )
{
	char *line = NULL;
	unsigned long len = 0;
	size_t size = 0;
	FILE *fp;
	static void *map[4000];  
	char typ;
	void *saved_ptr = NULL;
	unsigned int saved_id = 0;

	memset( map, 0, sizeof(void *)*4000 );

	// Get buf for current file contents
	fp = fopen( cmdfile, "r" );  // read input
	assert( fp != NULL ); 

	start_timer();

	while(1) {
		unsigned int id, id1, id2, bytes;
		size = getline( &line, &len, fp );
		if ( size == -1 ) break;

		if ( sscanf( line, "malloc %c %d %d\n", &typ, &id1, &id2 ) == 3 ) {
			switch( typ ) {
				struct A *addrA;
				struct B *addrB;
				struct C *addrC;
			case 'A':
				for ( id = id1; id <= id2; id++ ) {
					addrA = (struct A *)my_malloc(sizeof( struct A ));
					printf("> Putting A at %p into map at id %d\n", addrA, id );
					memset( USE_A( addrA ), 0, sizeof( struct A ));
					map[id] = (void *)addrA;
				}
				break;
			case 'B':
				for ( id = id1; id <= id2; id++ ) {
					addrB = (struct B *)my_malloc(sizeof( struct B ));
					printf("> Putting B at %p into map at id %d\n", addrB, id );
					memset( USE_B( addrB ), 0, sizeof( struct B ));
					map[id] = (void *)addrB;
				}
				break;
			case 'C':
				for ( id = id1; id <= id2; id++ ) {
					addrC = (struct C *)my_malloc(sizeof( struct C ));
					printf("> Putting C at %p into map at id %d\n", addrC, id );
					memset( USE_C( addrC ), 0, sizeof( struct C ));
					map[id] = (void *)addrC;
				}
				break;
			default:
				printf("%c: Not a known type\n", typ );
				break;
			}
		}
		else if ( sscanf( line, "free %d %d\n", &id1, &id2 ) == 2 ) {
			for( id = id1; id<=id2; id++) {
				my_free( USE( map[id] ));
				printf("< Freeing object at %p from map at id %d\n", USE( map[id] ), id );
				map[id] = (void *) NULL;
			}
		}
		else if ( sscanf( line, "write %d %d\n", &id, &bytes ) == 2) {
			void *obj = map[id];
			int i;

			printf( "+++ Write %d bytes to ID %d\n",
				bytes, id );

			if ( map[id] == NULL ) {
				printf("No allocated object for ID %d\n", id );
				continue;
			}

			// write bytes
			for ( i=0; i<bytes; i++ ) {
				if (( i % 3 ) == 0 )
					((char *) USE( obj ))[i] = 0;
				else 
					((char *) USE( obj ))[i] = 'a';   // put in some null terminators
			}

			// New
			printf("Canary: 0x%x\n", canary);

			// check for overwrite - check_canary success returns 0
			if ( check_canary( (void *)USE( obj ))) {
				printf("*** Canary overwritten for ID %d at %p using %d bytes ***\n",
				       id, USE( obj ), bytes );
			}
			else {
				printf("First non-zero byte of ID %d at %p is %c\n",
				       id, USE( obj ), ((char *)(USE( obj )))[1] );
			}
		}
		else if ( sscanf( line, "save %c %d\n", &typ, &id) ==2){
			void *obj = map[id];

			printf( "+++ Save ID %d:%p of type %c\n",
				id, obj, typ );

			if ( obj == NULL ) {
				printf("No allocated object for ID %d\n", id );
				continue;
			}

			if ( check_type ( USE( obj ), typ )) {
				printf("*** Type for ID %d at %p failed: used type %c ***\n",
				       id, USE( obj ), typ);
				continue;
			}

			saved_ptr = map[id];
			saved_id = id;
		}
		else if (strstr( line, "use\n") != 0) {
			void *obj = saved_ptr;

			printf( "+++ Use saved ptr %p of saved_id %d\n",
				obj, saved_id);

			if ( saved_ptr == NULL ) {
				printf("No saved pointer has been recorded\n" );
				continue;
			}

			if ( check_count( obj )) {
				printf("*** Count for ID %d at %p:%p failed ***\n",
				       id, obj, USE( obj ));
			}
			else {
				printf("First non-zero byte of ID %d at %p is %c\n",
				       id, USE( obj ), ((char *)(USE( obj )))[1] );
			}
		}
	}
	printf("Time taken: %f\n",get_timer());
	get_stats();
	free( line );
}



