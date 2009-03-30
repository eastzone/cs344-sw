/* Filename: socket_wrapper.c */

#include <errno.h>            /* errno */
#include <stdio.h>            /* vsnprintf */
#include <string.h>           /* strlen */
#include "helper.h"           /* true_or_die */
#include "socket_helper.h"

ssize_t read_once( int fd, void* buf, unsigned n ) {
    ssize_t nread;
    char* chbuf;

    true_or_die( fd >= 0, "Bad fd %d in read_once", fd );

    /* loop until 1 or more bytes have been read from fd, or an EOF or error occurs */
    chbuf = (char*)buf;
    while( 1 ) {
        if( (nread = read(fd, chbuf, n)) < 0 ) {
            if( errno == EINTR )
                continue;  /* interrupt: no bytes read: try again */
            else
                return -1; /* problem with read: error! */
        }
        else if( nread == 0 )
            return 0;      /* EOF */
        else
            return nread;
    }
}

ssize_t readn( int fd, void* buf, unsigned n ) {
    size_t nleft;
    ssize_t nread;
    char* chbuf;

    true_or_die( fd >= 0, "Bad fd %d in readn", fd );

    /* loop until n bytes have been read from fd, or an EOF or error occurs */
    chbuf = (char*)buf;
    nleft = n;
    while( nleft > 0 ) {
        if( (nread = read(fd, chbuf, nleft)) < 0 ) {
            if( errno == EINTR )
                continue;  /* interrupt: no bytes read: try again */
            else
                return -1; /* problem with read: error! */
        }
        else if( nread == 0 )
            break;         /* EOF */

        nleft -= nread;
        chbuf += nread;
    }

    return (n - nleft);
}

int read_search( int fd, const char* needle, search_state_t* state, int max_reads ) {
    int first_loop;
    int found_needle;
    int needle_len;
    ssize_t nread;
    char* offset;
    size_t amount;
    int j;

    first_loop = 1;
    found_needle = 0;
    needle_len = strlen( needle );
    assert( needle_len > 0 );
    nread = 0;

    do {
        /* check the buffer first in case needle is already there */
        if( !first_loop ) {
            if( search_state_grow_if_full( state ) == 2 )
                return -2; /* buffer full and cannot grow */

            /* try to read as much as we can */
            offset = state->chbuf + state->used;
            amount = state->capacity - state->used;
            if( (nread = read( fd, offset, amount )) < 0 ) {
                if( errno == EINTR )
                    continue;  /* interrupt: no bytes read: try again */
                else
                    return -1; /* problem with read: error! */
            }
            else if( nread == 0 )
                return 1;      /* EOF */
        }

        /* check for the needle in the newly read data */
        while( state->search_offset+needle_len <= state->used+nread ) {
            found_needle = 1;
            for( j=0; j<needle_len; j++ ) {
                if( state->chbuf[state->search_offset+j] != needle[j] ) {
                    found_needle = 0;
                    break; /* nope, must match all chars */
                }
            }
            if( found_needle ) {
                state->needle_offset = state->search_offset++;
                break;
            }
            state->search_offset++;
        }

        state->used += nread;
        first_loop = 0;

        if( max_reads && !found_needle ) {
            if( nread )
                max_reads -= 1;
            if( max_reads == 0 )
                return -3;
        }
    }
    while( !found_needle );

    return 0; /* success */
}

int writen( int fd, const void* buf, unsigned n ) {
    size_t nleft;
    ssize_t nwritten;
    char* chbuf;

    /* loop until n bytes have been written into fd, or an error occurs */
    chbuf = (char*)buf;
    nleft = n;
    while( nleft > 0 ) {
        if( (nwritten = write(fd, chbuf, nleft)) <= 0 ) {
            if( nwritten < 0 && errno == EINTR )
                continue;  /* interrupt: no bytes written: try again */
            else
                return -1; /* problem with write: error! */
        }

        nleft -= nwritten;
        chbuf += nwritten;
    }

    return 0; /* indicates success */
}

int writenf( int fd, const char* format, ... ) {
    int actual;
    char buf[WRITEN_MAX_LEN];

    va_list args;
    va_start( args, format );
    actual = vsnprintf( buf, WRITEN_MAX_LEN, format, args );
    va_end( args );

#ifdef _PRINT_TO_STDERR_NOT_SOCKET_
    fprintf( stderr, buf );
    return 0;
#endif

    if( actual < WRITEN_MAX_LEN )
        return writen( fd, &buf, actual ); /* doesn't incl. the trailing NUL */
    else {
        errno = ERANGE;
        return -1;
    }
}

int writenstr( int fd, const char* str ) {
    unsigned len;

#ifdef _PRINT_TO_STDERR_NOT_SOCKET_
    fprintf( stderr, str );
    return 0;
#endif

    len = strlen( str );
    return writen( fd, str, len );
}

int writenstrs( int fd, int num_args, ... ) {
    const char* str;
    int ret;
    va_list args;
    va_start( args, num_args );

    ret = 0;
    while( ret==0 && num_args-- > 0 ) {
        str = va_arg(args, const char*);
        ret = writenstr( fd, str );
    }

    va_end( args );
    return ret;
}
