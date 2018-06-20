/* nano_lib - ESP32 Any functions related to seed/private keys for Nano
 Copyright (C) 2018  Brian Pugh, James Coxon, Michael Smaili
 https://www.joltwallet.com/
 */

#include "unity.h"

#if CONFIG_NANOPARSE_BUILD_W_LWS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <esp_system.h>
#include "sodium.h"
#include "esp_log.h"

#include "nano_lib.h"
#include "nano_parse.h"

#define TEST_TAG "[nanoparse_lws]"
static const char *TAG = "[nanoparse_lws]";

#include "esp_event_loop.h"
#include "esp_wifi.h"
#include "esp_wpa2.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "wifi_credentials.h"


esp_err_t event_handler(void *ctx, system_event_t *event)
{
    uint8_t primary;
    wifi_second_chan_t second;
    esp_err_t err;
    
    switch(event->event_id) {
        case SYSTEM_EVENT_STA_START:
            ESP_LOGI(TAG, "SYSTEM_EVENT_STA_START");
            ESP_ERROR_CHECK(esp_wifi_connect());
            break;
        case SYSTEM_EVENT_STA_GOT_IP:
            err = esp_wifi_get_channel(&primary, &second);
            printf("channel=%d err=%d\n", primary, err);
            ESP_LOGI(TAG, "SYSTEM_EVENT_STA_GOT_IP");
            ESP_LOGI(TAG, "got ip:%s\n",
                     ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));
            break;
        case SYSTEM_EVENT_STA_DISCONNECTED:
            ESP_LOGI(TAG, "SYSTEM_EVENT_STA_DISCONNECTED");
            ESP_ERROR_CHECK(esp_wifi_connect());
            break;
        default:
            break;
    }
    return ESP_OK;
}

void wifi_connect(){
    wifi_config_t sta_config = {
        .sta = {
            .ssid      = WIFI_SSID,
            .password  = WIFI_PASS,
            .bssid_set = 0
        }
    };
    
    nvs_flash_init();
    tcpip_adapter_init();
    
    ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &sta_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_connect());
}
bool first_run = true;

static void wifi_setup(){
    if ( first_run ){
        wifi_connect();
        first_run = false;
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
}

TEST_CASE("WiFi Setup (debug util)", TEST_TAG){
    wifi_setup();
}

TEST_CASE("WiFi Block Count", TEST_TAG){
    wifi_setup();
    uint32_t count = nanoparse_lws_block_count();
    printf("Server Block Count: %d", count);
    TEST_ASSERT_MESSAGE(0 < count, "Block Count Zero");
}

TEST_CASE("WiFi Account Frontier Hash", TEST_TAG){
    hex256_t block_hash;
    nanoparse_lws_account_frontier(
            "xrb_3tw77cfpwfnkqrjb988sh91tzerwu5dfnzxy8b3u76r7a7xwnkawm37ctcsb",
            block_hash);
    TEST_ASSERT_EQUAL_STRING(
            "33832030C4F99FD37C8CD8399911D47150FCB90AE3A791970DBC8D05DFF93B8B",
            block_hash);
}

TEST_CASE("WiFi Account Frontier Block", TEST_TAG){
    nl_err_t res;

    /* Setup Test Vector */
    nl_block_t gt;
    sodium_memzero(&gt, sizeof(nl_block_t)); // Deterministic Test Compare
    nl_block_init( &gt );

    gt.type = STATE;
    nl_address_to_public(gt.account, "xrb_3h94iuxwu48uzokokwa991a3okkwypiugsb5a1ehzwfw33dxrsuu154iw5qr");
    sodium_hex2bin(gt.previous, sizeof(gt.previous),
            "99C7D1D10922710A9DA3ACECF6C060884E3CA1DE06C45667192119A66D2A5EBC",
            HEX_256, NULL, NULL, NULL);
    nl_address_to_public(gt.representative, "xrb_3h94iuxwu48uzokokwa991a3okkwypiugsb5a1ehzwfw33dxrsuu154iw5qr");
    gt.work = nl_parse_server_work_string("2d10d91bdc488a14");
    sodium_hex2bin(gt.signature, sizeof(gt.signature),
            "023C68D94EAB98ABB84DB5FE73D21130AF8D9984457491A59B6193C49E5633E3"
            "BC0932ECA1F6E536B876D7B7330CB7AC4A85B388BC59CC42C625BE1EB06AC90C",
            HEX_512, NULL, NULL, NULL);
    sodium_hex2bin(gt.link, sizeof(gt.link),
            "261F4B979009FD5665331AC72A95C8906285DCCAAACE2FD0F07AA39D738A1216",
            HEX_256, NULL, NULL, NULL);
    mbedtls_mpi_read_string(&(gt.balance), 10, "0");

    /* Test Parser */
    nl_block_t pred;
    sodium_memzero(&pred, sizeof(nl_block_t)); // Deterministic Test Compare
    nl_block_init( &pred );
    nl_address_to_public(pred.account, "xrb_3h94iuxwu48uzokokwa991a3okkwypiugsb5a1ehzwfw33dxrsuu154iw5qr");

    res =  nanoparse_lws_frontier_block(&pred);
    TEST_ASSERT_EQUAL(E_SUCCESS, res);
    TEST_ASSERT_EQUAL_MEMORY(&gt, &pred, sizeof(nl_block_t) - sizeof(mbedtls_mpi));
    TEST_ASSERT_EQUAL(0, mbedtls_mpi_cmp_mpi(&(gt.balance), &(pred.balance)));

    nl_block_free( &gt );
    nl_block_free( &pred );
}
#endif
