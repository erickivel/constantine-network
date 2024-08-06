
#include <sys/stat.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "context.h"
#include "pkg.defs.h"
#include "pkg.h"

static int context_ls_update_with_ack(Context*);

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

    errno = 0;

    Context* ctx = calloc(1, sizeof *ctx);
    if(!ctx) {
        return NULL;
    }

    return ctx;
}

/*
 *  get_file_size() -
 *
 *  Retrieves the size of the file specified by the path.
 *
 *  @path: The path to the file.
 *  @size: Pointer to a variable where the size of the file will be stored.
 *
 *  return:
 *    - '1' if the file size was successfully retrieved.
 *    - '0' if there was an error retrieving the file size.
 */
static inline int get_file_size(const char* path, size_t* size) {

    size_t ret;
    struct stat st;

    assert(path);

    ret = 0;
    if(!stat(path, &st)) {
        ret = 1;
        *size = st.st_size;
    }

    return ret;
}

/*
 *  init_download_initial_response() -
 *
 *  Initializes the initial response for a download by setting up the context
 *  window buffer with the size of the file.
 *
 *  @ctx   : Pointer to the Context structure that holds the state and buffer for
 *           the download.
 *  @path  : String containing the path to the file whose size will be used to
 *           initialize the response.
 *
 *  return:
 *    - '1' if the initialization is successful and the file size is obtained.
 *    - '0' if there is an error obtaining the file size.
 */
static int download_initial_response(Context* ctx, const char* path) {

    int ret;
    size_t size;
    
    assert(ctx);
    assert(path);

    ret = get_file_size(path, &size); 
    if(ret) {
        pkginit(
            &ctx->win.buf[0],
            sizeof size,
            0,
            PKG_DESCRIPTOR,
            (uint8_t*)&size
        );
    }

    return ret;
}

/*
 *  context_init_download() - 
 *
 *  Initializes the download context.
 *
 *  @ctx: Pointer to the Context structure to be initialized.
 *  @pkg: Pointer to the Pkg structure containing the package data.
 *
 *  return:
 *    - '1' if the context is successfully initialized
 *    - '0' if the initialization fails (e.g., file opening fails)
 */
static int context_init_download(Context* ctx, const Pkg* pkg) {

    int ret;
    char* asset;

    assert(ctx);
    assert(pkg);

    ret   = 0;
    asset = NULL;

    ctx->win.i = 1;
    ctx->type  = CTX_DOWNLOAD;

    asset = get_asset_path((char*)pkg->data.content, pkg->data.size);
    if(asset) {
        ctx->desc.fp = fopen(asset, "rb");
        if(ctx->desc.fp) {
            ret = download_initial_response(ctx, asset);
            if(!ret) {
                fclose(ctx->desc.fp);
                ctx->desc.fp = NULL;
            }
        }
    }

    free(asset);

    return ret;
}

/*
 *  ls_initial_response() - 
 *
 *  Initializes the initial response package for a list operation.
 *  This function is actually just  wrapper to context_update_with_ack().
 *
 *  @ctx: Context to initialize.
 *
 *  return:
 *    - '1' on success.
 *    - '0' on failure.
 */
static inline int ls_initial_response(Context* ctx) {

    return context_ls_update_with_ack(ctx);
}

/*
 *  context_init_ls() - 
 *
 *  Initializes the context for a list (ls) operation.
 *
 *  @ctx: Context to initialize.
 *  @pkg: Package containing the initial request.
 *
 *  return:
 *    - '0' on failure to initialize context.
 *    - '1' on successful initialization.
 */
static int context_init_ls(Context* ctx, const Pkg* pkg) {

    int ret;

    assert(ctx);
    assert(pkg);

    ret = 0;

    ctx->win.i = 1;
    ctx->type  = CTX_LS;

    ctx->desc.dp = get_assets_dir();
    if(ctx->desc.dp) {
        ret = ls_initial_response(ctx);
        if(!ret) {
            closedir(ctx->desc.dp);
            ctx->desc.dp = NULL;
        }
    } 

    return ret;
}

/*
 *  context_init() -
 *
 *  Initializes the context based on the package type.
 *
 *  @ctx: Pointer to the Context structure that will be initialized.
 *  @pkg: Pointer to the constant Pkg structure that contains 
 *        initialization data.
 *
 *  return:
 *    - '1' if the context is successfully initialized.
 *    - '0' if the context type is not recognized or if no 
 *      initialization is required.
 */
int context_init(Context* ctx, const Pkg* pkg) {

    assert(ctx);
    assert(pkg);

    ctx->indx = 0;

    if(PkgDownload(pkg)) {
        return context_init_download(ctx, pkg);
    } else {
        if(PkgLs(pkg)) {
            return context_init_ls(ctx, pkg);
        }
    }

    return 0;
}

/*
 *  initpkg_data_meta() -
 *
 *  Initializes the metadata for a data package, including setting the marker,
 *  type, and index, and calculating the CRC8 checksum.
 *
 *  @pkg : Pointer to the 'Pkg' structure to initialize.
 *  @indx: Index value to set in the package metadata.
 */
static inline void initpkg_data_meta(Pkg* pkg, size_t indx) {

    assert(pkg);

    pkg->data.marker = PKG_MARKER;
    pkg->data.type   = PKG_DATA;
    pkg->data.indx   = indx;

    crc8(
        pkg->raw + sizeof pkg->data.marker,
        pkg->data.size + 2,
        &pkg->data.crc8
    );
}

/*
 *  fill_context_buf_from_index() -
 *
 *  Fills the context buffer by reading data from a file and initializing the
 *  package structures. It handles the wrapping of index values and prepares the
 *  buffer for processing.
 *
 *  @ctx: Pointer to the 'Context' structure.
 *  @i  : Starting index to fill the buffer.
 *
 *  return:
 *    - '1' if the buffer was successfully filled and initialized.
 *    - '0' if there was an error reading from the file.
 *    - '-1' if the end of the file was reached and the buffer was finalized.
 */
static int fill_context_buf_from_index(Context* ctx, size_t i) {

    int ret;

    assert(ctx);

    ret = 1;
    for(; i < WINSZ && ret > 0; i++) {

        ret = pkgread(&ctx->win.buf[i], ctx->desc.fp);
        if(ret) {

            ctx->sent += ctx->win.buf[i].data.size;
            initpkg_data_meta(&ctx->win.buf[i], ctx->indx);
            incindx(ctx);
        }
    }

    ctx->win.i = i;
    if(ret < 0) {
        ctx->end = 1;
    }

    return ret;
}

/*
 *  context_download_update_with_ack() -
 *
 *  Reads data from a file descriptor into the window buffer and initializes the
 *  package structure. It updates the context index and prepares the buffer for
 *  processing.
 *
 *  @ctx: Pointer to the 'Context' structure.
 *
 *  return:
 *    - '1' if the data was successfully read and the buffer was initialized.
 *    - '0' if there was an error reading from the file.
 */
static int context_download_update_with_ack(Context* ctx) {

    assert(ctx);

    return fill_context_buf_from_index(ctx, 0);
}

/*
 *  context_ls_update_with_ack() -
 *
 *  Reads the next directory entry from 'ctx->descriptor.dp' and initializes a
 *  package structure with the entry's name. Updates the context index for
 *  tracking directory entries.
 *
 *  @ctx: Pointer to the 'Context' structure managing the directory listing.
 *
 *  return:
 *    -  '1' if an entry was successfully read and initialized.
 *    - '-1' if there was an error or no more entries in the directory.
 */
static int context_ls_update_with_ack(Context* ctx) {

    int ret;
    char* fname;
    size_t size;
    struct dirent* entry;

    assert(ctx);
    
    ret  = -1;
    while((entry = readdir(ctx->desc.dp))) {
        if(entry->d_type != DT_DIR) {
            fname = entry->d_name;
            size  = strlen(fname);
            pkginit(&ctx->win.buf[0], size, ctx->indx, PKG_SHOW, (uint8_t*)fname);
            ctx->sent += size;
            ret = 1;
            break;
        }
    }

    incindx(ctx);
    if(ret < 0) {
        ctx->end = 1;
        //ctx->completed = 1;
    }

    return ret;
}

/*
 *  context_update_with_ack() -
 *
 *  Updates the context based on the current state by either downloading data or
 *  listing data, and then processes the data accordingly with acknowledgment.
 *
 *  @ctx: Pointer to the Context structure containing the current state and data
 *  to be updated.
 *
 *  return:
 *    -  '1' if the context was successfully updated and processed.
 *    -  '0' if there was an error in updating or processing the context.
 *    - '-1' if the context was finalized.
 */
static int context_update_with_ack(Context* ctx) {

    assert(ctx);

    if(CtxEnd(ctx)) {
        ctx->completed = 1;
        return -1;
    }

    if(CtxDownload(ctx)) {
        return context_download_update_with_ack(ctx);
    } else {
        if(CtxLs(ctx)) {
            return context_ls_update_with_ack(ctx);
        } 
    }

    return 0;
}

/*
 *  find_nack_pkg() -
 *
 *  This function searches for a package with a specific index (nack) in the
 *  context's window buffer and returns the position of that package.
 *
 *  @ctx : Pointer to the 'Context' structure containing the window buffer.
 *  @nack: The index of the package to search for.
 *  @indx: Pointer to a size_t variable where the function will store the
 *         position of the package if found.
 *
 *  return:
 *    - '1' if the package with the specified index is found.
 *    - '0' if the package with the specified index is not found.
 */
static int find_nack_pkg(const Context* ctx, size_t nack, size_t* indx) {

    int found;
    size_t i;
    
    assert(ctx);

    found = 0;
    for(i = 0; i < WINSZ; i++) {
        if(ctx->win.buf[i].data.indx == nack) {
            found = 1;
            break;
        }
    }

    if(found && indx) {
        *indx = i;
    }

    return found;
}

/*
 *  context_download_update_with_nack() -
 *
 *  Handles a negative acknowledgment (NACK) for a download context 
 *  by adjusting the buffer and refilling it with the necessary data.
 *
 *  @ctx: Pointer to the 'Context' structure.
 *  @pkg: Pointer to the received 'Pkg' structure.
 *
 *  return:
 *    -  '1' if the buffer was successfully adjusted and refilled.
 *    -  '0' if the context cannot be updated.
 *    - '-1' if the context was finalized.
 */
static int context_download_update_with_nack(Context* ctx, const Pkg* pkg) {

    int found;
    size_t failed;
    size_t indx;

    assert(ctx);
    assert(pkg);

    indx  = (size_t)pkg->data.indx;
    found = find_nack_pkg(ctx, indx, &failed);
    if(found) {
        if(failed > 0) {
            memmove(&ctx->win.buf[0], &ctx->win.buf[failed], (ctx->win.i - failed) * sizeof ctx->win.buf[0]);
            if(ctx->end) {
                ctx->win.i = ctx->win.i - failed;
                return 1;
            }

            return fill_context_buf_from_index(ctx, ctx->win.i - failed);
        }
        return 1;
    }

    /*
     *  If the nack package is not in the context buffer, then
     *  it was not sent yet.
     */
    return fill_context_buf_from_index(ctx, 0);
}

/*
 *  context_update_with_nack() -
 *
 *  Handles a negative acknowledgment (NACK) by calling the 
 *  appropriate context-specific update function.
 *
 *  @ctx: Pointer to the 'Context' structure.
 *  @pkg: Pointer to the received 'Pkg' structure.
 *
 *  return:
 *    - '1' if the context was successfully updated.
 *    - '0' if the context cannot be updated.
 *    - '-1' if the context was finalized.
 */
static int context_update_with_nack(Context* ctx, const Pkg* pkg) {

    int found;

    assert(ctx);
    assert(pkg);

    if(CtxDownload(ctx)) {
        if(CtxEnd(ctx)) {
            found = find_nack_pkg(ctx, PkgIndx(pkg), NULL);
            if(!found) {
                ctx->completed = 1;
                return -1;
            }
        }
        return context_download_update_with_nack(ctx, pkg);
    } else {
        if(CtxLs(ctx)) {
            /*
             *  There's no need to handle 'ls' nack package reponses
             *  in a separate function. For that reason, we just return '1'
             *  and resend the package.
             */
            return 1;
        }
    }

    return 0;
}

/*
 *  context_update() - 
 *
 *  Updates the context based on the received package, handling 
 *  acknowledgments.
 *
 *  @ctx: Pointer to the Context structure that needs to be updated.
 *  @pkg: Pointer to the Pkg structure containing the received package data.
 *
 *  return:
 *    -  '1' if the context is successfully updated.
 *    -  '0' otherwise.
 *    - '-1' if the context was finalized.
 */
int context_update(Context* ctx, const Pkg* pkg) {

    assert(ctx);
    assert(pkg);

    if(PkgAck(pkg)) {
        return context_update_with_ack(ctx);
    } else {
        if(PkgNack(pkg)) {
            return context_update_with_nack(ctx, pkg);
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
 *  context_deinit_ls() - 
 *
 *  Deinitializes the ls context by closing the 
 *  directory descriptor.
 *
 *  @ctx: Pointer to the Context structure that 
 *        needs to be deinitialized.
 */
static inline void context_deinit_ls(Context* ctx) {

    if(ctx && ctx->desc.dp) {
        closedir(ctx->desc.dp);
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
        } else {
            if(CtxLs(ctx)) {
                context_deinit_ls(ctx);
            }
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

