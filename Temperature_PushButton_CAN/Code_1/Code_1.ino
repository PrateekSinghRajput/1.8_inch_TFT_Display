#include <SPI.h>
#include <mcp2515.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>

// TFT display pins
#define TFT_CS     2
#define TFT_DC     4
#define TFT_RST    15

// Initialize TFT display
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

// CAN bus setup
struct can_frame canMsg;
struct MCP2515 mcp2515(5); // CS pin on GPIO 5

// CAN IDs
#define CAN_TX_ID 0x036    // ID for incoming sensor data
#define CAN_ACK_ID 0x037   // ID for our acknowledgments

// Display refresh control
unsigned long lastDisplayUpdate = 0;
#define DISPLAY_REFRESH_INTERVAL 500 // Update display every 500ms

// Sensor data structure
typedef struct {
  float temperature;  // in Celsius
  bool doorState;     // true = open, false = closed
  bool buttonState;   // true = pressed, false = released
  unsigned long lastUpdate; // timestamp of last update
} SensorData;

SensorData currentData;
bool newDataAvailable = false;

void setup() {
  Serial.begin(115200);
  
  // Initialize TFT display
  tft.initR(INITR_BLACKTAB);
  tft.setRotation(3); // Landscape orientation
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextWrap(false);
  
  // Initialize CAN bus
  SPI.begin();
  mcp2515.reset();
  mcp2515.setBitrate(CAN_500KBPS, MCP_8MHZ);
  mcp2515.setNormalMode();

  // Initialize sensor data
  currentData.temperature = 0.0;
  currentData.doorState = false;
  currentData.buttonState = false;
  currentData.lastUpdate = 0;

  // Draw initial static UI elements
  drawStaticUI();
}

void loop() {
  // Check for incoming CAN messages
  if (mcp2515.readMessage(&canMsg) == MCP2515::ERROR_OK) {
    if (canMsg.can_id == CAN_TX_ID) {
      processIncomingData();
      newDataAvailable = true;
      
      // Send acknowledgment
      sendAcknowledgment();
    }
  }

  // Update display periodically or when new data arrives
  if (newDataAvailable || (millis() - lastDisplayUpdate >= DISPLAY_REFRESH_INTERVAL)) {
    updateDisplay();
    lastDisplayUpdate = millis();
    newDataAvailable = false;
  }
}

void processIncomingData() {
  // Extract temperature (2 bytes)
  int16_t tempInt = (canMsg.data[0] << 8) | canMsg.data[1];
  currentData.temperature = tempInt / 100.0;
  
  // Extract door state (1 byte)
  currentData.doorState = canMsg.data[2] > 0;
  
  // Extract button state (1 byte)
  currentData.buttonState = canMsg.data[3] > 0;
  
  currentData.lastUpdate = millis();
  
  Serial.print("Received: Temp=");
  Serial.print(currentData.temperature, 2);
  Serial.print("°C, Door=");
  Serial.print(currentData.doorState ? "OPEN" : "CLOSED");
  Serial.print(", Button=");
  Serial.println(currentData.buttonState ? "PRESSED" : "RELEASED");
}

void sendAcknowledgment() {
  canMsg.can_id = CAN_ACK_ID;
  canMsg.can_dlc = 0; // Empty acknowledgment
  mcp2515.sendMessage(&canMsg);
  Serial.println("Sent ACK");
}

void drawStaticUI() {
  tft.fillScreen(ST77XX_BLACK);
  
  // Draw title
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(1);
  tft.setCursor(10, 5);
  tft.print("CAN Sensor Monitor");

  // Draw temperature label
  tft.setTextColor(ST77XX_CYAN);
  tft.setCursor(10, 25);
  tft.print("Temperature:");

  // Draw door status label
  tft.setCursor(10, 65);
  tft.print("Door Status:");

  // Draw button status label
  tft.setCursor(10, 105);
  tft.print("Button Status:");

  // Draw last update label
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(10, 145);
  tft.print("Last Update:");
}

void updateDisplay() {
  // Update temperature (center-aligned)
  tft.fillRect(10, 40, tft.width() - 20, 20, ST77XX_BLACK);
  tft.setTextColor(ST77XX_YELLOW);
  tft.setTextSize(2);
  
  String tempStr = String(currentData.temperature, 2) + " C";
  int16_t x = (tft.width() - tempStr.length() * 12) / 2; // Approximate centering
  tft.setCursor(x, 40);
  tft.print(tempStr);

  // Update door status
  tft.fillRect(100, 65, tft.width() - 110, 20, ST77XX_BLACK);
  tft.setTextSize(1);
  if (currentData.doorState) {
    tft.setTextColor(ST77XX_RED);
    tft.setCursor(100, 65);
    tft.print("OPEN");
  } else {
    tft.setTextColor(ST77XX_GREEN);
    tft.setCursor(100, 65);
    tft.print("CLOSED");
  }

  // Update button status
  tft.fillRect(100, 105, tft.width() - 110, 20, ST77XX_BLACK);
  if (currentData.buttonState) {
    tft.setTextColor(ST77XX_RED);
    tft.setCursor(100, 105);
    tft.print("PRESSED");
  } else {
    tft.setTextColor(ST77XX_GREEN);
    tft.setCursor(100, 105);
    tft.print("RELEASED");
  }

  // Update last update time
  tft.fillRect(100, 145, tft.width() - 110, 20, ST77XX_BLACK);
  tft.setTextColor(ST77XX_WHITE);
  
  unsigned long secondsAgo = (millis() - currentData.lastUpdate) / 1000;
  String timeStr;
  if (currentData.lastUpdate == 0) {
    timeStr = "Never";
  } else if (secondsAgo < 60) {
    timeStr = String(secondsAgo) + "s ago";
  } else {
    timeStr = String(secondsAgo / 60) + "m ago";
  }
  
  tft.setCursor(100, 145);
  tft.print(timeStr);

  // Draw connection status indicator
  bool dataIsFresh = (millis() - currentData.lastUpdate) < 5000; // 5 second threshold
  tft.fillCircle(tft.width() - 15, 15, 5, dataIsFresh ? ST77XX_GREEN : ST77XX_RED);
}
