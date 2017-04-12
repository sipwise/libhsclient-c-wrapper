#include <stdio.h>
#include <stdint.h>
#include <time.h>

#include "hsclient-c-wrapper.h"

static int invalid_secret() {
    char *cfg[] = {
        "test", // argv[0]
        "host=127.0.0.1",
        "port=9997",
        "dbname=kamailio",
        "table=acc",
        "index=PRIMARY",
        "fields=method",
        "secret=wrongsecret",
    };
    size_t num_cfg = sizeof(cfg)/sizeof(char*);

    hs_res res = hs_open("acc-inv", cfg, num_cfg);
    if(!hs_success(res)) {
        printf("ok, rejected invalid secret\n");
        hs_free_result(res);
        return 0;
    } else {
        printf("fail, accepted invalid secret\n");
        hs_free_result(res);
        return -1;
    }
}

static int valid_secret() {
    char *cfg[] = {
        "test", // argv[0]
        "host=127.0.0.1",
        "port=9997",
        "dbname=kamailio",
        "table=acc",
        "index=PRIMARY",
        "fields=method",
        "secret=writesecret",
    };
    size_t num_cfg = sizeof(cfg)/sizeof(char*);

    hs_res res = hs_open("acc-val", cfg, num_cfg);
    if(!hs_success(res)) {
        printf("fail, rejected valid secret\n");
        hs_free_result(res);
        return -1;
    } else {
        printf("ok, accepted valid secret\n");
        hs_free_result(res);
        return 0;
    }
}

int main() {
    invalid_secret();
    valid_secret();
    return 0;
}
