#ifndef PWM_H
#define PWM_H

#include <stdint.h>

/* PWM GPIO assignments */
#define PWM_GPIO_1 12
#define PWM_GPIO_2 13

/* PWM channel indices */
#define PWM_CHANNEL_1 0
#define PWM_CHANNEL_2 1

/* Initialize PWM on GPIO12 and GPIO13 */
void pwm_init(void);

/* Set PWM duty cycle (0-100%) for selected channel */
void pwm_set_duty_percent(uint8_t channel, float duty_percent);

/* Set PWM output as 0-10V equivalent (0-10) for selected channel */
void pwm_set_voltage(uint8_t channel, float voltage);

#endif /* PWM_H */