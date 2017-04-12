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
    int ret;
    hs_con con;
    hs_res res = hs_connect(&con, cfg, num_cfg);
    if(!hs_success(res)) {
        printf("ok, rejected invalid secret\n");
        ret = 0;
    } else {
        printf("fail, accepted invalid secret\n");
        ret = -1;
    }
    hs_free_result(res);
    hs_close(con);
    return ret;
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
    int ret;
    hs_con con;

    hs_res res = hs_connect(&con, cfg, num_cfg);
    if(!hs_success(res)) {
        printf("fail, rejected valid secret\n");
        ret = -1;
    } else {
        printf("ok, accepted valid secret\n");
        ret = 0;
    }
    hs_free_result(res);
    hs_close(con);
    return ret;
}

int main() {
    invalid_secret();
    valid_secret();
    return 0;
}
