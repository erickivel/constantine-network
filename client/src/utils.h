#ifndef UTILS_H
#define UTILS_H

#define DEBUG

#ifdef DEBUG
#   define debug(fmt, ...) fprintf(stderr, fmt, ##__VA_ARGS__)
#else
#   define debug(fmt, ...) (void)0
#endif  /* DEBUG */

#endif  /* UTILS_H */
