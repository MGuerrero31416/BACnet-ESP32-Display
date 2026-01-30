/* Minimal example: connect to Wi‑Fi and initialize BACnet. */
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "wifi_helper.h"
#include "display.h"
#include "analog_value.h"
#include "binary_value.h"

/* bacnet-stack headers */
#include "bacnet/basic/object/device.h"
#include "bacnet/basic/object/av.h"
#include "bacnet/basic/object/bv.h"
#include "bacnet/basic/service/s_iam.h"
#include "bacnet/basic/tsm/tsm.h"
#include "bacnet/bacaddr.h"
#include "bacnet/basic/bbmd/h_bbmd.h"
#include "bacnet/datalink/datalink.h"
#include "bacnet/datalink/bip.h"
/* service handlers from bacnet-stack library */
#include "bacnet/basic/services.h"
#include "bacnet/basic/service/h_rp.h"
#include "bacnet/basic/service/h_rpm.h"
#include "bacnet/basic/service/h_wp.h"
#include "bacnet/basic/service/h_whois.h"
#include "bacnet/basic/service/h_cov.h"
#include "bacnet/npdu.h"
#include "bacnet/basic/npdu/h_npdu.h"

static const char *TAG = "bacnet";

/* BACnet device settings */
#define BACNET_DEVICE_INSTANCE 31416

// ========================================================================================
/* Set to 1 to override NVS values with code defaults on flash, 0 to use persisted values */
#define OVERRIDE_NVS_ON_FLASH 0
int override_nvs_on_flash = OVERRIDE_NVS_ON_FLASH;  /* Exported for AV/BV modules */
// =========================================================================================

/* BBMD foreign device registration */
#define BBMD_IP_OCTET_1 10
#define BBMD_IP_OCTET_2 113
#define BBMD_IP_OCTET_3 33
#define BBMD_IP_OCTET_4 1
#define BBMD_PORT 0xBAC0
#define BBMD_TTL_SECONDS 600

static void bacnet_register_with_bbmd(void);
static void bacnet_receive_task(void *pvParameters);
static void bacnet_cov_task(void *pvParameters);
static TaskHandle_t bacnet_cov_task_handle = NULL;

/* BACnet receive task - processes incoming BACnet messages */
static void bacnet_receive_task(void *pvParameters)
{
    (void)pvParameters;
    BACNET_ADDRESS src = {0};
    static uint8_t rx_buffer[600];  /* Smaller buffer in DRAM */
    uint16_t pdu_len = 0;

    ESP_LOGI(TAG, "BACnet receive task started");

    while (1) {
        /* Poll for incoming BACnet messages */
        memset(&src, 0, sizeof(src));
        pdu_len = bip_receive(&src, rx_buffer, sizeof(rx_buffer), 100);
        if (pdu_len > 0) {
            /* Save original source from UDP socket before NPDU decode modifies it */
            BACNET_ADDRESS orig_src = src;
            BACNET_ADDRESS dest = {0};
            BACNET_NPDU_DATA npdu_data = {0};
            int apdu_offset = bacnet_npdu_decode(
                rx_buffer, pdu_len, &dest, &src, &npdu_data);
            /* If NPDU didn't have source routing info, restore from UDP socket */
            if (src.len == 0) {
                src = orig_src;
            }
            if (apdu_offset > 0 && apdu_offset < (int)pdu_len) {
                apdu_handler(&src, &rx_buffer[apdu_offset], pdu_len - apdu_offset);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

/* Application entry: init Wi‑Fi and BACnet */
void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    
    /* If OVERRIDE_NVS_ON_FLASH is set, always erase NVS to reset to code defaults */
    if (OVERRIDE_NVS_ON_FLASH) {
        ESP_LOGI(TAG, "Override flag set - erasing NVS to reset to defaults");
        nvs_flash_erase();
        ret = nvs_flash_init();
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to reinitialize NVS after erase: %d", ret);
        } else {
            ESP_LOGI(TAG, "NVS reinitialized successfully");
        }
    } else if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGI(TAG, "NVS needs initialization");
        nvs_flash_erase();
        nvs_flash_init();
    } else if (ret == ESP_OK) {
        ESP_LOGI(TAG, "NVS initialized from existing data");
    }

    /* Initialize network stack (must be done before WiFi init) */
    esp_netif_init();
    esp_event_loop_create_default();

    wifi_init_sta();

    ESP_LOGI(TAG, "Initializing BACnet stack");
    datalink_set("bip");
    if (!datalink_init(NULL)) {
        ESP_LOGE(TAG, "Failed to initialize BACnet datalink");
        return;
    }

    bacnet_register_with_bbmd();

    Device_Init(NULL);
    Device_Set_Object_Instance_Number(BACNET_DEVICE_INSTANCE);
    Device_Set_Vendor_Identifier(260);
    Device_Object_Name_ANSI_Init("ESP32-BACnet");

    /* Register service handlers - using bacnet-stack library handlers */
    ESP_LOGI(TAG, "Registering BACnet service handlers");
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_WHO_IS, handler_who_is);
    apdu_set_unrecognized_service_handler_handler(handler_unrecognized_service);
    /* Read Property - REQUIRED for BACnet devices */
    apdu_set_confirmed_handler(SERVICE_CONFIRMED_READ_PROPERTY, handler_read_property);
    apdu_set_confirmed_handler(SERVICE_CONFIRMED_READ_PROP_MULTIPLE, handler_read_property_multiple);
    apdu_set_confirmed_handler(SERVICE_CONFIRMED_WRITE_PROPERTY, handler_write_property);
    apdu_set_confirmed_handler(SERVICE_CONFIRMED_SUBSCRIBE_COV, handler_cov_subscribe);
    apdu_set_confirmed_handler(SERVICE_CONFIRMED_SUBSCRIBE_COV_PROPERTY, handler_cov_subscribe_property);

    /* Initialize COV subscription list */
    handler_cov_init();

    /* Create BACnet objects (AV and BV) */
    bacnet_create_analog_values();
    bacnet_create_binary_values();

    ESP_LOGI(TAG, "Broadcasting I-Am");
    Send_I_Am(Handler_Transmit_Buffer);

    /* Initialize display */
    ESP_LOGI(TAG, "Initializing display");
    display_init();

    /* Start BACnet receive task to handle incoming messages */
    if (xTaskCreate(bacnet_receive_task, "bacnet_rx", 16384, NULL, 5, NULL) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create bacnet_rx task");
    }
    if (xTaskCreate(bacnet_cov_task, "bacnet_cov", 24576, NULL, 4, &bacnet_cov_task_handle) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create bacnet_cov task");
    }

    ESP_LOGI(TAG, "BACnet stack initialized - idle");

    /* Keep the task alive - maintenance + display updates */
    uint32_t display_tick = 0;
    while (1) {
        datalink_maintenance_timer(1);
        
        /* Update display every 2 seconds */
        if (++display_tick % 2 == 0) {
            float av1 = Analog_Value_Present_Value(1);
            float av2 = Analog_Value_Present_Value(2);
            float av3 = Analog_Value_Present_Value(3);
            float av4 = Analog_Value_Present_Value(4);
            int bv1 = Binary_Value_Present_Value(1);
            int bv2 = Binary_Value_Present_Value(2);
            int bv3 = Binary_Value_Present_Value(3);
            int bv4 = Binary_Value_Present_Value(4);
            display_update_values(av1, av2, av3, av4, bv1, bv2, bv3, bv4);
        }
        
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/* COV task - handles COV timer and notifications */
static void bacnet_cov_task(void *pvParameters)
{
    (void)pvParameters;
    uint32_t tick = 0;
    while (1) {
        handler_cov_timer_seconds(1);
        handler_cov_task();
        if (++tick % 60 == 0) {
            UBaseType_t watermark = uxTaskGetStackHighWaterMark(NULL);
            ESP_LOGI(TAG, "bacnet_cov stack watermark: %u", (unsigned)watermark);
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

static void bacnet_register_with_bbmd(void)
{
    BACNET_IP_ADDRESS bbmd_addr = { { BBMD_IP_OCTET_1, BBMD_IP_OCTET_2,
                                     BBMD_IP_OCTET_3, BBMD_IP_OCTET_4 },
                                    BBMD_PORT };
    int result = bvlc_register_with_bbmd(&bbmd_addr, BBMD_TTL_SECONDS);
    ESP_LOGI(TAG, "BBMD register result: %d", result);
}