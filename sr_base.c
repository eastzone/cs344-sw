/*-----------------------------------------------------------------------------
 * File: sr_base.c
 * Date: Spring 2002
 * Author: Martin Casado <casado@stanford.edu>
 *
 * Entry module to the low level networking subsystem of the router.
 *
 * Caveats:
 *
 *  - sys_thread_init(..) in sr_lwip_transport_startup(..) MUST be called from
 *    the main thread before other threads have been started.
 *
 *  - lwip requires that only one instance of the IP stack exist, therefore
 *    at the moment we don't support multiple instances of sr.  However
 *    support for this (given a cooperative tcp stack) would be simple,
 *    simple allow sr_init_low_level_subystem(..) to create new sr_instances
 *    each time they are called and return an identifier.  This identifier
 *    must be passed into sr_global_instance(..) to return the correct
 *    instance.
 *
 *  - lwip needs to keep track of all the threads so we use its
 *    sys_thread_new(), this is essentially a wrapper around
 *    pthread_create(..) that saves the thread's ID.  In the future, if
 *    we move away from lwip we should simply use pthread_create(..)
 *
 *
 *---------------------------------------------------------------------------*/

#ifdef _SOLARIS_
#define __EXTENSIONS__
#endif /* _SOLARIS_ */

/* get unistd.h to declare gethostname on linux */
#define __USE_BSD 1

#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>

#ifdef _LINUX_
#include <getopt.h>
#endif /* _LINUX_ */

#include "lwip/tcp.h"
#include "lwip/memp.h"
#include "lwip/transport_subsys.h"

#include "sr_vns.h"
#include "sr_base.h"
#include "sr_base_internal.h"

#ifdef _CPUMODE_
#include "sr_cpu_extension_nf2.h"
#endif

extern char* optarg;

static void usage(char* );
static int  sr_lwip_transport_startup(void);
static void sr_set_user(struct sr_instance* sr);
static void sr_init_instance(struct sr_instance* sr);
static void sr_low_level_network_subsystem(void *arg);
static void sr_destroy_instance(struct sr_instance* sr);

/**
 * Returns a logfile name which is in the format:
 *   log/topo<topo>-<host>-<count>.log
 *
 * The <count> is obtained from the file log/.count (the number in the file is
 * then incremented by one).
 *
 * @return logfile name (caller responsible for freeing it)
 */
static char* gen_logfile( unsigned topo, char* host ) {
    FILE* fp;
    char str_num[12];
    unsigned num;
    char* logfile;

    /* read the current count */
    fp = fopen( "log/.count", "r" );
    if( ! fp )
        die( "Error: could not open file log/.count for read-access" );

    fgets( str_num, 12, fp );
    sscanf( str_num, "%u", &num );
    fclose( fp );

    /* write the new count */
    fp = fopen( "log/.count", "w" );
    if( ! fp )
        die( "Error: could not open file log/.count for write-access" );

    fprintf( fp, "%u", num+1 );
    fclose( fp );

    /* create the logfile name */
    logfile = malloc( 512 * sizeof(char) );
    assert( logfile && "malloc failed" );
    snprintf( logfile, 512, "log/topo%u-%s-%u.log", topo, host, num );
    return logfile;
}

/*----------------------------------------------------------------------------
 * sr_init_low_level_subystem
 *
 * Entry method to the sr low level network subsystem. Responsible for
 * managing, connecting to the server, reserving the topology, reading
 * the hardware information and starting the packet recv(..) loop in a
 * seperate thread.
 *
 * Caveats :
 *  - this method can only be called once!
 *
 *---------------------------------------------------------------------------*/

int sr_init_low_level_subystem(int argc, char **argv)
{
    /* -- VNS default parameters -- */
    char *auth_key_file = "auth_key";
    char  *host   = "vrhost";
    char *user = 0;
    char  *rtable = "rtable";
    char  *itable = CPU_HW_FILENAME;
    char  *server = "171.67.71.19";
    uint16_t port =  3250;
    uint16_t topo =  0;
    int ospf = 0;

    char  *logfile = 0;
    int free_logfile = 0;

    /* -- singleton instance of router, passed to sr_get_global_instance
          to become globally accessible                                  -- */
    static struct sr_instance* sr = 0;

    int c;

    if ( sr )
    {
        fprintf(stderr,  "Warning: because of limitations in lwip, ");
        fprintf(stderr,  " sr supports 1 router instance per process. \n ");
        return 1;
    }

    sr = (struct sr_instance*) malloc(sizeof(struct sr_instance));

    while ((c = getopt(argc, argv, "hna:s:v:p:t:r:l:i:u:")) != EOF)
    {
        switch (c)
        {
            case 'h':
                usage(argv[0]);
                exit(0);
                break;
            case 'p':
                port = atoi((char *) optarg);
                break;
            case 't':
                topo = atoi((char *) optarg);
                break;
            case 'a':
                auth_key_file = optarg;
                break;
            case 'v':
                host = optarg;
                break;
            case 'u':
                user = optarg;
                break;
            case 'r':
                rtable = optarg;
                break;
            case 's':
                server = optarg;
                break;
            case 'l':
                logfile = optarg;
                if( strcmp( "auto", logfile )==0 ) {
                    logfile = gen_logfile( topo, host );
                    free_logfile = 1;
                }
                Debug("\nLOGGING to %s\n\n", logfile);
                break;
            case 'i':
                itable = optarg;
                break;
            case 'n':
                Debug("\nOSPF disabled!\n\n");
                ospf = 0;
                break;
        } /* switch */
    } /* -- while -- */

    Debug("\n");
#ifdef _CPUMODE_
    Debug(" < -- Starting sr in cpu mode -- >\n");
    Debug(" \n ");
#else
    Debug(" < -- Starting sr in router mode  -- >\n");
    Debug(" \n ");
#endif /* _CPUMODE_ */
    Debug("\n\n");

    /* -- required by lwip, must be called from the main thread -- */
    sys_thread_init();

    /* -- zero out sr instance and set default configurations -- */
    sr_init_instance(sr);
    sr->template[0] = '\0';
    strncpy(sr->auth_key_fn,auth_key_file,64);

    strncpy(sr->rtable, rtable, SR_NAMELEN);
#ifdef _CPUMODE_
    sr->topo_id = 0;
    strncpy(sr->vhost,  "cpu",    SR_NAMELEN);
    strncpy(sr->rtable, rtable, SR_NAMELEN);

    if ( sr_cpu_init_hardware(sr, CPU_HW_FILENAME) )
    { exit(1); }
    sr_integ_hw_setup(sr);
#else
    sr->topo_id = topo;
    strncpy(sr->vhost,  host,    SR_NAMELEN);
    strncpy( sr->server, server, SR_NAMELEN );
#endif

    if(! user )
    { sr_set_user(sr); }
    else
    { strncpy(sr->user, user,  SR_NAMELEN); }

    if ( gethostname(sr->lhost,  SR_NAMELEN) == -1 )
    {
        perror("gethostname(..)");
        return 1;
    }

    /* -- log all packets sent/received to logfile (if non-null) -- */
    sr_vns_init_log(sr, logfile);
    if( free_logfile ) free( logfile );

    sr_lwip_transport_startup();


#ifndef _CPUMODE_
    Debug("Client %s connecting to Server %s:%d\n",
            sr->user, server, port);
    Debug("Requesting topology %d\n", topo);

    /* -- connect to VNS and reserve host -- */
    if(sr_vns_connect_to_server(sr,port,server) == -1)
    { return 1; }

    /* read from server until the hardware is setup */
    while (! sr->hw_init )
    {
        if(sr_vns_read_from_server(sr) == -1 )
        {
            sr_destroy_instance(sr);
            die( "Error: could not get hardware information from the VNS server" );
        }
    }
    Debug("Hardware retrieved from VNS server\n");
#endif

    /* -- start low-level network thread -- */
    sys_thread_new(sr_low_level_network_subsystem, (void*)sr);

    return 0;
}/* -- main -- */

/*-----------------------------------------------------------------------------
 * Method: sr_set_subsystem(..)
 * Scope: Global
 *
 * Set the router core in sr_instance
 *
 *---------------------------------------------------------------------------*/

void sr_set_subsystem(struct sr_instance* sr, void* core)
{
    sr->interface_subsystem = core;
} /* -- sr_set_subsystem -- */


/*-----------------------------------------------------------------------------
 * Method: sr_get_subsystem(..)
 * Scope: Global
 *
 * Return the sr router core
 *
 *---------------------------------------------------------------------------*/

void* sr_get_subsystem(struct sr_instance* sr)
{
    return sr->interface_subsystem;
} /* -- sr_get_subsystem -- */

/*-----------------------------------------------------------------------------
 * Method: sr_get_global_instance(..)
 * Scope: Global
 *
 * Provide the world with access to sr_instance(..)
 *
 *---------------------------------------------------------------------------*/

struct sr_instance* sr_get_global_instance(struct sr_instance* sr)
{
    static struct sr_instance* sr_global_instance = 0;

    if ( sr )
    { sr_global_instance = sr; }

    return sr_global_instance;
}

static void sr_low_level_network_subsystem(void *arg)
{
    struct sr_instance* sr = (struct sr_instance*)arg;

    /* -- set argument as global singleton -- */
    sr_get_global_instance(sr);


#ifdef _CPUMODE_
    /* -- whizbang main loop ;-) */
    while( sr_cpu_input(sr) == 1);
#else
    /* -- whizbang main loop ;-) */
    while( sr_vns_read_from_server(sr) == 1);
#endif

   /* -- this is the end ... my only friend .. the end -- */
    sr_destroy_instance(sr);
} /* --  sr_low_level_network_subsystem -- */

/*-----------------------------------------------------------------------------
 * Method: sr_lwip_transport_startup(..)
 * Scope: local
 *---------------------------------------------------------------------------*/

static int sr_lwip_transport_startup(void)
{
    sys_init();
    mem_init();
    memp_init();
    pbuf_init();

    transport_subsys_init(0, 0);

    return 0;
} /* -- sr_lwip_transport_startup -- */


/*-----------------------------------------------------------------------------
 * Method: sr_set_user(..)
 * Scope: local
 *---------------------------------------------------------------------------*/

static void sr_set_user(struct sr_instance* sr)
{
    uid_t uid = getuid();
    struct passwd* pw = 0;

    /* REQUIRES */
    assert(sr);

    if(( pw = getpwuid(uid) ) == 0)
    {
        fprintf (stderr, "Error getting username, using something silly\n");
        strncpy(sr->user, "something_silly",  SR_NAMELEN);
    }
    else
    { strncpy(sr->user, pw->pw_name,  SR_NAMELEN); }

} /* -- sr_set_user -- */

/*-----------------------------------------------------------------------------
 * Method: sr_init_instance(..)
 * Scope:  Local
 *
 *----------------------------------------------------------------------------*/

static
void sr_init_instance(struct sr_instance* sr)
{
    /* REQUIRES */
    assert(sr);

    /* -- set argument as global singleton -- */
    sr_get_global_instance(sr);

    sr->sockfd   = -1;
    sr->user[0]  = 0;
    sr->vhost[0] = 0;
    sr->topo_id  = 0;
    sr->logfile  = 0;
    sr->hw_init  = 0;

    sr->interface_subsystem = 0;

    pthread_mutex_init(&(sr->send_lock), 0);

    sr_integ_init(sr);
} /* -- sr_init_instance -- */

/*-----------------------------------------------------------------------------
 * Method: sr_destroy_instance(..)
 * Scope:  local
 *
 *----------------------------------------------------------------------------*/

static void sr_destroy_instance(struct sr_instance* sr) {
    assert(sr);
    sr_integ_destroy(sr);
    free( sr );
}

/*-----------------------------------------------------------------------------
 * Method: usage(..)
 * Scope: local
 *---------------------------------------------------------------------------*/

static void usage(char* argv0)
{
    printf("Simple Router Client\n");
    printf("Format: %s [-h] [-v host] [-s server] [-p port] \n",argv0);
    printf("           [-t topo id] [-r rtable_file] [-l log_file] [-i interface_file]\n");
} /* -- usage -- */
