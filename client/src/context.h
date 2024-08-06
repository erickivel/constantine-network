#ifndef CONTEXT_H
#define CONTEXT_H

#include <dirent.h>
#include <stdio.h>

#include "context.defs.h"
#include "utils.h"
#include "pkg.h"

enum CtxType {

    CTX_DOWNLOAD,
    CTX_LS
};

typedef enum CtxType CtxType;

struct Context {

    CtxType type;

    int completed;
    int invalid;
    int ack;
    int skip;
    int error;

    size_t indx;
    size_t recv;
    size_t k;

    struct  {

        size_t i;
        Pkg buf;
    } win;

    union {

        FILE* fp;
    } desc;
};

typedef struct Context Context;

/*
 *  context_create() - 
 *
 *  Allocates a new 'Context' structure.
 *
 *  return:
 *    - A pointer to the newly allocated 'Context' structure.
 *    - 'NULL' if the allocation fails.
 */
extern Context* context_create(void);

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
extern int context_init(Context* ctx, CtxType type, const char* path);

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
extern int context_update(Context* ctx, Pkg* pkg);

/*
 *  context_deinit() - 
 *
 *  Deinitializes the context based on its type.
 *
 *  @ctx: Pointer to the Context structure that 
 *        needs to be deinitialized.
 */
extern void context_deinit(Context* ctx);

/*
 *  context_free() - 
 *
 *  Deinitializes and frees the context structure.
 *
 *  @ctx: Double pointer to the Context structure that 
 *        needs to be deinitialized and freed.
 */
extern void context_free(Context** ctx);

#endif
