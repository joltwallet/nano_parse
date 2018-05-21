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

TEST_CASE("Head Block Hash", TEST_TAG){
    TEST_IGNORE();
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


