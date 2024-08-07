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

    size_t end;
    size_t completed;
    size_t indx;
    size_t sent;
    size_t k;

    struct  {

        size_t i;
        Pkg buf[WINSZ];
    } win;

    union {

        FILE* fp;
        DIR*  dp;
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
 *  Initializes the context based on the package type.
 *
 *  @ctx: Pointer to the Context structure that will be initialized.
 *  @pkg: Pointer to the constant Pkg structure that contains initialization
 *        data.
 *
 *  return:
 *    - '1' if the context is successfully initialized.
 *    - '0' if the context type is not recognized or if no initialization is
 *          required.
 */
extern int context_init(Context* ctx, const Pkg* pkg);

/*
 *  context_update() - 
 *
 *  Updates the context based on the received package, handling acknowledgments.
 *
 *  @ctx: Pointer to the Context structure that needs to be updated.
 *  @pkg: Pointer to the Pkg structure containing the received package data.
 *
 *  return:
 *    -  '1' if the context is successfully updated.
 *    -  '0' otherwise.
 *    - '-1' if the context was finalized.
 */
extern int context_update(Context* ctx, const Pkg* pkg);

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
