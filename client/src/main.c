
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>

#include "context.h"
#include "socket.h"

#define TIMEOUT 0
#define DELTA   40

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
        "usage:\n"
        "%s --i <network-interface> --list\n"
        "%s --i <network-interface> --download <name>\n"
        "%s --i <network-intergace> --download <name> --exec <executable>\n",
        exec,
        exec,
        exec
    );
}

/*
 *  parse_args() -
 *
 *  Parses command-line arguments and sets the context type, network interface, 
 *  and file path parameters.
 *
 *  @argc: Number of arguments passed on the command line.
 *  @argv: List of arguments passed on the command line.
 *  @type: Pointer to store the context type (list or download).
 *  @intf: Pointer to store the network interface.
 *  @path: Pointer to store the file path to be downloaded.
 *  @exec: Pointer to store the executable's name.
 *
 *  return:
 *    - '1' if the arguments were parsed correctly.
 *    - '0' if there was an error parsing the arguments.
 */
static int parse_args(int argc, char** argv, CtxType* type, char** intf, char** path, char** exec) {

    int ctx;
    int infc;
    int i;

    assert(argv);
    assert(type);
    assert(path);
    assert(intf);

    ctx = 0;
    infc = 0;
    for(i = 1; i < argc; i++) {
        if(!strcmp(argv[i], "--i")) {
            if(i + 1 < argc) {
                *intf = argv[i + 1];
                infc = 1;
                i++;
            } else {
                return 0;
            }
        } else {

            if(!strcmp(argv[i], "--list")) {
                if(!ctx) {
                    *type = CTX_LS;
                    ctx = 1;
                    continue;
                }
                return 0;
            } else {

                if(!strcmp(argv[i], "--download")) {
                    if(!ctx) {
                        *type = CTX_DOWNLOAD;
                        ctx = 1;
                        if(i + 1 < argc) {
                            *path = argv[i + 1];
                            i++;
                            continue;
                        }
                        return 0;
                    }
                } else {

                    if(!strcmp(argv[i], "--exec")) {
                        if(i + 1 < argc) {
                            *exec = argv[i + 1];
                            i++;
                            continue;
                        } 
                        return 0;
                    } else {
                        return 0;
                    }
                }
            }
        }

    }

    return infc && ctx;
}

/*
 *  runapp() -
 *
 *  Executes a command by combining the executable name and 
 *  its argument into a single command string.
 *
 *  @exec: The name or path of the executable.
 *  @arg: The argument to pass to the executable.
 *
 *  return:
 *    - '1' if the command is successfully executed.
 *    - '0' if there is an error in creating or executing the command.
 */
static int runapp(const char* exec, const char* arg) {

    int ret;
    size_t esz;
    size_t asz;
    char* command;

    assert(exec);

    ret = 0;
    if(arg) {

        esz = strlen(exec);
        asz = strlen(arg);
        command = calloc(1, esz + asz + 2);

        if(command) {
            memcpy(command, exec, esz);
            memcpy(command + esz + 1, arg, asz);
            command[esz] = ' ';
            if(!system(command)) {
                ret = 1;
            }

            free(command);
        }
    }

    return ret;
}

/*
 *  process_error() - 
 *
 *  Processes an error package.
 *
 *  @pkg: The package containing the error message.
 *  @sock: The socket descriptor.
 */
static void process_error(const Pkg* pkg, int sock) {

    char str[64];
    size_t n;

    assert(pkg);

    n = pkg->data.size;
    str[n] = 0;
    memcpy(str, pkg->data.content, n);
    printf(RED"%s"RESET"\n", str);

    pkgsend_ack(sock);
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

    size_t i;
    size_t count;
    Pkg pkg;

    assert(ctx);

    count = 0;
    for(;;) {
        for(i = 0; i < ctx->win.i; i++) {
            if(pkgrecv(&pkg, sock, TIMEOUT) && ispkg(&pkg)) {
                count++;   
                if(!ctx->skip) {
                    debug("received package %zu.\n", (size_t)pkg.data.indx);
                    context_update(ctx, &pkg);
                    if(CtxCompleted(ctx)) {
                        debug("finalizing context.\n");
                        pkgsend_ack(sock);
                        goto _end;
                    } else {
                        if(ctx->ack) {
                            i = 0;
                            ctx->ack = 0;
                        } 
                    }
                } 
            }

            if(count == ctx->win.i) {
                debug("sending response.\n");
                pkgsend(&ctx->win.buf, sock);
                ctx->skip = 0;
                count = 0;
            }
        }
    }

_end:
    debug("context completed: %zu packages received.\n", ctx->k);

}

int main(int argc, char** argv) {

    int sock;
    char* path;
    char* exec;
    char* intf;

    Pkg pkg;
    CtxType type;
    Context* ctx;

    exec = NULL;
    if(!parse_args(argc, argv, &type, &intf, &path, &exec)) {
        usage(argv[0]);
        exit(1);
    }

    sock = socket_create(intf);
    if(sock < 0) {
        perror("error - failed to open socket");
        return 1;
    }

    ctx = context_create();
    if(ctx && context_init(ctx, type, path)) {
        for(;;) {
            pkgsend(&ctx->win.buf, sock);
            if(pkgrecv(&pkg, sock, 0) && pkgvalid(&pkg)) {
                if(PkgAck(&pkg)) {
                    process_context(ctx, sock);
                    break;
                } else {
                    if(PkgError(&pkg)) {
                        process_error(&pkg, sock);
                        exec = NULL;
                        break;
                    }
                }
            }
        }
    }

    context_free(&ctx);
    socket_close(sock);

    if(exec) {
        if(!runapp(exec, path)) {
            perror("error");
        }
    }

    return 0;
}
