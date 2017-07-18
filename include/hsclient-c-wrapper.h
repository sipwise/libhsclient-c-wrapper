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

typedef struct hs_filterkey_s {
    char *name;
    char *op;
    char *val;
} hs_filterkey;
typedef hs_filterkey hs_key;

typedef enum hs_value_type_e {
    HS_NULL,
    HS_INT,
    HS_DOUBLE,
    HS_STRING,
} hs_valtype;

typedef struct hs_value_s {
    char *name;
    union {
        int64_t val_int;
        char *val_string;
        double val_double;
    };
    hs_valtype type;
} hs_value;
typedef hs_value hs_val;

typedef struct hs_connection_s {
    uint32_t index;
} hs_connection;
typedef hs_connection* hs_con;

size_t hs_num_rows(hs_res res);
size_t hs_num_cols(hs_res res);
hs_row hs_fetch_row(hs_res res);
void hs_free_result(hs_res res);
int hs_success(hs_res res);
char* hs_get_error(hs_res res);
size_t hs_max();

hs_res hs_init_connection(hs_con *con,
    const char *host, uint16_t port,
    const char *dbname, const char *table,
    const char *index, const char *indexcols,
    const char *secret);

hs_res hs_prepare_query(hs_con con, hs_key** keys);



hs_res hs_connect(hs_con *con, char **cfg, size_t num_cfg);
void hs_close(hs_con con);

hs_res hs_select(hs_con con, char **keys, size_t num_keys, size_t limit, size_t offset);
hs_res hs_insert(hs_con con, char **keys, size_t num_keys);

hs_res hs_filtered_update(hs_con con, char **keys, size_t num_keys, char **filters, size_t num_filters, char **vals, size_t num_vals);
hs_res hs_update(hs_con con, char **keys, size_t num_keys, char **vals, size_t num_vals);

hs_res hs_filtered_delete(hs_con con, char **keys, size_t num_keys, char **filters, size_t num_filters);
hs_res hs_delete(hs_con con, char **keys, size_t num_keys);

#ifdef __cplusplus
}
#endif

#endif // _HSCLIENT_C_WRAPPER_H
