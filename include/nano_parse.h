/**
 * @file nano_parse.h
 * @brief Parses JSON strings from nano_node into data structures.
 * 
 * For example usage, check the test functions in this repository.
 *
 * @author Brian Pugh
 */

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

/**
 * @brief Parse the response from `block_count` rpc command.
 *
 * e.g. parses the `count` field of:
 * {
 *    "count": "1000",   
 *    "unchecked": "1"         
 * }
 * @param[in] json_data JSON data to parse
 * @return network block count
 */
uint32_t nanoparse_block_count(const char *json_data);

/**
 * @brief Parse the response from `work_generate` rpc command.
 *
 * e.g. parses the `work` field of:
 * {  
 *     "work": "2bf29ef00786a6bc"  
 * }
 *
 * @param[in] json_data JSON data to parse
 * @param[out] work The PoW nonce.
 * @return E_SUCCESS on success
 */
jolt_err_t nanoparse_work( const char *json_data, uint64_t *work);

/**
 * @brief Parse the first account response from `accounts_frontiers` rpc command.
 * e.g.
 * {  
 *   "frontiers" : {  
 *     "xrb_3t6k35gi95xu6tergt6p69ck76ogmitsa8mnijtpxm9fkcm736xtoncuohr3": "791AF413173EEE674A6FCF633B5DFC0F3C33F397F0DA08E987D9E0741D40D81A",  
 *   }  
 * }
 * @param[in] json_data JSON data to parse
 * @param[out[ frontier_block_hash Hash of the frontier block for the first account
 * @return E_SUCCESS on success
 */
jolt_err_t nanoparse_account_frontier(const char *json_data, hex256_t frontier_block_hash);

/**
 * @brief Parse the response from `block` rpc command.

 * e.g.
 *  {  
 *   "action": "block_hash",     
 *   "block": "{\n    
 *       \"type\": \"state\",\n    
 *       \"account\": \"xrb_3qgmh14nwztqw4wmcdzy4xpqeejey68chx6nciczwn9abji7ihhum9qtpmdr\",\n    
 *       \"previous\": \"F47B23107E5F34B2CE06F562B5C435DF72A533251CB414C51B2B62A8F63A00E4\",\n    
 *       \"representative\": \"xrb_1hza3f7wiiqa7ig3jczyxj5yo86yegcmqk3criaz838j91sxcckpfhbhhra1\",\n    
 *       \"balance\": \"1000000000000000000000\",\n    
 *       \"link\": \"19D3D919475DEED4696B5D13018151D1AF88B2BD3BCFF048B45031C1F36D1858\",\n    
 *       \"link_as_account\": \"xrb_18gmu6engqhgtjnppqam181o5nfhj4sdtgyhy36dan3jr9spt84rzwmktafc\",\n    
 *       \"signature\": \"3BFBA64A775550E6D49DF1EB8EEC2136DCD74F090E2ED658FBD9E80F17CB1C9F9F7BDE2B93D95558EC2F277FFF15FD11E6E2162A1714731B743D1E941FA4560A\",\n    
 *       \"work\": \"cab7404f0b5449d0\"\n
 *    }\n"
 * }

 * @param[in] json_data JSON data to parse
 * @param[out] block populated block structure. Must be previously initialized.
 * @return E_SUCCESS on success
 */
jolt_err_t nanoparse_block(const char *json_data, nl_block_t *block);

/**
 * @brief Parse the response from `accounts_pending` rpc command.
 * e.g.
 * {  
 *   "blocks" : {  
 *     "xrb_1111111111111111111111111111111111111111111111111117353trpda": ["142A538F36833D1CC78B94E11C766F75818F8B940771335C6C1B8AB880C5BB1D"],  
 *     "xrb_3t6k35gi95xu6tergt6p69ck76ogmitsa8mnijtpxm9fkcm736xtoncuohr3": ["4C1FEEF0BEA7F50BE35489A1233FE002B212DEA554B55B1B470D78BD8F210C74"]  
 *   }  
 * }
 * @param[in] json_data JSON data to parse
 * @param[out] block_hash Hash of the first pending block 
 * @param[out] amount Unverified transaction amount; shouldn't be taken at face value.
 * @return E_SUCCESS on success
 */
jolt_err_t nanoparse_pending_hash( const char *json_data,
        hex256_t pending_block_hash, mbedtls_mpi *amount);

/**
 * @brief Generates a `process` POST command to be sent to a nano_node.
 * e.g.
 * {  
 *   "action": "process",  
 *   "block": "{   
 *     \"type\": \"state\",   
 *     \"account\": \"xrb_1qato4k7z3spc8gq1zyd8xeqfbzsoxwo36a45ozbrxcatut7up8ohyardu1z\",   
 *     \"previous\": \"6CDDA48608C7843A0AC1122BDD46D9E20E21190986B19EAC23E7F33F2E6A6766\",   
 *     \"representative\": \"xrb_3pczxuorp48td8645bs3m6c3xotxd3idskrenmi65rbrga5zmkemzhwkaznh\",   
 *     \"balance\": \"40200000001000000000000000000000000\",   
 *     \"link\": \"87434F8041869A01C8F6F263B87972D7BA443A72E0A97D7A3FD0CCC2358FD6F9\",   
 *     \"signature\": \"A5DB164F6B81648F914E49CAB533900C389FAAD64FBB24F6902F9261312B29F730D07E9BCCD21D918301419B4E05B181637CF8419ED4DCBF8EF2539EB2467F07\",   
 *     \"work\": \"000bc55b014e807d\"   
 *   }"   
 * }
 * @param[in] block Block to source data from
 * @param[out] buffer to populate with a JSON string
 * @param[in] length of buf
 * @return E_SUCCESS on success
 */
jolt_err_t nanoparse_process(const nl_block_t *block, char *buf, size_t buf_len);

#if CONFIG_NANOPARSE_BUILD_W_LWS || CONFIG_NANOPARSE_BUILD_W_REST
uint32_t nanoparse_web_block_count();
jolt_err_t nanoparse_web_work(const hex256_t hash, uint64_t *work);
jolt_err_t nanoparse_web_account_frontier(const char *account_address, hex256_t frontier_block_hash);
jolt_err_t nanoparse_web_block(const hex256_t block_hash, nl_block_t *block);
jolt_err_t nanoparse_web_pending_hash( const char *account_address,
        hex256_t pending_block_hash, mbedtls_mpi *amount);
jolt_err_t nanoparse_web_frontier_block(nl_block_t *block);
jolt_err_t nanoparse_web_process(nl_block_t *block);
#endif

#endif
