
#include <sys/socket.h>
#include <sys/time.h>
#include <assert.h>
#include <unistd.h>

#include "pkg.h"
#include "pkg.defs.h"

/*
 *  add_byte_to_pkg() -
 *
 *  Adds a byte to the package at the specified index, with special 
 *  handling for certain byte values.
 *
 *  @pkg : Pointer to the Pkg structure where the byte will be added.
 *  @byte: Byte value to add to the package.
 */
static void add_byte_to_pkg(Pkg* pkg, uint8_t byte) {

    size_t i;

    assert(pkg);

    i = pkg->data.size;
    pkg->data.content[i] = byte;
    if(byte == 0x81 || byte == 0x88) {
        if(i + 1 < sizeof pkg->data.content) {
            pkg->data.content[i + 1] = 0xff;
            i++;
        }
    }
    pkg->data.size = ++i;
}

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
int pkgread(Pkg* pkg, FILE* fp) {

    int ret;
    uint8_t byte;

    assert(pkg);
    assert(fp);
   
    ret = 1;
    pkg->data.size = 0;
    for(; pkg->data.size < sizeof pkg->data.content;) {
        if(fread(&byte, sizeof byte, 1, fp) != 1) {
            if(feof(fp)) {
                ret = -1;
                break;
            }
        }
        add_byte_to_pkg(pkg, byte);
    }

    return ret;
}

/*
 *  ispkg() - 
 *
 *  Checks if a package contains a valid marker.
 *
 *  @pkg: Pointer to the Pkg structure to be checked.
 *
 *  return:
 *    - '1' if the package contains a valid marker.
 *    - '0' if the package does not contain a valid marker.
 */
int ispkg(const Pkg* pkg) {

    assert(pkg);
    return pkg->data.marker == PKG_MARKER;
}

/*
 *  timestamp() -
 *
 *  Gets the current timestamp in milliseconds.
 *
 *  return:
 *    - The current timestamp in milliseconds since the Epoch.
 */
static inline size_t timestamp(void) {
    
    struct timeval t;

    gettimeofday(&t, NULL);
    return t.tv_sec * 1000 + t.tv_usec / 1000;
}

/*
 *  settimeout() -
 *
 *  Sets the receive timeout option for a socket.
 *
 *  @sock   : The socket file descriptor on which to set the timeout.
 *  @timeout: The timeout value in milliseconds. If 'timeout' 
 *            is zero, the timeout will be disabled.
 */
static inline void settimeout(int sock, size_t timeout) {

    struct timeval tval;

    memset(&tval, 0, sizeof tval);
    if(timeout) {
        tval.tv_sec  = timeout / 1000;
        tval.tv_usec = (timeout % 1000) * 1000;
    }
    
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tval, sizeof tval);
}

/*
 *  pkgrecv_timeout() -
 *
 *  Receives data into a package from a socket with a specified timeout.
 *
 *  @pkg    : Pointer to the Pkg structure where the received data will be
 *            stored.
 *  @sock   : File descriptor of the socket from which data will be received.
 *  @timeout: Timeout value in milliseconds for receiving data.
 *
 *  return:
 *    - '1' if the data is successfully received within the timeout period.
 *    - '0' if there is an error or if the timeout period expires before
 *      receiving the data.
 */
static int pkgrecv_timeout(Pkg* pkg, int sock, size_t timeout) {

    int ret;
    int end;
    size_t start;

    assert(pkg);

    ret = 0;
    end = 0;

    start = timestamp();
    settimeout(sock, timeout);
    for(; !end ;) {
        if(recv(sock, pkg->raw, sizeof pkg->raw, 0) > 0) {
            if(ispkg(pkg)) {
                ret = 1;
                break;
            }
        }
        end = timestamp() - start > timeout;
    }
    settimeout(sock, 0);

    return ret;
}
/*
 *  pkgrecv_notimeout() - 
 *
 *  Receives data into a package from a socket without using a timeout.
 *
 *  @pkg   : Pointer to the Pkg structure where the received data will be stored.
 *  @sockfd: File descriptor of the socket from which data will be received.
 *
 *  return:
 *    - '1' if the data receive refers to a pkg.
 *    - '0' otherwise.
 */
static int pkgrecv_notimeout(Pkg* pkg, int sockfd) {

    assert(pkg);
    recv(sockfd, pkg->raw, sizeof pkg->raw, 0);

    return 1; 
}

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
int pkgrecv(Pkg* pkg, int sock, size_t timeout) {

    assert(pkg);
    if(timeout) {
        return pkgrecv_timeout(pkg, sock, timeout);
    }

    return pkgrecv_notimeout(pkg, sock);
}

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
int pkgsend(const Pkg* pkg, int sockfd) {

    assert(pkg);

    if(send(sockfd, pkg->raw, sizeof pkg->raw, 0) < 0) {
        return 0;
    }

    return 1;
}

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
int pkgvalid(const Pkg* pkg) {

    uint8_t crc;

    assert(pkg);

    crc8(pkg->raw + sizeof pkg->data.marker, 2 + pkg->data.size, &crc);
    return crc == pkg->data.crc8;
}

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
void pkginit(Pkg* pkg, uint8_t size, uint8_t indx, int type, const uint8_t* buf) {

    size_t i;

    assert(pkg);
    memset(pkg, 0, sizeof *pkg);

    pkg->data.marker = PKG_MARKER;
    pkg->data.size   = 0;
    pkg->data.indx   = indx;
    pkg->data.type   = type;
    
    if(buf) {
        for(i = 0; i < size; i++) {
            add_byte_to_pkg(pkg, buf[i]);
        }
        pkg->data.size++;
    }

    crc8(pkg->raw + sizeof pkg->data.marker, 2 + pkg->data.size, &pkg->data.crc8);
}

/*
 *  pkg_rmv_sentinel_bytes() -
 *
 *  Removes sentinel bytes (0x81 and 0x88) from the package's 
 *  content.
 *
 *  @pkg: Pointer to the Pkg structure from which sentinel 
 *        bytes will be removed.
 */
void pkg_rmv_sentinel_bytes(Pkg* pkg) {

    int flag;
    size_t i;
    size_t n;
    size_t s;
    uint8_t byte;
    uint8_t buf[64];
    
    assert(pkg);

    flag = 0;

    s = 0;
    n = pkg->data.size;
    for(i = 0; i < n; i++) {
        byte = pkg->data.content[i];
        if(!flag) {
            if(byte == 0x81 || byte == 0x88) {
                flag = 1;
            }
            buf[s] = byte;
            s++;
        } else {
            flag = 0;
        }
    }

    pkg->data.size = (uint8_t)s;
    memcpy(pkg->data.content, buf, pkg->data.size);
}

#ifdef DEBUG

/*
 *  pkgprint() -
 *
 *  Prints the contents of a package for debugging purposes.
 *
 *  @pkg: Pointer to the 'Pkg' structure to be printed.
 */
void (pkgprint)(const Pkg* pkg) {

    size_t i;

    assert(pkg);
    
    debug("%x ", pkg->data.marker);
    debug("%x ", pkg->data.size);
    debug("%x ", pkg->data.indx);
    debug("%x ", pkg->data.type);

    for(i = 0; i < pkg->data.size; i++) {
        debug("%x ", pkg->data.content[i]);
    }

    debug("%x\n", pkg->data.crc8);
}

#endif  /* DEBUG */
