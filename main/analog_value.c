#include "analog_value.h"
#include <string.h>
#include <stdio.h>
#include "esp_log.h"
#include "nvs_flash.h"

/* bacnet-stack headers */
#include "bacnet/basic/object/av.h"

static const char *TAG = "bacnet_av";
#define NVS_NAMESPACE "bacnet"

/* Override NVS values with code defaults - set in main config */
extern int override_nvs_on_flash;

/* ===========================================================================================
 * ANALOG VALUE CONFIGURATION - 
 * =========================================================================================== */

/* Analog Value instances to create */
static const uint32_t AV_INSTANCES[] = { 1, 2, 3, 4 };

/* Analog Value names */
static const char *AV_NAMES[] = {
    "AV1",
    "AV2",
    "AV3",
    "AV4"
};

/* Analog Value descriptions (per instance) */
static const char *AV_DESCRIPTIONS[] = {
    "PM2.5 from PMS5003",
    "Analog Value 2",
    "Analog Value 3",
    "Analog Value 4"
};

/* Analog Value units (per instance) */
static const uint16_t AV_UNITS[] = {
    UNITS_MICROGRAMS_PER_CUBIC_METER,  /* PM2.5 */
    UNITS_DEGREES_CELSIUS,             /* AV2 - reserved */
    UNITS_DEGREES_CELSIUS,             /* AV3 - reserved */
    UNITS_DEGREES_CELSIUS              /* AV4 - reserved */
};

/* Analog Value initial values (per instance) */
static const float AV_INITIAL_VALUES[] = {
    0.0f,
    0.0f,
    0.0f,
    0.0f
};

/* Analog Value COV increment (per instance) */
static const float AV_COV_INCREMENTS[] = {
    1.0f,
    1.0f,
    1.0f,
    1.0f
};

/* ===========================================================================================
 * END CONFIGURATION
 * =========================================================================================== */

void bacnet_nvs_save_av_name(uint32_t instance, const char *name, uint16_t length) {
    nvs_handle_t nvs_handle;
    char key[32];
    char buf[65] = {0};
    esp_err_t err;
    snprintf(key, sizeof(key), "analog_%lu_name", (unsigned long)instance);
    if (name && length > 0 && length < sizeof(buf)) {
        memcpy(buf, name, length);
        buf[length] = 0;
    }
    if ((err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle)) == ESP_OK) {
        if ((err = nvs_set_str(nvs_handle, key, buf)) == ESP_OK) {
            if ((err = nvs_commit(nvs_handle)) == ESP_OK) {
                ESP_LOGI(TAG, "Saved AV%lu name: %s", (unsigned long)instance, buf);
            } else {
                ESP_LOGE(TAG, "NVS commit failed for AV%lu name: %d", (unsigned long)instance, err);
            }
        } else {
            ESP_LOGE(TAG, "NVS set_str failed for AV%lu name: %d", (unsigned long)instance, err);
        }
        nvs_close(nvs_handle);
    } else {
        ESP_LOGE(TAG, "NVS open failed for AV%lu name: %d", (unsigned long)instance, err);
    }
}

void bacnet_nvs_save_av_desc(uint32_t instance, const char *desc, uint16_t length) {
    nvs_handle_t nvs_handle;
    char key[32];
    char buf[129] = {0};
    esp_err_t err;
    snprintf(key, sizeof(key), "analog_%lu_desc", (unsigned long)instance);
    if (desc && length > 0 && length < sizeof(buf)) {
        memcpy(buf, desc, length);
        buf[length] = 0;
    }
    if ((err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle)) == ESP_OK) {
        if ((err = nvs_set_str(nvs_handle, key, buf)) == ESP_OK) {
            if ((err = nvs_commit(nvs_handle)) == ESP_OK) {
                ESP_LOGI(TAG, "Saved AV%lu desc: %s", (unsigned long)instance, buf);
            } else {
                ESP_LOGE(TAG, "NVS commit failed for AV%lu desc: %d", (unsigned long)instance, err);
            }
        } else {
            ESP_LOGE(TAG, "NVS set_str failed for AV%lu desc: %d", (unsigned long)instance, err);
        }
        nvs_close(nvs_handle);
    } else {
        ESP_LOGE(TAG, "NVS open failed for AV%lu desc: %d", (unsigned long)instance, err);
    }
}

void bacnet_nvs_save_av_units(uint32_t instance, uint16_t units) {
    nvs_handle_t nvs_handle;
    char key[32];
    esp_err_t err;
    snprintf(key, sizeof(key), "analog_%lu_unit", (unsigned long)instance);
    if ((err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle)) == ESP_OK) {
        if ((err = nvs_set_u16(nvs_handle, key, units)) == ESP_OK) {
            if ((err = nvs_commit(nvs_handle)) == ESP_OK) {
                ESP_LOGI(TAG, "Saved AV%lu units: %u", (unsigned long)instance, units);
            } else {
                ESP_LOGE(TAG, "NVS commit failed for AV%lu units: %d", (unsigned long)instance, err);
            }
        } else {
            ESP_LOGE(TAG, "NVS set_u16 failed for AV%lu units: %d", (unsigned long)instance, err);
        }
        nvs_close(nvs_handle);
    } else {
        ESP_LOGE(TAG, "NVS open failed for AV%lu units: %d", (unsigned long)instance, err);
    }
}

void bacnet_nvs_save_av_pv(uint32_t instance, float value) {
    nvs_handle_t nvs_handle;
    char key[32];
    esp_err_t err;
    snprintf(key, sizeof(key), "analog_%lu_val", (unsigned long)instance);
    if ((err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle)) == ESP_OK) {
        if ((err = nvs_set_blob(nvs_handle, key, &value, sizeof(value))) == ESP_OK) {
            if ((err = nvs_commit(nvs_handle)) == ESP_OK) {
                ESP_LOGI(TAG, "Saved AV%lu value: %.2f", (unsigned long)instance, value);
            } else {
                ESP_LOGE(TAG, "NVS commit failed for AV%lu: %d", (unsigned long)instance, err);
            }
        } else {
            ESP_LOGE(TAG, "NVS set_blob failed for AV%lu: %d", (unsigned long)instance, err);
        }
        nvs_close(nvs_handle);
    } else {
        ESP_LOGE(TAG, "NVS open failed for AV%lu: %d", (unsigned long)instance, err);
    }
}

void bacnet_nvs_load_av(uint32_t instance) {
    nvs_handle_t nvs_handle;
    char key[32];
    static char av_names[4][65];  /* Persistent storage for loaded names */
    static char av_descs[4][129];  /* Persistent storage for loaded descriptions */
    uint8_t idx = (instance > 0 && instance <= 4) ? (instance - 1) : 0;
    uint16_t units = UNITS_PERCENT;
    float pv = 0.0f;
    size_t len;

    if (nvs_open(NVS_NAMESPACE, NVS_READONLY, &nvs_handle) != ESP_OK) {
        return;  /* NVS not initialized yet */
    }

    snprintf(key, sizeof(key), "analog_%lu_name", (unsigned long)instance);
    len = sizeof(av_names[idx]);
    if (nvs_get_str(nvs_handle, key, av_names[idx], &len) == ESP_OK) {
        Analog_Value_Name_Set(instance, av_names[idx]);
    }

    snprintf(key, sizeof(key), "analog_%lu_desc", (unsigned long)instance);
    len = sizeof(av_descs[idx]);
    if (nvs_get_str(nvs_handle, key, av_descs[idx], &len) == ESP_OK) {
        Analog_Value_Description_Set(instance, av_descs[idx]);
    }

    snprintf(key, sizeof(key), "analog_%lu_unit", (unsigned long)instance);
    if (nvs_get_u16(nvs_handle, key, &units) == ESP_OK) {
        Analog_Value_Units_Set(instance, units);
    }

    snprintf(key, sizeof(key), "analog_%lu_val", (unsigned long)instance);
    len = sizeof(pv);
    if (nvs_get_blob(nvs_handle, key, &pv, &len) == ESP_OK) {
        Analog_Value_Present_Value_Set(instance, pv, 16);
    }

    nvs_close(nvs_handle);
}

void bacnet_create_analog_values(void) {
    size_t i = 0;
    size_t num_instances = sizeof(AV_INSTANCES) / sizeof(AV_INSTANCES[0]);

    for (i = 0; i < num_instances; i++) {
        uint32_t instance = AV_INSTANCES[i];
        Analog_Value_Create(instance);
        Analog_Value_Name_Set(instance, AV_NAMES[i]);
        Analog_Value_Description_Set(instance, AV_DESCRIPTIONS[i]);
        Analog_Value_Units_Set(instance, AV_UNITS[i]);
        Analog_Value_Present_Value_Set(instance, AV_INITIAL_VALUES[i], 16);
        Analog_Value_COV_Increment_Set(instance, AV_COV_INCREMENTS[i]);
        Analog_Value_Reliability_Set(instance, RELIABILITY_NO_FAULT_DETECTED);
        Analog_Value_Out_Of_Service_Set(instance, false);
        /* Load persisted values from NVS (if any) - unless override flag is set */
        if (!override_nvs_on_flash) {
            bacnet_nvs_load_av(instance);
        }
    }

    ESP_LOGI(TAG, "Created %zu Analog Value objects", num_instances);
}
