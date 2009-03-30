/* Filename: cli_help.c */

#include <stdarg.h>
#include "cli_help.h"
#include "cli_network.h"
#include "helper.h"
#include "socket_helper.h"

#ifdef _STANDALONE_CLI_
#   define APP_NAME "cli"
#else
#   define APP_NAME "sr"
#endif

static int cli_send_multi_help( int fd, const char* prefix, int num_args, ... ) {
    cli_help_t help_type;
    int ret;
    va_list args;
    va_start( args, num_args );

    ret = (0 == writenstr( fd, prefix ));
    while( ret && num_args-- > 0 ) {
        help_type = va_arg(args, cli_help_t);
        ret = cli_send_help_to( fd, help_type );
    }

    va_end( args );
    return ret;
}

int cli_send_help_to( int fd, cli_help_t help_type ) {
    switch( help_type ) {
        case HELP_ALL:
            return cli_send_multi_help( fd, "",
4,
HELP_ACTION_HELP,
HELP_SHOW,
HELP_MANIP,
HELP_ACTION );

        case HELP_SHOW:
            return cli_send_multi_help( fd, "\
show [hw | ip | opt | ospf | vns]: display information about the router's current state\n",
5,
HELP_SHOW_HW,
HELP_SHOW_IP,
HELP_SHOW_OPT,
HELP_SHOW_OSPF,
HELP_SHOW_VNS );

          case HELP_SHOW_HW:
              return cli_send_multi_help( fd, "\
show <hw | hardware | cpu> [arp, interface (intf), route (rt),]: display \n\
information about the router's HW state\n",
4,
HELP_SHOW_HW_ABOUT,
HELP_SHOW_HW_ARP,
HELP_SHOW_HW_INTF,
HELP_SHOW_HW_ROUTE );

           case HELP_SHOW_HW_ABOUT:
                return 0==writenstr( fd, "\
show hw about: displays information about the NetFPGA card\n" );

           case HELP_SHOW_HW_ARP:
                return 0==writenstr( fd, "\
show hw arp: displays the HW ARP cache sorted by IP within Active, Pending, and Static types\n" );

           case HELP_SHOW_HW_INTF:
                return 0==writenstr( fd, "\
show hw interface: displays the HW's information about its interfaces\n" );

           case HELP_SHOW_HW_ROUTE:
                return 0==writenstr( fd, "\
show hw route: displays the HW routing table\n" );

          case HELP_SHOW_IP:
              return cli_send_multi_help( fd, "\
show ip [arp, interface (intf), route (rt),]: display information about the\n\
  router's IP state\n",
3,
HELP_SHOW_IP_ARP,
HELP_SHOW_IP_INTF,
HELP_SHOW_IP_ROUTE );

           case HELP_SHOW_IP_ARP:
                return 0==writenstr( fd, "\
show ip arp: displays the ARP cache sorted by IP within Active, Pending, and Static types\n" );

           case HELP_SHOW_IP_INTF:
                return 0==writenstr( fd, "\
show ip interface: displays the router's interfaces\n" );

           case HELP_SHOW_IP_ROUTE:
                return 0==writenstr( fd, "\
show ip route: displays the routing table sorted by longest prefixes first\n" );

          case HELP_SHOW_OPT:
              return cli_send_multi_help( fd, "\
show opt [verbose]: display information about options' current values\n",
1,
HELP_SHOW_OPT_VERBOSE );

            case HELP_SHOW_OPT_VERBOSE:
                return 0==writenstr( fd, "\
show opt verbose: displays whether this session has verbose mode enabled\n" );

          case HELP_SHOW_OSPF:
              return cli_send_multi_help( fd, "\
show ospf [neigh | topo]: display information about OSPF state\n",
2,
HELP_SHOW_OSPF_NEIGHBORS,
HELP_SHOW_OSPF_TOPOLOGY );

            case HELP_SHOW_OSPF_NEIGHBORS:
                return 0==writenstr( fd, "\
show ospf neigh: displays information about each interface's neighbors\n" );

            case HELP_SHOW_OSPF_TOPOLOGY:
                return 0==writenstr( fd, "\
show ospf topo: displays the current dynamically computed network topology\n" );

          case HELP_SHOW_VNS:
              return cli_send_multi_help( fd, "\
show vns [lhost, topo[logy], user, vhost]: display information about \n\
  the connection to VNS\n",
4,
HELP_SHOW_VNS_LHOST,
HELP_SHOW_VNS_TOPOLOGY,
HELP_SHOW_VNS_USER,
HELP_SHOW_VNS_VHOST );

            case HELP_SHOW_VNS_LHOST:
                return 0==writenstr( fd, "\
show vns lhost: displays the VNS local host address\n" );

            case HELP_SHOW_VNS_TOPOLOGY:
                return 0==writenstr( fd, "\
show vns topology: displays the VNS topology ID number\n" );

            case HELP_SHOW_VNS_USER:
                return 0==writenstr( fd, "\
show vns user: displays the VNS user name\n" );

            case HELP_SHOW_VNS_VHOST:
                return 0==writenstr( fd, "\
show vns vhost: displays the VNS virtual host address\n" );


        case HELP_MANIP:
            /* fall-through to HELP_MANIP_IP */

          case HELP_MANIP_IP:
            return cli_send_multi_help( fd, "\
ip [arp | interface (intf) | ospf | route (rt)] <command>: update the router's state\n",
4,
HELP_MANIP_IP_ARP,
HELP_MANIP_IP_INTF,
HELP_MANIP_IP_OSPF,
HELP_MANIP_IP_ROUTE );

           case HELP_MANIP_IP_ARP:
               return cli_send_multi_help( fd, "\
ip arp {add | del | purge} [<options>]: modify the ARP cache\n",
3,
HELP_MANIP_IP_ARP_ADD,
HELP_MANIP_IP_ARP_DEL,
HELP_MANIP_IP_ARP_PURGE_ALL );

             case HELP_MANIP_IP_ARP_ADD:
                 return 0==writenstr( fd, "\
ip arp add <IP> <MAC>: adds a static entry to the ARP cache which maps IP to MAC\n" );

             case HELP_MANIP_IP_ARP_DEL:
                 return 0==writenstr( fd, "\
ip arp del <IP>: removes the static entry in the ARP cache which maps IP to some\n\
  MAC (if such an entry exists)\n" );

             case HELP_MANIP_IP_ARP_PURGE_ALL:
                 return cli_send_multi_help( fd, "\
ip arp purge: removes all valid entries from the ARP cache\n",
2,
HELP_MANIP_IP_ARP_PURGE_DYN,
HELP_MANIP_IP_ARP_PURGE_STA );

             case HELP_MANIP_IP_ARP_PURGE_DYN:
                 return 0==writenstr( fd, "\
ip arp purge dynamic: removes all non-static, valid entries from the ARP cache\n" );

             case HELP_MANIP_IP_ARP_PURGE_STA:
                 return 0==writenstr( fd, "\
ip arp purge static: removes all static entries from the ARP cache\n" );

           case HELP_MANIP_IP_INTF:
               return cli_send_multi_help( fd, "\
ip interface <name> <down | up | <IP> <mask>>: modify the interface named <name>\n",
3,
HELP_MANIP_IP_INTF_SET,
HELP_MANIP_IP_INTF_DOWN,
HELP_MANIP_IP_INTF_UP );

             case HELP_MANIP_IP_INTF_SET:
                 return 0==writenstr( fd, "\
ip interface <name> <IP> <mask IP>: set the specified interface's IP address \n\
  and subnet mask\n" );

             case HELP_MANIP_IP_INTF_DOWN:
                 return 0==writenstr( fd, "\
ip interface <name> down: disable the specified interface\n" );

             case HELP_MANIP_IP_INTF_UP:
                 return 0==writenstr( fd, "\
ip interface <name> up: enable the specified interface\n" );

           case HELP_MANIP_IP_OSPF:
               return cli_send_multi_help( fd, "\
ip ospf <disable | enable>\n",
2,
HELP_MANIP_IP_OSPF_DOWN,
HELP_MANIP_IP_OSPF_UP );

             case HELP_MANIP_IP_OSPF_DOWN:
                 return 0==writenstr( fd, "\
ip ospf <disable|down>: disable the dynamic OSPF routing protocol\n" );

             case HELP_MANIP_IP_OSPF_UP:
                 return 0==writenstr( fd, "\
ip ospf <enable|up>: enable the dynamic OSPF routing protocol\n" );

           case HELP_MANIP_IP_ROUTE:
               return cli_send_multi_help( fd, "\
ip route {add | del | purge} [<options>]: modify the routing table\n",
3,
HELP_MANIP_IP_ROUTE_ADD,
HELP_MANIP_IP_ROUTE_DEL,
HELP_MANIP_IP_ROUTE_PURGE_ALL );

             case HELP_MANIP_IP_ROUTE_ADD:
                 return 0==writenstr( fd, "\
ip route add <dest IP> <gateway IP> <mask IP> <interface name>: add a static route to\n\
  <dest IP> with subnet mask <mask IP> via the next hop <gateway IP> out of\n\
  interface <interface name>\n" );

             case HELP_MANIP_IP_ROUTE_DEL:
                 return 0==writenstr( fd, "\
ip route del <dest IP> <mask IP>: remove a static route to <dest IP> with subnet mask <mask IP>\n" );

             case HELP_MANIP_IP_ROUTE_PURGE_ALL:
                 return cli_send_multi_help( fd, "\
ip route purge: remove all routes in the routing table\n",
2,
HELP_MANIP_IP_ROUTE_PURGE_DYN,
HELP_MANIP_IP_ROUTE_PURGE_STA );

             case HELP_MANIP_IP_ROUTE_PURGE_DYN:
                 return 0==writenstr( fd, "\
ip route purge <dyn|dynamic>: remove all dynamic routes in the routing table\n" );

             case HELP_MANIP_IP_ROUTE_PURGE_STA:
                 return 0==writenstr( fd, "\
ip route purge <sta|static>: remove all static routes in the routing table\n" );


        case HELP_ACTION:
            return cli_send_multi_help( fd, "",
5, /* intentionally omitting HELP_ACTION_HELP */
HELP_ACTION_DATE,
HELP_ACTION_EXIT,
HELP_ACTION_PING,
HELP_ACTION_SHUTDOWN,
HELP_ACTION_TRACE );

          case HELP_ACTION_DATE:
              return 0==writenstr( fd, "\
date: display the current date and time\n" );

          case HELP_ACTION_EXIT:
              return 0==writenstr( fd, "\
exit: terminate this interactive session\n" );

          case HELP_ACTION_HELP:
              return 0==writenstrs( fd, 2, APP_NAME, " \
usage: <command>\n\
SR Command Line Client, version 1.0\n\
Type '<command> ?' for help with a specific command.\n\
\n\
Available Commands:\n\
    date\n\
    exit\n\
    help\n\
    ip\n\
    set\n\
    show\n\
    shutdown\n\
    sping\n\
    strace\n\
    unset\n\
    ?\n" );

          case HELP_ACTION_PING:
              return 0==writenstr( fd, "\
sping <dest IP>: send an ICMP Echo Request to <dest IP>\n" );

          case HELP_ACTION_SHUTDOWN:
              return 0==writenstr( fd, "\
shutdown: power off the router\n" );

          case HELP_ACTION_TRACE:
              return 0==writenstr( fd, "\
strace <dest IP>: traceourte to <dest IP>\n" );


        case HELP_OPT:
            return cli_send_multi_help( fd, "\
<set|unset>: enables or disables options\n",
1,
HELP_OPT_VERBOSE );

          case HELP_OPT_VERBOSE:
              return 0==writenstr( fd, "\
verbose: if true, empty rows in the hardware tables are printed too\n" );


    default:
        return 0==writenstr( fd, "No help is available for that topic." );
    }
}
