/* ****************************************************************************
 * $Id: nf2util.h 3546 2008-04-03 00:12:27Z grg $
 *
 * Module: nf2util.h
 * Project: NetFPGA 2 Linux Kernel Driver
 * Description: Header file for kernel driver
 *
 * Change history:
 *
 */

#ifndef _NF2UTIL_H
#define _NF2UTIL_H	1

#define PATHLEN		80
#define DEVICE_STR_LEN 120
#define DEFAULT_IFACE "nf2c0"

/** Encapsulates version info about a NetFPGA. */
typedef struct {
    unsigned nf2_device_id;
    unsigned nf2_revision;
    char nf2_device_str[DEVICE_STR_LEN];
} nf2_device_info_t;

/*
 * Structure to represent an nf2 device to a user mode programs
 */
struct nf2device {
    char *device_name;
    int fd;
    int net_iface;
    nf2_device_info_t info;
};

/* Function declarations */
int readReg(struct nf2device *nf2, unsigned reg, unsigned *val);
int writeReg(struct nf2device *nf2, unsigned reg, unsigned val);
int check_iface(struct nf2device *nf2);
int openDescriptor(struct nf2device *nf2);
int closeDescriptor(struct nf2device *nf2);

#endif
