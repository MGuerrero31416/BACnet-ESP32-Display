#ifndef ANALOG_OUTPUT_H
#define ANALOG_OUTPUT_H

#include <stdint.h>
#include <stddef.h>

/* Create and initialize Analog Output objects */
void bacnet_create_analog_outputs(void);

/* NVS helper functions to persist AO properties */
void bacnet_nvs_save_ao_name(uint32_t instance, const char *name, uint16_t length);
void bacnet_nvs_save_ao_desc(uint32_t instance, const char *desc, uint16_t length);
void bacnet_nvs_save_ao_units(uint32_t instance, uint16_t units);
void bacnet_nvs_save_ao_pv(uint32_t instance, float value);
void bacnet_nvs_load_ao(uint32_t instance);

#endif /* ANALOG_OUTPUT_H */