#ifndef DISPLAY_H
#define DISPLAY_H

#ifdef __cplusplus
extern "C" {
#endif

/* Initialize the TFT display with Arduino and TFT_eSPI */
void display_init(void);

/* Update display with BACnet object values */
void display_update_values(float av1, float av2, float av3, float av4, int bv1, int bv2, int bv3, int bv4);

#ifdef __cplusplus
}
#endif

#endif /* DISPLAY_H */
