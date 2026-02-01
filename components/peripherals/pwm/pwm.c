#include "pwm.h"
#include "driver/ledc.h"
#include "esp_log.h"

#define PWM_FREQ_HZ 5000
#define PWM_RESOLUTION LEDC_TIMER_10_BIT
#define PWM_MAX_DUTY ((1 << 10) - 1)

static const char *TAG = "pwm";
static bool pwm_initialized = false;

static ledc_channel_t pwm_channel_from_index(uint8_t channel)
{
    switch (channel) {
        case PWM_CHANNEL_1:
            return LEDC_CHANNEL_0;
        case PWM_CHANNEL_2:
            return LEDC_CHANNEL_1;
        default:
            return LEDC_CHANNEL_0;
    }
}

static int pwm_gpio_from_index(uint8_t channel)
{
    switch (channel) {
        case PWM_CHANNEL_1:
            return PWM_GPIO_1;
        case PWM_CHANNEL_2:
            return PWM_GPIO_2;
        default:
            return PWM_GPIO_1;
    }
}

void pwm_init(void)
{
    if (pwm_initialized) {
        return;
    }

    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .timer_num = LEDC_TIMER_0,
        .duty_resolution = PWM_RESOLUTION,
        .freq_hz = PWM_FREQ_HZ,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ledc_timer_config(&ledc_timer);

    ledc_channel_config_t ch0 = {
        .gpio_num = PWM_GPIO_1,
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0,
        .hpoint = 0,
    };
    ledc_channel_config_t ch1 = {
        .gpio_num = PWM_GPIO_2,
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .channel = LEDC_CHANNEL_1,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0,
        .hpoint = 0,
    };

    ledc_channel_config(&ch0);
    ledc_channel_config(&ch1);

    pwm_initialized = true;
    ESP_LOGI(TAG, "PWM initialized: GPIO%d (CH0), GPIO%d (CH1)", PWM_GPIO_1, PWM_GPIO_2);
}

void pwm_set_duty_percent(uint8_t channel, float duty_percent)
{
    if (!pwm_initialized) {
        pwm_init();
    }

    if (duty_percent < 0.0f) {
        duty_percent = 0.0f;
    } else if (duty_percent > 100.0f) {
        duty_percent = 100.0f;
    }

    uint32_t duty = (uint32_t)((duty_percent / 100.0f) * PWM_MAX_DUTY);
    ledc_channel_t ledc_channel = pwm_channel_from_index(channel);
    ledc_set_duty(LEDC_HIGH_SPEED_MODE, ledc_channel, duty);
    ledc_update_duty(LEDC_HIGH_SPEED_MODE, ledc_channel);
}

void pwm_set_voltage(uint8_t channel, float voltage)
{
    if (voltage < 0.0f) {
        voltage = 0.0f;
    } else if (voltage > 10.0f) {
        voltage = 10.0f;
    }

    float duty_percent = (voltage / 10.0f) * 100.0f;
    pwm_set_duty_percent(channel, duty_percent);
    ESP_LOGI(TAG, "PWM CH%u (GPIO%d) set to %.2f V (%.1f%%)",
             (unsigned)channel, pwm_gpio_from_index(channel), voltage, duty_percent);
}