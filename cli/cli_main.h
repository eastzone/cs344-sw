/*
 * Filename: cli_main.h
 * Author: David Underhill
 * Purpose: defines the external entry point into the CLI
 */

#ifndef CLI_MAIN_H
#define CLI_MAIN_H

#ifdef _LINUX_
#include <stdint.h> /* uintX_t */
#endif
#include "search_state.h"

#define CLI_SHUTDOWN 0
#define CLI_ERROR    1

/** initial size of the read buffer for a client */
#define CLI_INIT_BUF 256

/** maximum size of the read buffer for a client */
#define CLI_MAX_BUF 2048

/** Encapsulates the data associated with a client. */
typedef struct cli_client_t {
    int fd;                    /* socket file descriptor to this client */
    search_state_t state;      /* state of the search for '\n' chars    */
    int verbose;               /* print empty HW entries if verbose true */
} cli_client_t;

/**
 * Starts the command-line interface server.  Returns when the server terminates
 * or encounters an unrecoverable error.
 *
 * @param port  the port to listen on
 *
 * @return CLI_SHUTDOWN on normal termination, CLI_ERROR on unrecoverable error
 */
int cli_main( uint16_t port );

#endif /* CLI_MAIN_H */
