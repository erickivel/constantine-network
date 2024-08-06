#ifndef PKG_H
#define PKG_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "pkg.defs.h"
#include "utils.h"
#include "crc8.h"

enum PkgType {
    PKG_ACK         = 0x00, 
    PKG_NACK        = 0x01, 
    PKG_LS          = 0x0A, 
    PKG_DOWNLOAD    = 0x0B,
    PKG_SHOW        = 0x10,
    PKG_DESCRIPTOR  = 0x11,
    PKG_DATA        = 0x12,
    PKG_END         = 0x1E,
    PKG_ERROR       = 0x1F
};

typedef enum PkgType PkgType;

union Pkg {

    uint8_t raw[68];
    struct {
        uint8_t  marker;
        uint16_t size : 6;
        uint16_t indx : 5;
        uint16_t type : 5;
        uint8_t  content[63];
        uint8_t  crc8;
    } data;
};

typedef union Pkg Pkg;

/*
 *  pkgread() - 
 *
 *  Reads data from a file into the package, handling special byte values.
 *
 *  @pkg: Pointer to the Pkg structure where the data will be stored.
 *  @fp : Pointer to the FILE object from which data will be read.
 *
 *  return:
 *    -  '1' if the data is successfully read and stored in the package.
 *    -  '0' if there is an error reading from the file.
 *    - '-1' if there was nothing left to read.
 */
extern int pkgread(Pkg* pkg, FILE* fp);

/*
 *  pkgrecv() -
 *
 *  Receives data into a package from a socket, with optional timeout.
 *
 *  @pkg    : Pointer to the Pkg structure where the received data will be
 *            stored.
 *  @sock   : File descriptor of the socket from which data will be received.
 *  @timeout: Timeout value in milliseconds for receiving data. If zero, no
 *            timeout is used.
 *
 *  return:
 *    - '1' if the data is successfully received.
 *    - '0' if there is an error receiving the data.
 */
extern int pkgrecv(Pkg* pkg, int sock, size_t timeout);

/*
 *  pkgsend() - 
 *
 *  Sends the raw data of the package over a socket.
 *
 *  @pkg : Pointer to the Pkg structure containing the data to be sent.
 *  @sock: File descriptor of the socket over which data will be sent.
 *
 *  return:
 *    - '1' if the data is successfully sent.
 *    - '0' if there is an error sending the data.
 */
extern int pkgsend(const Pkg* pkg, int sock);

/*
 *  pkgvalid() -
 *
 *  Validates the integrity of a package by calculating the CRC8 checksum
 *  and comparing it with the stored checksum in the package data.
 *
 *  @pkg: Pointer to the package structure to validate.
 *
 *  return:
 *    - '1' if the package is valid (checksum matches).
 *    - '0' if the package is invalid (checksum does not match).
 */
extern int pkgvalid(const Pkg* pkg);

/*
 *  pkginit() - 
 *
 *  Initializes a Pkg structure with provided values and data buffer.
 *
 *  @pkg : Pointer to the Pkg structure to be initialized.
 *  @size: Size of the data buffer.
 *  @indx: Index value to be set in the Pkg structure.
 *  @type: Type value to be set in the Pkg structure.
 *  @buf : Pointer to the data buffer to be copied into the 'Pkg' structure.
 */
extern void pkginit(Pkg* pkg, uint8_t size, uint8_t indx, int type, const uint8_t* buf);

/*
 *  pkg_rmv_sentinel_bytes() -
 *
 *  Removes sentinel bytes (0x81 and 0x88) from the package's 
 *  content.
 *
 *  @pkg: Pointer to the Pkg structure from which sentinel 
 *        bytes will be removed.
 */
extern void pkg_rmv_sentinel_bytes(Pkg* pkg);

#ifdef DEBUG

/*
 *  pkgprint() -
 *
 *  Prints the contents of a package for debugging purposes.
 *
 *  @pkg: Pointer to the 'Pkg' structure to be printed.
 */
extern void (pkgprint)(const Pkg* pkg);

#endif  /* DEBUG */

#endif  /* PKG_H */
