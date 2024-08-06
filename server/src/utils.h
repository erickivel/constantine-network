#ifndef UTILS_H
#define UTILS_H

#include <dirent.h>
#include <stdio.h>

#include "utils.defs.h"

/*
 *  get_asset_path() -
 *
 *  Constructs a full path to an asset by concatenating a base 
 *  path and an asset name.
 *
 *  @name: Name of the asset.
 *  @n   : Length of the asset name.
 *
 *  return:
 *    - Pointer to the allocated string containing the full path on success.
 *    - 'NULL' on failure.
 */
extern char* get_asset_path(const char* name, size_t n);

/*
 *  get_assets_dir() - 
 *
 *  Opens and returns a directory stream for the assets directory.
 *
 *  return:
 *    - Pointer to the directory stream on success.
 *    - 'NULL' on failure to open the directory.
 */
extern DIR* get_assets_dir(void);

#endif  /* UTILS_H */
