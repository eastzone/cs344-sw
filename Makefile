#------------------------------------------------------------------------------
# File: Makefile
# Date: Tue Feb 03 15:03:51 PST 2004 
# Author: Martin Casado
# 
# Note: This Makefile requires GNU make.
# 
# (c) 2001,2000 Stanford University
#
#------------------------------------------------------------------------------

all : sr

CC  = gcc

OSTYPE = $(shell uname)

ifeq ($(OSTYPE),Linux)
ARCH = -D_LINUX_
SOCK = -lnsl
endif

ifeq ($(OSTYPE),SunOS)
ARCH =  -D_SOLARIS_
SOCK = -lnsl -lsocket
endif

ifeq ($(OSTYPE),Darwin)
ARCH = -D_DARWIN_
SOCK =
endif

# --
# To run the sr in "cpu" mode to interact with the NetFPGA uncomment
# the last flag (-D_CPUMODE_)
# --

CFLAGS = -g -Wall -D_DEBUG_ $(ARCH) -I lwtcp -D_CPUMODE_

LIBS= $(SOCK) -lm -lresolv -lpthread
PFLAGS= -follow-child-processes=yes -cache-dir=/tmp/${USER} 
PURIFY= purify ${PFLAGS}

#------------------------------------------------------------------------------
LWTCP_SRCS = lwtcp/tcp.c lwtcp/tcp_input.c lwtcp/tcp_output.c\
             lwtcp/mem.c lwtcp/memp.c lwtcp/stats.c lwtcp/sys.c\
             lwtcp/inet.c lwtcp/pbuf.c lwtcp/sys_arch.c\
			 lwtcp/sockets.c lwtcp/api_lib.c lwtcp/api_msg.c\
			 lwtcp/transport_subsys.c lwtcp/udp.c lwtcp/icmp.c lwtcp/ip_addr.c\
			 lwtcp/err.c

LWTCP_OBJS = $(patsubst lwtcp/%.c, %.o, $(LWTCP_SRCS)) 
             

liblwtcp.a : $(LWTCP_OBJS)
	ar rcu liblwtcp.a $(LWTCP_OBJS)
#------------------------------------------------------------------------------

#------------------------------------------------------------------------------
SR_BASE_SRCS = sr_base.c sr_dumper.c sr_integration.c sr_lwtcp_glue.c\
               sr_vns.c sr_cpu_extension_nf2.c

SR_BASE_OBJS = $(patsubst %.c,%.o,$(SR_BASE_SRCS))

libsr_base.a : $(SR_BASE_OBJS)
	ar rcu libsr_base.a $(SR_BASE_OBJS)
#------------------------------------------------------------------------------


#------------------------------------------------------------------------------
SR_SRCS =  sr_main.c

SR_OBJS = $(patsubst %.c,%.o,$(SR_SRCS))

sr : $(SR_OBJS) libsr_base.a liblwtcp.a
	$(CC) $(CFLAGS) -o sr $(SR_OBJS) $(LIBS) libsr_base.a liblwtcp.a\
    libsr_base.a 
#------------------------------------------------------------------------------



#------------------------------------------------------------------------------
ALL_SRCS   = $(sort $(SR_SRCS) $(SR_BASE_SRCS) $(LWTCP_SRCS))

ALL_LWTCP_SRCS = $(filter lwtcp/%.c, $(ALL_SRCS))
ALL_SR_SRCS    = $(filter-out lwtcp/%.c, $(ALL_SRCS))  

ALL_LWTCP_OBJS = $(patsubst lwtcp/%.c,%.o, $(ALL_LWTCP_SRCS))
ALL_SR_OBJS    = $(patsubst %.c,%.o, $(ALL_SR_SRCS))

ALL_LWTCP_DEPS = $(patsubst lwtcp/%.c,.%.d,$(ALL_LWTCP_SRCS))
ALL_SR_DEPS    = $(patsubst %.c,.%.d,$(ALL_SR_SRCS))

$(ALL_SR_OBJS) : %.o : %.c
	$(CC) -c $(CFLAGS) $< -o $@
$(ALL_LWTCP_OBJS) : %.o : ./lwtcp/%.c
	$(CC) -c $(CFLAGS) $< -o $@

$(ALL_LWTCP_DEPS) : .%.d : ./lwtcp/%.c
	$(CC) -MM $(CFLAGS) -I lwtcp/ $<  > $@
$(ALL_SR_DEPS) : .%.d : %.c
	$(CC) -MM $(CFLAGS) -I lwtcp/ $<  > $@

include $(ALL_LWTCP_DEPS)
include $(ALL_SR_DEPS)

#------------------------------------------------------------------------------

#------------------------------------------------------------------------------
.PHONY : clean clean-deps dist    

clean:
	rm -f *.o *~ core.* sr *.dump *.tar tags *.a test_arp_subsystem\
          lwcli lwtcpsr sr_base.tar.gz

clean-deps:
	rm -f .*.d

dist: clean clean-deps
	(cd ..; tar --exclude-from=base/exclude -cvf sr_base.tar base/; gzip sr_base.tar; mv\
    sr_base.tar.gz base/)

tags:
	ctags *.c
#------------------------------------------------------------------------------
