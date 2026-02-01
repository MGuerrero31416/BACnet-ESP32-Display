#include "analog_output.h"
#include <string.h>
#include <stdio.h>
#include "esp_log.h"
#include "nvs_flash.h"

/* bacnet-stack headers */
#include "bacnet/basic/object/ao.h"

/* PWM peripheral */
#include "pwm.h"

static const char *TAG = "bacnet_ao";
#define NVS_NAMESPACE "bacnet"

/* Override NVS values with code defaults - set in main config */
extern int override_nvs_on_flash;

/* ===========================================================================================
 * ANALOG OUTPUT CONFIGURATION
 * =========================================================================================== */

/* Analog Output instances to create */
static const uint32_t AO_INSTANCES[] = { 1, 2 };

/* Analog Output names */
static const char *AO_NAMES[] = {
    "AO1",
    "AO2"
};

/* Analog Output descriptions (per instance) */
static const char *AO_DESCRIPTIONS[] = {
    "PWM 0-10V Output 1 (GPIO12)",
    "PWM 0-10V Output 2 (GPIO13)"
};

/* Analog Output units (per instance) */
static const uint16_t AO_UNITS[] = {
    UNITS_VOLTS,
    UNITS_VOLTS
};

/* Analog Output initial values (per instance) */
static const float AO_INITIAL_VALUES[] = {
    0.0f,
    0.0f
};

/* Analog Output COV increment (per instance) */
static const float AO_COV_INCREMENTS[] = {
    0.1f,
    0.1f
};

/* ===========================================================================================
 * END CONFIGURATION
 * =========================================================================================== */

static uint8_t ao_index_from_instance(uint32_t instance)
{
    return (instance > 0 && instance <= 2) ? (uint8_t)(instance - 1) : 0;
}

static uint8_t ao_pwm_channel_from_instance(uint32_t instance)
{
    switch (instance) {
        case 1:
            return PWM_CHANNEL_1;
        case 2:
            return PWM_CHANNEL_2;
        default:
            return PWM_CHANNEL_1;
    }
}

static void analog_output_write_callback(uint32_t object_instance, float old_value, float value)
{
    (void)old_value;
    uint8_t pwm_channel = ao_pwm_channel_from_instance(object_instance);
    pwm_set_voltage(pwm_channel, value);
}

void bacnet_nvs_save_ao_name(uint32_t instance, const char *name, uint16_t length)
{
    nvs_handle_t nvs_handle;
    char key[32];
    char buf[65] = {0};
    esp_err_t err;
    snprintf(key, sizeof(key), "ao_%lu_name", (unsigned long)instance);
    if (name && length > 0 && length < sizeof(buf)) {
        memcpy(buf, name, length);
        buf[length] = 0;
    }
    if ((err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle)) == ESP_OK) {
        if ((err = nvs_set_str(nvs_handle, key, buf)) == ESP_OK) {
            if ((err = nvs_commit(nvs_handle)) == ESP_OK) {
                ESP_LOGI(TAG, "Saved AO%lu name: %s", (unsigned long)instance, buf);
            } else {
                ESP_LOGE(TAG, "NVS commit failed for AO%lu name: %d", (unsigned long)instance, err);
            }
        } else {
            ESP_LOGE(TAG, "NVS set_str failed for AO%lu name: %d", (unsigned long)instance, err);
        }
        nvs_close(nvs_handle);
    } else {
        ESP_LOGE(TAG, "NVS open failed for AO%lu name: %d", (unsigned long)instance, err);
    }
}

void bacnet_nvs_save_ao_desc(uint32_t instance, const char *desc, uint16_t length)
{
    nvs_handle_t nvs_handle;
    char key[32];
    char buf[129] = {0};
    esp_err_t err;
    snprintf(key, sizeof(key), "ao_%lu_desc", (unsigned long)instance);
    if (desc && length > 0 && length < sizeof(buf)) {
        memcpy(buf, desc, length);
        buf[length] = 0;
    }
    if ((err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle)) == ESP_OK) {
        if ((err = nvs_set_str(nvs_handle, key, buf)) == ESP_OK) {
            if ((err = nvs_commit(nvs_handle)) == ESP_OK) {
                ESP_LOGI(TAG, "Saved AO%lu desc: %s", (unsigned long)instance, buf);
            } else {
                ESP_LOGE(TAG, "NVS commit failed for AO%lu desc: %d", (unsigned long)instance, err);
            }
        } else {
            ESP_LOGE(TAG, "NVS set_str failed for AO%lu desc: %d", (unsigned long)instance, err);
        }
        nvs_close(nvs_handle);
    } else {
        ESP_LOGE(TAG, "NVS open failed for AO%lu desc: %d", (unsigned long)instance, err);
    }
}

void bacnet_nvs_save_ao_units(uint32_t instance, uint16_t units)
{
    nvs_handle_t nvs_handle;
    char key[32];
    esp_err_t err;
    snprintf(key, sizeof(key), "ao_%lu_unit", (unsigned long)instance);
    if ((err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle)) == ESP_OK) {
        if ((err = nvs_set_u16(nvs_handle, key, units)) == ESP_OK) {
            if ((err = nvs_commit(nvs_handle)) == ESP_OK) {
                ESP_LOGI(TAG, "Saved AO%lu units: %u", (unsigned long)instance, units);
            } else {
                ESP_LOGE(TAG, "NVS commit failed for AO%lu units: %d", (unsigned long)instance, err);
            }
        } else {
            ESP_LOGE(TAG, "NVS set_u16 failed for AO%lu units: %d", (unsigned long)instance, err);
        }
        nvs_close(nvs_handle);
    } else {
        ESP_LOGE(TAG, "NVS open failed for AO%lu units: %d", (unsigned long)instance, err);
    }
}

void bacnet_nvs_save_ao_pv(uint32_t instance, float value)
{
    nvs_handle_t nvs_handle;
    char key[32];
    esp_err_t err;
    snprintf(key, sizeof(key), "ao_%lu_val", (unsigned long)instance);
    if ((err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle)) == ESP_OK) {
        if ((err = nvs_set_blob(nvs_handle, key, &value, sizeof(value))) == ESP_OK) {
            if ((err = nvs_commit(nvs_handle)) == ESP_OK) {
                ESP_LOGI(TAG, "Saved AO%lu value: %.2f", (unsigned long)instance, value);
            } else {
                ESP_LOGE(TAG, "NVS commit failed for AO%lu value: %d", (unsigned long)instance, err);
            }
        } else {
            ESP_LOGE(TAG, "NVS set_blob failed for AO%lu value: %d", (unsigned long)instance, err);
        }
        nvs_close(nvs_handle);
    } else {
        ESP_LOGE(TAG, "NVS open failed for AO%lu value: %d", (unsigned long)instance, err);
    }
}

void bacnet_nvs_load_ao(uint32_t instance)
{
    nvs_handle_t nvs_handle;
    char key[32];
    static char ao_names[2][65];
    static char ao_descs[2][129];
    uint8_t idx = ao_index_from_instance(instance);
    uint16_t units = UNITS_VOLTS;
    float pv = 0.0f;
    size_t len;

    if (nvs_open(NVS_NAMESPACE, NVS_READONLY, &nvs_handle) != ESP_OK) {
        return;  /* NVS not initialized yet */
    }

    snprintf(key, sizeof(key), "ao_%lu_name", (unsigned long)instance);
    len = sizeof(ao_names[idx]);
    if (nvs_get_str(nvs_handle, key, ao_names[idx], &len) == ESP_OK) {
        Analog_Output_Name_Set(instance, ao_names[idx]);
    }

    snprintf(key, sizeof(key), "ao_%lu_desc", (unsigned long)instance);
    len = sizeof(ao_descs[idx]);
    if (nvs_get_str(nvs_handle, key, ao_descs[idx], &len) == ESP_OK) {
        Analog_Output_Description_Set(instance, ao_descs[idx]);
    }

    snprintf(key, sizeof(key), "ao_%lu_unit", (unsigned long)instance);
    if (nvs_get_u16(nvs_handle, key, &units) == ESP_OK) {
        Analog_Output_Units_Set(instance, units);
    }

    snprintf(key, sizeof(key), "ao_%lu_val", (unsigned long)instance);
    len = sizeof(pv);
    if (nvs_get_blob(nvs_handle, key, &pv, &len) == ESP_OK) {
        Analog_Output_Present_Value_Set(instance, pv, 16);
    }

    nvs_close(nvs_handle);
}

void bacnet_create_analog_outputs(void)
{
    size_t i = 0;
    size_t num_instances = sizeof(AO_INSTANCES) / sizeof(AO_INSTANCES[0]);

    pwm_init();
    Analog_Output_Write_Present_Value_Callback_Set(analog_output_write_callback);

    for (i = 0; i < num_instances; i++) {
        uint32_t instance = AO_INSTANCES[i];
        Analog_Output_Create(instance);
        Analog_Output_Name_Set(instance, AO_NAMES[i]);
        Analog_Output_Description_Set(instance, AO_DESCRIPTIONS[i]);
        Analog_Output_Units_Set(instance, AO_UNITS[i]);
        Analog_Output_Min_Pres_Value_Set(instance, 0.0f);
        Analog_Output_Max_Pres_Value_Set(instance, 10.0f);
        Analog_Output_Present_Value_Set(instance, AO_INITIAL_VALUES[i], 16);
        Analog_Output_COV_Increment_Set(instance, AO_COV_INCREMENTS[i]);
        Analog_Output_Reliability_Set(instance, RELIABILITY_NO_FAULT_DETECTED);
        Analog_Output_Out_Of_Service_Set(instance, false);

        /* Load persisted values from NVS (if any) - unless override flag is set */
        if (!override_nvs_on_flash) {
            bacnet_nvs_load_ao(instance);
        }

        /* Apply initial PWM output */
        pwm_set_voltage(ao_pwm_channel_from_instance(instance),
            Analog_Output_Present_Value(instance));
    }

    ESP_LOGI(TAG, "Created %zu Analog Output objects", num_instances);
}