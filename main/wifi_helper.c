#include "wifi_helper.h"
#include "esp_log.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "User_Settings.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lwip/inet.h"
#include "lwip/ip4_addr.h"

static const char *TAG = "wifi";

/* Simple Wi-Fi setup (blocking until connected) */
void wifi_init_sta(void)
{
    /* Initialize networking stack (only call once in app_main context) */
    /* esp_netif_init() and esp_event_loop_create_default() should be called before this */
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    esp_wifi_set_storage(WIFI_STORAGE_RAM);

    esp_netif_t *esp_netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    if (USER_WIFI_USE_STATIC_IP && esp_netif) {
        esp_netif_dhcpc_stop(esp_netif);
        esp_netif_ip_info_t ip_info = {0};
        ip4_addr_t ip4 = {0};
        if (ip4addr_aton(USER_WIFI_STATIC_IP_ADDR, &ip4)) {
            ip_info.ip.addr = ip4.addr;
        }
        if (ip4addr_aton(USER_WIFI_STATIC_IP_GATEWAY, &ip4)) {
            ip_info.gw.addr = ip4.addr;
        }
        if (ip4addr_aton(USER_WIFI_STATIC_IP_NETMASK, &ip4)) {
            ip_info.netmask.addr = ip4.addr;
        }
        esp_netif_set_ip_info(esp_netif, &ip_info);
        ESP_LOGI(TAG, "Using static IP %s", USER_WIFI_STATIC_IP_ADDR);
    }

    wifi_config_t wifi_config = { 0 };
    strncpy((char *)wifi_config.sta.ssid, USER_WIFI_SSID, sizeof(wifi_config.sta.ssid) - 1);
    strncpy((char *)wifi_config.sta.password, USER_WIFI_PASS, sizeof(wifi_config.sta.password) - 1);
    wifi_config.sta.threshold.authmode = WIFI_AUTH_OPEN;
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();

    ESP_LOGI(TAG, "Connecting to Wi-Fi %s ...", USER_WIFI_SSID);
    esp_wifi_connect();

    /* Wait until IP is assigned - poll with longer timeout */
    int retry = 0;
    esp_netif_ip_info_t ip_info = {0};
    /* Reuse handle for IP status */
    esp_netif = esp_netif ? esp_netif : esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");

    while (retry < 50) {  /* Wait up to 10 seconds (50 * 200ms) */
        if (esp_netif && esp_netif_get_ip_info(esp_netif, &ip_info) == ESP_OK &&
            ip_info.ip.addr != 0) {
            ESP_LOGI(TAG, "WiFi connected with IP: " IPSTR, IP2STR(&ip_info.ip));
            ESP_LOGI(TAG, "WiFi connected to SSID: %s", USER_WIFI_SSID);
            return;
        }
        vTaskDelay(pdMS_TO_TICKS(200));
        retry++;
    }

    ESP_LOGW(TAG, "WiFi connection timeout - proceeding anyway");
}
