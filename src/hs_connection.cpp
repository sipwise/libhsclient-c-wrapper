#include <stdexcept>
#include <memory>
#include <unordered_map>
#include <vector>
#include <algorithm>

#include <handlersocket/hstcpcli.hpp>
#include <handlersocket/string_util.hpp>

#include "hsclient-c-wrapper.h"

typedef std::shared_ptr<dena::hstcpcli_i> hstcpcli_sptr;

typedef std::vector<std::string> hs_indexlist;
std::vector<hs_indexlist> g_indexlist;

struct hs_handle_s {
    dena::config config;
    hs_indexlist indexcols;    
    hstcpcli_sptr cli;
};
typedef struct hs_handle_s hs_handle;
typedef std::shared_ptr<hs_handle> hs_handle_sptr;
std::vector<hs_handle_sptr> g_handles;


extern hs_result *hs_create_result();
extern void hs_set_error(hs_result *res, const char* format, ...);

hstcpcli_sptr hs_get_cli(hs_connection *con) {
    hs_handle_sptr handle;
    try {
        handle = g_handles.at(con->index);
    } catch(const std::out_of_range &e) {
        //hs_set_error(r, "invalid connection index %lu", con->index);
        //return r;
        return NULL;
    }
    if(!handle) {
        //hs_set_error(r, "invalid connection index %lu", con->index);
        //return r;
        return NULL;
    }
    return handle->cli;
}

static hs_connection *hs_create_connection() {
    hs_connection *con = (hs_connection*)malloc(sizeof(hs_connection));
    if(!con) {
        return NULL;
    }
    con->index= 0;
    return con;
}

std::vector<std::string> hs_splitstring(const char *str, char c = ',')
{
    std::vector<std::string> res;
    do
    {
        const char *begin = str;
        while(*str != c && *str) {
            str++;
        }
        res.push_back(std::string(begin, str));
    } while (0 != *str++);
    return res;
}

hs_result* hs_init_connection(hs_connection **con,
        const char *host, uint16_t port,
        const char *dbname, const char* table,
        const char *index, const char* indexcols,
        const char *secret) {

    dena::config conf;
    dena::socket_args sockargs;

    hs_result *r = hs_create_result();
    if(!r) {
        return NULL;
    }

    *con = hs_create_connection();
    if(!*con) {
        hs_set_error(r, "failed to create connection: %s", strerror(errno));
        return r;
    }

    conf["host"] = std::string(host);
    conf["port"] = std::to_string(port);
    conf["dbname"] = std::string(dbname);
    conf["table"] = std::string(table);
    conf["index"] = std::string(index);
    conf["indexcols"] = std::string(indexcols);
    sockargs.set(conf);

    hs_indexlist index_list = hs_splitstring(indexcols, ',');

    hstcpcli_sptr cli = dena::hstcpcli_i::create(sockargs);
    hs_handle_sptr handle = hs_handle_sptr(new hs_handle);
    handle->cli = cli;
    handle->indexcols = index_list;
    handle->config = conf;
    g_handles.push_back(handle);
    (*con)->index = g_handles.size()-1;

    if(secret != NULL) {
        int code = 0;
        size_t numflds = 0;
        cli->request_buf_auth(secret, "1");
        do {
            if(cli->request_send() != 0) {
                hs_set_error(r, "failed to send auth request: %s", cli->get_error().c_str());
                hs_close(*con);
                break;
            }
            if((code = cli->response_recv(numflds)) != 0) {
                hs_set_error(r, "failed to receive auth response: %s", cli->get_error().c_str());
                hs_close(*con);
            }
            cli->response_buf_remove();
        } while(false);
    }
    return r;
}

hs_result* hs_prepare_query(hs_connection *con, hs_filterkey** keys) {
    printf("preparing result\n");
    hs_result *r = hs_create_result();
    if(!r) {
        return NULL;
    }

    /*
        1. if all keys->name and keys->op are found in sets, use con as is
        2. else find keys->name in index and prepare index list,
                prepare remaining names as filter list,
                open index, store lists in sets
    */

    hs_handle_sptr handle;
    hs_indexlist index_list;
    hstcpcli_sptr cli;
    
    try {
        handle = g_handles.at(con->index);
    } catch(const std::out_of_range &e) {
        hs_set_error(r, "invalid connection index %lu", con->index);
        return r;
    }
    if(!handle) {
        hs_set_error(r, "invalid connection index %lu", con->index);
        return r;
    }
    index_list = handle->indexcols;
    cli = handle->cli;

    std::vector<std::string> filter_list;
    for(hs_filterkey **k = keys; *k != NULL; ++k) {
        size_t pos;
        char *name = (*k)->name;
        printf("checking: %s %s %s\n", name, (*k)->op, (*k)->val);
        auto it = std::find(index_list.begin(), index_list.end(), std::string(name));
        if(it != index_list.end()) {
            pos = std::distance(index_list.begin(), it);
            printf("found key %s at position %lu in index list\n", name, pos);
            // TODO: add val in right index slot
        } else {
            printf("key %s not in index list, search filter list\n", name);
            auto it = std::find(filter_list.begin(), filter_list.end(), std::string(name));
            if(it != filter_list.end()) {
                pos = std::distance(filter_list.begin(), it);
                printf("found key %s at position %lu in filter list\n", name, pos);
            } else {
                filter_list.push_back(std::string(name));
                pos = filter_list.size()-1;
                printf("added key %s at position %lu in filter list\n", name, pos);
            }
            // TODO: add val in right filter slot
        }
    }

    std::string index;
    for(auto it = index_list.begin(); it != index_list.end(); it++) {
        if(it != index_list.begin()) {
            index.append(",");
        }
        index.append(*it);
    }

    std::string filter;
    for(auto it = filter_list.begin(); it != filter_list.end(); it++) {
        if(it != filter_list.begin()) {
            filter.append(",");
        }
        filter.append(*it);
    }

    printf("index=%s\n", index.c_str()); // that's not actually needed, just make sure we have all the vals
    printf("filter=%s\n", filter.c_str());
    // TODO: add field list


    std::string fields("strvalue,intvalue");
    do {
        int code;
        size_t numflds;
        cli->request_buf_open_index(con->index, handle->config["dbname"].c_str(), handle->config["table"].c_str(),
            handle->config["index"].c_str(), fields.c_str(), filter.c_str());
        if(cli->request_send() != 0) {
            hs_set_error(r, "failed to send opening request: %s", cli->get_error().c_str());
            hs_close(con);
            break;
        }
        if((code = cli->response_recv(numflds)) != 0) {
            hs_set_error(r, "failed to receive opening response: %s", cli->get_error().c_str());
            cli->response_buf_remove();
            hs_close(con);
            break;
        }
        cli->response_buf_remove();
    } while(false);

    return r;
}

void hs_close(hs_connection *con) {
    if(!con) {
        return;
    }
    g_handles.at(con->index) = NULL;
    // TODO: also clear g_indexlist.at(con->index) ?
    free(con);
}
