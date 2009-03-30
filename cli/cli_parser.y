/* Filename: cli_parser.y */

%{

#include <string.h>              /* memcpy(), memmove(), memset()     */
#include "cli.h"                 /* implementation of CLI methods     */
#include "cli_help.h"            /* cli_help_t, cli_send_help()       */
#include "cli_scanner.h"         /* for yylex                         */
#include "cli_parser.h"          /* parser external API               */
#include "helper.h"

/** buffer to feed to the parser */
static char cmd_buf[CLI_MAX_BUF+2];

/** number of bytes used in buf */
static unsigned cmd_blen;

/**
 * Manual Error Handler.  If not NULL, then yyerror will not print an error
 * message (the user is manually handling the error).  This points to the error
 * desc text to use in yyerror.  (Syntax Error: <meh_desc>).  If meh_desc is
 * NULL, and yyerror is called, then the argument passed to yyerror is used.
 */
const char* meh_desc;
int meh_has_usage;
int meh_ignore;
int meh_force;

/** Handles printing a nicely formatted error. */
void yyerror( const char* desc );
static int has_error;
char token_err[128];

/* convenience */
#define ERR(desc) parse_error(desc);
#define ERR_TMI   ERR("expected end-of-command (newline)")
#define ERR_WRONG ERR("unexpected token")
#define ERR_IP    ERR("expected IP address")
#define ERR_MAC   ERR("expected MAC address")
#define ERR_INTF  ERR("expected interface name")
#define ERR_NO_USAGE(desc) parse_error(desc); meh_force = 1; meh_has_usage = 0; meh_ignore = 0;
#define ERR_IGNORE meh_ignore = 1;

/* We only want to call a command once it is completely parsed (with the
   exception of help commands which fire on errors and can be safely executed
   immediately as actions).  This is a gross, but these structs hold the parsed
   data until it is time to execute a command. */
gross_object_t gobj;
gross_arp_t garp;
gross_intf_t gintf;
gross_route_t grt;
gross_ip_t gip;
gross_ip_int_t giip;
gross_option_t gopt;
#define SETC_FUNC0(func)      gobj.func_do0=func; gobj.func_do1=NULL; gobj.data=NULL
#define SETC_FUNC1(func)      gobj.func_do0=NULL; gobj.func_do1=(void (*)(void*))func; gobj.data=NULL
#define SETC_ARP_IP(func,xip)  SETC_FUNC1(func); gobj.data=&garp; garp.ip=xip
#define SETC_ARP(func,ip,xmac) SETC_ARP_IP(func,ip); memcpy(garp.mac, xmac, 6);
#define SETC_INTF(func,name)  SETC_FUNC1(func); gobj.data=&gintf; gintf.intf_name=name
#define SETC_INTF_SET(func,name,xip,sm) SETC_INTF(func,name); gintf.ip=xip; gintf.subnet_mask=sm
#define SETC_RT(func,xdest,xmask) SETC_FUNC1(func); gobj.data=&grt; grt.dest=xdest; grt.mask=xmask
#define SETC_RT_ADD(func,dest,xgw,mask,intf) SETC_RT(func,dest,mask); grt.gw=xgw; grt.intf_name=intf
#define SETC_IP(func,xip) SETC_FUNC1(func); gobj.data=&gip; gip.ip=xip
#define SETC_IP_INT(func,xip,xn) SETC_FUNC1(func); gobj.data=&giip; giip.ip=xip; giip.count=xn
#define SETC_OPT(func) SETC_FUNC1(func); gobj.data=&gopt

/** Clears out any previous command */
static void clear_command();

/** Run the command stored in gobj, if any. */
static void run_command();

/**
 * Convenience method for when a help command is called (directly or by an
 * error handler).
 */
#define HELP(h) cli_send_help(h); clear_command(); YYABORT;

%}

%union {
    int intVal;
    uint32_t ip;
    uint8_t mac[6];
    char string[MAX_STR_LEN];
}

/* Terminals with no attribute value */
%token  T_SHOW T_QUESTION T_NEWLINE T_ALL
%token  T_VNS T_USER T_VHOST T_LHOST T_TOPOLOGY
%token  T_IP T_ROUTE T_INTF T_ARP T_OSPF T_HW T_NEIGHBORS
%token  T_ADD T_DEL T_UP T_DOWN T_PURGE T_STATIC T_DYNAMIC T_ABOUT
%token  T_PING T_TRACE T_HELP T_EXIT T_SHUTDOWN T_FLOOD
%token  T_SET T_UNSET T_OPTION T_VERBOSE T_DATE

/* Terminals which evaluate to some attribute value */
%token   <intVal>       TAV_INT
%token   <ip>           TAV_IP
%token   <mac>          TAV_MAC
%token   <string>       TAV_STR

%%

Start : { clear_command(); } Command T_NEWLINE    { run_command(); YYACCEPT; }
      ;

Command : ShowCommand
        | ManipCommand
        | ActionCommand
        | OptionCommand
        | {ERR_NO_USAGE("Unknown Command")} error
        | /* do nothing */
        ;

ShowCommand : T_SHOW ShowType
            ;

ShowType : /* empty: show all */                  { SETC_FUNC0(cli_show_all); }
         | T_HW  ShowTypeHW
         | T_IP  ShowTypeIP
         | T_OSPF ShowTypeOSPF
         | T_VNS ShowTypeVNS
         | T_OPTION ShowTypeOption
         | HelpOrQ                                { HELP(HELP_SHOW); }
         ;

ShowTypeHW : /* empty: show all */                { SETC_FUNC0(cli_show_hw); }
           | T_ABOUT                              { SETC_FUNC0(cli_show_hw_about); }
           | T_ABOUT TMIorQ                       { HELP(HELP_SHOW_HW_ABOUT); }
           | T_ARP                                { SETC_FUNC0(cli_show_hw_arp); }
           | T_ARP TMIorQ                         { HELP(HELP_SHOW_HW_ARP); }
           | T_INTF                               { SETC_FUNC0(cli_show_hw_intf); }
           | T_INTF TMIorQ                        { HELP(HELP_SHOW_HW_INTF); }
           | T_ROUTE                              { SETC_FUNC0(cli_show_hw_route); }
           | T_ROUTE TMIorQ                       { HELP(HELP_SHOW_HW_ROUTE); }
           | WrongOrQ                             { HELP(HELP_SHOW_HW); }
           ;

ShowTypeIP : /* empty: show all */                { SETC_FUNC0(cli_show_ip); }
           | T_ARP                                { SETC_FUNC0(cli_show_ip_arp); }
           | T_ARP TMIorQ                         { HELP(HELP_SHOW_IP_ARP); }
           | T_INTF                               { SETC_FUNC0(cli_show_ip_intf); }
           | T_INTF TMIorQ                        { HELP(HELP_SHOW_IP_INTF); }
           | T_ROUTE                              { SETC_FUNC0(cli_show_ip_route); }
           | T_ROUTE TMIorQ                       { HELP(HELP_SHOW_IP_ROUTE); }
           | WrongOrQ                             { HELP(HELP_SHOW_IP); }
           ;

ShowTypeOSPF : /* empty: show all */                { SETC_FUNC0(cli_show_ospf); }
             | T_NEIGHBORS                          { SETC_FUNC0(cli_show_ospf_neighbors); }
             | T_NEIGHBORS TMIorQ                   { HELP(HELP_SHOW_OSPF_NEIGHBORS); }
             | T_TOPOLOGY                           { SETC_FUNC0(cli_show_ospf_topo); }
             | T_TOPOLOGY TMIorQ                    { HELP(HELP_SHOW_OSPF_TOPOLOGY); }
             | WrongOrQ                             { HELP(HELP_SHOW_OSPF); }
             ;

TMIorQ : HelpOrQ
       | {ERR_TMI} error
       ;

WrongOrQ : HelpOrQ
         | {ERR_WRONG} error
         ;

ShowTypeVNS : /* empty: show all */               { SETC_FUNC0(cli_show_vns); }
            | T_LHOST                             { SETC_FUNC0(cli_show_vns_lhost); }
            | T_LHOST TMIorQ                      { HELP(HELP_SHOW_VNS_LHOST); }
            | T_TOPOLOGY                          { SETC_FUNC0(cli_show_vns_topo); }
            | T_TOPOLOGY TMIorQ                   { HELP(HELP_SHOW_VNS_TOPOLOGY); }
            | T_USER                              { SETC_FUNC0(cli_show_vns_user); }
            | T_USER TMIorQ                       { HELP(HELP_SHOW_VNS_USER); }
            | T_VHOST                             { SETC_FUNC0(cli_show_vns_vhost); }
            | T_VHOST TMIorQ                      { HELP(HELP_SHOW_VNS_VHOST); }
            | WrongOrQ                            { HELP(HELP_SHOW_VNS); }
            ;

ManipCommand : T_IP ManipTypeIP
             ;

ManipTypeIP : T_ARP ManipTypeIPARP
            | T_INTF ManipTypeIPInterface
            | T_OSPF ManipTypeIPOSPF
            | T_ROUTE ManipTypeIPRoute
            ;

ManipTypeIPARP : WrongOrQ                         { HELP(HELP_MANIP_IP_ARP); }
               | T_ADD AddTransOrQ
               | T_DEL DelTransOrQ
               | T_PURGE                          { SETC_FUNC0(cli_manip_ip_arp_purge_all); }
               | T_PURGE TMIorQ                   { HELP(HELP_MANIP_IP_ARP_PURGE_ALL); }
               | T_PURGE T_DYNAMIC                { SETC_FUNC0(cli_manip_ip_arp_purge_dyn); }
               | T_PURGE T_DYNAMIC TMIorQ         { HELP(HELP_MANIP_IP_ARP_PURGE_DYN); }
               | T_PURGE T_STATIC                 { SETC_FUNC0(cli_manip_ip_arp_purge_sta); }
               | T_PURGE T_STATIC TMIorQ          { HELP(HELP_MANIP_IP_ARP_PURGE_STA); }
               ;

AddTransOrQ : HelpOrQ                             { HELP(HELP_MANIP_IP_ARP_ADD); }
            | {ERR_IP} error                      { HELP(HELP_MANIP_IP_ARP_ADD); }
            | TAV_IP {ERR_MAC} error              { HELP(HELP_MANIP_IP_ARP_ADD); }
            | TAV_IP TAV_MAC                      { SETC_ARP(cli_manip_ip_arp_add,$1,$2); }
            | TAV_IP TAV_MAC TMIorQ               { HELP(HELP_MANIP_IP_ARP_ADD); }
            ;

DelTransOrQ : HelpOrQ                             { HELP(HELP_MANIP_IP_ARP_DEL); }
            | {ERR_IP} error                      { HELP(HELP_MANIP_IP_ARP_DEL); }
            | TAV_IP                              { SETC_ARP_IP(cli_manip_ip_arp_del,$1); }
            | TAV_IP TMIorQ                       { HELP(HELP_MANIP_IP_ARP_DEL); }
            ;

ManipTypeIPInterface : HelpOrQ                        { HELP(HELP_MANIP_IP_INTF); }
                     | {ERR_INTF} error               { HELP(HELP_MANIP_IP_INTF); }
                     | TAV_STR WrongOrQ               { HELP(HELP_MANIP_IP_INTF); }
                     | TAV_STR TAV_IP {ERR_IP} error  { HELP(HELP_MANIP_IP_INTF_SET); }
                     | TAV_STR TAV_IP TAV_IP          { SETC_INTF_SET(cli_manip_ip_intf_set,$1,$2,$3); }
                     | TAV_STR TAV_IP TAV_IP TMIorQ   { HELP(HELP_MANIP_IP_INTF_SET); }
                     | TAV_STR T_UP                   { SETC_INTF(cli_manip_ip_intf_up,$1); }
                     | TAV_STR T_UP TMIorQ            { HELP(HELP_MANIP_IP_INTF_UP); }
                     | TAV_STR T_DOWN                 { SETC_INTF(cli_manip_ip_intf_down,$1); }
                     | TAV_STR T_DOWN TMIorQ          { HELP(HELP_MANIP_IP_INTF_DOWN); }
                     ;

ManipTypeIPOSPF : WrongOrQ                        { HELP(HELP_MANIP_IP_OSPF);           }
                | T_UP                            { SETC_FUNC0(cli_manip_ip_ospf_up);   }
                | T_UP TMIorQ                     { HELP(HELP_MANIP_IP_OSPF_UP);        }
                | T_DOWN                          { SETC_FUNC0(cli_manip_ip_ospf_down); }
                | T_DOWN TMIorQ                   { HELP(HELP_MANIP_IP_OSPF_DOWN);      }
                ;

ManipTypeIPRoute : WrongOrQ                       { HELP(HELP_MANIP_IP_ROUTE); }
                 | T_ADD RouteAddOrQ
                 | T_DEL RouteDelOrQ
                 | T_PURGE                        { SETC_FUNC0(cli_manip_ip_route_purge_all); }
                 | T_PURGE T_DYNAMIC              { SETC_FUNC0(cli_manip_ip_route_purge_dyn); }
                 | T_PURGE T_STATIC               { SETC_FUNC0(cli_manip_ip_route_purge_sta); }
                 | T_PURGE TMIorQ                 { HELP(HELP_MANIP_IP_ROUTE_PURGE_ALL); }
                 ;

RouteAddOrQ : HelpOrQ                               { HELP(HELP_MANIP_IP_ROUTE_ADD); }
            | {ERR_IP} error                        { HELP(HELP_MANIP_IP_ROUTE_ADD); }
            | TAV_IP {ERR_IP} error                 { HELP(HELP_MANIP_IP_ROUTE_ADD); }
            | TAV_IP TAV_IP {ERR_IP} error          { HELP(HELP_MANIP_IP_ROUTE_ADD); }
            | TAV_IP TAV_IP TAV_IP {ERR_INTF} error { HELP(HELP_MANIP_IP_ROUTE_ADD); }
            | TAV_IP TAV_IP TAV_IP TAV_STR          { SETC_RT_ADD(cli_manip_ip_route_add,$1,$2,$3,$4); }
            | TAV_IP TAV_IP TAV_IP TAV_STR TMIorQ   { HELP(HELP_MANIP_IP_ROUTE_ADD); }
            ;

RouteDelOrQ : HelpOrQ                             { HELP(HELP_MANIP_IP_ROUTE_DEL); }
            | {ERR_IP} error                      { HELP(HELP_MANIP_IP_ROUTE_DEL); }
            | TAV_IP {ERR_IP} error               { HELP(HELP_MANIP_IP_ROUTE_DEL); }
            | TAV_IP TAV_IP                       { SETC_RT(cli_manip_ip_route_del,$1,$2); }
            | TAV_IP TAV_IP TMIorQ                { HELP(HELP_MANIP_IP_ROUTE_DEL); }
            ;

ActionCommand : T_PING ActionPing
              | T_TRACE ActionTrace
              | ActionDate
              | ActionExit
              | ActionShutdown
              | ActionHelp
              ;

ActionPing : HelpOrQ                              { HELP(HELP_ACTION_PING); }
           | {ERR_IP} error                       { HELP(HELP_ACTION_PING); }
           | TAV_IP                               { SETC_IP(cli_ping,$1); }
           | T_FLOOD TAV_IP                       { SETC_IP_INT(cli_ping_flood,$2,100); }
           | T_FLOOD TAV_IP TMIorQ                { HELP(HELP_ACTION_PING); }
           | T_FLOOD TAV_INT TAV_IP               { SETC_IP_INT(cli_ping_flood,$3,$2); }
           | T_FLOOD TAV_INT TAV_IP TMIorQ        { HELP(HELP_ACTION_PING); }
           | TAV_IP TMIorQ                        { HELP(HELP_ACTION_PING); }
           ;

ActionTrace : HelpOrQ                             { HELP(HELP_ACTION_TRACE); }
            | {ERR_IP} error                      { HELP(HELP_ACTION_TRACE); }
            | TAV_IP                              { SETC_IP(cli_traceroute,$1); }
            | TAV_IP TMIorQ                       { HELP(HELP_ACTION_TRACE); }
            ;

ActionDate : T_DATE                               { SETC_FUNC0(cli_date); }
           | T_DATE TMIorQ                        { HELP(HELP_ACTION_DATE); }
           ;

ActionExit : T_EXIT                               { SETC_FUNC0(cli_exit); }
           | T_EXIT TMIorQ                        { HELP(HELP_ACTION_EXIT); }
           ;

ActionShutdown : T_SHUTDOWN                       { SETC_FUNC0(cli_shutdown); }
               | T_SHUTDOWN TMIorQ                { HELP(HELP_ACTION_SHUTDOWN); }
               ;

ActionHelp : HelpOrQ                              { HELP(HELP_ACTION_HELP); }
           | HelpOrQ T_ALL                        { HELP(HELP_ALL);         }
           | HelpOrQ T_ALL {ERR_IGNORE} error     { HELP(HELP_ALL); }
           | HelpOrQ T_SHOW                       { HELP(HELP_SHOW ); }
           | HelpOrQ T_SHOW T_HW                  { HELP(HELP_SHOW_HW); }
           | HelpOrQ T_SHOW T_HW T_ABOUT          { HELP(HELP_SHOW_HW_ABOUT); }
           | HelpOrQ T_SHOW T_HW T_ARP            { HELP(HELP_SHOW_HW_ARP); }
           | HelpOrQ T_SHOW T_HW T_INTF           { HELP(HELP_SHOW_HW_INTF); }
           | HelpOrQ T_SHOW T_HW T_ROUTE          { HELP(HELP_SHOW_HW_ROUTE); }
           | HelpOrQ T_SHOW T_IP                  { HELP(HELP_SHOW_IP); }
           | HelpOrQ T_SHOW T_IP T_ARP            { HELP(HELP_SHOW_IP_ARP); }
           | HelpOrQ T_SHOW T_IP T_INTF           { HELP(HELP_SHOW_IP_INTF); }
           | HelpOrQ T_SHOW T_IP T_ROUTE          { HELP(HELP_SHOW_IP_ROUTE); }
           | HelpOrQ T_SHOW T_OPTION              { HELP(HELP_SHOW_OPT); }
           | HelpOrQ T_SHOW T_OPTION T_VERBOSE    { HELP(HELP_SHOW_OPT_VERBOSE); }
           | HelpOrQ T_SHOW T_OSPF                { HELP(HELP_SHOW_OSPF); }
           | HelpOrQ T_SHOW T_OSPF T_NEIGHBORS    { HELP(HELP_SHOW_OSPF_NEIGHBORS); }
           | HelpOrQ T_SHOW T_OSPF T_TOPOLOGY     { HELP(HELP_SHOW_OSPF_TOPOLOGY); }
           | HelpOrQ T_SHOW T_VNS                 { HELP(HELP_SHOW_VNS); }
           | HelpOrQ T_SHOW T_VNS T_LHOST         { HELP(HELP_SHOW_VNS_LHOST); }
           | HelpOrQ T_SHOW T_VNS T_TOPOLOGY      { HELP(HELP_SHOW_VNS_TOPOLOGY); }
           | HelpOrQ T_SHOW T_VNS T_USER          { HELP(HELP_SHOW_VNS_USER); }
           | HelpOrQ T_SHOW T_VNS T_VHOST         { HELP(HELP_SHOW_VNS_VHOST); }
           | HelpOrQ T_IP                         { HELP(HELP_MANIP_IP); }
           | HelpOrQ T_IP T_ARP                   { HELP(HELP_MANIP_IP_ARP); }
           | HelpOrQ T_IP T_ARP T_ADD             { HELP(HELP_MANIP_IP_ARP_ADD); }
           | HelpOrQ T_IP T_ARP T_DEL             { HELP(HELP_MANIP_IP_ARP_DEL); }
           | HelpOrQ T_IP T_ARP T_PURGE           { HELP(HELP_MANIP_IP_ARP_PURGE_ALL); }
           | HelpOrQ T_IP T_ARP T_PURGE T_DYNAMIC { HELP(HELP_MANIP_IP_ARP_PURGE_DYN); }
           | HelpOrQ T_IP T_ARP T_PURGE T_STATIC  { HELP(HELP_MANIP_IP_ARP_PURGE_STA); }
           | HelpOrQ T_IP T_INTF                  { HELP(HELP_MANIP_IP_INTF); }
           | HelpOrQ T_IP T_INTF T_DOWN           { HELP(HELP_MANIP_IP_INTF_DOWN); }
           | HelpOrQ T_IP T_INTF T_UP             { HELP(HELP_MANIP_IP_INTF_UP); }
           | HelpOrQ T_IP T_ROUTE                 { HELP(HELP_MANIP_IP_ROUTE); }
           | HelpOrQ T_IP T_ROUTE T_ADD           { HELP(HELP_MANIP_IP_ROUTE_ADD); }
           | HelpOrQ T_IP T_ROUTE T_DEL           { HELP(HELP_MANIP_IP_ROUTE_DEL); }
           | HelpOrQ T_IP T_ROUTE T_PURGE         { HELP(HELP_MANIP_IP_ROUTE_PURGE_ALL); }
           | HelpOrQ T_IP T_ROUTE T_DYNAMIC       { HELP(HELP_MANIP_IP_ROUTE_PURGE_DYN); }
           | HelpOrQ T_IP T_ROUTE T_STATIC        { HELP(HELP_MANIP_IP_ROUTE_PURGE_STA); }
           | HelpOrQ T_DATE                       { HELP(HELP_ACTION_DATE); }
           | HelpOrQ T_EXIT                       { HELP(HELP_ACTION_EXIT); }
           | HelpOrQ T_PING                       { HELP(HELP_ACTION_PING); }
           | HelpOrQ T_SHUTDOWN                   { HELP(HELP_ACTION_SHUTDOWN); }
           | HelpOrQ T_TRACE                      { HELP(HELP_ACTION_TRACE); }
           | HelpOrQ T_SET                        { HELP(HELP_OPT); }
           | HelpOrQ T_SET T_VERBOSE              { HELP(HELP_OPT_VERBOSE); }
           | HelpOrQ T_UNSET                      { HELP(HELP_OPT); }
           | HelpOrQ T_UNSET T_VERBOSE            { HELP(HELP_OPT_VERBOSE); }
           | HelpOrQ {ERR_IGNORE} error           { HELP(HELP_ACTION_HELP); }
           ;

HelpOrQ : T_HELP
        | T_QUESTION
        ;

OptionCommand : OptionAction Option
              ;

OptionAction : T_SET                              { gopt.on = 1; }
             | T_UNSET                            { gopt.on = 0; }
             ;

Option : HelpOrQ                                  { HELP(HELP_OPT); }
       | T_VERBOSE                                { SETC_OPT(cli_opt_verbose); }
       | T_VERBOSE TMIorQ                         { HELP(HELP_OPT_VERBOSE); }
       ;

ShowTypeOption : /* empty: show all */            { SETC_FUNC0(cli_show_opt); }
               | T_VERBOSE                        { SETC_FUNC0(cli_show_opt_verbose); }
               | T_VERBOSE TMIorQ                 { HELP(HELP_SHOW_OPT_VERBOSE); }
               | WrongOrQ                         { HELP(HELP_SHOW_OPT); }
               ;
%%

static void clear_command() {
    gobj.func_do0 = NULL;
    gobj.func_do1 = NULL;
    gobj.data     = NULL;
    has_error     = 0;
    token_err[0] = '\0';
}

static void run_command() {
    if( has_error ) {
        if( token_err[0] != '\0' )
            cli_send_parse_error( 2, "Unexecpted Token Error: ", token_err );

        clear_command();
        return;
    }

    if( gobj.func_do0 ) {
        true_or_die( gobj.data == NULL, "Error: func_do0 and data set" );
        true_or_die( gobj.func_do1 == NULL, "Error: func_do1 set at same time as func_do0" );
        gobj.func_do0();
    }
    else if( gobj.func_do1 ) {
        true_or_die( gobj.data != NULL, "Error: func_do1 set but data is NULL" );
        gobj.func_do1( gobj.data );
    }

    clear_command();
}

void parse_error( const char* desc ) {
    if( !meh_desc ) meh_desc = desc;
    meh_has_usage = 1;
    meh_ignore = 0;
    meh_force = 0;
    has_error = 1;
}

void cli_parser_init() {
    yydebug = 0;
    meh_desc = NULL;
    has_error = meh_has_usage = meh_ignore = meh_force = 0;
    token_err[0] = '\0';
}

int cli_parser_handle_client( cli_client_t* client ) {
    YY_BUFFER_STATE yystate;
    char* sbuf;
    char* sbuf_loc;
    char* pch;
    int num;

    sbuf = client->state.chbuf;

    /* copy the command(s) */
    pch = strchr( sbuf, '\n' );
    true_or_die( pch!=NULL,
                 "Error: cli_parser_handle_client called when sbuf has no newline char" );
    do {
        cmd_blen = pch - sbuf + 1;
        memcpy( cmd_buf, sbuf, cmd_blen );

        /* add the two NUL bytes lex requires at the end */
        memset( cmd_buf + cmd_blen, '\0', 2 );

        /* give the buffer to lex (+2 => include NULs) */
        yystate = yy_scan_buffer( cmd_buf, cmd_blen + 2 );
        true_or_die( yystate != NULL,
                     "Error: the last 2 bytes are not NUL for flex?" );

        /* process the command(s) received */
        cli_scanner_init(); /* reset the line/col numbers */
        yyparse();
        yy_delete_buffer( yystate );

        /* bail out if socket closed or router shutting down */
        if( !cli_focus_is_alive() || cli_is_time_to_shutdown() ) {
            return 0; /* cleanup the connection */
        }

        /* continue searching from char after pch */
        sbuf_loc = pch + 1;
        pch = strchr( sbuf_loc, '\n' );
    }
    while( pch );

    /* move any extra bytes leftover to the front of the sbuf */
    num = client->state.used - client->state.needle_offset - 1;
    if( num > 0 ) {
        memmove( sbuf, sbuf+client->state.needle_offset, num );
        client->state.used = num;
    }
    else
        client->state.used = 0;

    /* we pulled all the needles out, so keep searching what is left */
    client->state.search_offset = 0;
    client->state.needle_offset = client->state.capacity + 1;

    cli_send_prompt();
    return cli_focus_is_alive(); /* true if client socket is still connected */
}

void yyerror( const char* desc ) {
    if( !meh_ignore ) {
        /* only print yyerror message if the manual error handler flag is unset */
        if( meh_desc ) {
            token_err[0] = '\0'; /* ignore any previous token errors */

            if( !meh_force && (strchr(yytext, '?') || strstr(yytext, "help")) )
                return; /* no-op: user handled it */
            else {
                cli_send_parse_error( 4,
                                      scanner_get_str_to_underline_error(STRLEN_PROMPT),
                                      "Syntax Error: ",
                                      meh_desc,
                                      meh_has_usage ? "\n  usage: " : "\n" );
            }
        }
        else
            strncpy( token_err, yytext, 128 );

        has_error = 1;
    }

    /* reset the manual error handler flag and desc */
    meh_desc = NULL;
    meh_ignore = 0;
    meh_force = 0;
}
