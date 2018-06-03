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
    TEST_ASSERT_EQUAL(E_SUCCESS, err);
    TEST_ASSERT_EQUAL_UINT(3512101046, work);
}

TEST_CASE("Account Frontier Hash", TEST_TAG){
    const char *json_data_1 = "{\n    \"frontiers\": {\n        \"xrb_3tw77cfpwfnkqrjb988sh91tzerwu5dfnzxy8b3u76r7a7xwnkawm37ctcsb\": \"33832030C4F99FD37C8CD8399911D47150FCB90AE3A791970DBC8D05DFF93B8B\"\n    }\n}\n";
    hex256_t hash;
    nl_err_t err;
    err = nanoparse_account_frontier(json_data_1, hash);
    TEST_ASSERT_EQUAL(E_SUCCESS, err);
    TEST_ASSERT_EQUAL_STRING(
            "33832030C4F99FD37C8CD8399911D47150FCB90AE3A791970DBC8D05DFF93B8B",
            hash);

    const char *json_data_2 = "{ \"hello\": \"world\" }"; 
    memset(hash, 0, sizeof(hash));
    err = nanoparse_account_frontier(json_data_2, hash);
    TEST_ASSERT_EQUAL(E_FAILURE, err);
    for(uint8_t i = 0; i < sizeof(hash); i++){
        TEST_ASSERT_EQUAL_UINT8(0, hash[i]);
    }
}

TEST_CASE("Parse Open Block", TEST_TAG){
    nl_err_t res;
    const char *json_data = "{\n    \"contents\": \"{\\n    \\\"type\\\": \\\"open\\\",\\n    \\\"source\\\": \\\"32E0D2FE367522FBFA29EB93940EC3AE5E1315DD9C6A73B3DE2A8BC683B64367\\\",\\n    \\\"representative\\\": \\\"xrb_3hd4ezdgsp15iemx7h81in7xz5tpxi43b6b41zn3qmwiuypankocw3awes5k\\\",\\n    \\\"account\\\": \\\"xrb_3dmtrrws3pocycmbqwawk6xs7446qxa36fcncush4s1pejk16ksbmakis78m\\\",\\n    \\\"work\\\": \\\"21bcc2816e10165d\\\",\\n    \\\"signature\\\": \\\"2CA07C59BF80B04515D49480EF0B5918BA29F998AB84120BB4B33A1A49BC028F0DB86CED729BF17B2CFF64F92011DC7F0089CDBF283C392F242A9F42DFA66000\\\"\\n}\\n\"\n}\n";

    // Setup Test Vector
    nl_block_t gt;
    nl_block_init( &gt );

    gt.type = OPEN;
    res = nl_address_to_public(gt.account, "xrb_3dmtrrws3pocycmbqwawk6xs7446qxa36fcncush4s1pejk16ksbmakis78m");
    TEST_ASSERT_EQUAL(E_SUCCESS, res);
    //gt.previous empty: no previous block
    res = nl_address_to_public(gt.representative, "xrb_3hd4ezdgsp15iemx7h81in7xz5tpxi43b6b41zn3qmwiuypankocw3awes5k");
    TEST_ASSERT_EQUAL(E_SUCCESS, res);

    nl_parse_server_work_string("21bcc2816e10165d", &(gt.work));
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
    nl_block_init( &pred );
    res = nanoparse_block(json_data, &pred);

    TEST_ASSERT_EQUAL(E_SUCCESS, res);
    TEST_ASSERT_TRUE(nl_block_equal(&(gt), &(pred)));

    nl_block_free( &gt );
    nl_block_free( &pred );
}

TEST_CASE("Parse Send Block", TEST_TAG){
    nl_err_t res;
    const char *json_data = "{\n    \"contents\": \"{\\n    \\\"type\\\": \\\"send\\\",\\n    \\\"previous\\\": \\\"66B2E0C0D2971A6372184FC851C959D4A2993749C78BA845D707873FB2C2EFDA\\\",\\n    \\\"destination\\\": \\\"xrb_3h94iuxwu48uzokokwa991a3okkwypiugsb5a1ehzwfw33dxrsuu154iw5qr\\\",\\n    \\\"balance\\\": \\\"0000000694140DC0A578AED10D000000\\\",\\n    \\\"work\\\": \\\"595ebaa13f83c1b2\\\",\\n    \\\"signature\\\": \\\"E523F20CAC1FF563F697C1D58E60FF0D72A9AC7B499799785490648E3F154FE4F464F6D7ECC4CD1A8072827E88F3D5805A8370F4A6DE06EDA8939E70E5113803\\\"\\n}\\n\"\n}\n";

    // Setup Test Vector
    nl_block_t gt;
    nl_block_init( &gt );

    gt.type = SEND;
    //gt.account empty: no account provided
    sodium_hex2bin(gt.previous, sizeof(gt.previous),
            "66B2E0C0D2971A6372184FC851C959D4A2993749C78BA845D707873FB2C2EFDA",
            HEX_256, NULL, NULL, NULL);
    //gt.representative empty: no account provided
    nl_parse_server_work_string("595ebaa13f83c1b2", &(gt.work));
    sodium_hex2bin(gt.signature, sizeof(gt.signature),
            "E523F20CAC1FF563F697C1D58E60FF0D72A9AC7B499799785490648E3F154FE4"
            "F464F6D7ECC4CD1A8072827E88F3D5805A8370F4A6DE06EDA8939E70E5113803",
            HEX_512, NULL, NULL, NULL);
    nl_address_to_public(gt.link, "xrb_3h94iuxwu48uzokokwa991a3okkwypiugsb5a1ehzwfw33dxrsuu154iw5qr");
    mbedtls_mpi_read_string(&(gt.balance), 16, "0000000694140DC0A578AED10D000000");
    
    // Test Parser
    nl_block_t pred;
    nl_block_init( &pred );
    res =  nanoparse_block(json_data, &pred);

    TEST_ASSERT_EQUAL(E_SUCCESS, res);
    TEST_ASSERT_TRUE(nl_block_equal(&(gt), &(pred)));

    nl_block_free( &gt );
    nl_block_free( &pred );
}

TEST_CASE("Parse Receive Block", TEST_TAG){
    nl_err_t res;
    const char *json_data = "{\n    \"contents\": \"{\\n    \\\"type\\\": \\\"receive\\\",\\n    \\\"previous\\\": \\\"755F515E56D7AE5467D454C61304320CA7363449580DE3B40B0F51C816C9A8F9\\\",\\n    \\\"source\\\": \\\"58C5B5344D85AAAEF1E7980B25E93DFB4834B6185EAD9A546D43F400370E1188\\\",\\n    \\\"work\\\": \\\"0c6589b8125613d8\\\",\\n    \\\"signature\\\": \\\"14EF1B6FA1CCD0B56EC2D8213A0708701BA322C3C3CB592D5C85D005CD3D51F24F4EE5954FE3C1EB5839004B7541E742AA1F7870FD81220A02319B96105D2D04\\\"\\n}\\n\"\n}\n";

    // Setup Test Vector
    nl_block_t gt;
    nl_block_init( &gt );

    gt.type = RECEIVE;
    //gt.account empty: no account provided
    sodium_hex2bin(gt.previous, sizeof(gt.previous),
            "755F515E56D7AE5467D454C61304320CA7363449580DE3B40B0F51C816C9A8F9",
            HEX_256, NULL, NULL, NULL);
    //gt.representative empty: no account provided
    nl_parse_server_work_string("0c6589b8125613d8", &(gt.work));
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
    nl_block_init( &pred );
    res =  nanoparse_block(json_data, &pred);

    TEST_ASSERT_EQUAL(E_SUCCESS, res);
    TEST_ASSERT_TRUE(nl_block_equal(&(gt), &(pred)));

    nl_block_free( &gt );
    nl_block_free( &pred );
}

TEST_CASE("Parse Change Block", TEST_TAG){
    nl_err_t res;
    const char *json_data = "{\n    \"contents\": \"{\\n    \\\"type\\\": \\\"change\\\",\\n    \\\"previous\\\": \\\"AF9C1D46AAE66CC8F827904ED02D4B3D95AA98B1FF058352BA6B670BEFD40231\\\",\\n    \\\"representative\\\": \\\"xrb_1cwswatjifmjnmtu5toepkwca64m7qtuukizyjxsghujtpdr9466wjmn89d8\\\",\\n    \\\"work\\\": \\\"e8c2c556c9cfb6e2\\\",\\n    \\\"signature\\\": \\\"A039A7BF5E54B44F45A8E1AD9940A81C87CC66C04AFA738367956629A5EF49E49D297FA3CDD195BDA8373D144F9E1D4641737E7F372CEAB5AD2F3B8E9852A30D\\\"\\n}\\n\"\n}\n";

    // Setup Test Vector
    nl_block_t gt;
    nl_block_init( &gt );

    gt.type = CHANGE;
    //gt.account empty: no account provided
    sodium_hex2bin(gt.previous, sizeof(gt.previous),
            "AF9C1D46AAE66CC8F827904ED02D4B3D95AA98B1FF058352BA6B670BEFD40231",
            HEX_256, NULL, NULL, NULL);
    nl_address_to_public(gt.representative, "xrb_1cwswatjifmjnmtu5toepkwca64m7qtuukizyjxsghujtpdr9466wjmn89d8");
    nl_parse_server_work_string("e8c2c556c9cfb6e2", &(gt.work));
    sodium_hex2bin(gt.signature, sizeof(gt.signature),
            "A039A7BF5E54B44F45A8E1AD9940A81C87CC66C04AFA738367956629A5EF49E4"
            "9D297FA3CDD195BDA8373D144F9E1D4641737E7F372CEAB5AD2F3B8E9852A30D",
            HEX_512, NULL, NULL, NULL);
    sodium_memzero(gt.link, sizeof(gt.link));
    //gt.balance empty: no balance provided
    
    // Test Parser
    nl_block_t pred;
    nl_block_init( &pred );
    res =  nanoparse_block(json_data, &pred);

    TEST_ASSERT_EQUAL(E_SUCCESS, res);
    TEST_ASSERT_TRUE(nl_block_equal(&(gt), &(pred)));

    nl_block_free( &gt );
    nl_block_free( &pred );
}

TEST_CASE("Parse State Block", TEST_TAG){
    nl_err_t res;
    const char *json_data = "{\n    \"contents\": \"{\\n    \\\"type\\\": \\\"state\\\",\\n    \\\"account\\\": \\\"xrb_1qzafeo4zpe6oykprr6oyb7jqgbkmezwfwzu3r99jbtfx8jyqe4p14h4d7pb\\\",\\n    \\\"previous\\\": \\\"6736060E4780522B1B89F5FFBE337CF5854171A06438E4929E4FEFC9211DA655\\\",\\n    \\\"representative\\\": \\\"xrb_1qzafeo4zpe6oykprr6oyb7jqgbkmezwfwzu3r99jbtfx8jyqe4p14h4d7pb\\\",\\n    \\\"balance\\\": \\\"0\\\",\\n    \\\"link\\\": \\\"5FE86B2A2FD984AFA56C6095F24B1BB9329B3FC6F3FB0E0E78A74DE9A3EBB056\\\",\\n    \\\"link_as_account\\\": \\\"xrb_1qzafeo4zpe6oykprr6oyb7jqgbkmezwfwzu3r99jbtfx8jyqe4p14h4d7pb\\\",\\n    \\\"signature\\\": \\\"A8702746CFE1F43F0C9AC427381A06F279B578F175FFB3111394AAFB8846DB8E9310976956AC1A2156BF75A462A195DD5574AD35975F262377573B46E2B62904\\\",\\n    \\\"work\\\": \\\"6aa2c8a6e053c0d4\\\"\\n}\\n\"\n}\n";

    // Setup Test Vector
    nl_block_t gt;
    nl_block_init( &gt );

    gt.type = STATE;
    nl_address_to_public(gt.account, "xrb_1qzafeo4zpe6oykprr6oyb7jqgbkmezwfwzu3r99jbtfx8jyqe4p14h4d7pb");
    sodium_hex2bin(gt.previous, sizeof(gt.previous),
            "6736060E4780522B1B89F5FFBE337CF5854171A06438E4929E4FEFC9211DA655",
            HEX_256, NULL, NULL, NULL);
    nl_address_to_public(gt.representative, "xrb_1qzafeo4zpe6oykprr6oyb7jqgbkmezwfwzu3r99jbtfx8jyqe4p14h4d7pb");
    nl_parse_server_work_string("6aa2c8a6e053c0d4", &(gt.work));
    sodium_hex2bin(gt.signature, sizeof(gt.signature),
            "A8702746CFE1F43F0C9AC427381A06F279B578F175FFB3111394AAFB8846DB8E"
            "9310976956AC1A2156BF75A462A195DD5574AD35975F262377573B46E2B62904",
            HEX_512, NULL, NULL, NULL);
    sodium_hex2bin(gt.link, sizeof(gt.link),
            "5FE86B2A2FD984AFA56C6095F24B1BB9329B3FC6F3FB0E0E78A74DE9A3EBB056",
            HEX_256, NULL, NULL, NULL);
    mbedtls_mpi_read_string(&(gt.balance), 10, "0");
    
    // Test Parser
    nl_block_t pred;
    nl_block_init( &pred );
    res =  nanoparse_block(json_data, &pred);

    TEST_ASSERT_EQUAL(E_SUCCESS, res);
    TEST_ASSERT_TRUE(nl_block_equal(&(gt), &(pred)));

    nl_block_free( &gt );
    nl_block_free( &pred );
}

TEST_CASE("Parse Malformed (no work)", TEST_TAG){
    nl_err_t res;
    const char *json_data = "{\n    \"contents\": \"{\\n    \\\"type\\\": \\\"state\\\",\\n    \\\"account\\\": \\\"xrb_1qzafeo4zpe6oykprr6oyb7jqgbkmezwfwzu3r99jbtfx8jyqe4p14h4d7pb\\\",\\n    \\\"previous\\\": \\\"6736060E4780522B1B89F5FFBE337CF5854171A06438E4929E4FEFC9211DA655\\\",\\n    \\\"representative\\\": \\\"xrb_1qzafeo4zpe6oykprr6oyb7jqgbkmezwfwzu3r99jbtfx8jyqe4p14h4d7pb\\\",\\n    \\\"balance\\\": \\\"0\\\",\\n    \\\"link\\\": \\\"5FE86B2A2FD984AFA56C6095F24B1BB9329B3FC6F3FB0E0E78A74DE9A3EBB056\\\",\\n    \\\"link_as_account\\\": \\\"xrb_1qzafeo4zpe6oykprr6oyb7jqgbkmezwfwzu3r99jbtfx8jyqe4p14h4d7pb\\\",\\n    \\\"signature\\\": \\\"A8702746CFE1F43F0C9AC427381A06F279B578F175FFB3111394AAFB8846DB8E9310976956AC1A2156BF75A462A195DD5574AD35975F262377573B46E2B62904\\\",\\n \\n}\\n\"\n}\n";

    // Test Parser
    nl_block_t pred;
    nl_block_init( &pred );
    res =  nanoparse_block(json_data, &pred);

    TEST_ASSERT_EQUAL(E_FAILURE, res);

    nl_block_free( &pred );
}

TEST_CASE("Pending Hash and Amount", TEST_TAG){
    nl_err_t res;
    hex256_t pending_hash;
    mbedtls_mpi amount;
    mbedtls_mpi_init( &amount );
    const char *json_data = "{\n    \"blocks\": {\n        \"xrb_1111111111111111111111111111111111111111111111111111hifc8npp\": {\n            \"00003F1C2F438F98F77771BBD140A58E976BF7A3B2D8EA8D9016DA7AED92EB14\": {\n                \"amount\": \"1\",\n                \"source\": \"xrb_1hnt56nto4id54wt66rpttwd4xgmzucohdpeacc3yzrnzr9g1tm9tkk9s8jc\"\n            }\n        }\n    }\n}\n";
    res = nanoparse_pending_hash( json_data, pending_hash, &amount);

    TEST_ASSERT_EQUAL(E_SUCCESS, res);
    TEST_ASSERT_EQUAL(0, mbedtls_mpi_cmp_int(&(amount), 1));

    mbedtls_mpi_free( &amount );
}
