#include <stdexcept>
#include <memory>
#include <unordered_map>

#include <errno.h>
#include <string.h>
#include <stdarg.h>

#include <handlersocket/hstcpcli.hpp>
#include <handlersocket/string_util.hpp>

#include "hsclient-c-wrapper.h"

typedef struct dena::hstcpcli_filter hs_filter;
typedef std::shared_ptr<dena::hstcpcli_i> hstcpcli_sptr;

extern hstcpcli_sptr hs_get_cli(hs_connection *con);
extern hs_result *hs_create_result();
extern void hs_set_error(hs_result *res, const char* format, ...);
extern int hs_set_element(hs_result *res, size_t row, size_t col, const char *val);
extern int hs_add_row(hs_result *res);

static hs_result* hs_query_full(hs_connection *con, char **keys, size_t num_keys, size_t limit, size_t offset, const dena::string_ref op_ref, char **vals, size_t num_vals, const dena::string_ref mod_ref, char **filters, size_t num_filters) {

    std::vector<dena::string_ref> keyrefs;
    std::vector<dena::string_ref> valrefs;
    std::vector<hs_filter> filterrefs;

    int code = 0;
    size_t numflds = 0;

    hs_result *r = hs_create_result();
    if(!r) {
        return NULL;
    }

    hstcpcli_sptr cli = hs_get_cli(con);
    if(!cli) {
        hs_set_error(r, "invalid connection index %lu", con->index);
        return r;
    }

    for(size_t i = 0; i < num_keys; ++i) {
        const dena::string_ref ref(keys[i], strlen(keys[i]));
        keyrefs.push_back(ref);
    }
    for(size_t i = 0; i < num_vals; ++i) {
        if(vals[i] == NULL) {
            valrefs.push_back(dena::string_ref(0, 1));
        } else {
            valrefs.push_back(dena::string_ref(vals[i], strlen(vals[i])));
        }
    }
    for(size_t i = 0; i < num_filters; ++i) {
        hs_filter ref;
        ref.filter_type = dena::string_ref("F", 1); // F for filter, W for while (stop on first mismatch)
        ref.op = dena::string_ref("=", 1);
        ref.val = dena::string_ref(filters[i], strlen(filters[i]));
        filterrefs.push_back(ref);
    }

    cli->request_buf_exec_generic(con->index,
        op_ref, num_keys == 0 ? 0 : &keyrefs[0], num_keys,
        limit, offset,
        mod_ref, num_vals == 0 ? 0 : &valrefs[0], num_vals,
        num_filters == 0 ? 0 : &filterrefs[0], num_filters);

    if(cli->request_send() != 0) {
        hs_set_error(r, "failed to send hs request: %s", cli->get_error().c_str());
        return r;
    }

    do {
        if((code = cli->response_recv(numflds)) != 0) {
            hs_set_error(r, "failed to receive hs response: %s", cli->get_error().c_str());
            break;
        }
        r->cols = numflds;
        int rowcnt = 0;
        while(true) {
            const dena::string_ref *const row = cli->get_next_row();
            if(row == 0) {
                break;
            }
            if(hs_add_row(r) < 0) {
                break;
            }
            for(size_t i = 0; i < numflds; ++i) {
                const std::string val(row[i].begin(), row[i].size());
                if(hs_set_element(r, rowcnt, i, val.c_str()) < 0) {
                    break;
                }
            }
            rowcnt++;
        }
    } while(false);
    cli->response_buf_remove();

    return r;
}

static hs_result* hs_query(hs_connection *con, char **keys, size_t num_keys, size_t limit, size_t offset, const dena::string_ref op_ref) {
    return hs_query_full(con, keys, num_keys, limit, offset, op_ref, NULL, 0, dena::string_ref(), NULL, 0);
}

hs_result* hs_select(hs_connection *con, char **keys, size_t num_keys, size_t limit, size_t offset) {
    const dena::string_ref op_ref("=", 1);
    return hs_query(con, keys, num_keys, limit, offset, op_ref);
}

hs_result* hs_insert(hs_connection *con, char **keys, size_t num_keys) {
    const dena::string_ref op_ref("+", 1);
    return hs_query(con, keys, num_keys, 0, 0, op_ref);
}

hs_result* hs_filtered_update(hs_connection *con, char **keys, size_t num_keys, char **filters, size_t num_filters, char **vals, size_t num_vals) {
    const dena::string_ref op_ref("=", 1);
    const dena::string_ref mod_ref("U", 1);
    return hs_query_full(con, keys, num_keys, hs_max(), 0, op_ref, vals, num_vals, mod_ref, filters, num_filters);
}

hs_result* hs_delete(hs_connection *con, char **keys, size_t num_keys) {
    return hs_filtered_delete(con, keys, num_keys, NULL, 0);
}

hs_result* hs_filtered_delete(hs_connection *con, char **keys, size_t num_keys, char **filters, size_t num_filters) {
    const dena::string_ref op_ref("=", 1);
    const dena::string_ref mod_ref("D", 1);
    return hs_query_full(con, keys, num_keys, hs_max(), 0, op_ref, NULL, 0, mod_ref, filters, num_filters);
}

hs_result* hs_update(hs_connection *con, char **keys, size_t num_keys, char **vals, size_t num_vals) {
    return hs_filtered_update(con, keys, num_keys, NULL, 0, vals, num_vals);
}
