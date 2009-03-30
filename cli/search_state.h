/*
 * Filename: search_state.h
 * Author: David Underhill
 * Purpose:  Define structure and functions for searching
 */

#ifndef SEARCH_STATE_H
#define SEARCH_STATE_H

#include <stdlib.h>

/** encapsulates the state for a search in an incoming byte stream */
typedef struct search_state_t {
    char*  chbuf;          /* buffer to hold the request line and headers    */
    size_t capacity;       /* total bytes allocated to the buffer            */
    size_t max_capacity;   /* max number of bytes capacity may grow to       */
    size_t used;           /* number of bytes used (read into) the buffer    */
    size_t search_offset;  /* offset into buf where we'll start searching    */

    size_t needle_offset;  /* result: offset where the last needle was found
                              (only valid if search returns successfully)    */
} search_state_t;

/**
 * Initialize a search state.
 *
 * @param init_size  initial size of the buffer in bytes (greater than 0)
 * @param max_size   max size of the buffer in bytes (greater than init_size)
 *
 * @return a new heap-allocated search state on success, or NULL otherwise.
 *         The returned object should be freed with search_state_destroy.
 */
void search_state_init( search_state_t* state, size_t init_size, size_t max_size );

/** Free the memory associated with a search state */
void search_state_destroy( search_state_t* state );

/**
 * Doubles the size of the buffer if it is full.
 *
 * @return 0 nothing was done; the buffer is not full
 *         1 buffer was grown
 *         2 buffer is full and as big as it may get
 *        -1 error: an attempt to grow the buffer failed
 */
int search_state_grow_if_full( search_state_t* state );

#endif /* SEARCH_STATE_H */
