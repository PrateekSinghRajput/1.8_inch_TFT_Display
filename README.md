# ESP32 BMP180 & 1.8" TFT Display Projects 🎉

Welcome to the ESP32 projects repository featuring BMP180 sensor integration, 1.8-inch TFT LCD display demos, and an I2C scanner utility! Perfect for makers and developers working with ESP32 microcontrollers and sensor displays. 🚀

---

## Projects Included

### 1. ESP32_BMP_180_1.8_inch_TFT_Display 🌡️🌤️
- Displays temperature and barometric pressure from the BMP180 sensor on a 1.8" TFT LCD.
- Uses SPI for the TFT display and I2C for the BMP180 sensor.
- Ideal for weather stations and environmental monitoring.

### 2. ESP32_With_1.8_nch_TFT_LCD_Module_Clock ⏰
- A colorful digital clock displayed on the 1.8" TFT LCD.
- Simple and visually appealing clock demo for ESP32.

### 3. ESP32_With_I2C_Scan 🔍
- Scans and lists all I2C devices connected to the ESP32.
- Handy for debugging and verifying I2C connections.

---

## Hardware Requirements 🧰

- ESP32 Development Board
- 1.8" TFT LCD (ST7735 driver, 128x160 resolution)
- BMP180 Barometric Pressure Sensor (I2C)

---

## Wiring Summary 🔌

| Component      | Pin on ESP32 | Connection Details           |
|----------------|--------------|-----------------------------|
| TFT VCC        | 3.3V         | Power                       |
| TFT GND        | GND          | Ground                      |
| TFT SCL (CLK)  | GPIO 18      | SPI Clock                   |
| TFT SDA (MOSI) | GPIO 23      | SPI MOSI                    |
| TFT RES        | GPIO 4       | Reset                      |
| TFT DC         | GPIO 2       | Data/Command                |
| TFT CS         | GPIO 5       | Chip Select                 |
| BMP180 VCC     | 3.3V         | Power                       |
| BMP180 GND     | GND          | Ground                      |
| BMP180 SDA     | GPIO 21      | I2C Data                   |
| BMP180 SCL     | GPIO 22      | I2C Clock                  |

*Note: Pin assignments can be modified in the code as needed.*

---

## Software Setup 💻

1. Open the project in Arduino IDE.

2. Install required libraries via Library Manager:
- Adafruit BMP085/BMP180
- Adafruit GFX
- Adafruit ST7735 and ST7789
- Wire (I2C support)

3. Select your ESP32 board and COM port.

4. Upload the desired example sketch.

---

## Usage Tips 💡

- Customize pin definitions in the code if your wiring differs.
- Use the I2C scanner sketch first to confirm sensor connections.
- Adjust display colors and fonts in the TFT demo sketches to your preference.

---

## Enjoy building with ESP32! 😄🛠️
