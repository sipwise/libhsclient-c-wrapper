#ifndef _HSCLIENT_C_WRAPPER_H
#define _HSCLIENT_C_WRAPPER_H

#include <stdint.h>
#include <stdlib.h>
#include <limits.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef char* hs_element;
typedef hs_element* hs_row;
typedef hs_row* hs_table;
typedef struct hs_result_s {
    hs_table table;
    size_t cols, rows;
    size_t row_iter;
    char error[1024];
    int success;
} hs_result;
typedef hs_result* hs_res;

size_t hs_num_rows(hs_res res);
size_t hs_num_cols(hs_res res);
hs_row hs_fetch_row(hs_res res);
void hs_free_result(hs_res res);
int hs_success(hs_res res);
char* hs_get_error(hs_res res);
size_t hs_max() { return UINT_MAX; }

// creates a new hs handle with the given name.
// 0 on success, -1 on error
int hs_create(char *name, char **cfg, size_t num_cfg);

// executes a select on the hs handle with the given name
hs_res hs_select(char *name, char **keys, size_t num_keys, size_t limit, size_t offset);

// executes an insert on the hs handle with the given name
hs_res hs_insert(char *name, char **keys, size_t num_keys);

#ifdef __cplusplus
}
#endif

#endif // _HSCLIENT_C_WRAPPER_H
