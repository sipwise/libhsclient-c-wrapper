#include <stdio.h>
#include <stdint.h>
#include <limits.h>

#include "hsclient-c-wrapper.h"

size_t runs = 100000;

static void print_result(hs_res res) {
    hs_row row;
    size_t i;
    printf("got %lu rows\n", hs_num_rows(res));
    while((row = hs_fetch_row(res)) != NULL) {
        printf("ROW:");
        for(i = 0; i < hs_num_cols(res); ++i) {
            printf(" %s", row[i]);
        }
        printf("\n");
    }
}

static int fetch_prefs() {
    char *cfg[] = {
        "test", // argv[0]
        "host=127.0.0.1",
        "port=9996",
        "dbname=kamailio",
        "table=usr_preferences",
        "index=ua_idx",
        "fields=attribute,value",
        "secret=readsecret"
    };
    size_t num_cfg = sizeof(cfg)/sizeof(char*);

    char *keys[] = {
        "fcd7f63c-a99e-4f13-befe-df0a9d1941f8"
    };
    size_t num_keys = sizeof(keys)/sizeof(char*);

    uint32_t limit = UINT_MAX, offset = 0;
    size_t i = 0;
    hs_res res = NULL;
    hs_con con = NULL;

    res = hs_connect(&con, cfg, num_cfg);
    if(!hs_success(res)) {
        printf("failed to open usr_prefs connection: %s\n", hs_get_error(res));
        hs_free_result(res);
        return -1;
    }
    hs_free_result(res);

    res = NULL;
    for(i = 0; i < runs; ++i) {
        hs_free_result(res);
        res = hs_select(con, keys, num_keys, limit, offset);
        if(!hs_success(res)) {
            printf("failed to select subs: %s\n", hs_get_error(res));
            hs_free_result(res);
            return -1;
        }
    }
    print_result(res);
    hs_free_result(res);
    hs_close(con);

    return 0;
}

static int fetch_subs() {
    char *cfg[] = {
        "test", // argv[0]
        "host=127.0.0.1",
        "port=9996",
        "dbname=kamailio",
        "table=subscriber",
        "index=account_idx",
        "fields=id,password,uuid",
        "secret=readsecret"
    };
    size_t num_cfg = sizeof(cfg)/sizeof(char*);

    char *keys[] = {
        "cust_subscriber_5_1491536164",
        "test1491536163.example.org"
    };
    size_t num_keys = sizeof(keys)/sizeof(char*);

    uint32_t limit = UINT_MAX, offset = 0;

    size_t i = 0;
    hs_res res = NULL;
    hs_con con;

    res = hs_connect(&con, cfg, num_cfg);
    if(!hs_success(res)) {
        printf("failed to open usr_prefs connection: %s\n", hs_get_error(res));
        hs_free_result(res);
        return -1;
    }
    hs_free_result(res);
    res = NULL;

    for(i = 0; i < runs; ++i) {
        hs_free_result(res);
        res = hs_select(con, keys, num_keys, limit, offset);
        if(!hs_success(res)) {
            printf("failed to select subs: %s\n", hs_get_error(res));
            hs_free_result(res);
            return -1;
        }
    }
    print_result(res);
    hs_free_result(res);
    hs_close(con);

    return 0;
}

int main() {
    fetch_prefs();
    fetch_subs();
    return 0;
}
