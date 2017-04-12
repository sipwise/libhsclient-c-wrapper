#include <stdio.h>
#include <stdint.h>
#include <limits.h>

#include "hsclient-c-wrapper.h"

static int fetch_prefs() {
	char *cfg[] = {
		"test", // argv[0]
		"host=127.0.0.1",
		"port=9996",
		"dbname=kamailio",
		"table=usr_preferences",
		"index=ua_idx",
		"fields=attribute,value"
	};
	size_t num_cfg = sizeof(cfg)/sizeof(char*);

	char *keys[] = {
		"fcd7f63c-a99e-4f13-befe-df0a9d1941f8"
	};
	size_t num_keys = sizeof(keys)/sizeof(char*);

	uint32_t limit = UINT_MAX, offset = 0;

	int rows;
	uint32_t index;

	if(hs_create("usr_prefs", cfg, num_cfg) < 0) {
		printf("failed to create usr_prefs handle\n");
		return -1;
	}
	rows = hs_select("usr_prefs", keys, num_keys, limit, offset);
	if(rows < 0) {
		printf("failed to select\n");
		return -1;
	}

	printf("selected %d rows\n", rows);

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
		"fields=password,uuid"
	};
	size_t num_cfg = sizeof(cfg)/sizeof(char*);

	char *keys[] = {
		"cust_subscriber_5_1491536164",
		"test1491536163.example.org"
	};
	size_t num_keys = sizeof(keys)/sizeof(char*);

	uint32_t limit = UINT_MAX, offset = 0;

	int rows;
	uint32_t index;

	if(hs_create("subs", cfg, num_cfg) < 0) {
		printf("failed to create usr_prefs handle\n");
		return -1;
	}
	rows = hs_select("subs", keys, num_keys, limit, offset);
	if(rows < 0) {
		printf("failed to select\n");
		return -1;
	}

	printf("selected %d rows\n", rows);

	return 0;	
}

int main() {
	fetch_prefs();
	fetch_subs();
	return 0;
}
