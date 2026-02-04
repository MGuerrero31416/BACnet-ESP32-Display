#include "wifi_helper.h"
#include "esp_log.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lwip/inet.h"
#include "lwip/ip4_addr.h"

static const char *TAG = "wifi";

/* Replace with your SSID / PASS or read from menuconfig */
#define WIFI_SSID "BACnetBridge"        
#define WIFI_PASS "@Pi31416"

/* Set to 1 to use static IP, 0 for DHCP */
#define WIFI_USE_STATIC_IP 1

/* Static IP settings */
#define WIFI_STATIC_IP_ADDR    "10.120.245.92"
#define WIFI_STATIC_IP_GATEWAY "10.210.245.254"
#define WIFI_STATIC_IP_NETMASK "255.255.255.0"

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
#if WIFI_USE_STATIC_IP
    if (esp_netif) {
        esp_netif_dhcpc_stop(esp_netif);
        esp_netif_ip_info_t ip_info = {0};
        ip4_addr_t ip4 = {0};
        if (ip4addr_aton(WIFI_STATIC_IP_ADDR, &ip4)) {
            ip_info.ip.addr = ip4.addr;
        }
        if (ip4addr_aton(WIFI_STATIC_IP_GATEWAY, &ip4)) {
            ip_info.gw.addr = ip4.addr;
        }
        if (ip4addr_aton(WIFI_STATIC_IP_NETMASK, &ip4)) {
            ip_info.netmask.addr = ip4.addr;
        }
        esp_netif_set_ip_info(esp_netif, &ip_info);
        ESP_LOGI(TAG, "Using static IP %s", WIFI_STATIC_IP_ADDR);
    }
#endif

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
    /* Reuse handle for IP status */
    esp_netif = esp_netif ? esp_netif : esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");

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
