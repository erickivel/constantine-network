#ifndef CONTEXT_DEFS_H
#define CONTEXT_DEFS_H

#define WINSZ 5

#define CtxEnd(ctx)         ((ctx)->end)
#define CtxCompleted(ctx)   ((ctx)->completed)
#define CtxDownload(ctx)    ((ctx)->type == CTX_DOWNLOAD)
#define CtxLs(ctx)          ((ctx)->type == CTX_LS)

/*
 *  incindx() -
 *
 *  @ctx:
 */
#define incindx(ctx)                        \
    do {                                    \
        (ctx)->indx++;                      \
        if((ctx)->indx == PKG_MAX_IND) {    \
            (ctx)->indx = 0;                \
        }                                   \
    } while(0)

#endif  /* CONTEXT_DEFS_H */
