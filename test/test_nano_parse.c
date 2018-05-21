#include "unity.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <esp_system.h>
#include "sodium.h"
#include "esp_log.h"

#include "nano_lib.h"
#include "nano_parse.h"

#define TEST_TAG "[nanoparse]"
static const char *TAG = "[nanoparse]";

TEST_CASE("Block Count", TEST_TAG){
    uint32_t count;
    const char *json_data = "{\n    \"count\": \"9493688\",\n    \"unchecked\": \"18360\"\n}\n";
    count = nanoparse_block_count(json_data);
    TEST_ASSERT_EQUAL_UINT(9493688, count);
}

TEST_CASE("Work", TEST_TAG){
    const char *json_data = "{\n    \"work\": \"bf0dc663d15668b6\"\n}\n";
    uint64_t work;
    nl_err_t err;
    err = nanoparse_work( json_data, &work);
    TEST_ASSERT_EQUAL_UINT(3512101046, work);
}

TEST_CASE("Account Frontier Hash", TEST_TAG){
    const char *json_data = "{\n    \"frontiers\": {\n        \"xrb_3tw77cfpwfnkqrjb988sh91tzerwu5dfnzxy8b3u76r7a7xwnkawm37ctcsb\": \"33832030C4F99FD37C8CD8399911D47150FCB90AE3A791970DBC8D05DFF93B8B\"\n    }\n}\n";
    hex256_t hash;
    nl_err_t err;
    err = nanoparse_account_frontier(json_data,
            "xrb_3tw77cfpwfnkqrjb988sh91tzerwu5dfnzxy8b3u76r7a7xwnkawm37ctcsb",
            hash);
    TEST_ASSERT_EQUAL_STRING(
            "33832030C4F99FD37C8CD8399911D47150FCB90AE3A791970DBC8D05DFF93B8B",
            hash);
}

TEST_CASE("Parse Open Block", TEST_TAG){
    TEST_IGNORE();
}

TEST_CASE("Parse Send Block", TEST_TAG){
    TEST_IGNORE();
}

TEST_CASE("Parse Receive Block", TEST_TAG){
    TEST_IGNORE();
}

TEST_CASE("Parse Change Block", TEST_TAG){
    TEST_IGNORE();
}

TEST_CASE("Parse State Block", TEST_TAG){
    TEST_IGNORE();
}


