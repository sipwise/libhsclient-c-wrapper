#include <stdexcept>
#include <memory>
#include <unordered_map>

#include <errno.h>
#include <string.h>
#include <stdarg.h>

#include <handlersocket/hstcpcli.hpp>
#include <handlersocket/string_util.hpp>

#include "hsclient-c-wrapper.h"

typedef struct hs_handle_s {
    uint32_t index;
} hs_handle;
typedef std::shared_ptr<hs_handle> hs_handle_ptr;

typedef struct dena::hstcpcli_filter hs_filter;

typedef std::shared_ptr<dena::hstcpcli_i> hstcpcli_sptr;
std::vector<hstcpcli_sptr> g_clis;
std::unordered_map<char*, hs_handle_ptr> g_index;
uint32_t g_last_index;

static hs_result *hs_create_result() {
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

static void hs_set_error(hs_result *res, const char* format, ...) {
    va_list args;
    va_start(args, format);
    snprintf(res->error, sizeof(res->error), format, args);
    va_end(args);
    res->success = 0;
}

static int hs_add_row(hs_result *res) {
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

static int hs_set_element(hs_result *res, size_t row, size_t col, const char *val) {
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

static hs_result* hs_query_full(char *name, char **keys, size_t num_keys, size_t limit, size_t offset, const dena::string_ref op_ref, char **vals, size_t num_vals, const dena::string_ref mod_ref, char **filters, size_t num_filters) {

    std::vector<dena::string_ref> keyrefs;
    std::vector<dena::string_ref> valrefs;
    std::vector<hs_filter> filterrefs;

    int code = 0;
    size_t numflds = 0;

    hs_result *r = hs_create_result();
    if(!r) {
        return NULL;
    }

    auto searchidx = g_index.find(name);
    if(searchidx == g_index.end()) {
        hs_set_error(r, "handle name %s does not exist", name);
        return r;
    }
    hs_handle_ptr handle = searchidx->second;

    for(size_t i = 0; i < num_keys; ++i) {
        const dena::string_ref ref(keys[i], strlen(keys[i]));
        keyrefs.push_back(ref);
    }
    for(size_t i = 0; i < num_vals; ++i) {
        const dena::string_ref ref(vals[i], strlen(vals[i]));
        valrefs.push_back(ref);
    }
    for(size_t i = 0; i < num_filters; ++i) {
        hs_filter ref;
        ref.filter_type = dena::string_ref("F", 1); // F for filter, W for while (stop on first mismatch)
        ref.op = dena::string_ref("=", 1);
        ref.val = dena::string_ref(filters[i], strlen(filters[i]));
        filterrefs.push_back(ref);
    }

    hstcpcli_sptr cli;
    try {
        cli = g_clis.at(handle->index);
    } catch(const std::out_of_range &e) {
        hs_set_error(r, "invalid index %lu", handle->index);
        return r;
    }

    cli->request_buf_exec_generic(handle->index,
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

static hs_result* hs_query(char *name, char **keys, size_t num_keys, size_t limit, size_t offset, const dena::string_ref op_ref) {
    return hs_query_full(name, keys, num_keys, limit, offset, op_ref, NULL, 0, dena::string_ref(), NULL, 0);
}

hs_result* hs_select(char *name, char **keys, size_t num_keys, size_t limit, size_t offset) {
    const dena::string_ref op_ref("=", 1);
    return hs_query(name, keys, num_keys, limit, offset, op_ref);
}

hs_result* hs_insert(char *name, char **keys, size_t num_keys) {
    const dena::string_ref op_ref("+", 1);
    return hs_query(name, keys, num_keys, 0, 0, op_ref);
}

hs_result* hs_filtered_update(char *name, char **keys, size_t num_keys, char **filters, size_t num_filters, char **vals, size_t num_vals) {
    const dena::string_ref op_ref("=", 1);
    const dena::string_ref mod_ref("U", 1);
    return hs_query_full(name, keys, num_keys, hs_max(), 0, op_ref, vals, num_vals, mod_ref, filters, num_filters);
}

hs_result* hs_delete(char *name, char **keys, size_t num_keys) {
    return hs_filtered_delete(name, keys, num_keys, NULL, 0);
}

hs_result* hs_filtered_delete(char *name, char **keys, size_t num_keys, char **filters, size_t num_filters) {
    const dena::string_ref op_ref("=", 1);
    const dena::string_ref mod_ref("D", 1);
    return hs_query_full(name, keys, num_keys, hs_max(), 0, op_ref, NULL, 0, mod_ref, filters, num_filters);
}

hs_result* hs_update(char *name, char **keys, size_t num_keys, char **vals, size_t num_vals) {
    return hs_filtered_update(name, keys, num_keys, NULL, 0, vals, num_vals);
}

hs_result* hs_open(char *name, char **cfg, size_t num_cfg) {
    dena::config conf;
    dena::socket_args sockargs;
    hs_handle_ptr handle;
    uint32_t idx;

    hs_result *r = hs_create_result();
    if(!r) {
        return NULL;
    }

    if(g_index.empty()) {
        idx = g_last_index = 0;
    } else {
        if(g_index.find(name) != g_index.end()) {
            hs_set_error(r, "handle name %s already exists", name);
            return r;
        }
        idx = ++g_last_index;
    }
    handle = hs_handle_ptr(new hs_handle);
    handle->index = idx;
    g_index.insert({name, handle});

    parse_args(num_cfg, cfg, conf);
    sockargs.set(conf);

    // TODO: we're casting from an auto_ptr to a shared_ptr!
    // is this ok?
    hstcpcli_sptr cli = dena::hstcpcli_i::create(sockargs);
    g_clis.push_back(cli);
    idx = g_clis.size()-1;

    const std::string dbname = conf.get_str("dbname");
    const std::string table  = conf.get_str("table");
    const std::string index = conf.get_str("index", "PRIMARY");
    const std::string fields = conf.get_str("fields");
    const std::string filters = conf.get_str("filters");
    const std::string secret = conf.get_str("secret", "");

    do {
        int code = 0;
        size_t numflds = 0;
        if(secret.length()) {
            cli->request_buf_auth(secret.c_str(), "1");
            if(cli->request_send() != 0) {
                hs_set_error(r, "failed to send auth request: %s", cli->get_error().c_str());
                g_clis.pop_back();
                break;
            }
            if((code = cli->response_recv(numflds)) != 0) {
                hs_set_error(r, "failed to receive auth response: %s", cli->get_error().c_str());
                cli->response_buf_remove();
                g_clis.pop_back();
                break;
            }
            cli->response_buf_remove();
        }
        cli->request_buf_open_index(idx, dbname.c_str(), table.c_str(),
            index.c_str(), fields.c_str(), filters.c_str());
        if(cli->request_send() != 0) {
            hs_set_error(r, "failed to send opening request: %s", cli->get_error().c_str());
            g_clis.pop_back();
            break;
        }
        if((code = cli->response_recv(numflds)) != 0) {
            hs_set_error(r, "failed to receive opening response: %s", cli->get_error().c_str());
            cli->response_buf_remove();
            g_clis.pop_back();
            break;
        }
        cli->response_buf_remove();
    } while(false);

    return r;
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
