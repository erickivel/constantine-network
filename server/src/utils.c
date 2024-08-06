
#include <stdlib.h>
#include <string.h>

#include "utils.h"

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
char* get_asset_path(const char* name, size_t n) {

    char* path;
    size_t bytes;

    bytes = sizeof ASSETS_PATH;
    path  = calloc(n + bytes, sizeof *path);
    if(!path) {
        return NULL;
    }

    memcpy(path, ASSETS_PATH, bytes - 1);
    memcpy(path + bytes - 1, name, n);
    path[bytes + n] = 0;

    return path;
}

/*
 *  get_assets_dir() - 
 *
 *  Opens and returns a directory stream for the assets directory.
 *
 *  return:
 *    - Pointer to the directory stream on success.
 *    - 'NULL' on failure to open the directory.
 */
DIR* get_assets_dir(void) {

    DIR* dir;

    dir = opendir(ASSETS_PATH);
    if(!dir) {
        return NULL;
    }

    return dir;
}
