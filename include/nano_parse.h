/* nano_lib - ESP32 Any functions related to seed/private keys for Nano
 Copyright (C) 2018  Brian Pugh, James Coxon, Michael Smaili
 https://www.joltwallet.com/
 
 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 3 of the License, or
 (at your option) any later version.
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software Foundation,
 Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
 */

#ifndef __INCLUDE_NANO_PARSE_H__
#define __INCLUDE_NANO_PARSE_H__

#include "nano_lib.h"

uint32_t nanoparse_block_count(const char *json_data);
uint32_t nanoparse_lws_block_count();
nl_err_t nanoparse_work( const char *json_data, uint64_t *work);
nl_err_t nanoparse_account_frontier(const char *json_data, hex256_t frontier_block_hash);
nl_err_t nanoparse_block(const char *json_data, nl_block_t *block);
nl_err_t nanoparse_pending_hash( const char *json_data,
        hex256_t pending_block_hash, mbedtls_mpi *amount);
nl_err_t nanoparse_process(const nl_block_t *block, char *buf, size_t buf_len);

#if CONFIG_NANOPARSE_BUILD_W_LWS
nl_err_t nanoparse_lws_work(const hex256_t hash, uint64_t *work);
nl_err_t nanoparse_lws_account_frontier(const char *account_address, hex256_t frontier_block_hash);
nl_err_t nanoparse_lws_block(const hex256_t block_hash, nl_block_t *block);
nl_err_t nanoparse_lws_pending_hash( const char *account_address,
        hex256_t pending_block_hash, mbedtls_mpi *amount);
nl_err_t nanoparse_lws_frontier_block(nl_block_t *block);
nl_err_t nanoparse_lws_process(nl_block_t *block);
#endif

#if 0
nl_err_t get_block(char *block_hash, nl_block_t *block);
nl_err_t get_pending(char *account_address,
        hex256_t pending_block_hash, mbedtls_mpi *amount);
nl_err_t get_frontier(char *account_address, hex256_t frontier_block_hash);
int process_block(nl_block_t *block);
#endif

#endif
