#include <OneWire.h>
#include <DallasTemperature.h>
#include <SPI.h>
#include <mcp2515.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET    -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Temperature sensor setup
#define ONE_WIRE_BUS 14 // GPIO where the DS18B20 is connected
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// CAN bus setup
struct can_frame canMsg;
struct MCP2515 mcp2515(5); // CS pin is GPIO 5

// CAN communication parameters
#define MAX_RETRIES 3
#define CAN_TX_ID 0x036    // CAN ID for transmitting data
#define CAN_ACK_ID 0x037   // CAN ID for acknowledgment

// Door sensor (reed switch) setup
#define DOOR_SENSOR_PIN 34 // GPIO for door sensor
bool lastDoorState = false;
unsigned long lastDoorDebounceTime = 0;
unsigned long doorDebounceDelay = 50;

// Push button setup
#define BUTTON_PIN 12      // GPIO for push button
bool lastButtonState = false;
unsigned long lastButtonDebounceTime = 0;
unsigned long buttonDebounceDelay = 50;

// RGB LED setup
#define RED_PIN 25
#define GREEN_PIN 33
#define BLUE_PIN 32

// Message structure for CAN data
typedef struct {
  int16_t temperature;  // Temperature in hundredths of a degree (e.g., 25.12°C = 2512)
  uint8_t doorState;    // 0 = closed, 1 = open
  uint8_t buttonState;  // 0 = not pressed, 1 = pressed
} SensorData;

void setup() {
  Serial.begin(115200);
  SPI.begin();

  // Initialize OLED
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.println("Initializing...");
  display.display();

  // Initialize CAN bus
  mcp2515.reset();
  mcp2515.setBitrate(CAN_500KBPS, MCP_8MHZ);
  mcp2515.setNormalMode();

  // Initialize temperature sensor
  sensors.begin();

  // Initialize door sensor
  pinMode(DOOR_SENSOR_PIN, INPUT_PULLUP);
  
  // Initialize push button
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  
  // Initialize RGB LED
  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);
  setRGBColor(0, 0, 255); // Blue during initialization

  display.clearDisplay();
  display.setCursor(0,0);
  display.println("Setup complete");
  display.println("Ready to send data");
  display.display();
  delay(1000);
  
  setRGBColor(0, 255, 0); // Green when ready
}

void loop() {
  static unsigned long lastSendTime = 0;
  unsigned long currentTime = millis();
  
  // Read all sensors
  SensorData sensorData = readSensors();
  
  // Send data every second or when door/button state changes
  if ((currentTime - lastSendTime >= 1000) || 
      (sensorData.doorState != lastDoorState) || 
      (sensorData.buttonState != lastButtonState)) {
    
    lastDoorState = sensorData.doorState;
    lastButtonState = sensorData.buttonState;
    
    bool success = sendDataViaCAN(sensorData);
    
    if (success) {
      setRGBColor(0, 255, 0); // Green for success
      lastSendTime = currentTime;
    } else {
      setRGBColor(255, 0, 0); // Red for failure
    }
    
    // Update display
    updateDisplay(sensorData, success);
  }
  
  // Small delay to prevent excessive looping
  delay(10);
}

SensorData readSensors() {
  SensorData data;
  
  // Read temperature
  sensors.requestTemperatures(); 
  float temperatureC = sensors.getTempCByIndex(0);
  data.temperature = (int16_t)(temperatureC * 100); // Convert to hundredths of a degree
  
  // Read door sensor with debounce
  bool currentDoorReading = digitalRead(DOOR_SENSOR_PIN);
  if (currentDoorReading != lastDoorState) {
    lastDoorDebounceTime = millis();
  }
  if ((millis() - lastDoorDebounceTime) > doorDebounceDelay) {
    data.doorState = currentDoorReading ? 0 : 1; // Inverted because we're using INPUT_PULLUP
  } else {
    data.doorState = lastDoorState;
  }
  
  // Read button with debounce
  bool currentButtonReading = digitalRead(BUTTON_PIN);
  if (currentButtonReading != lastButtonState) {
    lastButtonDebounceTime = millis();
  }
  if ((millis() - lastButtonDebounceTime) > buttonDebounceDelay) {
    data.buttonState = currentButtonReading ? 0 : 1; // Inverted because we're using INPUT_PULLUP
  } else {
    data.buttonState = lastButtonState;
  }
  
  return data;
}

bool sendDataViaCAN(SensorData data) {
  // Prepare CAN message (8 bytes total)
  canMsg.can_id = CAN_TX_ID;
  canMsg.can_dlc = 8;
  
  // Pack the data into CAN message
  canMsg.data[0] = (data.temperature >> 8) & 0xFF; // MSB of temperature
  canMsg.data[1] = data.temperature & 0xFF;        // LSB of temperature
  canMsg.data[2] = data.doorState;
  canMsg.data[3] = data.buttonState;
  // Remaining bytes can be used for other data or left as 0
  canMsg.data[4] = 0;
  canMsg.data[5] = 0;
  canMsg.data[6] = 0;
  canMsg.data[7] = 0;
  
  bool messageSent = false;
  int retries = 0;
  
  while (!messageSent && retries < MAX_RETRIES) {
    if (mcp2515.sendMessage(&canMsg) == MCP2515::ERROR_OK) {
      Serial.println("Data sent via CAN");
      
      // Wait for acknowledgment
      unsigned long startTime = millis();
      bool ackReceived = false;
      
      while (millis() - startTime < 500) { // Wait up to 500ms for ACK
        if (mcp2515.readMessage(&canMsg) == MCP2515::ERROR_OK) {
          if (canMsg.can_id == CAN_ACK_ID) {
            ackReceived = true;
            break;
          }
        }
      }
      
      if (ackReceived) {
        Serial.println("ACK received");
        messageSent = true;
      } else {
        Serial.println("ACK not received, retrying...");
        retries++;
      }
    } else {
      Serial.println("Error sending CAN message, retrying...");
      retries++;
    }
  }
  
  return messageSent;
}

void updateDisplay(SensorData data, bool commSuccess) {
  display.clearDisplay();
  display.setCursor(0,0);
  
  // Display temperature
  display.print("Temp: ");
  display.print(data.temperature / 100.0, 2);
  display.println(" C");
  
  // Display door state
  display.print("Door: ");
  display.println(data.doorState ? "OPEN" : "CLOSED");
  
  // Display button state
  display.print("Button: ");
  display.println(data.buttonState ? "PRESSED" : "RELEASED");
  
  // Display communication status
  display.print("CAN: ");
  display.println(commSuccess ? "OK" : "FAIL");
  
  display.display();
}

void setRGBColor(uint8_t red, uint8_t green, uint8_t blue) {
  // Invert values if using common anode RGB LED
  analogWrite(RED_PIN, 255 - red);
  analogWrite(GREEN_PIN, 255 - green);
  analogWrite(BLUE_PIN, 255 - blue);
}
