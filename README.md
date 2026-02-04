# ESP32 BACnet MS/TP WiFi Display

ESP32-based BACnet/IP device with ST7789 TFT display featuring 20 BACnet objects: 4 Analog Values, 4 Binary Values, 4 Analog Inputs, 4 Binary Inputs, and 4 Binary Outputs. Includes built-in PMS5003 air quality sensor for PM2.5/PM1.0/PM10 monitoring.

It simmultaneouslly connects the BACnet device through WiFi and MS/TP (using a MAX RS485 module)

You can easily add extra BACnet objects and map them to ESP32 GPIO for analog and digital inputs/outputs.

## Features

- **BACnet/IP Protocol**: Full BACnet/IP stack implementation
- **BACnet MS/TP**: RS485 MS/TP support alongside BACnet/IP
- **Live Display**: Real-time monitoring of BACnet objects on 170x320 TFT display
- **20 BACnet Objects**:
  - 4 Analog Values (AV1-4) - read/write with COV and NVS persistence
  - 4 Binary Values (BV1-4) - read/write with COV and NVS persistence
  - 4 Analog Inputs (AI1-4) - read-only sensor inputs with COV and NVS persistence
  - 4 Binary Inputs (BI1-4) - read-only binary states with COV and NVS persistence
  - 4 Binary Outputs (BO1-4) - writable control outputs with COV and NVS persistence
- **WiFi Connectivity**: ESP32 with built-in WiFi for BACnet/IP communication
- **Arduino Framework**: Leverages Arduino ecosystem for easy hardware control
- **Change of Value (COV)**: Implements BACnet COV notifications for efficient real-time updates
- **Persistent Storage**: Attribute values modifiable from BACnet supervisor are automatically saved to ESP32 non-volatile memory (NVS) for retention across power cycles
- **Air Quality Monitoring**: PMS5003 PM2.5/PM1.0/PM10 sensor with automatic BACnet integration

## Photos
![ESP32 BACnet](docs/images/ESP32_BACnet.jpg)

![YABE Tool](docs/images/YABE.png)

## Hardware Requirements

- **Microcontroller**: ESP32-WROOM-32
- **Display**: ST7789 SPI TFT (170x320 pixels)
- **Display Connections**:
  - MOSI: GPIO 23
  - SCLK: GPIO 18
  - CS: GPIO 15
  - DC: GPIO 2
  - RST: GPIO 4
  - BL (Backlight): GPIO 32

## Hardware Components

### ST7789 TFT Display
- **Resolution**: 170x320 pixels
- **Interface**: SPI (4-wire)
- **Driver**: Custom TFT_eSPI component with offset calibration for clone displays

### PMS5003 Air Quality Sensor
- **Model**: Plantower PMS5003
- **Communication**: UART (9600 baud, 8N1)
- **Connections**:
  - PMS5003 TX → ESP32 GPIO25 (RX1)
  - PMS5003 RX → ESP32 GPIO26 (TX1)
  - SET (Sleep Control): GPIO 27 (LOW = AWAKE, HIGH = SLEEP) — controlled by BO1 (PMS5003_SET)
  - Power: 5V (requires 5V supply, not 3.3V)
  - GND: ESP32 GND
- **Measurements**:
  - PM1.0 (atmospheric)
  - PM2.5 (atmospheric) → mapped to **Analog Value 1** in BACnet (configurable)
  - PM10 (atmospheric)
  - Particle counts (0.3µm - 10µm ranges)
- **BACnet Mapping**: By default, PM2.5 is written to AV1. You can select any sensor parameter (PM1.0, PM2.5, PM10, or particle counts) and map it to any Analog Value object (AV1-AV4) by modifying the pms5003_task in [main/main.c](main/main.c).
- **Update Frequency**: 2-second intervals
- **Response**: ~2 seconds to environmental changes
- **Features**:
  - Fast and responsive sensor readings
  - Automatic byte-swapping for big-endian protocol
  - Checksum validation on all frames
  - Sensor disconnect detection with BACnet error indication (-1 value)

### WiFi Connectivity
- Built-in ESP32 WiFi for BACnet/IP communication
- Configured via SSID/password in wifi_helper.c
- Static IP option in wifi_helper.c. Set WIFI_USE_STATIC_IP to 1 or 0 in wifi_helper.c

### BACnet MS/TP (RS485)
- **Transceiver**: MAX485 or equivalent RS485 converter
- **UART**: UART2
- **Connections**:
  - DI (TX) → ESP32 GPIO17
  - RO (RX) → ESP32 GPIO16
  - DE/RE → ESP32 GPIO5
- **Baud Rate**: 38400 (default)

## GPIO Summary

| Pin     | Component   | Signal              |
|---------|-------------|---------------------|
| GPIO 2  | TFT Display | DC (Data/Command)   |
| GPIO 4  | TFT Display | RST (Reset)         |
| GPIO 15 | TFT Display | CS (Chip Select)    |
| GPIO 16 | MAX485      | RO (RX)             |
| GPIO 17 | MAX485      | DI (TX)             |
| GPIO 5  | MAX485      | DE/RE               |
| GPIO 25 | PMS5003     | RX (sensor TX)      |
| GPIO 26 | PMS5003     | TX (sensor RX)      |
| GPIO 27 | PMS5003     | SET (Sleep Control) |
| GPIO 18 | TFT Display | SCLK (SPI Clock)    |
| GPIO 23 | TFT Display | MOSI (SPI Data)     |
| GPIO 32 | TFT Display | BACKLIGHT           |

## Build Requirements

- ESP-IDF v5.5.1
- Python 3.11+
- xtensa-esp-elf toolchain

## Building

```bash
cd c:\esp\BACnet-ESP32-Display
idf.py build
```

## Flashing

```bash
idf.py flash -p COM3
```

Or use the provided build/flash tasks in VS Code.

## Monitoring Serial Output

```bash
idf.py monitor -p COM3
```

## Configuration

### Display Offset Calibration

The ST7789 display has a framebuffer offset that's compensated in [components/TFT_eSPI/User_Setup.h](components/TFT_eSPI/User_Setup.h):

```c
#define TFT_COLSTART 17   // Horizontal offset
#define TFT_ROWSTART 40   // Vertical offset
```

These values are specific to cheap Chinese clones and may need adjustment for your hardware.

### FreeRTOS Configuration

Arduino framework requires FreeRTOS tick rate of 1000Hz. This is set in [sdkconfig](sdkconfig):

```
CONFIG_FREERTOS_HZ=1000
```

### BACnet Object Configuration

- **Analog Values (AV1-4)**: Configure names, descriptions, units, and initial values in [main/analog_value.c](main/analog_value.c) - **ANALOG VALUE CONFIGURATION** section

- **Binary Values (BV1-4)**: Configure names, descriptions, active/inactive text, and initial states in [main/binary_value.c](main/binary_value.c) - **BINARY VALUE CONFIGURATION** section

- **Analog Inputs (AI1-4)**: Configure names, descriptions, units, and COV increments in [main/analog_input.c](main/analog_input.c) - **ANALOG INPUT CONFIGURATION** section. Read-only inputs suitable for sensor integration.

- **Binary Inputs (BI1-4)**: Configure names, descriptions, active/inactive text in [main/binary_input.c](main/binary_input.c) - **BINARY INPUT CONFIGURATION** section. Read-only binary states.

- **Binary Outputs (BO1-4)**: Configure names, descriptions, active/inactive text, and initial states in [main/binary_output.c](main/binary_output.c) - **BINARY OUTPUT CONFIGURATION** section. Writable control outputs with priority support.

### Sensor Data Mapping

- **PMS5003 Parameters**: Select which sensor parameter (PM1.0, PM2.5, PM10, or particle counts) to map to each Analog Value object in [main/main.c](main/main.c) - look for `pms5003_task()` function where sensor data is written to BACnet objects. Currently, PM2.5 atmospheric is written to AV1.

## Architecture

### Components

- **[components/bacnet-stack](components/bacnet-stack)** - BACnet/IP stack (modified from bacnet-stack/bacnet-stack)
- **[components/TFT_eSPI](components/TFT_eSPI)** - TFT graphics library
- **[main](main/)** - Application code
  - `main.c` - BACnet initialization and main loop
  - `analog_value.c/h` - Analog Value object creation and NVS persistence
  - `binary_value.c/h` - Binary Value object creation and NVS persistence
  - `analog_input.c/h` - Analog Input object creation and NVS persistence
  - `binary_input.c/h` - Binary Input object creation and NVS persistence
  - `binary_output.c/h` - Binary Output object creation and NVS persistence
  - `display.cpp` - TFT display driver
  - `wifi_helper.c` - WiFi configuration helpers

### Display Layout

| Item | Type | Display |
|------|------|---------|
| AV1 | Analog Value | Numeric (1 decimal) |
| AV2 | Analog Value | Numeric (1 decimal) |
| AV3 | Analog Value | Numeric (1 decimal) |
| AV4 | Analog Value | Numeric (1 decimal) |
| BV1 | Binary Value | ON/OFF + Status Dot (Blue=OFF, Green=ON) |
| BV2 | Binary Value | ON/OFF + Status Dot (Blue=OFF, Green=ON) |
| BV3 | Binary Value | ON/OFF + Status Dot (Blue=OFF, Green=ON) |
| BV4 | Binary Value | ON/OFF + Status Dot (Blue=OFF, Green=ON) |

## BACnet Integration

The device broadcasts its Device ID and manages BACnet objects that can be read/written by any BACnet/IP or BACnet MS/TP client (e.g., YABE, Tridium Niagara, Metasys).

### BACnet Objects Exposed

- **Device**: 130 (configurable in main.c)
- **Analog Values**: Instance 1, 2, 3, 4
- **Binary Values**: Instance 1, 2, 3, 4

## Modifications to bacnet-stack

This project uses the official [bacnet-stack](https://github.com/bacnet-stack/bacnet-stack) with the following modifications:

- **[components/bacnet-stack/](components/bacnet-stack/)** - Configured as ESP-IDF component
- Simplified for embedded systems (reduced features, optimized for ESP32)
- WiFi-based BACnet/IP instead of Ethernet

For a list of specific changes, see [BACNET_STACK_CHANGES.md](BACNET_STACK_CHANGES.md) (if available).

## Development Notes

### Display Boundary Constants

The display code uses boundary constants for easy layout modification:

```c
#define DISP_X0    17      // Left edge
#define DISP_Y0    40      // Top edge
#define DISP_X1    151     // Right edge
#define DISP_Y1    278     // Bottom edge
#define DISP_WIDTH 135
#define DISP_HEIGHT 239
```

Position all elements relative to these constants to avoid hardcoding coordinates.

## Troubleshooting

### Display offset issues
If text appears misaligned, adjust `TFT_COLSTART` and `TFT_ROWSTART` in User_Setup.h and recompile.

### WiFi connection fails
Check SSID/password in wifi_helper.c and ensure your router is compatible with ESP32's WiFi drivers.

### Linker errors with Arduino
Ensure `CONFIG_FREERTOS_HZ=1000` is set in sdkconfig and rebuild with `idf.py fullclean && idf.py build`.


## References

- [BACnet Stack GitHub](https://github.com/bacnet-stack/bacnet-stack)
- [ESP-IDF Documentation](https://docs.espressif.com/projects/esp-idf/en/stable/)
- [Arduino-ESP32 GitHub](https://github.com/espressif/arduino-esp32)
- [TFT_eSPI GitHub](https://github.com/Bodmer/TFT_eSPI)


