#include "unity.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <esp_system.h>
#include "sodium.h"
#include "esp_log.h"
#include "mbedtls/bignum.h"

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
    nl_err_t res;
    const char *json_data = "{\n    \"contents\": \"{\\n    \\\"type\\\": \\\"open\\\",\\n    \\\"source\\\": \\\"32E0D2FE367522FBFA29EB93940EC3AE5E1315DD9C6A73B3DE2A8BC683B64367\\\",\\n    \\\"representative\\\": \\\"xrb_3hd4ezdgsp15iemx7h81in7xz5tpxi43b6b41zn3qmwiuypankocw3awes5k\\\",\\n    \\\"account\\\": \\\"xrb_3dmtrrws3pocycmbqwawk6xs7446qxa36fcncush4s1pejk16ksbmakis78m\\\",\\n    \\\"work\\\": \\\"21bcc2816e10165d\\\",\\n    \\\"signature\\\": \\\"2CA07C59BF80B04515D49480EF0B5918BA29F998AB84120BB4B33A1A49BC028F0DB86CED729BF17B2CFF64F92011DC7F0089CDBF283C392F242A9F42DFA66000\\\"\\n}\\n\"\n}\n";

    // Setup Test Vector
    nl_block_t gt;
    sodium_memzero(&gt, sizeof(nl_block_t)); // Deterministic Test Compare
    nl_block_init( &gt );

    gt.type = OPEN;
    nl_address_to_public(gt.account, "xrb_3dmtrrws3pocycmbqwawk6xs7446qxa36fcncush4s1pejk16ksbmakis78m");
    //gt.previous empty: no previous block
    nl_address_to_public(gt.representative, "xrb_3hd4ezdgsp15iemx7h81in7xz5tpxi43b6b41zn3qmwiuypankocw3awes5k");
    gt.work = nl_parse_server_work_string("21bcc2816e10165d");
    sodium_hex2bin(gt.signature, sizeof(gt.signature),
            "2CA07C59BF80B04515D49480EF0B5918BA29F998AB84120BB4B33A1A49BC028F"
            "0DB86CED729BF17B2CFF64F92011DC7F0089CDBF283C392F242A9F42DFA66000",
            HEX_512, NULL, NULL, NULL);
    sodium_hex2bin(gt.link, sizeof(gt.link),
            "32E0D2FE367522FBFA29EB93940EC3AE5E1315DD9C6A73B3DE2A8BC683B64367",
            HEX_256, NULL, NULL, NULL);
    //gt.balance empty: Cant read balance from an Open Block
    
    // Test Parser
    nl_block_t pred;
    sodium_memzero(&pred, sizeof(nl_block_t)); // Deterministic Test Compare
    nl_block_init( &pred );
    res =  nanoparse_block(json_data, &pred);

    TEST_ASSERT_EQUAL(E_SUCCESS, res);
    // Can't memcmp mbedtls_mpi since its a pointer
    TEST_ASSERT_EQUAL_MEMORY(&gt, &pred, sizeof(nl_block_t) - sizeof(mbedtls_mpi));
    TEST_ASSERT_EQUAL(0, mbedtls_mpi_cmp_mpi(&(gt.balance), &(pred.balance)));

    nl_block_free( &gt );
    nl_block_free( &pred );
}

TEST_CASE("Parse Send Block", TEST_TAG){
    nl_err_t res;
    const char *json_data = "{\n    \"contents\": \"{\\n    \\\"type\\\": \\\"send\\\",\\n    \\\"previous\\\": \\\"66B2E0C0D2971A6372184FC851C959D4A2993749C78BA845D707873FB2C2EFDA\\\",\\n    \\\"destination\\\": \\\"xrb_3h94iuxwu48uzokokwa991a3okkwypiugsb5a1ehzwfw33dxrsuu154iw5qr\\\",\\n    \\\"balance\\\": \\\"0000000694140DC0A578AED10D000000\\\",\\n    \\\"work\\\": \\\"595ebaa13f83c1b2\\\",\\n    \\\"signature\\\": \\\"E523F20CAC1FF563F697C1D58E60FF0D72A9AC7B499799785490648E3F154FE4F464F6D7ECC4CD1A8072827E88F3D5805A8370F4A6DE06EDA8939E70E5113803\\\"\\n}\\n\"\n}\n";

    // Setup Test Vector
    nl_block_t gt;
    sodium_memzero(&gt, sizeof(nl_block_t)); // Deterministic Test Compare
    nl_block_init( &gt );

    gt.type = SEND;
    //gt.account empty: no account provided
    sodium_hex2bin(gt.previous, sizeof(gt.previous),
            "66B2E0C0D2971A6372184FC851C959D4A2993749C78BA845D707873FB2C2EFDA",
            HEX_256, NULL, NULL, NULL);
    //gt.representative empty: no account provided
    gt.work = nl_parse_server_work_string("595ebaa13f83c1b2");
    sodium_hex2bin(gt.signature, sizeof(gt.signature),
            "E523F20CAC1FF563F697C1D58E60FF0D72A9AC7B499799785490648E3F154FE4"
            "F464F6D7ECC4CD1A8072827E88F3D5805A8370F4A6DE06EDA8939E70E5113803",
            HEX_512, NULL, NULL, NULL);
    nl_address_to_public(gt.link, "xrb_3h94iuxwu48uzokokwa991a3okkwypiugsb5a1ehzwfw33dxrsuu154iw5qr");
    mbedtls_mpi_read_string(&(gt.balance), 16, "0000000694140DC0A578AED10D000000");
    
    // Test Parser
    nl_block_t pred;
    sodium_memzero(&pred, sizeof(nl_block_t)); // Deterministic Test Compare
    nl_block_init( &pred );
    res =  nanoparse_block(json_data, &pred);

    TEST_ASSERT_EQUAL(E_SUCCESS, res);
    // Can't memcmp mbedtls_mpi since its a pointer
    TEST_ASSERT_EQUAL_MEMORY(&gt, &pred, sizeof(nl_block_t) - sizeof(mbedtls_mpi));
    TEST_ASSERT_EQUAL(0, mbedtls_mpi_cmp_mpi(&(gt.balance), &(pred.balance)));

    nl_block_free( &gt );
    nl_block_free( &pred );
}

TEST_CASE("Parse Receive Block", TEST_TAG){
    nl_err_t res;
    const char *json_data = "{\n    \"contents\": \"{\\n    \\\"type\\\": \\\"receive\\\",\\n    \\\"previous\\\": \\\"755F515E56D7AE5467D454C61304320CA7363449580DE3B40B0F51C816C9A8F9\\\",\\n    \\\"source\\\": \\\"58C5B5344D85AAAEF1E7980B25E93DFB4834B6185EAD9A546D43F400370E1188\\\",\\n    \\\"work\\\": \\\"0c6589b8125613d8\\\",\\n    \\\"signature\\\": \\\"14EF1B6FA1CCD0B56EC2D8213A0708701BA322C3C3CB592D5C85D005CD3D51F24F4EE5954FE3C1EB5839004B7541E742AA1F7870FD81220A02319B96105D2D04\\\"\\n}\\n\"\n}\n";

    // Setup Test Vector
    nl_block_t gt;
    sodium_memzero(&gt, sizeof(nl_block_t)); // Deterministic Test Compare
    nl_block_init( &gt );

    gt.type = RECEIVE;
    //gt.account empty: no account provided
    sodium_hex2bin(gt.previous, sizeof(gt.previous),
            "755F515E56D7AE5467D454C61304320CA7363449580DE3B40B0F51C816C9A8F9",
            HEX_256, NULL, NULL, NULL);
    //gt.representative empty: no account provided
    gt.work = nl_parse_server_work_string("0c6589b8125613d8");
    sodium_hex2bin(gt.signature, sizeof(gt.signature),
            "14EF1B6FA1CCD0B56EC2D8213A0708701BA322C3C3CB592D5C85D005CD3D51F2"
            "4F4EE5954FE3C1EB5839004B7541E742AA1F7870FD81220A02319B96105D2D04",
            HEX_512, NULL, NULL, NULL);
    sodium_hex2bin(gt.link, sizeof(gt.link),
            "58C5B5344D85AAAEF1E7980B25E93DFB4834B6185EAD9A546D43F400370E1188",
            HEX_256, NULL, NULL, NULL);
    //gt.balance empty: no balance provided
    
    // Test Parser
    nl_block_t pred;
    sodium_memzero(&pred, sizeof(nl_block_t)); // Deterministic Test Compare
    nl_block_init( &pred );
    res =  nanoparse_block(json_data, &pred);

    TEST_ASSERT_EQUAL(E_SUCCESS, res);
    // Can't memcmp mbedtls_mpi since its a pointer
    TEST_ASSERT_EQUAL_MEMORY(&gt, &pred, sizeof(nl_block_t) - sizeof(mbedtls_mpi));
    TEST_ASSERT_EQUAL(0, mbedtls_mpi_cmp_mpi(&(gt.balance), &(pred.balance)));

    nl_block_free( &gt );
    nl_block_free( &pred );
}

TEST_CASE("Parse Change Block", TEST_TAG){
    TEST_IGNORE();
}

TEST_CASE("Parse State Block", TEST_TAG){
    TEST_IGNORE();
}


