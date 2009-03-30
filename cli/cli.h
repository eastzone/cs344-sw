/*
 * Filename: cli.h
 * Author: David Underhill
 * Purpose: Implement CLI commands.
 */

#ifndef CLI_H
#define CLI_H

#ifdef _LINUX_
#include <stdint.h> /* uintX_t */
#endif
#include "cli_help.h"

#define PROMPT "~> "
#define STRLEN_PROMPT 3

/* This nasty struct stores values until we know it is time to execute the
   command (very gross).  Better perhaps to have a class hierarchy with children
   storing the data they need and all sharing a virtual execute() method.  But
   alas, this is pure C, so we'll do this somewhat gross thing instead. */
typedef struct {
    void (*func_do0)(void);  /* use this method if data is NULL */
    void (*func_do1)(void*); /* use this method if data is !NULL */
    void* data;              /* data to call func_do1 with, if any */
} gross_object_t;

typedef struct {
    uint32_t ip;
    uint8_t mac[6];
} gross_arp_t;

typedef struct {
    const char* intf_name;
    uint32_t ip;
    uint32_t subnet_mask;
} gross_intf_t;

typedef struct {
    uint32_t dest;
    uint32_t gw;
    uint32_t mask;
    const char* intf_name;
} gross_route_t;

typedef struct {
    uint32_t ip;
} gross_ip_t;

typedef struct {
    uint32_t ip;
    unsigned count;
} gross_ip_int_t;

typedef struct {
    int on;
} gross_option_t;

/** Initiliazes the CLI global variables. */
void cli_init();

/** Returns true when 'shutdown' has been received */
int cli_is_time_to_shutdown();

/** whether the last set focus fd is still live */
int cli_focus_is_alive();

/** Sets the active outgoing fd and marks the current focus fd as live. */
void cli_focus_set( const int sfd, int* verbose );

/**
 * Wrapper for cli_send_help.  Tries to send the requested help information with
 * the file-scope fd.  If it fails, fd_alive is set to FALSE.  Does nothing if
 * fd_alive is already FALSE.
 */
void cli_send_help( cli_help_t help_type );

/**
 * Sends the num_args strings to the user.  If it fails, fd_alive is set to
 * FALSE.  Does nothing if fd_alive is already FALSE.
 */
void cli_send_parse_error( int num_args, ... );

/** Sends the CLI welcome string to the client. */
void cli_send_welcome();

/** Sends the command prompt string to the client. */
void cli_send_prompt();

void cli_show_all();

#ifndef _CPUMODE_
    void cli_send_no_hw_str();
#   define cli_show_hw       cli_send_no_hw_str
#   define cli_show_hw_about cli_send_no_hw_str
#   define cli_show_hw_arp   cli_send_no_hw_str
#   define cli_show_hw_intf  cli_send_no_hw_str
#   define cli_show_hw_route cli_send_no_hw_str
#   ifndef _MANUAL_MODE_
#       define _VNS_MODE_
#   endif
#else
    void cli_show_hw();
    void cli_show_hw_about();
    void cli_show_hw_arp();
    void cli_show_hw_intf();
    void cli_show_hw_route();
#endif

void cli_show_ip();
void cli_show_ip_arp();
void cli_show_ip_intf();
void cli_show_ip_route();

void cli_show_opt();
void cli_show_opt_verbose();

void cli_show_ospf();
void cli_show_ospf_neighbors();
void cli_show_ospf_topo();

#ifndef _VNS_MODE_
    void cli_send_no_vns_str();
#   define cli_show_vns        cli_send_no_vns_str
#   define cli_show_vns_lhost  cli_send_no_vns_str
#   define cli_show_vns_topo   cli_send_no_vns_str
#   define cli_show_vns_user   cli_send_no_vns_str
#   define cli_show_vns_vhost  cli_send_no_vns_str
#else
    void cli_show_vns();
    void cli_show_vns_lhost();
    void cli_show_vns_topo();
    void cli_show_vns_user();
    void cli_show_vns_vhost();
#endif

void cli_manip_ip_arp_add( gross_arp_t* data );
void cli_manip_ip_arp_del( gross_arp_t* data );
void cli_manip_ip_arp_purge_all();
void cli_manip_ip_arp_purge_sta();
void cli_manip_ip_arp_purge_dyn();

void cli_manip_ip_intf_set( gross_intf_t* data );
void cli_manip_ip_intf_down( gross_intf_t* data );
void cli_manip_ip_intf_up( gross_intf_t* data );

void cli_manip_ip_ospf_down();
void cli_manip_ip_ospf_up();

void cli_manip_ip_route_add( gross_route_t* data );
void cli_manip_ip_route_del( gross_route_t* data );
void cli_manip_ip_route_purge_all();
void cli_manip_ip_route_purge_dyn();
void cli_manip_ip_route_purge_sta();

/* Display the current date and time. */
void cli_date();

/** Terminate this client's connection to the router CLI. */
void cli_exit();

/** Send a ping to dest. */
void cli_ping( gross_ip_t* data );
void cli_ping_flood( gross_ip_int_t* data );

/** Processes a shutdown command from a client. */
void cli_shutdown();

/** Traceroute to dest. */
void cli_traceroute( gross_ip_t* data );

/** set/unset/show the verbose mode */
void cli_opt_verbose( gross_option_t* data );

#endif /* CLI_H */
