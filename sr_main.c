/*-----------------------------------------------------------------------------
 * file:  sr_main.c
 * Date:  Mon Feb 02 21:48:59 PST 2004
 *        (adapted from sr_cli_main.c Wed Nov 26 03:02:17 PST 2003)
 * Author:  Martin Casado <casado@stanford.edu>
 *
 * Description:
 *
 * Main function for sr_router.  Here we basically handle the command
 * line options and start the 'lower level' routing functionality which
 * involves connecting to the VNS server and negotiating the topology
 * and host information.
 *
 * The router functionality should be extended to support:
 *
 *   - ARP
 *   - ICMP
 *   - IP
 *   - TCP
 *   - Routing
 *   - OSPF
 *
 * Once the router is fully implemented it should support 'user level' network
 * code using standard BSD sockets.  To demonstrate this, main(..) must be
 * extended to support a fully interactive CLI from which you can telnet into
 * your router, configure the routing table and the interfaces, print
 * statistics and any other advanced functionality you choose to support.
 *
 *---------------------------------------------------------------------------*/

#include <stdio.h>
#include <unistd.h>
#include "cli/cli_main.h"
#include "sr_base.h"

/** run the command-line interface on CLI_PORT */
#define CLI_PORT 2300

int main(int argc, char** argv)
{
    /* --
     *  start the low-level network subsystem
     *
     *  Note: sr recognizes the following command
     *        line args (in getopt format)
     *        "hs:v:p:c:t:r:l:" and doesn't
     *        clean argv
     *                                            -- */

    sr_init_low_level_subystem(argc, argv);

    /* start the command-line interface (blocks until the router terminates) */
    if( cli_main( CLI_PORT ) == CLI_ERROR )
        fprintf( stderr, "Error: unable to setup the command-line interface server\n" );

    return 0;
} /* -- main -- */
