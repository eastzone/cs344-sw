/*
 * Filename: cli_cmd.c
 * Purpose: help test the parser
 */

#include <stdio.h>
#include <string.h>
#include "cli.h"
#include "cli_scanner.h"
#include "cli_parser.h"

#define READ_FROM_STDIN

#ifndef READ_FROM_STDIN
static void add_to_buffer( byte* buf, unsigned* len, unsigned maxlen, char* str ) {
    /* don't overrun the buffer */
    assert( *len + strlen(str) + 2 < maxlen );

    /* copy the string into the next available bytes */
    strcpy( (char*)buf + *len, str );
    *len += strlen( str );

    /* set the last two bytes to NUL */
    memset( buf + *len, 0, 2 );
}
#endif

int main() {
#ifndef READ_FROM_STDIN
    static byte buf[100];
    unsigned len, maxlen;
    YY_BUFFER_STATE yy_state;
    int i;

    len = 0;
    maxlen = 100;
    i = 0;

    /* create a buffer to scan */
    add_to_buffer( buf, &len, maxlen, "show vns user\n" );
    add_to_buffer( buf, &len, maxlen, "help\n" );

    /* tell lex to scan our buffer in-place */
    yy_state = yy_scan_buffer( (char*)buf, len + 2 ); /* +2 ==> double-terminating NULs */
    assert( yy_state );
#else
    static int verbose;
    verbose = 1;
    cli_focus_set( -1, &verbose );
#endif

    cli_parser_init();
    while( !cli_is_time_to_shutdown() && cli_focus_is_alive() ) {
        cli_scanner_init(); /* reset col info for each parse */
        yyparse();

#ifndef READ_FROM_STDIN
        if( i == 1 ) {
            yy_delete_buffer( yy_state );

            len = 0;
            add_to_buffer( buf, &len, maxlen, "shutdown\n" );
            yy_state = yy_scan_buffer( (char*)buf, len + 2 );
            assert( yy_state );
        }
        else if( i == 2 )
            break;

        i += 1;
#endif
    }

#ifndef READ_FROM_STDIN
    if( yy_state )
        yy_delete_buffer( yy_state );
#endif

    return 0;
}
