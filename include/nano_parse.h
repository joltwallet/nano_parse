/* nano_lib - ESP32 Any functions related to seed/private keys for Nano
 Copyright (C) 2018  Brian Pugh, James Coxon, Michael Smaili
 https://www.joltwallet.com/
 */

#ifndef __INCLUDE_NANO_PARSE_H__
#define __INCLUDE_NANO_PARSE_H__

#include "nano_lib.h"
#include "jolttypes.h"

/* Saftey Note: It is the user's responsibility that all json_data strings
 * are properly null-terminated */

uint32_t nanoparse_block_count(const char *json_data);
uint32_t nanoparse_lws_block_count();
jolt_err_t nanoparse_work( const char *json_data, uint64_t *work);
jolt_err_t nanoparse_account_frontier(const char *json_data, hex256_t frontier_block_hash);
jolt_err_t nanoparse_block(const char *json_data, nl_block_t *block);
jolt_err_t nanoparse_pending_hash( const char *json_data,
        hex256_t pending_block_hash, mbedtls_mpi *amount);
jolt_err_t nanoparse_process(const nl_block_t *block, char *buf, size_t buf_len);

#if CONFIG_NANOPARSE_BUILD_W_LWS
jolt_err_t nanoparse_lws_work(const hex256_t hash, uint64_t *work);
jolt_err_t nanoparse_lws_account_frontier(const char *account_address, hex256_t frontier_block_hash);
jolt_err_t nanoparse_lws_block(const hex256_t block_hash, nl_block_t *block);
jolt_err_t nanoparse_lws_pending_hash( const char *account_address,
        hex256_t pending_block_hash, mbedtls_mpi *amount);
jolt_err_t nanoparse_lws_frontier_block(nl_block_t *block);
jolt_err_t nanoparse_lws_process(nl_block_t *block);
#endif

#if 0
jolt_err_t get_block(char *block_hash, nl_block_t *block);
jolt_err_t get_pending(char *account_address,
        hex256_t pending_block_hash, mbedtls_mpi *amount);
jolt_err_t get_frontier(char *account_address, hex256_t frontier_block_hash);
int process_block(nl_block_t *block);
#endif

#endif
