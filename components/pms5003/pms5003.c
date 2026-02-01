#include "pms5003.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include <string.h>

static const char *TAG = "PMS5003";

// GPIO pins for PMS5003
#define PMS5003_RX_PIN 16      // RX on GPIO 16
#define PMS5003_TX_PIN 17      // TX on GPIO 17
#define PMS5003_SET_PIN 5      // SET pin on GPIO 5 (LOW = awake, HIGH = sleep)
#define PMS5003_UART_NUM UART_NUM_1
#define PMS5003_UART_BAUD 9600
#define PMS5003_BUF_SIZE 1024

// Thread-safe data storage
static pms5003_data_t current_data = {0};
static SemaphoreHandle_t pm_mutex = NULL;

/**
 * @brief Initialize PMS5003 sensor using ESP-IDF UART driver
 */
void pms5003_init(void)
{
    ESP_LOGI(TAG, "Initializing PMS5003 sensor...");
    
    // Configure GPIO5 as PMS5003_SET pin (LOW = awake, HIGH = sleep)
    gpio_config_t gpio_cfg = {
        .pin_bit_mask = (1ULL << PMS5003_SET_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&gpio_cfg);
    gpio_set_level(PMS5003_SET_PIN, 0);  // Start with sensor AWAKE (LOW)
    ESP_LOGI(TAG, "GPIO5 configured as PMS5003_SET (set to AWAKE)");
    
    // Configure UART1 for PMS5003
    uart_config_t uart_config = {
        .baud_rate = PMS5003_UART_BAUD,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };
    
    // Install UART driver with larger buffer
    ESP_ERROR_CHECK(uart_driver_install(PMS5003_UART_NUM, PMS5003_BUF_SIZE * 2, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(PMS5003_UART_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(PMS5003_UART_NUM, PMS5003_TX_PIN, PMS5003_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    
    ESP_LOGI(TAG, "UART1 configured (RX=GPIO%d, TX=GPIO%d, Baud=%d)", PMS5003_RX_PIN, PMS5003_TX_PIN, PMS5003_UART_BAUD);
    
    ESP_LOGI(TAG, "PMS5003 initialization complete");
}

/**
 * @brief Read data from PMS5003 sensor - matches Arduino implementation
 * Reads 32-byte frame including header with byte swapping
 */
bool pms5003_read(pms5003_data_t *data)
{
    uint8_t buffer[32];
    int length = 0;
    
    // Flush old data to get freshest reading (sensor sends data every ~1 second)
    uart_flush_input(PMS5003_UART_NUM);
    vTaskDelay(pdMS_TO_TICKS(50)); // Wait for fresh data
    
    // Try to read first byte with short timeout - if nothing comes, sensor is disconnected
    uint8_t byte;
    length = uart_read_bytes(PMS5003_UART_NUM, &byte, 1, pdMS_TO_TICKS(100));
    if (length != 1) {
        return false;  // Timeout - no data available, sensor likely disconnected
    }
    
    // If not frame start (0x42), discard and return false
    if (byte != 0x42) {
        return false;  // Not at frame start
    }
    
    // Store first header byte and read remaining 31 bytes
    buffer[0] = 0x42;
    length = uart_read_bytes(PMS5003_UART_NUM, buffer + 1, 31, pdMS_TO_TICKS(100));
    
    if (length != 31) {
        ESP_LOGW(TAG, "Failed to read complete frame, got %d bytes", length);
        return false;
    }
    
    // Verify second header byte
    if (buffer[1] != 0x4D) {
        ESP_LOGE(TAG, "Invalid frame header, got 0x%02X", buffer[1]);
        return false;
    }
    
    // Calculate checksum (sum of bytes 0-29, compare with bytes 30-31)
    uint16_t sum = 0;
    for (int i = 0; i < 30; i++) {
        sum += buffer[i];
    }
    
    uint16_t received_checksum = ((uint16_t)buffer[30] << 8) | buffer[31];
    
    if (sum != received_checksum) {
        ESP_LOGE(TAG, "Checksum error: calculated 0x%04X received 0x%04X", sum, received_checksum);
        return false;
    }
    
    // Parse with byte swapping - matching Arduino code exactly
    // Start from buffer[2] (after 0x42 0x4D header)
    uint16_t *ptr = (uint16_t*)(buffer + 2);
    uint16_t *data_ptr = (uint16_t*)data;
    
    // Read 15 uint16_t values (30 bytes) and byte-swap each
    for (int i = 0; i < 15; i++) {
        uint16_t v = ptr[i];
        // Byte swap: network byte order to native
        data_ptr[i] = (v >> 8) | (v << 8);
    }
    
    ESP_LOGI(TAG, "PM1.0: %u, PM2.5: %u, PM10: %u μg/m³", data->pm1_0, data->pm2_5, data->pm10);
    return true;
}

/**
 * @brief Put sensor into sleep mode
 */
void pms5003_sleep(void)
{
    ESP_LOGI(TAG, "PMS5003 sleep mode not implemented");
}

/**
 * @brief Wake sensor from sleep mode
 */
void pms5003_wake(void)
{
    ESP_LOGI(TAG, "PMS5003 wake mode not implemented");
}

/**
 * @brief Print sensor data to serial monitor
 */
void pms5003_print_data(const pms5003_data_t *data)
{
    printf("\n========== PMS5003 Sensor Data ==========\n");
    // printf("PM1.0:  %u μg/m³ (STD: %u μg/m³)\n", data->pm1_0, data->pm1_0_atm);
    // printf("PM2.5:  %u μg/m³ (STD: %u μg/m³)\n", data->pm2_5, data->pm2_5_atm);
    // printf("PM10:   %u μg/m³ (STD: %u μg/m³)\n", data->pm10, data->pm10_atm);
    printf("PM1.0:  %u μg/m³\n", data->pm1_0_atm);
    printf("PM2.5:  %u μg/m³\n", data->pm2_5_atm);
    printf("PM10:   %u μg/m³\n", data->pm10_atm);
    printf("\nParticle counts (per 0.1L air):\n");
    printf("  >0.3μm:  %u\n", data->particles_0_3);
    printf("  >0.5μm:  %u\n", data->particles_0_5);
    printf("  >1.0μm:  %u\n", data->particles_1_0);
    printf("  >2.5μm:  %u\n", data->particles_2_5);
    printf("  >5.0μm:  %u\n", data->particles_5_0);
    printf("  >10.0μm: %u\n", data->particles_10_0);
    printf("=========================================\n\n");
}

/**
 * @brief Background task for continuous sensor reading
 */
static void pms5003_read_task(void *pvParameters)
{
    uint32_t interval_ms = (uint32_t)(uintptr_t)pvParameters;
    pms5003_data_t data;
    
    ESP_LOGI(TAG, "PMS5003 sensor task started (interval: %lu ms)", interval_ms);
    
    for (;;) {
        if (pms5003_read(&data) == true) {
            // Store data with mutex protection
            if (pm_mutex != NULL) {
                if (xSemaphoreTake(pm_mutex, portMAX_DELAY) == pdTRUE) {
                    memcpy(&current_data, &data, sizeof(pms5003_data_t));
                    xSemaphoreGive(pm_mutex);
                }
            } else {
                memcpy(&current_data, &data, sizeof(pms5003_data_t));
            }
        } else {
            ESP_LOGW(TAG, "Failed to read from PMS5003 sensor");
        }
        
        vTaskDelay(pdMS_TO_TICKS(interval_ms));
    }
}

/**
 * @brief Start continuous sensor reading task
 */
void pms5003_start_task(uint32_t interval_ms)
{
    if (interval_ms < 1000) {
        interval_ms = 1000;  // Minimum 1 second
    }
    
    // Create mutex if not already created
    if (pm_mutex == NULL) {
        pm_mutex = xSemaphoreCreateMutex();
        if (pm_mutex == NULL) {
            ESP_LOGE(TAG, "Failed to create PM data mutex");
            return;
        }
    }
    
    xTaskCreate(pms5003_read_task, "pms5003_task", 4096, 
                (void *)(uintptr_t)interval_ms, 5, NULL);
    
    ESP_LOGI(TAG, "PMS5003 reading task created");
}

/**
 * @brief Get current PM1.0 value (thread-safe)
 */
uint16_t pms5003_get_pm1_0(void)
{
    uint16_t value = 0;
    
    if (pm_mutex != NULL) {
        if (xSemaphoreTake(pm_mutex, portMAX_DELAY) == pdTRUE) {
            value = current_data.pm1_0;
            xSemaphoreGive(pm_mutex);
        }
    } else {
        value = current_data.pm1_0;
    }
    
    return value;
}

/**
 * @brief Get current PM2.5 value (thread-safe)
 */
uint16_t pms5003_get_pm2_5(void)
{
    uint16_t value = 0;
    
    if (pm_mutex != NULL) {
        if (xSemaphoreTake(pm_mutex, portMAX_DELAY) == pdTRUE) {
            value = current_data.pm2_5;
            xSemaphoreGive(pm_mutex);
        }
    } else {
        value = current_data.pm2_5;
    }
    
    return value;
}

/**
 * @brief Get current PM10 value (thread-safe)
 */
uint16_t pms5003_get_pm10(void)
{
    uint16_t value = 0;
    
    if (pm_mutex != NULL) {
        if (xSemaphoreTake(pm_mutex, portMAX_DELAY) == pdTRUE) {
            value = current_data.pm10;
            xSemaphoreGive(pm_mutex);
        }
    } else {
        value = current_data.pm10;
    }
    
    return value;
}

/**
 * @brief Get all current sensor data (thread-safe)
 */
void pms5003_get_data(pms5003_data_t *data)
{
    if (data == NULL) {
        ESP_LOGE(TAG, "pms5003_get_data: NULL pointer");
        return;
    }
    
    if (pm_mutex != NULL) {
        if (xSemaphoreTake(pm_mutex, portMAX_DELAY) == pdTRUE) {
            memcpy(data, &current_data, sizeof(pms5003_data_t));
            xSemaphoreGive(pm_mutex);
        }
    } else {
        memcpy(data, &current_data, sizeof(pms5003_data_t));
    }
}
/**
 * @brief Control PMS5003 SET pin (GPIO5) from BACnet Binary Output
 * OFF/INACTIVE (0) = AWAKE (GPIO5 LOW)
 * ON/ACTIVE (1) = SLEEP (GPIO5 HIGH)
 */
void pms5003_set_gpio_from_bo(uint32_t state)
{
    uint32_t gpio_level = (state != 0) ? 1 : 0;  // 1 = sleep, 0 = awake
    gpio_set_level(PMS5003_SET_PIN, gpio_level);
    ESP_LOGI(TAG, "PMS5003_SET (GPIO5)=%lu (%s)",
             gpio_level, gpio_level ? "SLEEP" : "AWAKE");
}