/*
 * Filename: cli_scanner.h
 * Author: David Underhill
 * Purpose: (F)lex input file to generate a scanner for the CLI.
 */

#ifndef CLI_SCANNER_H
#define CLI_SCANNER_H

/** flex structure forward declaration */
struct yy_buffer_state;
typedef struct yy_buffer_state* YY_BUFFER_STATE;

/** text of the lexeme just scanned */
extern char* yytext;

/** Initialize the CLI scanner */
void cli_scanner_init();

/**
 * Get the text of the current line being read (up to the character(s)
 * processed so far).
 */
const char* cli_scanner_get_cur_line();

/** Get the start index of the previously scanned lexeme. */
unsigned cli_scanner_prev_lexeme_offset();

/** Gets the length of the previously scanned lexeme. */
unsigned cli_scanner_prev_lexeme_length();

/** Gets a string which would underline the previously scanned lexeme. */
char* scanner_get_str_to_underline_error( unsigned bump );

/** Removes the rest of the line from the lexer buffer. */
void get_rest_of_line();

/* read from the current buffer and return the code of the scanned token */
int yylex();

/** deletes a flex buffer */
void yy_delete_buffer( YY_BUFFER_STATE buffer );

/** set the buffer to be scanned for tokens */
void* yy_scan_buffer(char *base, unsigned size);

#endif /* CLI_SCANNER_H */
