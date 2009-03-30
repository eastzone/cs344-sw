/* Filename: helper.c */

#include <arpa/inet.h>        /* htonl */
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "helper.h"

#define ALLSPFRouters (htonl(0xE0000005)) /* 224.0.0.5 */

void die( const char* format, ... ) {
    va_list args;
    va_start( args, format );

    vfprintf( stderr, format, args );
    fprintf( stderr, "\n" );

    va_end( args );
    assert( 0 );
    exit( 1 );
}

void true_or_die( int is_ok, const char* format, ... ) {
    if( !is_ok ) {
        va_list args;
        va_start( args, format );

        vfprintf( stderr, format, args );
        fprintf( stderr, "\n" );

        va_end( args );
        assert( 0 );
        exit( 1 );
    }
}

void ip_to_string( char* buf, uint32_t ip ) {
    uint8_t* bytes;

    if( ip == ALLSPFRouters ) {
        snprintf( buf, STRLEN_IP, "ROUTER-OSPF-IP" );
        return;
    }

    bytes = (uint8_t*)&ip;

    snprintf( buf, STRLEN_IP, "%u.%u.%u.%u",
              bytes[0],
              bytes[1],
              bytes[2],
              bytes[3] );
}

void mac_to_string( char* buf, uint8_t* mac ) {
    snprintf( buf, STRLEN_MAC, "%X%X:%X%X:%X%X:%X%X:%X%X:%X%X",
              mac[0] >> 4, 0x0F & mac[0],
              mac[1] >> 4, 0x0F & mac[1],
              mac[2] >> 4, 0x0F & mac[2],
              mac[3] >> 4, 0x0F & mac[3],
              mac[4] >> 4, 0x0F & mac[4],
              mac[5] >> 4, 0x0F & mac[5] );
}

/* ones(x) -- counts the number of bits in x with the value 1.
 * Based on Hacker's Delight (2003) by Henry S. Warren, Jr.
 */
uint32_t ones( register uint32_t x ) {
  x -= ((x >> 1) & 0x55555555);
  x = (((x >> 2) & 0x33333333) + (x & 0x33333333));
  x = (((x >> 4) + x) & 0x0f0f0f0f);
  x += (x >> 8);
  x += (x >> 16);

  return( x & 0x0000003f );
}

void subnet_to_string( char* buf, uint32_t subnet, uint32_t mask ) {
    unsigned num_ones;
    uint8_t* bytes;

    num_ones = ones(mask);
    bytes = (uint8_t*)&subnet;

    if( num_ones == 0 && subnet == 0 )
        snprintf( buf, STRLEN_SUBNET, "<catch-all>" );
    else if( num_ones <= 8 )
        snprintf( buf, STRLEN_SUBNET, "%u/%u",
                  bytes[0], num_ones );
    else if( num_ones <= 16 )
        snprintf( buf, STRLEN_SUBNET, "%u.%u/%u",
                  bytes[0], bytes[1], num_ones );
    else if( num_ones <= 24 )
        snprintf( buf, STRLEN_SUBNET, "%u.%u.%u/%u",
                  bytes[0], bytes[1], bytes[2], num_ones );
    else
        snprintf( buf, STRLEN_SUBNET, "%u.%u.%u.%u/%u",
                  bytes[0], bytes[1], bytes[2], bytes[3], num_ones );
}

void time_to_string( char* buf, unsigned sec ) {
    static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

    /* lock protects the static buffer which ctime populats */
    time_t t = sec;
    pthread_mutex_lock( &lock );
    strncpy( buf, ctime(&t), STRLEN_TIME );
    pthread_mutex_unlock( &lock );
}

const char* quick_ip_to_string( uint32_t ip ) {
    static char str_ip[STRLEN_IP];
    ip_to_string( str_ip, ip );
    return str_ip;
}

const char* quick_mac_to_string( uint8_t* mac ) {
    static char str_mac[STRLEN_MAC];
    mac_to_string( str_mac, mac );
    return str_mac;
}

void* malloc_or_die( unsigned size ) {
    void* ret;
    ret = malloc( size );
    true_or_die( ret!=NULL, "Error: malloc failed" );
    return ret;
}
