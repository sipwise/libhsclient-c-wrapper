#include <stdio.h>
#include <stdint.h>
#include <time.h>

#include "hsclient-c-wrapper.h"

size_t runs = 10000;

static int insert_accs() {
    char *cfg[] = {
        "test", // argv[0]
        "host=127.0.0.1",
        "port=9997",
        "dbname=kamailio",
        "table=acc",
        "index=PRIMARY",
        "fields=method,from_tag,to_tag,callid,sip_code,sip_reason,"\
            "time,time_hires,src_leg,dst_leg,dst_user,dst_ouser,"\
            "dst_domain,src_user,src_domain",
        "secret=writesecret"
    };
    size_t num_cfg = sizeof(cfg)/sizeof(char*);
    time_t now = time(NULL);
    struct tm *tim = localtime(&now);
    char timstr[32] = "";
    char timstr_hires[32] = "";
    struct timespec timspec;

    strftime(timstr, sizeof(timstr), "%F %T", tim);
    clock_gettime(CLOCK_REALTIME, &timspec);
    snprintf(timstr_hires, sizeof(timstr_hires), "%.03f", 
        (double)(timspec.tv_sec + ((double)timspec.tv_nsec/1000000000)));

    char *keys[] = {
        "INVITE", "test-from-tag", "test-to-tag", "test-call-id", "200", "OK",
        timstr, timstr_hires, "test-src-leg", "test-dst-leg", "test-dst-user", "test-dst-ouser",
        "test-dst-domain", "test-src-user", "tst-src-domain"
    };
    size_t num_keys = sizeof(keys)/sizeof(char*);

    size_t i = 0;
    hs_res res = NULL;
    hs_con con;

    if(hs_connect(&con, cfg, num_cfg) < 0) {
        printf("failed to open acc connection\n");
        return -1;
    }
    for(i = 0; i < runs; ++i) {
        res = hs_insert(con, keys, num_keys);
        if(res == NULL) {
            printf("failed to insert acc\n");
            return -1;
        }
        if(!hs_success(res)) {
            printf("failed to insert acc: %s\n", hs_get_error(res));
            hs_free_result(res);
            return -1;
        }
        hs_free_result(res);
    }
    hs_close(con);

    return 0;
}

int main() {
    insert_accs();
    return 0;
}
