/* Filename: cli_main.c */

#include <errno.h>
#include "lwip/sys.h"
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include "cli.h"
#include "cli_local.h"
#include "cli_main.h"
#include "cli_network.h"
#include "cli_scanner.h"
#include "cli_parser.h"
#include "socket_helper.h"
#include "helper.h"

#define CLI_ERR_MSG "Warning: terminating client"

/* forward decl */
void real_close( int fd );

#ifdef _STANDALONE_CLI_
#   define THREAD_RETURN_NIL return NULL
#   define THREAD_RETURN_TYPE void*
#   define make_thread(func,arg) { \
        pthread_t tid;                                      \
        true_or_die( pthread_create(&tid,NULL,func,arg)==0, \
                     "pthread_create failed" );             \
    }
#else
#   define THREAD_RETURN_NIL return
#   define THREAD_RETURN_TYPE void
#   define make_thread(func,arg) sys_thread_new(func,arg);
#endif

static pthread_mutex_t parser_lock;

/** Cleans up client_to_free and returns the next client */
void cli_client_cleanup( cli_client_t* client_to_free ) {
    /* initial cleanup */
    if(client_to_free->fd < 0)
        real_close(-client_to_free->fd);
    else
        close(client_to_free->fd);

    search_state_destroy( &client_to_free->state );
}

/**
 * Copies the received data and (if appropriate) parses it and sends back a
 * response to the client.  This method will not block as long as client->fd has
 * data waiting to be read.  Otherwise, it will block until one read can be
 * completed.
 *
 * @return non-zero if client is still alive
 */
int cli_client_handle_request( cli_client_t* client ) {
    int ret;
    int ret_grow;

    /* search for \n and block until it is found (or socket is closed) */
    ret = read_search( client->fd, "\n", &client->state, 1 );

    /* add terminating NUL so we can use strchr on the valid part of the buffer */
    ret_grow = search_state_grow_if_full( &client->state );
    if( ret_grow==2 || ret_grow==-1 )
        ret = -2;
    else
        client->state.chbuf[ client->state.used ] = '\0';

    /* handle the result of reading from the socket */
    switch( ret ) {
        case -3:
            /* data received but no newline yet */
            return 1;

        case -2:
            /* data received but ran out of buffer space */
            return 0;

        case -1:
            /* read error */
            return 0;

        case 1:
            true_or_die( strchr( client->state.chbuf, '\n' ) == NULL,
                         "Error: Unexpected newline found after EOF received" );
            /* EOF received */
            return 0;

        case 0:
            /* tell the parser to execute the client's command(s) */
            pthread_mutex_lock( &parser_lock );
            cli_focus_set( client->fd, &client->verbose );
            int b = cli_parser_handle_client( client );
            pthread_mutex_unlock( &parser_lock );
            return b;

        default:
            die( "Error: invalid return in cli_client_handle_request" );
            return 0; /* to make the compiler happy */
    }
}

/**
 * Detaches and then handles pclient exclusively.  If pclient sends a shutdown
 * message, then the thread shuts down the whole router.  If pclient quits, then
 * the thread terminates after cleaning up.
 */
THREAD_RETURN_TYPE cli_client_handler_main( void* pclient ) {
    cli_client_t* client;

    client = (cli_client_t*)pclient;
    client->verbose = 0;
    pthread_detach( pthread_self() );

    /* ignore the broken pipe signal */
    signal( SIGPIPE, SIG_IGN );

    /* first read always pulls crap data from socket for some reason, so just
       get it now */
    cli_client_handle_request( client );
    client->state.used = client->state.search_offset = client->state.needle_offset = 0;

    /* welcome the client */
    pthread_mutex_lock( &parser_lock );
    cli_focus_set( client->fd, &client->verbose );
    cli_send_welcome();
    cli_send_prompt();
    pthread_mutex_unlock( &parser_lock );

    while( 1 ) {
        int res = cli_client_handle_request( client );

        /* if the client shutdown the router, bail now */
        if( cli_is_time_to_shutdown() ) {
            exit( 1 );
        }

        /* cleanup the client's memory & terminate this thread if client quit */
        if( !res ) {
            cli_client_cleanup( client );
            free( client );
            THREAD_RETURN_NIL;
        }
    }
}

int cli_main( uint16_t port ) {
    struct sockaddr_in addr;
    struct sockaddr client_addr;
    int bindfd;
    int clientfd;
    cli_client_t* client;
#ifdef _STANDALONE_CLI_
    unsigned sock_len;
#else
    int sock_len;
#endif

    sock_len = sizeof(struct sockaddr);
    pthread_mutex_init( &parser_lock, NULL );

    cli_init();
    cli_parser_init();
    cli_scanner_init();

    /* create a socket to listen for connections on */
    bindfd = socket( AF_INET, SOCK_STREAM, 0 );
    if( bindfd == -1 ) {
        fprintf( stderr, "Error: unable to create a IPv4 TCP socket" );
        return CLI_ERROR;
    }

    /* bind to the requested port */
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = 0;
    memset(&(addr.sin_zero), 0, sizeof(addr.sin_zero));
    if( bind(bindfd, (struct sockaddr*)&addr, sizeof(struct sockaddr)) ) {
        fprintf( stderr, "Error: CLI unable to bind to port %u", port );
        return CLI_ERROR;
    }

    /* listen for clients */
    listen( bindfd, 10 );

#ifndef _STANDALONE_CLI_
    /* listen locally too */
    cli_local_init( port );
#endif

    while( !cli_is_time_to_shutdown() ) {
        /* wait for a new client */
        clientfd = accept(bindfd, &client_addr, &sock_len);
        if( clientfd == - 1 ) {
            if( errno == EINTR ) {
                /* Interrupt received, going back to accept() */
                continue;
            }

            /* some error */
            perror( "accept() failed" );
            continue;
        }
        /* accepted a new connection client */

        /* create a client record */
        client = malloc_or_die( sizeof(cli_client_t) );
        client->fd = clientfd;
        search_state_init( &client->state, CLI_INIT_BUF, CLI_MAX_BUF );

        /* spawn a new thread to handle the client */
        make_thread( cli_client_handler_main, client );
    }

    return CLI_SHUTDOWN;
}
