#include <stdio.h>
#include <stdint.h>
#include <time.h>

#include "hsclient-c-wrapper.h"

/*
create table test (
    id int(11) not null auto_increment primary key,
    uuid varchar(255) not null default '',
    attribute varchar(255) not null default '',
    somekey varchar(255) not null default '',
    strvalue varchar(255) default null,
    intvalue int(11) not null default 0,
    key ua_idx(uuid,attribute)
);
insert into test values(NULL, 'update-key', 'testattr', 'someval', 'testval', 42);
*/

static int update_prefs() {

    char *host = "127.0.0.1";
    uint16_t port = 9997;
    char *dbname = "kamailio";
    char *table = "test";
    char *index = "ua_idx";
    char *indexcols = "uuid,attribute";
    char *secret = "writesecret";

    hs_key *keys[] = {
        &((hs_key){ .name = "uuid",      .op = "=", .val = "update-key" }),
        &((hs_key){ .name = "attribute", .op = "=", .val = "testattr"   }),
        &((hs_key){ .name = "somekey",   .op = "=", .val = "someval"    }),
        NULL
    };

    hs_val *vals[] = {
        &((hs_val){ .name = "strval", .type = HS_NULL }),
        &((hs_val){ .name = "intval", .val_int = 99, .type = HS_INT }),
        NULL
    };

    hs_res res;
    hs_con con;

    res = hs_init_connection(&con, host, port, dbname, table, index, indexcols, secret);
    if(!hs_success(res)) {
        printf("failed to init usr_pref connection: %s\n", hs_get_error(res));
        hs_free_result(res);
        return -1;
    }
    hs_free_result(res);

    res = hs_prepare_query(con, keys);
    if(!hs_success(res)) {
        printf("failed to prepare query: %s\n", hs_get_error(res));
        hs_free_result(res);
        return -1;
    }
    hs_free_result(res);

#if 0
    for(i = 0; i < runs; ++i) {
        res = hs_update(con, vals);
        if(!hs_success(res)) {
            printf("failed to update prefs: %s\n", hs_get_error(res));
            hs_free_result(res);
            return -1;
        }
        hs_free_result(res);
    }
#endif
    hs_close(con);

    return 0;
}

int main() {
    update_prefs();
    return 0;
}
