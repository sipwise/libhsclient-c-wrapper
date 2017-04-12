#ifndef _HSCLIENT_C_WRAPPER_H
#define _HSCLIENT_C_WRAPPER_H

#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

// creates a new hs handle with the given name.
// 0 on success, -1 on error
int hs_create(char *name, char **cfg, size_t num_cfg);

// executes a select on the hs handle with the given name
int64_t hs_select(char *name, char **keys, size_t num_keys, size_t limit, size_t offset);

#ifdef __cplusplus
}
#endif

#endif // _HSCLIENT_C_WRAPPER_H
