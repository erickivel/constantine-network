
#include <linux/if_packet.h>
#include <net/ethernet.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "socket.h"

/*
 *  sockaddr_ll_init() - 
 *
 *  Initializes a sockaddr_ll structure.
 *
 *  @addr   : Pointer to the sockaddr_ll structure to initialize.
 *  @ifindex: Interface index.
 */
static inline void sockaddr_ll_init(struct sockaddr_ll* addr, int ifindex) {

    addr->sll_family = AF_PACKET;
    addr->sll_protocol = htons(ETH_P_ALL);
    addr->sll_ifindex = ifindex;
}

/*
 *  packet_mreq_init() - 
 *
 *  Initializes a packet_mreq structure.
 *
 *  @mreq   : Pointer to the packet_mreq structure to initialize.
 *  @ifindex: Interface index.
 */
static inline void packet_mreq_init(struct packet_mreq* mreq, int ifindex) {

    mreq->mr_ifindex = ifindex;
    mreq->mr_type = PACKET_MR_PROMISC;
}

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
int socket_create(const char* interface) {

    int sockfd;
    int ifindex;
    struct sockaddr_ll addr;
    struct packet_mreq mreq;

    assert(interface);

    memset(&addr, 0, sizeof addr);
    memset(&mreq, 0, sizeof mreq);

    ifindex = if_nametoindex(interface); 
    sockfd  = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if(sockfd < 0) {
        return -1;
    }

    sockaddr_ll_init(&addr, ifindex);
    packet_mreq_init(&mreq, ifindex);

    if(bind(sockfd, (struct sockaddr*)&addr, sizeof addr) < 0) {
        close(sockfd);
        return -1;
    }

    if(setsockopt(sockfd, SOL_PACKET, PACKET_ADD_MEMBERSHIP, &mreq, sizeof mreq) < 0) {
        close(sockfd);
        return -1;
    }

    return sockfd;
}

/*
 *  socket_close() - 
 *
 *  Closes the specified socket file descriptor.
 *
 *  @sockfd: File descriptor of the socket to close.
 */
void socket_close(int sockfd) {

    if(sockfd >= 0) {
        close(sockfd);
    }
}
