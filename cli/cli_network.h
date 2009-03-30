/*
 * Filename: cli_network.h
 * Author: David Underhill
 * Purpose: setup network includes
 */

#ifdef _STANDALONE_CLI_
#   include <netinet/in.h>
#   include <sys/types.h>
#   include <sys/socket.h>
#   include <unistd.h>
#else
#   define LWIP_COMPAT_SOCKETS
#   include "lwip/sockets.h"
#   include "lwip/sys.h"
#endif
