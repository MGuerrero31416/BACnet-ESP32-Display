#define USER_SETUP_INFO "ESP32-WROOM-32 1.9in ST7789 170x320"

// Driver
#define ST7789_DRIVER      // Standard for this panel; try ST7789_2_DRIVER if issues
//#define ST7789_2_DRIVER

// Resolution (working area is smaller than 136x240??)
#define TFT_WIDTH  170
#define TFT_HEIGHT 320 

// SPI Pins (fixed on board)
#define TFT_MOSI 23
#define TFT_SCLK 18
#define TFT_CS   15
#define TFT_DC    2
#define TFT_RST   4

// Backlight (active-high, as per your note)
#define TFT_BL   32
#define TFT_BACKLIGHT_ON HIGH  // HIGH to turn ON

// SPI Frequency (stable value)
#define SPI_FREQUENCY  20000000  // Was 27000000 or 40000000

// Color/Panel Options (colors OK, so keep)
#define TFT_RGB_ORDER TFT_BGR  // Or TFT_RGB if swapped
#define TFT_INVERSION_ON       // Comment out if washed out

// ST7789 specific offsets for your display
//#define TFT_COLSTART 17
//#define TFT_ROWSTART 40
//#define ST7789_COLEND (ST7789_COLSTART + TFT_WIDTH - 1)
//#define ST7789_ROWEND (ST7789_ROWSTART + TFT_HEIGHT - 1)
#define TFT_OFFSET_X 0
#define TFT_OFFSET_Y 0

// Offsets (0 fixed your left clipping)
//#define TFT_OFFSET_X 17   // Column start offset
//#define TFT_OFFSET_Y 40   // Row start offset

// Fonts (includes font 4 for your usage)
#define LOAD_GLCD   // Font 1. Original Adafruit font, default GLCD
#define LOAD_FONT2  // Font 2. Small 16px high font
#define LOAD_FONT4  // Font 4. Medium 26px high font
#define LOAD_FONT6  // Font 6. Large 48px font
#define LOAD_FONT7  // Font 7. 7 segment 48px font
#define LOAD_FONT8  // Font 8. Large 75px font
#define LOAD_GFXFF  // FreeFonts. Include access to the 48 GFX free fonts
#define SMOOTH_FONT // Smooth font anti-aliasing

// Touch (disabled)
#define TOUCH_CS -1