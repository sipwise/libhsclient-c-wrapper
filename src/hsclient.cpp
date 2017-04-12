#include <stdexcept>
#include <memory>
#include <unordered_map>

#include <handlersocket/hstcpcli.hpp>
#include <handlersocket/string_util.hpp>

#include "hsclient-c-wrapper.h"

typedef std::shared_ptr<dena::hstcpcli_i> hstcpcli_sptr;
std::vector<hstcpcli_sptr> g_clis;
std::unordered_map<char*, uint32_t> g_index;
uint32_t g_last_index;

int hs_create(char *name, char **cfg, size_t num_cfg) {
	dena::config conf;
	dena::socket_args sockargs;

	uint32_t idx;
	if(g_index.empty()) {
		idx = g_last_index = 0;
	} else {
		if(g_index.find(name) != g_index.end()) {
			fprintf(stderr, "Handle with name %s already exists\n", name);
			return -1;
		}
		idx = ++g_last_index;
	}
	g_index.insert({name, idx});
	printf("name %s has index %u\n", name, idx);

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

	cli->request_buf_open_index(idx, dbname.c_str(), table.c_str(),
		index.c_str(), fields.c_str());

	return 0;
}

int64_t hs_select(char *name, char **keys, size_t num_keys, size_t limit, size_t offset) {
	const dena::string_ref op_ref("=", 1);
	std::vector<dena::string_ref> keyrefs;

	uint32_t idx;
	auto searchidx = g_index.find(name);
	if(searchidx == g_index.end()) {
		fprintf(stderr, "Handle with name %s does not exist\n", name);
		return -1;
	}
	idx = searchidx->second;

	for(size_t i = 0; i < num_keys; ++i) {
		const dena::string_ref ref(keys[i], strlen(keys[i]));	
		keyrefs.push_back(ref);
	}

	hstcpcli_sptr cli;
	try {
		cli = g_clis.at(idx);
	} catch(const std::out_of_range &e) {
		fprintf(stderr, "invalid index %u\n", idx);
		return -1;
	}

	cli->request_buf_exec_generic(idx, op_ref,
		num_keys == 0 ? 0 : &keyrefs[0],
		num_keys, limit, offset, dena::string_ref(), 0, 0);

	int code = 0;
	size_t numflds = 0;
	int num_rows = 0;
	int ret = -1;
	do {
		if(cli->request_send() != 0) {
			fprintf(stderr, "request_send: %s\n", cli->get_error().c_str());
			break;
		}
		if((code = cli->response_recv(numflds)) != 0) {
			fprintf(stderr, "response_recv: %s\n", cli->get_error().c_str());
			break;
		}
	} while(false);
	cli->response_buf_remove();

	do {
		if((code = cli->response_recv(numflds)) != 0) {
			fprintf(stderr, "response_recv: %s\n", cli->get_error().c_str());
			break;
		}
		ret = 0;
		while(true) {
			const dena::string_ref *const row = cli->get_next_row();
			if(row == 0) {
				break;
			}
			num_rows++;
			printf("REC:");
			for(size_t i = 0; i < numflds; ++i) {
				const std::string val(row[i].begin(), row[i].size());
				printf(" %s", val.c_str());
			}
			printf("\n");
		}
	} while(false);
	cli->response_buf_remove();

	ret = (ret < 0) ? ret : num_rows;
	return ret;
}

