#include "binary_output.h"
#include <string.h>
#include <stdio.h>
#include "esp_log.h"
#include "nvs_flash.h"

/* bacnet-stack headers */
#include "bacnet/basic/object/bo.h"

/* PMS5003 control header */
#include "pms5003.h"

/* FreeRTOS for GPIO sync task */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "bacnet_bo";
#define NVS_NAMESPACE "bacnet"

/* Override NVS values with code defaults - set in main config */
extern int override_nvs_on_flash;

/* ===========================================================================================
 * BINARY OUTPUT CONFIGURATION
 * =========================================================================================== */

/* Binary Output instances to create */
static const uint32_t BO_INSTANCES[] = { 1, 2, 3, 4 };

/* Binary Output names */
static const char *BO_NAMES[] = {
    "PMS5003_SET",
    "BO2",
    "BO3",
    "BO4"
};

/* Binary Output descriptions (per instance) */
static const char *BO_DESCRIPTIONS[] = {
    "PMS5003 Sleep Mode Control",
    "Binary Output 2",
    "Binary Output 3",
    "Binary Output 4"
};

/* Binary Output active/inactive text (per instance) */
static const char *BO_ACTIVE_TEXT[] = {
    "SLEEP",
    "ON",
    "ON",
    "ON"
};

static const char *BO_INACTIVE_TEXT[] = {
    "AWAKE",
    "OFF",
    "OFF",
    "OFF"
};

/* Binary Output initial states (per instance) */
static const BACNET_BINARY_PV BO_INITIAL_VALUES[] = {
    BINARY_INACTIVE,
    BINARY_INACTIVE,
    BINARY_INACTIVE,
    BINARY_INACTIVE
};

/* ===========================================================================================
 * END CONFIGURATION
 * =========================================================================================== */

void bacnet_nvs_save_bo_name(uint32_t instance, const char *name, uint16_t length) {
    nvs_handle_t nvs_handle;
    char key[32];
    char buf[65] = {0};
    esp_err_t err;
    snprintf(key, sizeof(key), "bo_%lu_name", (unsigned long)instance);
    if (name && length > 0 && length < sizeof(buf)) {
        memcpy(buf, name, length);
        buf[length] = 0;
    }
    if ((err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle)) == ESP_OK) {
        if ((err = nvs_set_str(nvs_handle, key, buf)) == ESP_OK) {
            if ((err = nvs_commit(nvs_handle)) == ESP_OK) {
                ESP_LOGI(TAG, "Saved BO%lu name: %s", (unsigned long)instance, buf);
            } else {
                ESP_LOGE(TAG, "NVS commit failed for BO%lu name: %d", (unsigned long)instance, err);
            }
        } else {
            ESP_LOGE(TAG, "NVS set_str failed for BO%lu name: %d", (unsigned long)instance, err);
        }
        nvs_close(nvs_handle);
    } else {
        ESP_LOGE(TAG, "NVS open failed for BO%lu name: %d", (unsigned long)instance, err);
    }
}

void bacnet_nvs_save_bo_desc(uint32_t instance, const char *desc, uint16_t length) {
    nvs_handle_t nvs_handle;
    char key[32];
    char buf[129] = {0};
    esp_err_t err;
    snprintf(key, sizeof(key), "bo_%lu_desc", (unsigned long)instance);
    if (desc && length > 0 && length < sizeof(buf)) {
        memcpy(buf, desc, length);
        buf[length] = 0;
    }
    if ((err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle)) == ESP_OK) {
        if ((err = nvs_set_str(nvs_handle, key, buf)) == ESP_OK) {
            if ((err = nvs_commit(nvs_handle)) == ESP_OK) {
                ESP_LOGI(TAG, "Saved BO%lu desc: %s", (unsigned long)instance, buf);
            } else {
                ESP_LOGE(TAG, "NVS commit failed for BO%lu desc: %d", (unsigned long)instance, err);
            }
        } else {
            ESP_LOGE(TAG, "NVS set_str failed for BO%lu desc: %d", (unsigned long)instance, err);
        }
        nvs_close(nvs_handle);
    } else {
        ESP_LOGE(TAG, "NVS open failed for BO%lu desc: %d", (unsigned long)instance, err);
    }
}

void bacnet_nvs_save_bo_pv(uint32_t instance, uint8_t value) {
    nvs_handle_t nvs_handle;
    char key[32];
    esp_err_t err;
    snprintf(key, sizeof(key), "bo_%lu_val", (unsigned long)instance);
    if ((err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle)) == ESP_OK) {
        if ((err = nvs_set_u8(nvs_handle, key, value)) == ESP_OK) {
            if ((err = nvs_commit(nvs_handle)) == ESP_OK) {
                ESP_LOGI(TAG, "Saved BO%lu value: %u", (unsigned long)instance, value);
            } else {
                ESP_LOGE(TAG, "NVS commit failed for BO%lu value: %d", (unsigned long)instance, err);
            }
        } else {
            ESP_LOGE(TAG, "NVS set_u8 failed for BO%lu value: %d", (unsigned long)instance, err);
        }
        nvs_close(nvs_handle);
    } else {
        ESP_LOGE(TAG, "NVS open failed for BO%lu value: %d", (unsigned long)instance, err);
    }
}

void bacnet_nvs_load_bo(uint32_t instance) {
    nvs_handle_t nvs_handle;
    char key[32];
    static char bo_names[4][65];  /* Persistent storage for loaded names */
    static char bo_descs[4][129];  /* Persistent storage for loaded descriptions */
    uint8_t idx = (instance > 0 && instance <= 4) ? (instance - 1) : 0;
    uint8_t pv = BINARY_INACTIVE;
    size_t len;

    if (nvs_open(NVS_NAMESPACE, NVS_READONLY, &nvs_handle) != ESP_OK) {
        return;  /* NVS not initialized yet */
    }

    snprintf(key, sizeof(key), "bo_%lu_name", (unsigned long)instance);
    len = sizeof(bo_names[idx]);
    if (nvs_get_str(nvs_handle, key, bo_names[idx], &len) == ESP_OK) {
        Binary_Output_Name_Set(instance, bo_names[idx]);
    }

    snprintf(key, sizeof(key), "bo_%lu_desc", (unsigned long)instance);
    len = sizeof(bo_descs[idx]);
    if (nvs_get_str(nvs_handle, key, bo_descs[idx], &len) == ESP_OK) {
        Binary_Output_Description_Set(instance, bo_descs[idx]);
    }

    snprintf(key, sizeof(key), "bo_%lu_val", (unsigned long)instance);
    if (nvs_get_u8(nvs_handle, key, &pv) == ESP_OK) {
        Binary_Output_Present_Value_Set(instance, (BACNET_BINARY_PV)pv, 16);
    }

    nvs_close(nvs_handle);
}

void bacnet_create_binary_outputs(void) {
    size_t i = 0;
    size_t num_instances = sizeof(BO_INSTANCES) / sizeof(BO_INSTANCES[0]);

    for (i = 0; i < num_instances; i++) {
        uint32_t instance = BO_INSTANCES[i];
        Binary_Output_Create(instance);
        Binary_Output_Name_Set(instance, BO_NAMES[i]);
        Binary_Output_Description_Set(instance, BO_DESCRIPTIONS[i]);
        Binary_Output_Active_Text_Set(instance, BO_ACTIVE_TEXT[i]);
        Binary_Output_Inactive_Text_Set(instance, BO_INACTIVE_TEXT[i]);
        Binary_Output_Present_Value_Set(instance, BO_INITIAL_VALUES[i], 16);
        Binary_Output_Out_Of_Service_Set(instance, false);
        /* Load persisted values from NVS (if any) - unless override flag is set */
        if (!override_nvs_on_flash) {
            bacnet_nvs_load_bo(instance);
        }
    }

    /* Set initial GPIO state for BO1 (PMS5003_SET) */
    uint8_t bo1_initial_state = Binary_Output_Present_Value(BO_INSTANCES[0]);
    pms5003_set_gpio_from_bo(bo1_initial_state);
    
    ESP_LOGI(TAG, "Created %zu Binary Output objects", num_instances);
}

/* Update GPIO5 when BO1 (PMS5003_SET) is written from BACnet */
void bacnet_bo1_gpio_update(uint8_t state)
{
    pms5003_set_gpio_from_bo(state);
}

/* Task to monitor BO1 and sync PMS5003_SET when value changes */
static uint8_t last_bo1_state = BINARY_INACTIVE;  /* Track last known BO1 state */

static void bo1_gpio_sync_task(void *pvParameters)
{
    (void)pvParameters;
    ESP_LOGI(TAG, "BO1 GPIO sync task started - monitoring BO1 for changes");
    
    while (1) {
        uint8_t current_bo1_state = Binary_Output_Present_Value(BO_INSTANCES[0]);
        
        /* If BO1 state changed, update PMS5003_SET */
        if (current_bo1_state != last_bo1_state) {
            ESP_LOGI(TAG, "BO1 state changed from %u to %u - updating PMS5003_SET", last_bo1_state, current_bo1_state);
            pms5003_set_gpio_from_bo(current_bo1_state);
            last_bo1_state = current_bo1_state;
        }
        
        vTaskDelay(pdMS_TO_TICKS(100));  /* Check every 100ms */
    }
}

void bacnet_create_binary_outputs_with_gpio_sync(void) {
    bacnet_create_binary_outputs();
    
    /* Start GPIO sync task */
    if (xTaskCreate(bo1_gpio_sync_task, "bo1_gpio_sync", 2048, NULL, tskIDLE_PRIORITY + 1, NULL) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create BO1 GPIO sync task");
    }
}
