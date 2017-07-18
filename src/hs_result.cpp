#include <stdexcept>
#include <memory>
#include <unordered_map>

#include <errno.h>
#include <string.h>
#include <stdarg.h>

#include <handlersocket/hstcpcli.hpp>
#include <handlersocket/string_util.hpp>

#include "hsclient-c-wrapper.h"

size_t hs_max() {
    return UINT_MAX;
}

hs_result *hs_create_result() {
    hs_result *res = (hs_result*)malloc(sizeof(hs_result));
    if(!res) {
        fprintf(stderr, "failed to allocate result: %s\n", strerror(errno));
        return NULL;
    }
    snprintf(res->error, sizeof(res->error), "success");
    res->success = 1;
    res->rows = 0;
    res->cols = 0;
    res->row_iter = 0;
    res->table = NULL;
    return res;
}

void hs_set_error(hs_result *res, const char* format, ...) {
    va_list args;
    va_start(args, format);
    snprintf(res->error, sizeof(res->error), format, args);
    va_end(args);
    res->success = 0;
}

int hs_add_row(hs_result *res) {
    hs_row row = (hs_row)malloc(res->cols * sizeof(hs_element));
    if(!row) {
        hs_set_error(res, "failed to allocate row memory: %s", strerror(errno));
        return -1;
    }
    memset(row, 0, res->cols * sizeof(hs_element));
    if(!res->rows) {
        res->table = (hs_table)malloc(sizeof(hs_row));
        if(!res->table) {
            hs_set_error(res, "failed to allocate table memory: %s", strerror(errno));
            free(row);
            return -1;
        }
    } else {
        res->table = (hs_table)realloc(res->table, (res->rows+1) * sizeof(hs_row));
        if(!res->table) {
            hs_set_error(res, "failed to increase table memory: %s", strerror(errno));
            free(row);
            return -1;
        }
    }
    res->table[res->rows++] = row;
    return 0;
}

int hs_set_element(hs_result *res, size_t row, size_t col, const char *val) {
    if(row > res->rows) {
            hs_set_error(res, "failed row boundary check while setting element");
            return -1;
    }
    if(col > res->cols) {
            hs_set_error(res, "failed col boundary check while setting element");
            return -1;
    }
    res->table[row][col] = strdup(val);
    return 0;
}

size_t hs_num_rows(hs_result *res) {
    return res->rows;
}

size_t hs_num_cols(hs_result *res) {
    return res->cols;
}

hs_row hs_fetch_row(hs_result *res) {
    if(res->row_iter == res->rows) {
        return NULL;
    }
    return res->table[res->row_iter++];
}

void hs_free_result(hs_result *res) {
    if(!res || !res->table) {
        return;
    }
    for(size_t row = 0; row < res->rows; ++row) {
        if(!res->table[row]) {
            continue;
        }
        for(size_t col = 0; col < res->cols; ++col) {
            if(res->table[row][col]) {
                free(res->table[row][col]);
            }
        }
        free(res->table[row]);
    }
    free(res->table);
    free(res);
}

int hs_success(hs_result *res) {
    if(res) {
        return res->success;
    } else {
        return 0;
    }
}

char* hs_get_error(hs_result *res) {
    if(!res) {
        return (char*) "invalid result handle";
    } else {
        return res->error;
    }
}
