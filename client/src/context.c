
#include <sys/statvfs.h>
#include <sys/stat.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "context.h"
#include "pkg.defs.h"
#include "pkg.h"

/*
 *  context_create() - 
 *
 *  Allocates a new 'Context' structure.
 *
 *  return:
 *    - A pointer to the newly allocated 'Context' structure.
 *    - 'NULL' if the allocation fails.
 */
Context* context_create(void) {

    Context* ctx = calloc(1, sizeof *ctx);
    if(!ctx) {
        return NULL;
    }

    return ctx;
}

/*
 *  context_init_download() - 
 *
 *  Initializes the download context.
 *
 *  @ctx : Pointer to the context structure.
 *  @path: Path of the file to be downloaded.
 *
 *  return:
 *    - '1' if the context is successfully initialized.
 *    - '0' otherwise.
 */
static int context_init_download(Context* ctx, const char* path) {

    int ret;

    assert(ctx);
    assert(path);

    ret = 0;

    ctx->win.i = 1;
    ctx->desc.fp = fopen(path, "wb");
    if(ctx->desc.fp) {
        pkginit(&ctx->win.buf, strlen(path), 0, PKG_DOWNLOAD, (uint8_t*)path);
        ret = 1;
    }

    return ret;
}

/*
 *  context_init_ls() - 
 *
 *  Initializes the context for a list (ls) operation.
 *
 *  @ctx: Context to initialize.
 *
 *  return:
 *    - '0' on failure to initialize context.
 *    - '1' on successful initialization.
 */

static int context_init_ls(Context* ctx) {

    assert(ctx);

    ctx->win.i = 1;
    pkginit(
        &ctx->win.buf,
        0,
        0,
        PKG_LS,
        NULL
    );

    return 1;
}

/*
 *  context_init() -
 *
 *  Initializes the context based on the type and path.
 *
 *  @ctx : Pointer to the context structure to initialize.
 *  @type: Type of the context (e.g., download or list).
 *  @path: Path of the file to be downloaded (if applicable).
 *
 *  return:
 *    - '1' if the context is successfully initialized.
 *    - '0' if the context type is not recognized or if 
 *      initialization fails.
 */
int context_init(Context* ctx, CtxType type, const char* path) {

    assert(ctx);

    ctx->indx = 0;
    ctx->type = type;

    if(Download(type)) {
        return context_init_download(ctx, path);
    } else {
        if(Ls(type)) {
            return context_init_ls(ctx);
        }
    }

    return 0;
}

/*
 *  init_pkg_with_ack() -
 *
 *  Initializes a package with an acknowledgment (ACK) type.
 *
 *  @pkg: Pointer to the package to initialize.
 */
static inline void init_pkg_with_ack(Pkg* pkg) {
    
    pkginit(
        pkg,
        0,
        0,
        PKG_ACK,
        NULL
    );
}

/*
 *  init_pkg_with_nack() -
 *
 *  Initializes a package with a negative acknowledgment (NACK) type.
 *
 *  @pkg : Pointer to the package to initialize.
 *  @indx: Index to be included in the NACK package.
 */
static inline void init_pkg_with_nack(Pkg* pkg, size_t indx) {

    pkginit(
        pkg,
        0,
        (uint8_t)indx,
        PKG_NACK,
        NULL
    );
}

/*
 *  context_update_with_data() -
 *
 *  Updates the context with data from a received package.
 *
 *  @ctx: Pointer to the context structure.
 *  @pkg: Pointer to the received package.
 *
 *  return:
 *    - '1' if the context is successfully updated.
 *    - '0' if there was an error updating the context.
 */
static int context_update_with_data(Context* ctx, Pkg* pkg) {

    int nack;
    int valid;

    assert(ctx);
    assert(pkg);

    ctx->win.i = WINSZ;

    nack  = 1;
    valid = pkgvalid(pkg);
    if(valid) {
        if(pkg->data.indx == ctx->indx) {
            pkg_rmv_sentinel_bytes(pkg);
            ctx->recv += pkg->data.size;
            fwrite(pkg->data.content, pkg->data.size, 1, ctx->desc.fp);
            init_pkg_with_ack(&ctx->win.buf);
            incindx(ctx);
            nack = 0;
        } 
    }

    if(nack) {
        init_pkg_with_nack(&ctx->win.buf, ctx->indx);
        ctx->skip = 1;
    }


    return 1;
}

/*
 *  has_disk_space() -
 *
 *  Checks if there is enough disk space for a given size.
 *
 *  @size: Required size in bytes.
 *
 *  return:
 *    - '1' if there is enough disk space.
 *    - '0' if there is not enough disk space.
 */
static int has_disk_space(size_t size) {

    struct statvfs st;

    if(statvfs(".", &st) < 0) {
        return 0;
    }

    return st.f_bsize * st.f_bavail > size;
}

/*
 *  context_update_with_descriptor() -
 *
 *  Updates the context with a descriptor from a received package.
 *
 *  @ctx: Pointer to the context structure.
 *  @pkg: Pointer to the received package.
 *
 *  return:
 *    - '1' if the context is successfully updated.
 *    - '0' if there was an error updating the context.
 */
static int context_update_with_descriptor(Context* ctx, Pkg* pkg) {

    int ret;
    int valid;
    size_t size;

    assert(ctx);
    assert(pkg);


    ret = 0;
    size = 0;
    valid = pkgvalid(pkg);
    if(valid) {
        if(pkg->data.indx == ctx->indx) {
            size = (size_t)pkg->data.size;
            if(has_disk_space(size)) {
                init_pkg_with_ack(&ctx->win.buf);
                ret = 1;
                ctx->recv += size;
            }
        }
    }

    if(!ret) {
        init_pkg_with_nack(&ctx->win.buf, ctx->indx);
    }

    return ret;
}

/*
 *  context_update_with_show() -
 *
 *  Updates the context with a show package.
 *
 *  @ctx: Pointer to the context structure.
 *  @pkg: Pointer to the received package.
 *
 *  return:
 *    - '1' if the context is successfully updated.
 *    - '0' if there was an error updating the context.
 */
static int context_update_with_show(Context* ctx, Pkg* pkg) {

    int valid;
    char str[64];
    size_t size;

    assert(ctx);
    assert(pkg);

    valid = pkgvalid(pkg);
    if(valid) {

        size = pkg->data.size;
        if(pkg->data.indx == ctx->indx) {
            ctx->recv += size;
            init_pkg_with_ack(&ctx->win.buf);
            memcpy(str, pkg->data.content, size);
            str[size] = 0;
            printf(RED"- %s"RESET"\n", str);
            incindx(ctx);

            return 1;
        }
    }

    init_pkg_with_nack(pkg, ctx->indx);

    return 1;
}

/*
 *  context_update_with_meta_pkgs() -
 *
 *  Updates the context with meta packages.
 *
 *  @ctx: Pointer to the context structure.
 *  @pkg: Pointer to the received package.
 *
 *  return:
 *    - '1' if the context is successfully updated.
 *    - '0' if there was an error updating the context.
 */
static int context_update_with_meta_pkgs(Context* ctx, Pkg* pkg) {

    int valid;

    assert(ctx);
    assert(pkg);

    valid = pkgvalid(pkg);
    if(valid) {
        if(PkgEnd(pkg)) {
            return ctx->completed = 1;
        }
    }

    return 0;
}

/*
 *  context_download_update() -
 *
 *  Updates the download context with a received package.
 *
 *  @ctx: Pointer to the context structure.
 *  @pkg: Pointer to the received package.
 *
 *  return:
 *    - '1' if the context is successfully updated.
 *    - '0' if there was an error updating the context.
 */
static int context_download_update(Context* ctx, Pkg* pkg) {

    assert(ctx);
    assert(pkg);

    if(PkgData(pkg)) {
        return context_update_with_data(ctx, pkg);
    } else {
        if(PkgDescriptor(pkg)) {
            return context_update_with_descriptor(ctx, pkg);
        } 
    }

    return context_update_with_meta_pkgs(ctx, pkg);
}

/*
 *  context_ls_update() -
 *
 *  Updates the list (ls) context with a received package.
 *
 *  @ctx: Pointer to the context structure.
 *  @pkg: Pointer to the received package.
 *
 *  return:
 *    - '1' if the context is successfully updated.
 *    - '0' if there was an error updating the context.
 */
static int context_ls_update(Context* ctx, Pkg* pkg) {

    assert(ctx);
    assert(pkg);

    if(PkgShow(pkg)) {
        return context_update_with_show(ctx, pkg);
    }

    return context_update_with_meta_pkgs(ctx, pkg);
}

/*
 *  context_update() - 
 *
 *  Updates the context with a received package.
 *
 *  @ctx: Pointer to the context structure.
 *  @pkg: Pointer to the received package.
 *
 *  return:
 *    - '1' if the context is successfully updated.
 *    - '0' otherwise.
 */
int context_update(Context* ctx, Pkg* pkg) {

    assert(ctx);
    assert(pkg);

    if(CtxDownload(ctx)) {
        return context_download_update(ctx, pkg);
    } else {
        if(CtxLs(ctx)) {
            return context_ls_update(ctx, pkg);
        }
    }

    return 0;
}

/*
 *  context_deinit_download() - 
 *
 *  Deinitializes the download context by closing 
 *  the file descriptor.
 *
 *  @ctx: Pointer to the Context structure that needs 
 *        to be deinitialized.
 */
static inline void context_deinit_download(Context* ctx) {

    if(ctx && ctx->desc.fp) {
        fclose(ctx->desc.fp);
    }
}

/*
 *  context_deinit() - 
 *
 *  Deinitializes the context based on its type.
 *
 *  @ctx: Pointer to the Context structure that 
 *        needs to be deinitialized.
 */
void context_deinit(Context* ctx) {

    if(ctx) {
        if(CtxDownload(ctx)) {
            context_deinit_download(ctx);
        } 
    }
}

/*
 *  context_free() - 
 *
 *  Deinitializes and frees the context structure.
 *
 *  @ctx: Double pointer to the Context structure that 
 *        needs to be deinitialized and freed.
 */
void context_free(Context** ctx) {

    if(ctx) {
        context_deinit(*ctx);
        free(*ctx);
        *ctx = NULL;
    }
}

