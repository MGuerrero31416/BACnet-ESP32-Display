#include "wifi_helper.h"
#include "esp_log.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "wifi";

/* Replace with your SSID / PASS or read from menuconfig */
#define WIFI_SSID "UN-Guest"
#define WIFI_PASS ""

/* Simple Wi-Fi setup (blocking until connected) */
void wifi_init_sta(void)
{
    /* Initialize networking stack (only call once in app_main context) */
    /* esp_netif_init() and esp_event_loop_create_default() should be called before this */
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    esp_wifi_set_storage(WIFI_STORAGE_RAM);

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
            .threshold.authmode = WIFI_AUTH_OPEN,
        },
    };
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();

    ESP_LOGI(TAG, "Connecting to Wi-Fi %s ...", WIFI_SSID);
    esp_wifi_connect();

    /* Wait until IP is assigned - poll with longer timeout */
    int retry = 0;
    esp_netif_ip_info_t ip_info = {0};
    esp_netif_t *esp_netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");

    while (retry < 50) {  /* Wait up to 10 seconds (50 * 200ms) */
        if (esp_netif && esp_netif_get_ip_info(esp_netif, &ip_info) == ESP_OK &&
            ip_info.ip.addr != 0) {
            ESP_LOGI(TAG, "WiFi connected with IP: " IPSTR, IP2STR(&ip_info.ip));
            return;
        }
        vTaskDelay(pdMS_TO_TICKS(200));
        retry++;
    }

    ESP_LOGW(TAG, "WiFi connection timeout - proceeding anyway");
}
