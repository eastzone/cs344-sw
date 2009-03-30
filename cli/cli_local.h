/*
 * Filename: cli_local.h
 * Author: David Underhill
 * Purpose: provides support for accepting CLI client on the local interface
 */

#ifndef CLI_LOCAL
#define CLI_LOCAL

/* Initializes the thread which will listen for clients on the local IP stack. */
void cli_local_init( uint16_t port );

#endif /* CLI_LOCAL */
