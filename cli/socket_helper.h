/*
 * Filename: socket_wrapper.h
 * Author: David Underhill
 *
 * Purpose:  Define wrapper functions to handle some error conditions of various
 *           sockets methods.  This also includes various socket-related header
 *           files.
 *
 * Acknowledgements: The general wrapper function design as well as some of
 *           their implementations (noted below) are based on the
 *           following book:
 *            W. Richard Stevens, Bill Fenner, and Andrew M. Rudoff. UNIX Network
 *            Programming. Addison-Wesley. Volume 1, Edition 3, 2007. 89.
 */

#ifndef SOCKET_WRAPPER_H
#define SOCKET_WRAPPER_H

#ifdef REAL_SOCKET_WRAPPER_H
#error Including both real and lwip socket wrappers together
#endif

#include "cli_network.h"
#include "search_state.h"

/** maximum size of a line which witenf can create */
#define WRITEN_MAX_LEN 4096

/**
 * Read from fd until at least one byte is read, the socket has an error, or an
 * EOF is encountered.
 *
 * @param fd   file descriptor to read from
 * @param buf  buffer to read into
 * @param n    length of buf
 *
 * @return Number of bytes read into buf, or -1 if an error occurs.  If 0 is
 *         returned, then an EOF was encountered.
 */
ssize_t read_once( int fd, void* buf, unsigned n );

/**
 * Read n bytes into buf from the file descriptor fd.
 *
 * The implementation of this function is based on code in the following book:
 *  W. Richard Stevens, Bill Fenner, and Andrew M. Rudoff. UNIX Network
 *  Programming. Addison-Wesley. Volume 1, Edition 3, 2007. 89.
 *
 * @param fd   file descriptor to read from
 * @param buf  buffer to read into; must be at least size n
 * @param n    number of bytes to read into buf
 *
 * @return Number of bytes read into buf, or -1 if an error occurs.  If an error
 *         does not occur, the number of bytes read will be n unless an EOF was
 *         encountered.
 *
 */
ssize_t readn( int fd, void* buf, unsigned n );

/**
 * Read from the file descriptor fd until a particular needle is received.  It
 * may return without reading the specified value if an error occurs or the end
 * of the file is encountered.  The needle is not gaurunteed to be the at the
 * end of the used portion of the buffer -- it may have arrived in the midst of
 * other bytes.
 *
 * The search_state_t struct contains parameters (and one result) which are
 * used to manage the buffer and searches.  Please see its definition for
 * details on the purpose of its parameters.
 *
 * This function does not maintain any internal state between calls which makes
 * it safe to call on different file descriptors from different threads of
 * execution.  It may be interleaved with other calls to read on the same file
 * descriptor as long as the calls do not overlap.
 *
 * @param fd      file descriptor to read from
 * @param needle  the string being searched for; required: strlen(needle) > 0
 * @param state   search state as described above
 * @param max_reads  maximum number of times to read from the socket (0 means no
 *                   limit; 1 means this function won't block if select says fd
 *                   has data available for reading).
 *
 * @return  0  if the needle was found
 *          1  if an EOF was encountered (needle not found)
 *         -1  if an error occurred (needle not found)
 *         -2  if the buffer grew to its max size & filled up w/o finding needle
 *         -3  if max_reads were done w/o finding the needle
 *
 *         If -1 is returned, errno will be set appropriately.  In particular,
 *         ERANGE means the buffer was grown to the max size and the needle was
 *         not found by the time the buffer was full.  ENOMEM means the function
 *         tried to grow the buffer but was unable to allocate the additional
 *         memory needed.  Other errno values correspond to errors returned from
 *         read (will not be EINTR).
 */
int read_search( int fd, const char* needle, search_state_t* state,
                 int max_reads );

/**
 * Write n bytes from buf into the file descriptor fd.
 *
 * The implementation of this function is based on code in the following book:
 *  W. Richard Stevens, Bill Fenner, and Andrew M. Rudoff. UNIX Network
 *  Programming. Addison-Wesley. Volume 1, Edition 3, 2007. 89.
 *
 * @param fd   file descriptor to write to
 * @param buf  buffer to read from; must be at least size n
 * @param n    number of bytes to write from buf into fd
 *
 * @return 0 on success or -1 if an error occurs.
 */
int writen( int fd, const void* buf, unsigned n );

/**
 * Write formatted printf style output into the file descriptor fd.
 *
 * @param fd   file descriptor to write to
 * @param buf  buffer to read from; must be at least size n
 * @param n    number of bytes to write from buf into fd
 *
 * @return 0 on success or -1 if an error occurs.  If the error occurs due to
 * the formatted output being larger than MAX_LEN, then errno will be ERANGE.
 * Otherwise, the error is due to a problem encountered when trying to write the
 * output to the socket.
 */
int writenf( int fd, const char* format, ... );

/**
 * Write the specified string to the file descriptor fd.  This uses the string
 * in-place and is therefore more efficient for unformatted strings than writenf
 * which uses snprintf to build the formatted string.
 *
 * @param fd   file descriptor to write to
 * @param str  string to send
 *
 * @return 0 on success or -1 if an errors occurs.
 */
int writenstr( int fd, const char* str );

/**
 * Write the specified string(s) to the file descriptor fd.  This uses the
 * string in-place and is therefore more efficient for unformatted strings than
 * writenf which uses snprintf to build the formatted string.
 *
 * @param fd        file descriptor to write to
 * @param num_args  number of strings in the ...
 * @param ...       the strings to send
 *
 * @return 0 on success or -1 if an errors occurs.
 */
int writenstrs( int fd, int num_args, ... );

#endif /* SOCKET_WRAPPER_H */
