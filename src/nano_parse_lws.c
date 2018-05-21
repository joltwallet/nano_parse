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
   
    strupper(hash);
    snprintf( (char *) rpc_command, sizeof(rpc_command), "{\"action\":\"work_generate\",\"hash\":\"%s\"}", hash );
    network_get_data((unsigned char *)rpc_command, (unsigned char *)rx_string, sizeof(rx_string));

    return nanoparse_work(rx_string, work);
}

nl_err_t nanoparse_lws_account_frontier(const char *account_address, hex256_t frontier_block_hash){
    char rpc_command[NANOPARSE_CMD_BUF_LEN];
    char rx_string[NANOPARSE_RX_BUF_LEN];
    
    strlower(account_address);
    snprintf( (char *) rpc_command, sizeof(rpc_command),
            "{\"action\":\"accounts_frontiers\",\"accounts\":[\"%s\"]}",
            account_address);
    network_get_data((unsigned char *)rpc_command, (unsigned char *)rx_string, sizeof(rx_string));

    return nanoparse_frontier(rx_string, account_address, frontier_block_hash);
}

nl_err_t nanoparse_lws_block(char *block_hash, nl_block_t *block){
    char rpc_command[NANOPARSE_CMD_BUF_LEN];
    char rx_string[NANOPARSE_RX_BUF_LEN];

    snprintf( (char *) rpc_command, sizeof(rpc_command),
             "{\"action\":\"block\",\"hash\":\"%s\"}",
             block_hash);
    network_get_data((unsigned char *)rpc_command, (unsigned char *)rx_string, sizeof(rx_string));

    return nanoparse_block(rx_string, block);
}

#endif

