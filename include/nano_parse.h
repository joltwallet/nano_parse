#ifndef __INCLUDE_NANO_PARSE_H__
#define __INCLUDE_NANO_PARSE_H__

//#ifdef ESP32
#include "nano_lib.h"
//#else
//#include "../components/nano_lib/include/nano_lib.h"
//#endif

int get_block_count();
nl_err_t get_work(char *hash, uint64_t *work);
int get_head(nl_block_t *block);
nl_err_t get_block(char *block_hash, nl_block_t *block);
nl_err_t get_pending(char *account_address,
        hex256_t pending_block_hash, mbedtls_mpi *amount);
nl_err_t get_frontier(char *account_address, hex256_t frontier_block_hash);
int process_block(nl_block_t *block);

#endif
