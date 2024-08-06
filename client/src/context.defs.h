#ifndef CONTEXT_DEFS_H
#define CONTEXT_DEFS_H

#define WINSZ 5

#define Download(type)      ((type) == CTX_DOWNLOAD)
#define Ls(type)            ((type) == CTX_LS)

#define CtxEnd(ctx)         ((ctx)->end)
#define CtxCompleted(ctx)   ((ctx)->completed)
#define CtxDownload(ctx)    (Download((ctx)->type))
#define CtxLs(ctx)          (Ls((ctx)->type))

/*
 *  incindx() -
 *
 *  @ctx:
 */
#define incindx(ctx)                                        \
    do {                                                    \
        (ctx)->indx = ((ctx)->indx + 1) % PKG_MAX_IND;      \
    } while(0)

#define RED     "\033[31m"
#define RESET   "\033[0m"

#endif  /* CONTEXT_DEFS_H */
