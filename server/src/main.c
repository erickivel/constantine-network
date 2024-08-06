
#include <assert.h>
#include <stdlib.h>

#include "context.h"
#include "socket.h"

#define TIMEOUT 5000
#define DELTA   40

#define ERROR_MSG       "Invalid Operation."
#define ERROR_MSG_SIZE  sizeof ERROR_MSG

/*
 *  usage() -
 *
 *  Prints the usage information for the program, 
 *  showing how to use it.
 *
 *  @exec: The name of the executable.
 */
static void usage(const char* exec) {

    printf(
        "usage: %s <network-interface>\n",
        exec
    );
}

/*
 *  sendwin() -
 *
 *  Sends the packages stored in the window buffer over the 
 *  specified socket.
 *
 *  @ctx : Pointer to the 'Context' structure containing the window buffer.
 *  @sock: Socket file descriptor to send the packages over.
 */
static inline void sendwin(const Context* ctx, int sock) {

    size_t i;

    assert(ctx);

    for(i = 0; i < ctx->win.i; i++)  {
        debug("sending package %zu.\n", (size_t)ctx->win.buf[i].data.indx);
        pkgsend(&ctx->win.buf[i], sock);
    }
}

/*
 *  process_context_end() -
 *
 *  Handles the finalization of the context by sending 
 *  'end' or 'error' packages
 *
 *
 *  @ctx : Pointer to the 'Context' structure.
 *  @sock: Socket file descriptor.
 */
static void process_context_end(Context* ctx, int sock, PkgType type) {

    char*  msg;
    char*  tpe;
    size_t count;
    size_t size;

    Pkg pkg;
    Pkg snd;

    assert(ctx);

    msg  = NULL;
    tpe  = "end";
    size = 0;
    if(type == PKG_ERROR) {
        msg  = ERROR_MSG;
        size = ERROR_MSG_SIZE;
        tpe  = "error";
    }

    pkginit(&snd, size, 0, type, (uint8_t*)msg);
    count = 0;
    for(; count < DELTA;) {
        debug("sending %s.\n", tpe);
        pkgsend(&snd, sock);
        if(pkgrecv(&pkg, sock, TIMEOUT) && pkgvalid(&pkg)) {
            if(PkgAck(&pkg)) {
                break;
            }
        }
        count++;
    }
}

/*
 *  process_context() -
 *
 *  Handles the context processing loop by sending windowed data and receiving
 *  packages, updating the context accordingly, and handling the context end
 *  condition.
 *
 *  @ctx : Pointer to the 'Context' structure.
 *  @sock: Socket file descriptor.
 */
static void process_context(Context* ctx, int sock) {

    Pkg pkg;

    assert(ctx);

    for(;;) {
        sendwin(ctx, sock);
        if(pkgrecv(&pkg, sock, TIMEOUT)) {
            debug("package received.\n");
            if(pkgvalid(&pkg)) {
                debug("valid package received.\n");
                context_update(ctx, &pkg); 
                if(CtxCompleted(ctx)) {
                    debug("finalizing context.\n");
                    process_context_end(ctx, sock, PKG_END);
                    break;
                }
            } 
        }
    }

    debug("context completed: %zu bytes sent.\n", ctx->sent);
}

int main(int argc, char** argv) {

    int sock;
    Pkg pkg;
    Context* ctx;

    if(argc < 2) {
        usage(argv[0]);
        exit(1);
    }

    sock = socket_create(argv[1]);
    if(sock < 0) {
        perror("error - failed to open socket");
        return 1;
    }

    for(;;) {
        pkgrecv(&pkg, sock, 0);
        if(pkgvalid(&pkg) && iscontext(&pkg)) {
            ctx = context_create();
            if(ctx) {
                debug("context created.\n");
                if(context_init(ctx, &pkg)) {
                    debug("context initialized... sending ack.\n");
                    pkgsend_ack(sock);
                    process_context(ctx, sock);
                } else {
                    process_context_end(ctx, sock, PKG_ERROR);
                }
            }
            context_free(&ctx);
            memset(&pkg, 0, sizeof pkg);
        }
    }

    return 0;
}
