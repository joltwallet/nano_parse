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
#endif

static const char TAG[] = "nano_parse";

char* deblank(char* input)
{
    int i,j;
    char *output=input;
    for (i = 0, j = 0; i<strlen(input); i++,j++)
    {
        if (input[i]!=' ')
            output[j]=input[i];
        else
            j--;
    }
    output[j]=0;
    return output;
}

char* replace(char* str, char* a, char* b)
{
    int len  = strlen(str);
    int lena = strlen(a), lenb = strlen(b);
    for (char* p = str; (p = strstr(p, a)); ++p) {
        if (lena != lenb) // shift end as needed
            memmove(p+lenb, p+lena,
                    len - (p - str) + lenb);
        memcpy(p, b, lenb);
    }
    return str;
}

uint32_t nanoparse_block_count( const char *json_data ){
    /* Parses rai_node rpc response for action "block count"
     * Returns network block count */
    uint32_t count_int;
    const cJSON *count = NULL;
    cJSON *json = cJSON_Parse(json_data);
    
    count = cJSON_GetObjectItemCaseSensitive(json, "count");
    if ( cJSON_IsString(count) && (count->valuestring != NULL) ){
        count_int = atoi(count->valuestring);
    }
    else{
        count_int = 0;
    }
    
    cJSON_Delete(json);
    return count_int;
}

nl_err_t nanoparse_work( const char *json_data, uint64_t *work){
    const cJSON *json_work = NULL;
    cJSON *json = cJSON_Parse(json_data);
    
    json_work = cJSON_GetObjectItemCaseSensitive(json, "work");
    if (cJSON_IsString(json_work) && (json_work->valuestring != NULL)){
        *work = nl_parse_server_work_string(json_work->valuestring);
    }
    else{
        ESP_LOGE(TAG, "Work Parse Failure.");
        return E_FAILURE;
    }
    
    cJSON_Delete(json);
    return E_SUCCESS;
}

nl_err_t nanoparse_account_frontier(const char *json_data, const char *account_address, hex256_t frontier_block_hash){
    /* Returns the hash (33 characters) of the head block of the account */
    const cJSON *frontiers = NULL;
    const cJSON *account = NULL;
    nl_err_t outcome;
    
    cJSON *json = cJSON_Parse((char *)json_data);
    frontiers = cJSON_GetObjectItemCaseSensitive(json, "frontiers");
    account = cJSON_GetObjectItemCaseSensitive(frontiers, account_address);
    
    if (cJSON_IsString(account) && (account->valuestring != NULL)){
        strlcpy(frontier_block_hash, account->valuestring, HEX_256);
        outcome = E_SUCCESS;
    }
    else{
        outcome = E_FAILURE;
    }
    
    cJSON_Delete(json);
    return outcome;
}

nl_err_t nanoparse_block(const char *json_data, nl_block_t *block){
    const cJSON *json_contents = NULL;
    const cJSON *json_type = NULL;
    const cJSON *json_previous = NULL;
    const cJSON *json_link = NULL;
    const cJSON *json_representative = NULL;
    const cJSON *json_account = NULL;
    const cJSON *json_balance = NULL;
    const cJSON *json_work = NULL;
    const cJSON *json_signature = NULL;
    
    cJSON *json = cJSON_Parse((char *)json_data);
    json_contents = cJSON_GetObjectItemCaseSensitive(json, "contents");
    char *string = cJSON_Print(json_contents);
    
    ESP_LOGI(TAG, "get_block: %s", string);
    
    char* new_string = replace(string, "\\n", "\\");
    for (char* p = new_string; (p = strchr(p, '\\')); ++p) {
        *p = ' ';
    }
    
    char * new_string_nws = deblank(new_string);
    
    new_string_nws[0] = ' ';
    new_string_nws[strlen(new_string_nws)-1] = ' ';
    
    cJSON *nested_json = cJSON_Parse(new_string_nws);

    /********************
     * Parse Block Type *
     ********************/
    json_type = cJSON_GetObjectItemCaseSensitive(nested_json, "type");
    if (cJSON_IsString(json_type) && (json_type->valuestring != NULL)){
        ESP_LOGI(TAG, "get_block: Type: %s", json_type->valuestring);
        if (strcmp(json_type->valuestring, "state") == 0){
            block->type = STATE;
        }
        else if (strcmp(json_type->valuestring, "send") == 0){
            block->type = SEND;
        }
        else if (strcmp(json_type->valuestring, "receive") == 0){
            block->type = RECEIVE;
        }
        else if (strcmp(json_type->valuestring, "open") == 0){
            block->type = OPEN;
        }
        else if (strcmp(json_type->valuestring, "change") == 0){
            block->type = CHANGE;
        }
        
        ESP_LOGI(TAG, "get_block: block.type: %d", block->type);
    }

    /*****************
     * Parse Account *
     *****************/
    json_account = cJSON_GetObjectItemCaseSensitive(nested_json, "account");
    if (cJSON_IsString(json_account) && (json_account->valuestring != NULL)){
        ESP_LOGI(TAG, "get_block: Account: %s", json_account->valuestring);
        nl_address_to_public(block->account, json_account->valuestring);
    }

    /******************
     * Parse Previous *
     ******************/
    json_previous = cJSON_GetObjectItemCaseSensitive(nested_json, "previous");
    if (cJSON_IsString(json_previous) && (json_previous->valuestring != NULL)){
        sodium_hex2bin(block->previous, sizeof(block->previous),
                       json_previous->valuestring,
                       HEX_256, NULL, NULL, NULL);
    }

    /************************
     * Parse Representative *
     ************************/
    json_representative = cJSON_GetObjectItemCaseSensitive(nested_json, "representative");
    if (cJSON_IsString(json_representative) && (json_representative->valuestring != NULL)){
        ESP_LOGI(TAG, "get_block: Representative: %s", json_representative->valuestring);
        nl_address_to_public(block->representative, json_representative->valuestring);
    }

    /*******************
     * Parse Signature *
     *******************/
    json_signature = cJSON_GetObjectItemCaseSensitive(nested_json, "signature");
    if (cJSON_IsString(json_signature) && (json_signature->valuestring != NULL)){
        ESP_LOGI(TAG, "get_block: Signature: %s", json_signature->valuestring);
        sodium_hex2bin(block->signature, sizeof(block->signature),
                       json_signature->valuestring,
                       HEX_512, NULL, NULL, NULL);
    }
    
    /**************
     * Parse Link *
     **************/
    // For state: link is hexidecimal ("link")
    // For open: link is hash of the pairing send block ("source")
    // For change: link is nothing
    // For send: link is destination pub key ("destination")
    // For receive: link is hash of the pairing send block ("source")
    if (cJSON_IsString(json_link) && (json_link->valuestring != NULL)){
        sodium_hex2bin(block->link, sizeof(block->link),
                       json_link->valuestring,
                       HEX_256, NULL, NULL, NULL);
    }

    if ( block->type == STATE ){
        json_link = cJSON_GetObjectItemCaseSensitive(nested_json, "link");
    }
    else if(block->type == OPEN || block->type == RECEIVE){
        json_link = cJSON_GetObjectItemCaseSensitive(nested_json, "source");
    }
    if ( cJSON_IsString(json_link) && (json_link->valuestring != NULL) ){
        sodium_hex2bin(block->link, sizeof(block->link),
                       json_link->valuestring,
                       HEX_256, NULL, NULL, NULL);
    }
    else if(block->type == SEND ){
        json_link = cJSON_GetObjectItemCaseSensitive(nested_json, "destination");
        if ( cJSON_IsString(json_link) && (json_link->valuestring != NULL) ){
            nl_address_to_public(block->link, json_link->valuestring);
        }
    }

    /**************
     * Parse Work *
     **************/
    json_work = cJSON_GetObjectItemCaseSensitive(nested_json, "work");
    if (cJSON_IsString(json_work) && (json_work->valuestring != NULL)){
        ESP_LOGI(TAG, "get_block: Work: %s", json_work->valuestring);
        block->work = nl_parse_server_work_string(json_work->valuestring);
    }

    /*****************
     * Parse Balance *
     *****************/
    json_balance = cJSON_GetObjectItemCaseSensitive(nested_json, "balance");
    if (cJSON_IsString(json_balance) && (json_balance->valuestring != NULL)){
        ESP_LOGI(TAG, "get_block: Balance: %s\n", json_balance->valuestring);
        
        mbedtls_mpi current_balance;
        mbedtls_mpi_init(&current_balance);
        if (block->type == SEND){
            mbedtls_mpi_read_string(&current_balance, 16, json_balance->valuestring);
        }
        else {
            mbedtls_mpi_read_string(&current_balance, 10, json_balance->valuestring);
        }
        mbedtls_mpi_copy( &(block->balance), &current_balance);
        mbedtls_mpi_free (&current_balance);
    }
    cJSON_Delete(json);
    
    return E_SUCCESS;
}

#if 0 
nl_err_t get_pending(char *account_address,
        hex256_t pending_block_hash, mbedtls_mpi *amount){
    char rpc_command[NANOPARSE_CMD_BUF_LEN];
    char rx_string[NANOPARSE_RX_BUF_LEN];

    nl_err_t outcome = E_FAILURE;
    
    strlower(account_address);
    
    snprintf( (char *) rpc_command, sizeof(rpc_command),
             "{\"action\":\"accounts_pending\","
             "\"count\": 1,"
             "\"source\": \"true\","
             "\"accounts\":[\"%s\"]}",
             account_address);
    
    network_get_data((unsigned char *)rpc_command, (unsigned char *)rx_string, sizeof(rx_string));
    
    const cJSON *blocks = NULL;
    const cJSON *account = NULL;
    
    cJSON *json = cJSON_Parse((char *)rx_string);
    
    blocks = cJSON_GetObjectItemCaseSensitive(json, "blocks");
    account = cJSON_GetObjectItemCaseSensitive(blocks, account_address);

    cJSON *current_element = NULL;
    char *current_key = NULL;
    cJSON_ArrayForEach(current_element, account) {
        current_key = current_element->string;
        if (current_key != NULL) {
            strlcpy(pending_block_hash, current_key, HEX_256);
            const cJSON *pending_contents = cJSON_GetObjectItemCaseSensitive(
                    account, current_key);
            const cJSON *amount_obj = cJSON_GetObjectItemCaseSensitive(
                    pending_contents, "amount");
            mbedtls_mpi_read_string(amount, 10, amount_obj->valuestring);
            outcome = E_SUCCESS;
        }
    }
    
    cJSON_Delete(json);
    return outcome;
}

int get_head(nl_block_t *block){
    char rpc_command[NANOPARSE_CMD_BUF_LEN];
    char rx_string[NANOPARSE_RX_BUF_LEN];
    char account_address[ADDRESS_BUF_LEN];
    
    //Now convert this to an XRB address
    nl_err_t res;
    res = nl_public_to_address(account_address,
                               sizeof(account_address),
                               block->account);
    strupper(account_address);
    ESP_LOGI(TAG, "get_head: Address: %s\n", account_address);
    
    //Get latest block from server
    //First get frontier
    hex256_t frontier_block_hash;
    
    int frontier_outcome = get_frontier(account_address, frontier_block_hash);
    ESP_LOGI(TAG, "get_head: Frontier Block: %s", frontier_block_hash);
    
    if (frontier_outcome == 0){
        return 1;
    }
    
    //Now get the block info
    snprintf( (char *) rpc_command, sizeof(rpc_command),
             "{\"action\":\"block\",\"hash\":\"%s\"}",
             frontier_block_hash);
    
    network_get_data((unsigned char *) rpc_command, 
            (unsigned char *) rx_string, sizeof(rx_string));
    
    block->type = STATE;
    
    const cJSON *json_contents = NULL;
    const cJSON *json_previous = NULL;
    const cJSON *json_link = NULL;
    const cJSON *json_representative = NULL;
    const cJSON *json_account = NULL;
    const cJSON *json_balance = NULL;
    const cJSON *json_work = NULL;
    const cJSON *json_signature = NULL;
    
    cJSON *json = cJSON_Parse((char *)rx_string);
    
    json_contents = cJSON_GetObjectItemCaseSensitive(json, "contents");
    char *string = cJSON_Print(json_contents);
    
    char* new_string = replace(string, "\\n", "\\");
    
    for (char* p = new_string; (p = strchr(p, '\\')); ++p) {
        *p = ' ';
    }
    
    char * new_string_nws = deblank(new_string);
    
    new_string_nws[0] = ' ';
    new_string_nws[strlen(new_string_nws)-1] = ' ';
    
    cJSON *nested_json = cJSON_Parse(new_string_nws);
    
    json_account = cJSON_GetObjectItemCaseSensitive(nested_json, "account");
    
    if (cJSON_IsString(json_account) && (json_account->valuestring != NULL))
    {
        ESP_LOGI(TAG, "get_head: Account: %s", json_account->valuestring);
        //TODO We need to convert this to a public key
        sodium_hex2bin(block->account, sizeof(block->account),
                       json_account->valuestring,
                       HEX_256, NULL, NULL, NULL);
    }
    
    json_previous = cJSON_GetObjectItemCaseSensitive(nested_json, "previous");
    if (cJSON_IsString(json_previous) && (json_previous->valuestring != NULL))
    {
        
        sodium_hex2bin(block->previous, sizeof(block->previous),
                       json_previous->valuestring,
                       HEX_256, NULL, NULL, NULL);
    }
    
    json_representative = cJSON_GetObjectItemCaseSensitive(nested_json, "representative");
    if (cJSON_IsString(json_representative) && (json_representative->valuestring != NULL))
    {
        ESP_LOGI(TAG, "get_head: Representative: %s", json_representative->valuestring);
        //TODO We need to convert this to a public key
        sodium_hex2bin(block->representative, sizeof(block->representative),
                       json_representative->valuestring,
                       HEX_256, NULL, NULL, NULL);
    }
    
    json_signature = cJSON_GetObjectItemCaseSensitive(nested_json, "signature");
    if (cJSON_IsString(json_signature) && (json_signature->valuestring != NULL))
    {
        sodium_hex2bin(block->signature, sizeof(block->signature),
                       json_signature->valuestring,
                       HEX_512, NULL, NULL, NULL);
    }
    
    json_link = cJSON_GetObjectItemCaseSensitive(nested_json, "link");
    if (cJSON_IsString(json_link) && (json_link->valuestring != NULL))
    {
        sodium_hex2bin(block->link, sizeof(block->link),
                       json_link->valuestring,
                       HEX_256, NULL, NULL, NULL);
    }
    
    json_work = cJSON_GetObjectItemCaseSensitive(nested_json, "work");
    if (cJSON_IsString(json_work) && (json_work->valuestring != NULL))
    {
        block->work = nl_parse_server_work_string(json_work->valuestring);
    }
    
    json_balance = cJSON_GetObjectItemCaseSensitive(nested_json, "balance");
    if (cJSON_IsString(json_balance) && (json_balance->valuestring != NULL))
    {
        ESP_LOGI(TAG, "get_head: Balance: %s", json_balance->valuestring);
        mbedtls_mpi current_balance;
        mbedtls_mpi_init(&current_balance);
        mbedtls_mpi_read_string(&current_balance, 10, json_balance->valuestring);
        mbedtls_mpi_copy( &(block->balance), &current_balance);
        mbedtls_mpi_free (&current_balance);
        
    }
    
    cJSON_Delete(json);
    
    return 0;
}

int process_block(nl_block_t *block){
    
    //Account (convert bin to address)
    char account_address[ADDRESS_BUF_LEN];
    nl_err_t res;
    res = nl_public_to_address(account_address,
                               sizeof(account_address),
                               block->account);
    strlower(account_address);
    ESP_LOGI(TAG, "process_block: Address: %s", account_address);
    
    //Previous (convert bin to hex)
    hex256_t previous_hex;
    sodium_bin2hex(previous_hex, sizeof(previous_hex),
                   block->previous, sizeof(block->previous));
    ESP_LOGI(TAG, "process_block: Previous: %s", previous_hex);
    
    //Representative (convert bin to address)
    char representative_address[ADDRESS_BUF_LEN];
    res = nl_public_to_address(representative_address,
                               sizeof(representative_address),
                               block->representative);
    strlower(representative_address);
    ESP_LOGI(TAG, "process_block: Representative: %s", representative_address);
    
    //Balance (convert mpi to string)
    char balance_buf[66];
    size_t n;
    memset(balance_buf, 0, sizeof(balance_buf));
    mbedtls_mpi_write_string(&(block->balance), 10, balance_buf, sizeof(balance_buf)-1, &n);
    ESP_LOGI(TAG, "process_block: Balance: %s", balance_buf);
    
    //Link (convert bin to hex)
    hex256_t link_hex;
    sodium_bin2hex(link_hex, sizeof(link_hex),
                   block->link, sizeof(block->link));
    strupper(link_hex);
    ESP_LOGI(TAG, "process_block: Link: %s", link_hex);
    
    //Work (keep as hex)
    ESP_LOGI(TAG, "process_block: Work: %llx", block->work);
    
    //Signature (convert bin to hex)
    hex512_t signature_hex;
    sodium_bin2hex(signature_hex, sizeof(signature_hex),
                   block->signature, sizeof(block->signature));
    strupper(signature_hex);
    ESP_LOGI(TAG, "Block->signature: %s", signature_hex);
    
    char new_block[NANOPARSE_CMD_BUF_LEN];
    hex64_t work;
    nl_generate_server_work_string(work, block->work);
    int error = snprintf( (char *) new_block, sizeof(new_block),
                         "{"
                         "\"action\":\"process\","
                         "\"block\":\""
                         "{"
                         "\\\"type\\\":\\\"state\\\","
                         "\\\"account\\\":\\\"%s\\\","
                         "\\\"previous\\\":\\\"%s\\\","
                         "\\\"representative\\\":\\\"%s\\\","
                         "\\\"balance\\\":\\\"%s\\\","
                         "\\\"link\\\":\\\"%s\\\","
                         "\\\"work\\\":\\\"%s\\\","
                         "\\\"signature\\\":\\\"%s\\\""
                         "}"
                         "\"}",
            account_address, previous_hex, representative_address, balance_buf,
            link_hex, work, signature_hex);
    
    ESP_LOGI(TAG, "\nprocess_block: Block: %d\n%s\n", error, new_block);
    
    unsigned char rx_string[NANOPARSE_RX_BUF_LEN];
    
    network_get_data((unsigned char *)new_block, (unsigned char *)rx_string, sizeof(rx_string));
    
    ESP_LOGI(TAG, "%s\n", rx_string);
    
    return 0;
}
#endif
