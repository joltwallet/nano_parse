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

TEST_CASE("Account Frontier Hash", TEST_TAG){
    hex256_t block_hash;
    nanoparse_lws_account_frontier(
            "xrb_3tw77cfpwfnkqrjb988sh91tzerwu5dfnzxy8b3u76r7a7xwnkawm37ctcsb",
            block_hash);
    TEST_ASSERT_EQUAL_STRING(
            "33832030C4F99FD37C8CD8399911D47150FCB90AE3A791970DBC8D05DFF93B8B",
            block_hash);
}
#endif
