#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sodium.h>
#include "esp_log.h"
#include "cJSON.h"

#include "nano_lib.h"
#include "nano_parse.h"

#if CONFIG_NANOPARSE_BUILD_W_LWS

#include "nano_lws.h"
#define NANOPARSE_CMD_BUF_LEN 1024
#define NANOPARSE_RX_BUF_LEN 1024

static const char TAG[] = "nano_parse";


uint32_t nanoparse_lws_block_count(){
    /* Uses nano_lws to get data from rai_node */
    char rpc_command[NANOPARSE_CMD_BUF_LEN];
    char rx_string[NANOPARSE_RX_BUF_LEN];
    
    snprintf( (char *) rpc_command, sizeof(rpc_command),
            "{\"action\":\"block_count\"}" );
    network_get_data((unsigned char *)rpc_command, (unsigned char *)rx_string,
            sizeof(rx_string));

    return nanoparse_block_count(rx_string);
}

nl_err_t nanoparse_lws_work(const hex256_t hash, uint64_t *work){
    char rpc_command[NANOPARSE_CMD_BUF_LEN];
    char rx_string[NANOPARSE_RX_BUF_LEN];
   
    //strupper(hash);
    snprintf( (char *) rpc_command, sizeof(rpc_command), "{\"action\":\"work_generate\",\"hash\":\"%s\"}", hash );
    network_get_data((unsigned char *)rpc_command, (unsigned char *)rx_string, sizeof(rx_string));

    return nanoparse_work(rx_string, work);
}

nl_err_t nanoparse_lws_account_frontier(const char *account_address, hex256_t frontier_block_hash){
    char rpc_command[NANOPARSE_CMD_BUF_LEN];
    char rx_string[NANOPARSE_RX_BUF_LEN];
    
    //strlower(account_address);
    snprintf( (char *) rpc_command, sizeof(rpc_command),
            "{\"action\":\"accounts_frontiers\",\"accounts\":[\"%s\"]}",
            account_address);
    network_get_data((unsigned char *)rpc_command, (unsigned char *)rx_string, sizeof(rx_string));

    return nanoparse_account_frontier(rx_string, frontier_block_hash);
}

nl_err_t nanoparse_lws_block(const hex256_t block_hash, nl_block_t *block){
    char rpc_command[NANOPARSE_CMD_BUF_LEN];
    char rx_string[NANOPARSE_RX_BUF_LEN];

    snprintf( (char *) rpc_command, sizeof(rpc_command),
             "{\"action\":\"block\",\"hash\":\"%s\"}",
             block_hash);
    network_get_data((unsigned char *)rpc_command, (unsigned char *)rx_string, sizeof(rx_string));

    return nanoparse_block(rx_string, block);
}

nl_err_t nanoparse_lws_pending_hash( const char *account_address,
        hex256_t pending_block_hash, mbedtls_mpi *amount){
    char rpc_command[NANOPARSE_CMD_BUF_LEN];
    char rx_string[NANOPARSE_RX_BUF_LEN];
    
    strlower(account_address);
    snprintf( (char *) rpc_command, sizeof(rpc_command),
             "{\"action\":\"accounts_pending\","
             "\"count\": 1,"
             "\"source\": \"true\","
             "\"accounts\":[\"%s\"]}",
             account_address);
    network_get_data((unsigned char *)rpc_command, (unsigned char *)rx_string, sizeof(rx_string));

    return nanoparse_pending_hash(rx_string, pending_block_hash, amount);
}

nl_err_t nanoparse_lws_frontier_block(nl_block_t *block){
    /* Convenience function to get frontier hash and block contents.
     * Fills in block's field according to account in block->account.
     *
     * General Steps:
     *     1) Get Frontier Block Hash
     *     2) Query that hash
     *     3) Populate the struct nl_block_t
     */

    char address[ADDRESS_BUF_LEN];
   
    // Convert public key to an address
    nl_err_t res;
    res = nl_public_to_address(address, sizeof(address), block->account);
    if( E_SUCCESS != res ){
        return res;
    }

    strupper(address);
    ESP_LOGI(TAG, "frontier_block: Address: %s\n", address);
    
    /* Get latest block from server */
    // First get frontier block hash
    hex256_t frontier_block_hash;
    res = nanoparse_lws_account_frontier(address, frontier_block_hash);
    if( E_SUCCESS != res ){
        return res;
    }
    ESP_LOGI(TAG, "frontier_block: Frontier Block: %s", frontier_block_hash);
    
    // Now get the block contents
    return nanoparse_lws_block(frontier_block_hash, block);
}

nl_err_t nanoparse_lws_process(nl_block_t *block){
    nl_err_t res;
    char rpc_command[NANOPARSE_CMD_BUF_LEN];
    unsigned char rx_string[NANOPARSE_RX_BUF_LEN];

    res = nanoparse_process(block, rpc_command, sizeof(rpc_command));
    if( E_SUCCESS != res ){
        return res;
    }
    return network_get_data((unsigned char *)rpc_command,
            (unsigned char *)rx_string, sizeof(rx_string));
}

#endif

