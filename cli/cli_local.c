/* Filename: cli_local.c */

#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include "cli_local.h"
#include "cli_main.h"
#include "helper.h"

/* forward decl */
void cli_client_handler_main( void* pclient );
int cli_is_time_to_shutdown();
void sys_thread_new(void (* thread)(void *arg), void *arg);

void cli_local_main( void* pport ) {
    uint16_t port;
    struct sockaddr_in addr;
    struct sockaddr client_addr;
    unsigned sock_len = sizeof(struct sockaddr);
    int bindfd;
    int clientfd;
    cli_client_t* client;

    port = *(uint16_t*)pport;
    free( pport );

    /* create a socket to listen for connections on */
    bindfd = socket( AF_INET, SOCK_STREAM, 0 );
    if( bindfd == -1 ) {
        die( "Error: unable to create a real IPv4 TCP socket" );
    }

    /* allow reuse of the port */
    int reuse = 1;
    if( setsockopt( bindfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse) ) )
        die( "Error: SO_REUSEADDR failed" );

    /* bind to the requested port */
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = 0;
    memset(&(addr.sin_zero), 0, sizeof(addr.sin_zero));
    if( bind(bindfd, (struct sockaddr*)&addr, sizeof(struct sockaddr)) ) {
        die( "Error: unable to bind to local port %u", port );
    }

    /* listen for clients */
    listen( bindfd, 10 );

    while( !cli_is_time_to_shutdown() ) {
        /* wait for a new client */
        clientfd = accept(bindfd, &client_addr, &sock_len);
        if( clientfd == - 1 ) {
            if( errno == EINTR ) {
                /* Interrupt received, going back to real accept() */
                continue;
            }

            /* some error */
            fprintf( stderr, "Warrning: real accept() returned an error code (%d)", errno );
            continue;
        }

        /* create a client record */
        client = malloc_or_die( sizeof(cli_client_t) );
        client->fd = -clientfd; /* negative means use the real sockets */
        search_state_init( &client->state, CLI_INIT_BUF, CLI_MAX_BUF );

        /* spawn a new thread to handle the client */
        sys_thread_new( cli_client_handler_main, client );
    }

    exit( 0 );
}

void cli_local_init( uint16_t port ) {
    uint16_t* pport = malloc_or_die( sizeof(*pport) );
    *pport = port;
    sys_thread_new( cli_local_main, pport );
}
