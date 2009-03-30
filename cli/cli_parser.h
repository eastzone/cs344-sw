/*
 * Filename: cli_parser.h
 * Author: David Underhill
 * Purpose: Bison/Ycc input file to generate the parser for the CLI.
 */

#ifndef CLI_PARSER_H
#define CLI_PARSER_H

#include "cli_main.h"

#define MAX_STR_LEN 32

/** Initialize the parser */
void cli_parser_init();

/**
 * Handle a command which has arrived at client.  This implies that a newline
 * characters must have been received in the client's state object.  Responses
 * will be sent to the socket file descriptor client->fd.
 */
int cli_parser_handle_client( cli_client_t* client );

/** try to parse a stream */
int yyparse();

/**
 * Sets the Manual Error Handler flag to TRUE and the error desc text to the
 * specified text.
 */
void parse_error( const char* desc );

#endif /* CLI_PARSER_H */
