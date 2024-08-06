#ifndef SOCKET_H
#define SOCKET_H

/*
 *  socket_create() - 
 *
 *  Creates a raw socket and sets it to promiscuous mode.
 *
 *  @interface: Name of the network interface.
 *
 *  return:
 *    - File descriptor of the created socket on success.
 *    - '-1' on failure.
 */
extern int socket_create(const char* interface);

/*
 *  socket_close() - 
 *
 *  Closes the specified socket file descriptor.
 *
 *  @sockfd: File descriptor of the socket to close.
 */
extern void socket_close(int sockfd);

#endif
