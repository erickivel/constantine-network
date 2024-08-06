#ifndef UTILS_DEFS_H
#define UTILS_DEFS_H

#define DEBUG

#ifdef DEBUG
#   define debug(fmt, ...) fprintf(stderr, fmt, ##__VA_ARGS__)
#else
#   define debug(fmt, ...) (void)0
#endif  /* DEBUG */

#define ASSETS_PATH "./assets/"

#endif  /* UTILS_DEFS_G */
