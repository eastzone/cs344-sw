/*
 * Filename: cli_test.c
 * Purpose: help test the parser
 */

#include <stdio.h>
#include <string.h>
#include "cli.h"
#include "cli_scanner.h"
#include "cli_parser.h"

int main() {
    static int verbose;
    verbose = 1;
    cli_focus_set( -1, &verbose );

    cli_parser_init();
    while( cli_focus_is_alive() ) {
        cli_scanner_init(); /* reset col info for each parse */
        yyparse();
    }

    return 0;
}
