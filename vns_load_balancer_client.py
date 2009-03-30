#!/usr/bin/env python
"""Gets the routing table for an available VNS topology and starts the sr
binary.  It prefers to reuse the last topology this client used if it is still
available."""

from optparse import OptionParser
import os
import socket
import struct
import sys

# file where the most recently used topology number is stored
LAST_TOPO_NUM_FN = '.last_topo_num'

# file where the routing table is stored
RTABLE = 'rtable'

# server info
SERVER_ADDR = 'vns-1.stanford.edu'
SERVER_PORT = 25039

def main(argv=sys.argv[1:]):
    usage = """usage: %prog [options]
Negotiates for an available topology and then connects a VNS client to the VNS server on that topology.
"""
    parser = OptionParser(usage)
    parser.add_option("-t", "--topo",
                      type="int",
                      help="preferred topology to connect to [default: more recently used topology]")

    (options, args) = parser.parse_args(argv)
    if len(args) > 0:
        parser.error("too many arguments")

    # determine which topology to request (0 => no preference)
    default_topo = 0
    if options.topo == None:
        if os.path.exists(LAST_TOPO_NUM_FN):
            fh = open(LAST_TOPO_NUM_FN, 'r')
            lines = fh.readlines()
            fh.close()
            if len(lines) >= 1:
                default_topo = lines[0]
    else:
        default_topo = options.topo

    try:
        default_topo = int(default_topo)
    except ValueError:
        default_topo = None

    request_topology(default_topo)

def request_topology(topo):
    """Contacts the server on SERVER to reserve a topology."""
    if topo == 0:
        print 'requesting an sr topology ...'
    else:
        print 'requesting sr topology #%u ...' % topo

    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect((SERVER_ADDR, SERVER_PORT))
    s.send(struct.pack('> I', topo))
    s.shutdown(socket.SHUT_WR) # half-close

    # get the response
    accum = ''
    buf = s.recv(4096)
    while len(buf) != 0:
        accum += buf
        buf = s.recv(4096)
    s.close()
    fmt = '> I'
    sz = struct.calcsize(fmt)
    if len(accum) < sz:
        print 'response too small: only got %uB' % len(accum)
        return -1
    topo = struct.unpack(fmt, accum[:sz])[0]
    rtable = accum[sz:]

    # save the routing table
    fh = open(RTABLE, 'w')
    fh.write(rtable)
    fh.close()
    print 'got sr topology #%u: \n%s' % (topo, rtable)

    # remember most recently assigned topo
    fh = open(LAST_TOPO_NUM_FN, 'w')
    fh.write('%u\n' % topo)
    fh.close()

    # run sr
    cmd = './sr -s %s -t %u -r %s -v vrhost' % (SERVER_ADDR, topo, RTABLE)
    print cmd
    return os.system(cmd)

if __name__ == "__main__":
    sys.exit(main())
