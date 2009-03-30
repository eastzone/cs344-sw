/*
 * Filename: cli_help.h
 * Author: David Underhill
 * Purpose: Provide help strings for the CLI.
 */

#ifndef CLI_HELP_H
#define CLI_HELP_H

/** enumerates the different topics on which help is available */
typedef enum cli_help_t {
    HELP_ALL,

    HELP_SHOW,
      HELP_SHOW_HW,
       HELP_SHOW_HW_ABOUT,
       HELP_SHOW_HW_ARP,
       HELP_SHOW_HW_INTF,
       HELP_SHOW_HW_ROUTE,
      HELP_SHOW_IP,
       HELP_SHOW_IP_ARP,
       HELP_SHOW_IP_INTF,
       HELP_SHOW_IP_ROUTE,
      HELP_SHOW_OPT,
       HELP_SHOW_OPT_VERBOSE,
      HELP_SHOW_OSPF,
       HELP_SHOW_OSPF_NEIGHBORS,
       HELP_SHOW_OSPF_TOPOLOGY,
      HELP_SHOW_VNS,
        HELP_SHOW_VNS_LHOST,
        HELP_SHOW_VNS_TOPOLOGY,
        HELP_SHOW_VNS_USER,
        HELP_SHOW_VNS_VHOST,

    HELP_MANIP,
      HELP_MANIP_IP,
       HELP_MANIP_IP_ARP,
         HELP_MANIP_IP_ARP_ADD,
         HELP_MANIP_IP_ARP_DEL,
         HELP_MANIP_IP_ARP_PURGE_ALL,
         HELP_MANIP_IP_ARP_PURGE_DYN,
         HELP_MANIP_IP_ARP_PURGE_STA,
       HELP_MANIP_IP_INTF,
         HELP_MANIP_IP_INTF_SET,
         HELP_MANIP_IP_INTF_DOWN,
         HELP_MANIP_IP_INTF_UP,
       HELP_MANIP_IP_OSPF,
         HELP_MANIP_IP_OSPF_DOWN,
         HELP_MANIP_IP_OSPF_UP,
       HELP_MANIP_IP_ROUTE,
         HELP_MANIP_IP_ROUTE_ADD,
         HELP_MANIP_IP_ROUTE_DEL,
         HELP_MANIP_IP_ROUTE_PURGE_ALL,
         HELP_MANIP_IP_ROUTE_PURGE_DYN,
         HELP_MANIP_IP_ROUTE_PURGE_STA,

    HELP_ACTION,
      HELP_ACTION_DATE,
      HELP_ACTION_EXIT,
      HELP_ACTION_HELP,
      HELP_ACTION_PING,
      HELP_ACTION_SHUTDOWN,
      HELP_ACTION_TRACE,

    HELP_OPT,
      HELP_OPT_VERBOSE
} cli_help_t;

/**
 * Sends the help string for the specified help_type via the socket file
 * descriptor fd.
 *
 * @return 0 on failure to send, otherwise success
 */
int cli_send_help_to( int fd, cli_help_t help_type );

#endif /* CLI_HELP_H */
