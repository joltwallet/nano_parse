#ifndef __INCLUDE_NANO_PARSE_H__
#define __INCLUDE_NANO_PARSE_H__

//#ifdef ESP32
#include "nano_lib.h"
//#else
//#include "../components/nano_lib/include/nano_lib.h"
//#endif

uint32_t nanoparse_block_count(const char *json_data);
uint32_t nanoparse_lws_block_count();

nl_err_t nanoparse_work( const char *json_data, uint64_t *work);
nl_err_t nanoparse_lws_work(const char *hash, uint64_t *work);

nl_err_t nanoparse_account_frontier(const char *json_data, const char *account_address, hex256_t frontier_block_hash);
nl_err_t nanoparse_lws_account_frontier(const char *account_address, hex256_t frontier_block_hash);

nl_err_t nanoparse_block(const char *json_data, nl_block_t *block);
nl_err_t nanoparse_lws_block(char *block_hash, nl_block_t *block);

nl_err_t nanoparse_pending_hash( const char *json_data,
        hex256_t pending_block_hash, mbedtls_mpi *amount);
nl_err_t nanoparse_lws_pending_hash( const char *account_address,
        hex256_t pending_block_hash, mbedtls_mpi *amount);



int get_head(nl_block_t *block);
nl_err_t get_block(char *block_hash, nl_block_t *block);
nl_err_t get_pending(char *account_address,
        hex256_t pending_block_hash, mbedtls_mpi *amount);
nl_err_t get_frontier(char *account_address, hex256_t frontier_block_hash);
int process_block(nl_block_t *block);

#endif
