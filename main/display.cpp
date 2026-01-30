#include "display.h"
#include <Arduino.h>
#include <TFT_eSPI.h>
#include <stdio.h>

static TFT_eSPI tft = TFT_eSPI();

// Display boundary constants (visible area: 135x239 pixels)
#define DISP_X0    17      // Left edge
#define DISP_Y0    40      // Top edge
#define DISP_X1    151     // Right edge (17 + 135 - 1)
#define DISP_Y1    278     // Bottom edge (40 + 239 - 1)
#define DISP_WIDTH 135
#define DISP_HEIGHT 239

// Helper: Calculate center coordinates
#define DISP_CENTER_X  ((DISP_X0 + DISP_X1) / 2)
#define DISP_CENTER_Y  ((DISP_Y0 + DISP_Y1) / 2)

extern "C" void display_init(void) {
    // Initialize Arduino framework
    initArduino();
    
    // Turn on backlight (GPIO 32)
    pinMode(32, OUTPUT);
    digitalWrite(32, HIGH);
    
    // Initialize TFT
    tft.begin();
    tft.setRotation(0);  // Portrait mode
    tft.fillScreen(TFT_BLACK);
    
    // Draw header (top 30 pixels of display area)
    //tft.setTextColor(TFT_WHITE, TFT_BLUE);
    //tft.fillRect(DISP_X0, DISP_Y0, DISP_WIDTH, 30, TFT_BLUE);
    //tft.drawRect(DISP_X0, DISP_Y0, DISP_WIDTH, 30, TFT_CYAN);
    //tft.setTextSize(2);
    //tft.setCursor(DISP_X0 + 3, DISP_Y0 + 8);
    //tft.print("BACnet ESP32");
    
    // Draw labels (8 items with 30px spacing)
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.setTextSize(2);
    int label_y = DISP_Y0 + 10;
    int label_spacing = 30;
    
    tft.setCursor(DISP_X0 + 3, label_y);
    tft.print("AV1:");
    
    tft.setCursor(DISP_X0 + 3, label_y + label_spacing);
    tft.print("AV2:");
    
    tft.setCursor(DISP_X0 + 3, label_y + 2*label_spacing);
    tft.print("AV3:");
    
    tft.setCursor(DISP_X0 + 3, label_y + 3*label_spacing);
    tft.print("AV4:");
    
    tft.setCursor(DISP_X0 + 3, label_y + 4*label_spacing);
    tft.print("BV1:");
    
    tft.setCursor(DISP_X0 + 3, label_y + 5*label_spacing);
    tft.print("BV2:");
    
    tft.setCursor(DISP_X0 + 3, label_y + 6*label_spacing);
    tft.print("BV3:");
    
    tft.setCursor(DISP_X0 + 3, label_y + 7*label_spacing);
    tft.print("BV4:");
    
    printf("Display initialized\n");
}

extern "C" void display_update_values(float av1, float av2, float av3, float av4, int bv1, int bv2, int bv3, int bv4) {
    char buf[32];
    
    // Value area coordinates
    int value_x = DISP_X0 + 63;  // Right side for values
    int value_y = DISP_Y0 + 10;
    int value_spacing = 30;
    int value_width = 70;
    int value_height = 20;
    
    // Clear value areas (keep labels)
    for (int i = 0; i < 8; i++) {
        tft.fillRect(value_x, value_y + i*value_spacing, value_width, value_height, TFT_BLACK);
    }
    
    // Update values
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextSize(2);
    
    // AV1
    tft.setCursor(value_x, value_y);
    snprintf(buf, sizeof(buf), "%.1f", av1);
    tft.print(buf);
    
    // AV2
    tft.setCursor(value_x, value_y + value_spacing);
    snprintf(buf, sizeof(buf), "%.1f", av2);
    tft.print(buf);
    
    // AV3
    tft.setCursor(value_x, value_y + 2*value_spacing);
    snprintf(buf, sizeof(buf), "%.1f", av3);
    tft.print(buf);
    
    // AV4
    tft.setCursor(value_x, value_y + 3*value_spacing);
    snprintf(buf, sizeof(buf), "%.1f", av4);
    tft.print(buf);
    
    // BV1
    tft.setCursor(value_x, value_y + 4*value_spacing);
    tft.print(bv1 ? "ON " : "OFF");
    tft.fillCircle(value_x + 48, value_y + 4*value_spacing + 8, 8, bv1 ? TFT_GREEN : TFT_BLUE);
    
    // BV2
    tft.setCursor(value_x, value_y + 5*value_spacing);
    tft.print(bv2 ? "ON " : "OFF");
    tft.fillCircle(value_x + 48, value_y + 5*value_spacing + 8, 8, bv2 ? TFT_GREEN : TFT_BLUE);
    
    // BV3
    tft.setCursor(value_x, value_y + 6*value_spacing);
    tft.print(bv3 ? "ON " : "OFF");
    tft.fillCircle(value_x + 48, value_y + 6*value_spacing + 8, 8, bv3 ? TFT_GREEN : TFT_BLUE);
    
    // BV4
    tft.setCursor(value_x, value_y + 7*value_spacing);
    tft.print(bv4 ? "ON " : "OFF");
    tft.fillCircle(value_x + 48, value_y + 7*value_spacing + 8, 8, bv4 ? TFT_GREEN : TFT_BLUE);
}
