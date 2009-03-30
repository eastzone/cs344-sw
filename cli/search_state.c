/* Filename: search_state.c */

#include <errno.h>            /* errno */
#include <stdlib.h>           /* malloc, realloc */
#include <string.h>           /* memset */
#include "helper.h"
#include "search_state.h"

void search_state_init( search_state_t* state, size_t init_size, size_t max_size ) {
    if( init_size == 0 || max_size < init_size ) {
        die( "Error: invalid size" );
    }

    if( (state->chbuf = calloc( 1, init_size )) == NULL ) {
        die( "Error: calloc chbuf failed in search_state_init" );
    }

    state->capacity = init_size;
    state->max_capacity = max_size;
    state->used = 0;
    state->search_offset = 0;
    state->needle_offset = 0;
}

void search_state_destroy( search_state_t* state ) {
    free( state->chbuf );
}

int search_state_grow_if_full( search_state_t* state ) {
    size_t newsize;
    char* newmem;

    /* grow the buffer if it is full */
    if( state->used == state->capacity ) {

        /* double the size, but not beyond the max allowed */
        newsize = 2 * state->capacity;
        if( newsize > state->max_capacity ) {
            if( state->used == state->max_capacity ) {
                errno = ERANGE;
                return 2;
            }
            newsize = state->max_capacity;
        }

        newmem = realloc( state->chbuf, newsize );
        if( newmem == NULL ) {
            errno = ENOMEM;
            die( "Error: malloc failed in search_state_grow_if_full" );
            return -1;
        }

        state->capacity = newsize;
        state->chbuf    = newmem;
        return 1;
    }

    return 0;
}
