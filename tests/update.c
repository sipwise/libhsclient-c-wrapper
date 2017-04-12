#include <stdio.h>
#include <stdint.h>
#include <time.h>

#include "hsclient-c-wrapper.h"

size_t runs = 10000;

static int update_prefs() {

    char *valstrings[] = {
        "parallel-1",
        "serial-1",
        "seqential-1"
    };

    char *cfg[] = {
        "test", // argv[0]
        "host=127.0.0.1",
        "port=9997",
        "dbname=kamailio",
        "table=usr_preferences",
        "index=ua_idx",
        "fields=value",
        "filters=attribute"
    };
    size_t num_cfg = sizeof(cfg)/sizeof(char*);

    char *keys[] = {
        "fcd7f63c-a99e-4f13-befe-df0a9d1941f8"
    };
    size_t num_keys = sizeof(keys)/sizeof(char*);

    char *filters[] = {
        "cloud_pbx_hunt_policy"
    };
    size_t num_filters = sizeof(filters)/sizeof(char*);

    size_t i = 0;
    hs_res res = NULL;

    if(hs_open("usr_pref", cfg, num_cfg) < 0) {
        printf("failed to create usr_pref handle\n");
        return -1;
    }
    for(i = 0; i < runs; ++i) {
        char *vals[] = {
            valstrings[i % 3]
        };
        size_t num_vals = sizeof(vals)/sizeof(char*);

        res = hs_filtered_update("usr_pref", keys, num_keys, filters, num_filters, vals, num_vals);
        if(res == NULL) {
            printf("failed to update prefs\n");
            return -1;
        }
        if(!hs_success(res)) {
            printf("failed to update prefs: %s\n", hs_get_error(res));
            hs_free_result(res);
            return -1;
        }
        hs_free_result(res);
    }

    return 0;
}

int main() {
    update_prefs();
    return 0;
}
