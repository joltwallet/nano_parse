/* nano_lib - ESP32 Any functions related to seed/private keys for Nano
 Copyright (C) 2018  Brian Pugh, James Coxon, Michael Smaili
 https://www.joltwallet.com/
 */
//#define LOG_LOCAL_LEVEL 4

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sodium.h>
#include "esp_log.h"
#include "cJSON.h"

#include "nano_lib.h"
#include "jolttypes.h"
#include "nano_parse.h"

#if CONFIG_NANOPARSE_BUILD_W_LWS
#include "nano_lws.h"
#define NANOPARSE_CMD_BUF_LEN 1024
#define NANOPARSE_RX_BUF_LEN 1024
#endif

static const char TAG[] = "nano_parse";


static char* deblank( char* str ) {
    /* Replaces all spaces from input string */

    int i, j;
    for (i = 0, j = 0; i<strlen(str); i++,j++) {
        if (str[i]!=' ' && str[i]!='\t') {
            str[j]=str[i];
        }
        else {
            j--;
        }
    }
    str[j]=0;
    return str;
}

static char* replace(char* str, char* a, char* b) {
    /* Replaces all occurences of string "a" with string "b" in str */
    if( strlen(b) > strlen(a) ){
        return NULL;
    }
    int len  = strlen(str);
    int lena = strlen(a), lenb = strlen(b);
    for (char* p = str; (p = strstr(p, a)); ++p) {
        if (lena != lenb) { // shift end as needed
            memmove(p+lenb, p+lena,
                    len - (p - str) - lena);
        }
        memcpy(p, b, lenb);
    }
    return str;
}

uint32_t nanoparse_block_count( const char *json_data ){
    /* Parses rai_node rpc response for action "block count"
     * Returns uint32_t network block count 
     * Returns 0 on error
     * */
    uint32_t count_int;
    const cJSON *count = NULL;
    cJSON *json = cJSON_Parse(json_data);
    
    count = cJSON_GetObjectItemCaseSensitive(json, "count");
    if ( cJSON_IsString(count) && (count->valuestring != NULL) ){
        count_int = atoi(count->valuestring);
    }
    else{
        ESP_LOGI(TAG, "unable to parse json");
        count_int = 0;
    }
    
    cJSON_Delete(json);
    return count_int;
}

jolt_err_t nanoparse_work( const char *json_data, uint64_t *work){
    /* Parses rai_node rpc response for "work_generate"
     * Returns uint64_t work */
    jolt_err_t outcome;
    const cJSON *json_work = NULL;
    cJSON *json = cJSON_Parse(json_data);
    
    json_work = cJSON_GetObjectItemCaseSensitive(json, "work");
    if (cJSON_IsString(json_work) && (json_work->valuestring != NULL)){
        outcome = nl_parse_server_work_string(json_work->valuestring, work);
        if( E_SUCCESS != outcome ){
            goto exit;
        }
        outcome = E_SUCCESS;
    }
    else{
        ESP_LOGE(TAG, "Work Parse Failure.");
        outcome = E_FAILURE;
        goto exit;
    }

    exit:
        cJSON_Delete(json);
        return outcome;
}

jolt_err_t nanoparse_account_frontier(const char *json_data, hex256_t frontier_block_hash){
    /* Returns the hash (33 characters) of the head block of the account */
    const cJSON *frontiers = NULL;
    const cJSON *account = NULL;
    jolt_err_t outcome = E_FAILURE;
    
    cJSON *json = cJSON_Parse((char *)json_data);
    frontiers = cJSON_GetObjectItemCaseSensitive(json, "frontiers");
    
    cJSON *current_element = NULL;
    char *current_key = NULL;
    cJSON_ArrayForEach(current_element, frontiers){
        current_key = current_element->string;
        if (current_key != NULL) {
            account = cJSON_GetObjectItemCaseSensitive(frontiers, current_key);
            if (cJSON_IsString(account) && (account->valuestring != NULL)){
                strlcpy(frontier_block_hash, account->valuestring, HEX_256);
                outcome = E_SUCCESS;
            }
        }
        break;
    }
    
    cJSON_Delete(json);
    return outcome;
}

jolt_err_t nanoparse_block(const char *json_data, nl_block_t *block){
    /* Parses rai_node rpc response to "block".
     * Returns populated block */
    uint8_t n_parse = 0, expected_n_parse;
    jolt_err_t outcome; // return value
    const cJSON *json_contents = NULL;
    const cJSON *json_type = NULL;
    const cJSON *json_previous = NULL;
    const cJSON *json_link = NULL;
    const cJSON *json_representative = NULL;
    const cJSON *json_account = NULL;
    const cJSON *json_balance = NULL;
    const cJSON *json_work = NULL;
    const cJSON *json_signature = NULL;
    cJSON *nested_json = NULL;
    char *content_string = NULL;
    
    ESP_LOGD(TAG, "Received json_data:\n%s\n", json_data);

    cJSON *json = cJSON_Parse((char *)json_data);
    if(!json){
        outcome = E_FAILURE;
        ESP_LOGI(TAG, "nanoparse_block: failed to parse json data.");
        goto exit;
    }

    json_contents = cJSON_GetObjectItemCaseSensitive(json, "contents");
    if(json_contents){
        content_string = cJSON_Print(json_contents);
        
        // remove double escaped newlines
        replace(content_string, "\\n", " ");
        if( NULL == content_string ) {
            outcome = E_FAILURE;
            goto exit;
        }

        // remove all double escaped slashes
        for (char* p = content_string; (p = strchr(p, '\\')); ++p) {
            *p = ' ';
        }
        
        // Remove Whitespace
        deblank(content_string);

        // Remove starting and end quotes
        content_string[0] = ' ';
        content_string[strlen(content_string)-1] = ' ';

        ESP_LOGI(TAG, "nanoparse_block: deblanked:\n %s", content_string);
        
        nested_json = cJSON_Parse(content_string);
    }
    else{
        nested_json = json;
    }

    /********************
     * Parse Block Type *
     ********************/
    json_type = cJSON_GetObjectItemCaseSensitive(nested_json, "type");
    if (cJSON_IsString(json_type) && (json_type->valuestring != NULL)){
        ESP_LOGI(TAG, "nanoparse_block: Type: %s", json_type->valuestring);

        if (strcmp(json_type->valuestring, "state") == 0){
            block->type = STATE;
            expected_n_parse = 6;
        }
        else if (strcmp(json_type->valuestring, "send") == 0){
            block->type = SEND;
            expected_n_parse = 4;
        }
        else if (strcmp(json_type->valuestring, "receive") == 0){
            block->type = RECEIVE;
            expected_n_parse = 3;
        }
        else if (strcmp(json_type->valuestring, "open") == 0){
            block->type = OPEN;
            expected_n_parse = 4;
        }
        else if (strcmp(json_type->valuestring, "change") == 0){
            block->type = CHANGE;
            expected_n_parse = 3;
        }
        else{
            ESP_LOGI(TAG, "nanoparse_block: 'type' field not recognized ");
            outcome = E_FAILURE;
            goto exit;
        }
        
        n_parse++;
        ESP_LOGD(TAG, "n_parse incremented: %d", n_parse);
        ESP_LOGI(TAG, "nanoparse_block: block.type: %d", block->type);
    }
    else {
        ESP_LOGI(TAG, "nanoparse_block: Unable to find key 'type' ");
        outcome = E_FAILURE;
        goto exit;
    }

    /*****************
     * Parse Account *
     *****************/
    json_account = cJSON_GetObjectItemCaseSensitive(nested_json, "account");
    if (cJSON_IsString(json_account) && (json_account->valuestring != NULL)){
        ESP_LOGI(TAG, "nanoparse_block: Account: %s", json_account->valuestring);
        outcome = nl_address_to_public(block->account, json_account->valuestring);
        if( E_SUCCESS != outcome){
            ESP_LOGE(TAG, "Bad \"account\"");
            goto exit;
        }
        n_parse++;
        ESP_LOGD(TAG, "n_parse incremented: %d", n_parse);
    }

    /******************
     * Parse Previous *
     ******************/
    json_previous = cJSON_GetObjectItemCaseSensitive(nested_json, "previous");
    if (cJSON_IsString(json_previous) && (json_previous->valuestring != NULL)){
        sodium_hex2bin(block->previous, sizeof(block->previous),
                       json_previous->valuestring,
                       HEX_256, NULL, NULL, NULL);
        ESP_LOGI(TAG, "nanoparse_block: Previous: %s", json_previous->valuestring);
        n_parse++;
        ESP_LOGD(TAG, "n_parse incremented: %d", n_parse);
    }

    /************************
     * Parse Representative *
     ************************/
    json_representative = cJSON_GetObjectItemCaseSensitive(nested_json, "representative");
    if (cJSON_IsString(json_representative) && (json_representative->valuestring != NULL)){
        ESP_LOGI(TAG, "nanoparse_block: Representative: %s", json_representative->valuestring);
        outcome = nl_address_to_public(block->representative, json_representative->valuestring);
        if( E_SUCCESS != outcome){
            ESP_LOGE(TAG, "Bad \"representative\"");
            goto exit;
        }
        n_parse++;
        ESP_LOGD(TAG, "n_parse incremented: %d", n_parse);
    }

    /*******************
     * Parse Signature *
     *******************/
    json_signature = cJSON_GetObjectItemCaseSensitive(nested_json, "signature");
    if (cJSON_IsString(json_signature) && (json_signature->valuestring != NULL)){
        ESP_LOGI(TAG, "nanoparse_block: Signature: %s", json_signature->valuestring);
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
        ESP_LOGI(TAG, "nanoparse_block: Link: %s", json_link->valuestring);
        n_parse++;
        ESP_LOGD(TAG, "n_parse incremented: %d", n_parse);
    }
    else if(block->type == SEND ){
        json_link = cJSON_GetObjectItemCaseSensitive(nested_json, "destination");
        if ( cJSON_IsString(json_link) && (json_link->valuestring != NULL) ){
            outcome = nl_address_to_public(block->link, json_link->valuestring);
            if( E_SUCCESS != outcome){
                ESP_LOGE(TAG, "Bad \"destination\"");
                goto exit;
            }
            n_parse++;
            ESP_LOGD(TAG, "n_parse incremented: %d", n_parse);
        }
    }

    /**************
     * Parse Work *
     **************/
    json_work = cJSON_GetObjectItemCaseSensitive(nested_json, "work");
    if (cJSON_IsString(json_work) && (json_work->valuestring != NULL)){
        ESP_LOGI(TAG, "nanoparse_block: Work: %s", json_work->valuestring);
        if(strlen(json_work->valuestring) == 16) {
            outcome = nl_parse_server_work_string(json_work->valuestring, &(block->work));
            if( E_SUCCESS != outcome){
                ESP_LOGE(TAG, "Bad \"work\"");
                goto exit;
            }
        }
        else {
            ESP_LOGE(TAG, "Bad \"work\"");
            outcome = E_FAILURE;
            goto exit;
        }
    }

    /*****************
     * Parse Balance *
     *****************/
    json_balance = cJSON_GetObjectItemCaseSensitive(nested_json, "balance");
    if (cJSON_IsString(json_balance) && (json_balance->valuestring != NULL)){
        ESP_LOGI(TAG, "nanoparse_block: Balance: %s\n", json_balance->valuestring);
        
        mbedtls_mpi current_balance;
        mbedtls_mpi_init(&current_balance);
        // todo: error handle mbed_mpi_read_string
        if (block->type == SEND){
            mbedtls_mpi_read_string(&current_balance, 16, json_balance->valuestring);
        }
        else {
            mbedtls_mpi_read_string(&current_balance, 10, json_balance->valuestring);
        }
        mbedtls_mpi_copy( &(block->balance), &current_balance);
        mbedtls_mpi_free( &current_balance );
        n_parse++;
        ESP_LOGD(TAG, "n_parse incremented: %d", n_parse);
    }

    /***********************
     * Confirm Parse Count *
     ***********************/
    if(n_parse == expected_n_parse){
        outcome = E_SUCCESS;
    }
    else{
        ESP_LOGE(TAG, "Parsed %d mandatory fields; expected to parse %d",
                n_parse, expected_n_parse);
        outcome = E_FAILURE;
        goto exit;
    }

    exit:
        if( NULL != content_string ) {
            free(content_string);
            cJSON_Delete(nested_json);
        }
        cJSON_Delete(json);
        return outcome;
}


jolt_err_t nanoparse_pending_hash( const char *json_data,
        hex256_t pending_block_hash, mbedtls_mpi *amount){
    jolt_err_t outcome = E_FAILURE;
    const cJSON *blocks = NULL;
    const cJSON *account = NULL;
    
    cJSON *json = cJSON_Parse(json_data);
    
    blocks = cJSON_GetObjectItemCaseSensitive(json, "blocks");

    cJSON *current_element = NULL;
    char *current_key = NULL;
    cJSON_ArrayForEach(current_element, blocks){
        current_key = current_element->string;
        if (current_key != NULL) {
            account = cJSON_GetObjectItemCaseSensitive(blocks, current_key);
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
                break;
            }
        }
        break;
    }
    
    cJSON_Delete(json);
    return outcome;
}

jolt_err_t nanoparse_process(const nl_block_t *block, char *buf, size_t buf_len){

    /* Account Address (convert bin to address) */
    char account_address[ADDRESS_BUF_LEN];
    jolt_err_t res;
    res = nl_public_to_address(account_address, sizeof(account_address),
            block->account);
    strlwr(account_address);
    ESP_LOGI(TAG, "process_block: Address: %s", account_address);
    
    /* Previous (convert bin to hex) */
    hex256_t previous_hex;
    sodium_bin2hex(previous_hex, sizeof(previous_hex),
                   block->previous, sizeof(block->previous));
    ESP_LOGI(TAG, "process_block: Previous: %s", previous_hex);
    
    /* Representative (convert bin to address) */
    char representative_address[ADDRESS_BUF_LEN];
    res = nl_public_to_address(representative_address,
            sizeof(representative_address), block->representative);
    strlwr(representative_address);
    ESP_LOGI(TAG, "process_block: Representative: %s", representative_address);
    
    /* Balance (convert mpi to string) */
    char balance_buf[66];
    size_t n;
    memset(balance_buf, 0, sizeof(balance_buf));
    mbedtls_mpi_write_string(&(block->balance), 10, balance_buf, sizeof(balance_buf)-1, &n);
    ESP_LOGI(TAG, "process_block: Balance: %s", balance_buf);
    
    /* Link (convert bin to hex) */
    hex256_t link_hex;
    sodium_bin2hex(link_hex, sizeof(link_hex),
                   block->link, sizeof(block->link));
    strupr(link_hex);
    ESP_LOGI(TAG, "process_block: Link: %s", link_hex);
    
    /* Work (keep as hex; byteswap) */
    hex64_t work;
    nl_generate_server_work_string(work, block->work);
    ESP_LOGI(TAG, "process_block: Work: %s", work);
    
    /* Signature (convert bin to hex) */
    hex512_t signature_hex;
    sodium_bin2hex(signature_hex, sizeof(signature_hex),
                   block->signature, sizeof(block->signature));
    strupr(signature_hex);
    ESP_LOGI(TAG, "Block->signature: %s", signature_hex);
   
    /* Combine into rai_node RPC command */
    int buf_req_len = snprintf( (char *) buf, buf_len,
            "{"
                "\"action\":\"process\","
                "\"block\":\"{"
                    "\\\"type\\\":\\\"state\\\","
                    "\\\"account\\\":\\\"%s\\\","
                    "\\\"previous\\\":\\\"%s\\\","
                    "\\\"representative\\\":\\\"%s\\\","
                    "\\\"balance\\\":\\\"%s\\\","
                    "\\\"link\\\":\\\"%s\\\","
                    "\\\"work\\\":\\\"%s\\\","
                    "\\\"signature\\\":\\\"%s\\\""
            "}\"}",
            account_address, previous_hex, representative_address, balance_buf,
            link_hex, work, signature_hex);

    if(buf_req_len > buf_len){
        return E_INSUFFICIENT_BUF;
    }
    else{
        ESP_LOGI(TAG, "\nprocess_block: Block: %s\n", buf);
        return E_SUCCESS;
    }
}
