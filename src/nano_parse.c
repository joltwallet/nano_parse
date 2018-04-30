#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sodium.h>
#include "cJSON.h"

#ifdef ESP32
#include "nano_lib.h"
#include "nano_parse.h"
#include "nano_lws.h"
#else
#include "../components/nano_lib/include/nano_lib.h"
#include "../include/nano_parse.h"
#include "../components/nano_lws/include/nano_lws.h"
#endif

#include "helpers.h"

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

int get_block_count(){
    /* Works; returns the integer processed block count */
    int count_int;
    char rpc_command[1024];
    char rx_string[1024];
    
    snprintf( (char *) rpc_command, 1024, "{\"action\":\"block_count\"}" );
    network_get_data((unsigned char *)rpc_command, (unsigned char *)rx_string);
    
    const cJSON *count = NULL;
    cJSON *json = cJSON_Parse((char *)rx_string);
    
    count = cJSON_GetObjectItemCaseSensitive(json, "count");
    if (cJSON_IsString(count) && (count->valuestring != NULL))
    {
        count_int = atoi(count->valuestring);
    }
    else{
        count_int = 0;
    }
    
    cJSON_Delete(json);
    
    return count_int;
}

int get_work(nl_block_t *block){
    
    //Previous (convert bin to hex)
    hex512_t previous_hex;
    sodium_bin2hex(previous_hex, sizeof(previous_hex),
                   block->previous, sizeof(block->previous));
    printf("Previous: %s\n", previous_hex);
    
    unsigned char rpc_command[512];
    unsigned char rx_string[1024];
    
    snprintf( (char *) rpc_command, 512, "{\"action\":\"work_generate\",\"hash\":\"%s\"}", previous_hex );
    
    network_get_data((unsigned char *)rpc_command, (unsigned char *)rx_string);
    
    const cJSON *json_work = NULL;
    cJSON *json = cJSON_Parse((char *)rx_string);
    
    json_work = cJSON_GetObjectItemCaseSensitive(json, "work");
    if (cJSON_IsString(json_work) && (json_work->valuestring != NULL))
    {
        block->work = strtoull(json_work->valuestring, NULL, 16);
    }
    else{
        printf("Error\n");
    }
    
    cJSON_Delete(json);
    
    return 0;
    
}

int get_frontier(char *account_address, char *frontier_block_hash){
    
    int outcome;
    
    strlower(account_address);
    
    unsigned char rpc_command[512];
    unsigned char rx_string[1024];
    
    snprintf( (char *) rpc_command, 512,
            "{\"action\":\"accounts_frontiers\",\"accounts\":[\"%s\"]}",
            account_address);
    
    network_get_data((unsigned char *)rpc_command, (unsigned char *)rx_string);
    
    const cJSON *frontiers = NULL;
    const cJSON *account = NULL;
    
    cJSON *json = cJSON_Parse((char *)rx_string);
    
    frontiers = cJSON_GetObjectItemCaseSensitive(json, "frontiers");
    
    account = cJSON_GetObjectItemCaseSensitive(frontiers, account_address);
    
    if (cJSON_IsString(account) && (account->valuestring != NULL))
    {
        strcpy(frontier_block_hash, account->valuestring);
        outcome = 1;
    }
    else{
        outcome = 0;
    }
    
    cJSON_Delete(json);
    
    return outcome;
}

int get_head(nl_block_t *block){
    
    char account_address[ADDRESS_BUF_LEN];
    
    //Now convert this to an XRB address
    nl_err_t res;
    res = nl_public_to_address(account_address,
                               sizeof(account_address),
                               block->account);
    strupper(account_address);
    printf("Address: %s\n", account_address);
    
    //Get latest block from server
    //First get frontier
    char frontier_block_hash[65];
    
    get_frontier(account_address, frontier_block_hash);
    //strcpy(frontier_block_hash, "54E3CDEEDF790136FF8FD47105D1008F46BA42A1EC7790A1B43E1AC381EDFA80");
    printf("Frontier Block: %s\n", frontier_block_hash);
    
    //Now get the block info
    unsigned char rpc_command[1024];
    unsigned char rx_string[1024];
    
    snprintf( (char *) rpc_command, 1024,
             "{\"action\":\"block\",\"hash\":\"%s\"}",
             frontier_block_hash);
    
    network_get_data(rpc_command, rx_string);
    
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
        printf("Account: %s\n", json_account->valuestring);
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
        printf("Representative: %s\n", json_representative->valuestring);
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
        block->work = json_work->valuestring;
    }
    
    json_balance = cJSON_GetObjectItemCaseSensitive(nested_json, "balance");
    if (cJSON_IsString(json_balance) && (json_balance->valuestring != NULL))
    {
        printf("Balance: %s\n", json_balance->valuestring);
        mbedtls_mpi current_balance;
        printf("1\n");
        mbedtls_mpi_init(&current_balance);
        printf("2\n");
        mbedtls_mpi_read_string(&current_balance, 10, json_balance->valuestring);
        printf("3\n");
        mbedtls_mpi_copy( &(block->balance), &current_balance);
        printf("4\n");
        mbedtls_mpi_free (&current_balance);
        printf("5\n");
        
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
    printf("Address: %s\n", account_address);
    
    //Previous (convert bin to hex)
    hex512_t previous_hex;
    sodium_bin2hex(previous_hex, sizeof(previous_hex),
                   block->previous, sizeof(block->previous));
    printf("Previous: %s\n", previous_hex);
    
    //Representative (convert bin to address)
    char representative_address[ADDRESS_BUF_LEN];
    res = nl_public_to_address(representative_address,
                               sizeof(representative_address),
                               block->representative);
    strlower(representative_address);
    printf("Representative: %s\n", representative_address);
    
    //Balance (convert mpi to string)
    char balance_buf[64];
    size_t n;
    memset(balance_buf, 0, sizeof(balance_buf));
    mbedtls_mpi_write_string(&(block->balance), 10, balance_buf, sizeof(balance_buf)-1, &n);
    printf("Balance: %s\n", balance_buf);
    
    //Link (convert bin to hex)
    hex256_t link_hex;
    sodium_bin2hex(link_hex, sizeof(link_hex),
                   block->link, sizeof(block->link));
    printf("Link: %s\n", link_hex);
    
    //Work (keep as hex)
    printf("Work: %llx\n", block->work);
    
    //Signature (convert bin to hex)
    hex512_t signature_hex;
    sodium_bin2hex(signature_hex, sizeof(signature_hex),
                   block->signature, sizeof(block->signature));
    printf("Block->signature: %s\n", signature_hex);
    
    unsigned char new_block[1024];
    int error = snprintf( (char *) new_block,1024,
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
                         "\\\"work\\\":\\\"%llx\\\","
                         "\\\"signature\\\":\\\"%s\\\""
                         "}"
                         "\"}",
                                     account_address, previous_hex, representative_address, balance_buf, link_hex, block->work, signature_hex
                         );
    
    printf("\nBlock: %d\n%s\n", error, new_block);
    
    unsigned char rx_string[1024];
    
    network_get_data(new_block, rx_string);
    
    printf("%s\n", rx_string);
    
    
    return 0;
}
