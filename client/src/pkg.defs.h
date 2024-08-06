#ifndef PKG_DEFS_H
#define PKG_DEFS_H

#include <string.h>

#ifdef DEBUG
#   define pkgprint(pkg) (pkgprint)(pkg);
#else
#   define pkgprint(pkg) (void)0
#endif  /* DEBUG */

#define PKG_MAX_IND 32
#define PKG_MARKER  0x7E

/*
 *  pkgsend_ack() -
 *
 *  Sends an acknowledgment package through the specified socket.
 *
 *  @sock: File descriptor of the socket through which the 
 *         acknowledgment package will be sent.
 */
#define pkgsend_ack(sock)                                           \
    do {                                                            \
        Pkg pa;                                                     \
        memset(&pa, 0, sizeof pa);                                  \
        pkginit(&pa, 0, 0, PKG_ACK, NULL);                          \
        pkgsend(&pa, sock);                                         \
    } while(0)

/*
 *  pkgsend_nack() -
 *
 *  Sends a 'NACK' package through the specified socket.
 *
 *  @sock: File descriptor of the socket through which the 
 *         'NACK' package will be sent.
 */
#define pkgsend_nack(sock)                                          \
    do {                                                            \
        Pkg pn;                                                     \
        memset(&pn, 0, sizeof pn);                                  \
        pkginit(&pn, 0, 0, PKG_ACK, NULL);                          \
        pkgsend(&pn, sock);                                         \
    } while(0)

/*
 *
 *  pkgsend_end() -
 *  
 *  Sends an 'end' package through the specified socket.
 *
 *  @sock: File descriptor of the socket through which the 
 *         'end' package will be sent.
 */
#define pkgsend_end(sock)                                           \
    do {                                                            \
        Pkg pe;                                                     \
        memset(&pe, 0, sizeof pe);                                  \
        pkginit(&pe, 0, 0, PKG_END, NULL);                          \
        pkgsend(&pe, sock);                                         \
    } while(0)

/*
 *
 *  pkgsend_error() -
 *  
 *  Sends an 'error' package through the specified socket.
 *
 *  @sock: File descriptor of the socket through which the 
 *         'error' package will be sent.
 */
#define pkgsend_error(sock)                                         \
    do {                                                            \
        Pkg pe;                                                     \
        char buf[] = "Invalid Operation";                           \
        memset(&pe, 0, sizeof pe);                                  \
        pkginit(&pe, sizeof buf, 0, PKG_END, (uint8_t*)buf);        \
        pkgsend(&pe, sock);                                         \
    } while(0)


#define PkgAck(pkg)         ((pkg)->data.type == PKG_ACK)
#define PkgEnd(pkg)         ((pkg)->data.type == PKG_END)
#define PkgNack(pkg)        ((pkg)->data.type == PKG_NACK)
#define PkgError(pkg)       ((pkg)->data.type == PKG_ERROR)
#define PkgData(pkg)        ((pkg)->data.type == PKG_DATA)
#define PkgShow(pkg)        ((pkg)->data.type == PKG_SHOW)
#define PkgDownload(pkg)    ((pkg)->data.type == PKG_DOWNLOAD)
#define PkgDescriptor(pkg)  ((pkg)->data.type == PKG_DESCRIPTOR)
#define PkgLs(pkg)          ((pkg)->data.type == PKG_LS)
#define PkgIndx(pkg)        ((pkg)->data.indx)

#define iscontext(pkg)      ((pkg)->data.type == PKG_LS || (pkg)->data.type == PKG_DOWNLOAD)

#endif  /* PKG_DEFS_H */
