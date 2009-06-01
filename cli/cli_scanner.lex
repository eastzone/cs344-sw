/* Filename: cli_scanner.lex */

%{

#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include "cli_parser.h"
#include "y.tab.h"

#define MAX_LINE_LEN 256
#define TAB_SIZE 8
#define YY_NO_UNISTD_H /* tell flex not to include unistd.h */

/** current col number being scanend */
static unsigned startCol, endCol;

/** saves the current line's text */
char line[MAX_LINE_LEN];

/**
 * This function is installed as the YY_USER_ACTION. This is a place to group
 * code common to all actions.  On each match, we fill in the fields to record
 * its location and update our column counter.
 */
static void do_before_each_action();
#define YY_USER_ACTION do_before_each_action();

/** copies yytext into yylval.string (minus first/last char if QUOTED is 1) */
static int copy_yytext( int is_quoted );

/**
 * converts a string to an IP address
 * @return TAV_IP token on success, otherwise 0
 */
static int string_to_ip( char* str, uint32_t* result_ip );

/**
 * converts a string to a MAC address
 * @return T_MAC_CONST token on success, otherwise 0
 */
static int string_to_mac( char* str, uint8_t* result_mac );

%}

 /* Definitions (to make rules more readable) */
DIGIT             ([0-9])
HEX_DIGIT         ([0-9a-fA-F])
HEX_INTEGER       (0[Xx]{HEX_DIGIT}+)
DEC_INTEGER       ({DIGIT}+)
IP_ADDR           ({DEC_INTEGER}"."{DEC_INTEGER}"."{DEC_INTEGER}"."{DEC_INTEGER})
HEX_PAIR          ({HEX_DIGIT}{HEX_DIGIT})
MAC_ADDR          ({HEX_PAIR}:{HEX_PAIR}:{HEX_PAIR}:{HEX_PAIR}:{HEX_PAIR}:{HEX_PAIR})
NAME              ([a-zA-Z][a-zA-Z_0-9]*)

BEG_STRING        (\"[^"\n]*)
STRING            ({BEG_STRING}\")
/* " this is so the code looks decent in emacs :) */

%% /* begin rules section */

"\r\n"       { startCol = endCol = 0; return T_NEWLINE; }
"\n"         { startCol = endCol = 0; return T_NEWLINE; }
[ ]          { endCol += 1; }
[\t]         { endCol += TAB_SIZE - endCol%TAB_SIZE + 1; }


 /* -------------     Keywords     ------------- */

 /* ************* Print Operations ************* */
"show"       { return T_SHOW;      }
"?"          { return T_QUESTION;  }
"all"        { return T_ALL;       }

 /* VNS related values */
"vns"        { return T_VNS;       }
"user"       { return T_USER;      }
"vhost"      { return T_VHOST;     }
"lhost"      { return T_LHOST;     }
"topo"       { return T_TOPOLOGY;  }
"topology"   { return T_TOPOLOGY;  }

 /* IP related values */
"ip"         { return T_IP;        }
"rt"         { return T_ROUTE;     }
"route"      { return T_ROUTE;     }
"intf"       { return T_INTF;      }
"interface"  { return T_INTF;      }
"arp"        { return T_ARP;       }
"ospf"       { return T_OSPF;      }
"cpu"        { return T_HW;        }
"hardware"   { return T_HW;        }
"hw"         { return T_HW;        }
"neighbors"  { return T_NEIGHBORS; }
"neighbor"   { return T_NEIGHBORS; }
"neigh"      { return T_NEIGHBORS; }

 /* ********* Manipulation Operations ********** */
"add"        { return T_ADD;       }
"del"        { return T_DEL;       }
"delete"     { return T_DEL;       }
"up"         { return T_UP;        }
"enable"     { return T_UP;        }
"down"       { return T_DOWN;      }
"disable"    { return T_DOWN;      }
"purge"      { return T_PURGE;     }
"static"     { return T_STATIC;    }
"dyn"        { return T_DYNAMIC;   }
"dynamic"    { return T_DYNAMIC;   }
"about"      { return T_ABOUT;     }
"set"        { return T_SET;       }
"unset"      { return T_UNSET;     }
"option"     { return T_OPTION;    }
"opt"        { return T_OPTION;    }
"verbose"    { return T_VERBOSE;   }
"v"          { return T_VERBOSE;   }
"flood"      { return T_FLOOD;     }
"-f"         { return T_FLOOD;     }

 /* ***************** Actions ****************** */
"ping"       { return T_PING;      }
"sping"      { return T_PING;      }
"trace"      { return T_TRACE;     }
"strace"     { return T_TRACE;     }
"help"       { return T_HELP;      }
"quit"       { return T_EXIT;      }
"exit"       { return T_EXIT;      }
"shutdown"   { return T_SHUTDOWN;  }
"date"       { return T_DATE;      }
"now"        { return T_DATE;      }
"time"       { return T_DATE;      }

 /* **************** Constants ***************** */
{DEC_INTEGER}       { yylval.intVal = strtol(yytext, NULL, 10);
                      return TAV_INT; }

{HEX_INTEGER}       { yylval.intVal = strtol(yytext, NULL, 16);
                      return TAV_INT; }

{IP_ADDR}           { return string_to_ip(yytext, &yylval.ip); }

{MAC_ADDR}          { return string_to_mac(yytext, yylval.mac); }

{NAME}              { return copy_yytext(0); }

{STRING}            { return copy_yytext(1); }

{BEG_STRING}        { parse_error("Unterminated String Constant"); return 0; }

 /* *********** Default Rule (error) *********** */
.                   { parse_error("Unrecognized lexeme"); return 0; }

%%

static int copy_yytext( int is_quoted ) {
    unsigned len, start;

    len = strlen( yytext ) - (is_quoted ? 2 : 0);
    if( len > MAX_STR_LEN ) {
        parse_error( "String too long (max is %u chars)" );
        return 0;
    }

    if( is_quoted )
        start = 1;
    else
        start = 0;

    if( len > MAX_STR_LEN )
        len = MAX_STR_LEN;

    strncpy( yylval.string, yytext+start, len );
    return TAV_STR;
}

void get_rest_of_line() {
    char c;

    /* grab the rest of the line */
    do {
        c = input();
        if( c=='\0' ) c = '\n';

        if( endCol < MAX_LINE_LEN - 1 )
            line[endCol++] = c;
    }
    while( c != '\n' );
    line[endCol] = '\0';
}

#define CLI_MAX_OFFSET 256
char* scanner_get_str_to_underline_error( unsigned bump ) {
    static char buf[CLI_MAX_OFFSET+3];
    unsigned offset, offset_len;

    offset = (startCol > 0 ) ? startCol - 1 : 0;
    offset += bump;
    offset_len = (endCol > startCol ) ? endCol - startCol : 1;

    /* make sure a reasonable offset was set */
    if( offset+offset_len > CLI_MAX_OFFSET || (offset == 0 && offset_len == 1) )
        buf[0] = '\0';
    else {
        /* fill buf with appropriate spaces then carrots (^) */
        memset( buf, ' ', offset );
        memset( buf+offset, '^', offset_len );
        buf[offset+offset_len] = '\n';
        buf[offset+offset_len+1] = '\0';
    }

    return buf;
}

static int string_to_ip( char* str, uint32_t* result_ip ) {
    struct in_addr ip;
    if( inet_aton(str, &ip) == 0 ) {
        parse_error( "invalid IP address" );
        return 0;
    }
    else {
        *result_ip = ip.s_addr;
        return TAV_IP;
    }
}

static int string_to_mac( char* str, uint8_t* result_mac ) {
    unsigned mac_octet[6];

    if( sscanf( str, "%X:%X:%X:%X:%X:%X",
                &mac_octet[0], &mac_octet[1], &mac_octet[2],
                &mac_octet[3], &mac_octet[4], &mac_octet[5] ) != 6 ) {
        parse_error( "invalid IP address" );
        return 0;
    }
    else {
        result_mac[0] = mac_octet[0];
        result_mac[1] = mac_octet[1];
        result_mac[2] = mac_octet[2];
        result_mac[3] = mac_octet[3];
        result_mac[4] = mac_octet[4];
        result_mac[5] = mac_octet[5];
        return TAV_MAC;
    }
}

void cli_scanner_init() {
    yy_flex_debug = 0;
    startCol = endCol = 0;
}

const char* cli_scanner_get_cur_line() {
    return line;
}

unsigned cli_scanner_prev_lexeme_offset() {
    return startCol;
}

unsigned cli_scanner_prev_lexeme_length() {
    return endCol - startCol + 1;
}

static void do_before_each_action() {
    startCol = endCol;
    endCol += yyleng;

    strncpy( line + startCol, yytext, MAX_LINE_LEN - startCol );
}

int yywrap() {
    return 1; /* no more buffers to read: just the first one */
}
