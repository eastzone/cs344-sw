/* Filename: helper.h
 * Author: David Underhill
 * Purpose:  define some helper functions
 */

#ifndef HELPER_H
#define HELPER_H

#include <assert.h>
#include <stdarg.h>
#ifdef _LINUX_
#include <stdint.h> /* uintX_t */
#endif

/* 4 octets of 3 chars each, 3 periods, 1 nul => 16 chars */
#define STRLEN_IP  16

/* 12 hex chars, 5 colons, 1 nul => 18 bytes */
#define STRLEN_MAC 18

/* add '/' and two numbers at most to a typical IP */
#define STRLEN_SUBNET (STRLEN_IP+3)

/** length of a time string */
#define STRLEN_TIME 26

/**
 * Terminates the process.
 *
 * If message is not NULL, then an error message will be sent to stderr. The
 * The formatted message will be printed to stderr as if fprintf( stderr,
 * format, .. ) was called except a newline character will be appended.
 */
void die( const char* format, ... );

/**
 * Converts ip into a string and stores it in buf.  buf must be at least
 * STRLEN_IP bytes long.
 */
void ip_to_string( char* buf, uint32_t ip );

/**
 * Converts mac into a string and stores it in buf.  buf must be at least
 * STRLEN_MAC bytes long.
 */
void mac_to_string( char* buf, uint8_t* mac );

/**
 * Converts a subnet into a string and stores it in buf.  buf must be at least
 * STRLEN_SUBNET bytes long.
 */
void subnet_to_string( char* buf, uint32_t subnet, uint32_t mask );

/**
 * Converts a unix timestamp to a string.
 */
void time_to_string( char* buf, unsigned sec );

/**
 * This is a non-reentrant ip to string conversion.  The returned buffer is
 * reused each call.
 */
const char* quick_ip_to_string( uint32_t ip );

/**
 * This is a non-reentrant MAC to string conversion.  The returned buffer is
 * reused each call.
 */
const char* quick_mac_to_string( uint8_t* mac );

/**
 * Terminates the process if the argument is not true.
 *
 * If message is not NULL, then an error message will be sent to stderr. The
 * The formatted message will be printed to stderr as if fprintf( stderr,
 * format, .. ) was called except a newline character will be appended.
 */
void true_or_die( int is_ok, const char* format, ... );

void* malloc_or_die( unsigned size );

#endif /* HELPER_H */
