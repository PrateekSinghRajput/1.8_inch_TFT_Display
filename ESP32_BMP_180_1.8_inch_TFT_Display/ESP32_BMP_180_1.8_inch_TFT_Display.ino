#include <Wire.h>
#include <Adafruit_BMP085.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <WiFi.h>
#include <WebServer.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

// TFT display pins
#define TFT_CS    5
#define TFT_DC    2
#define TFT_RST   4
#define TFT_MOSI  23
#define TFT_SCLK  18

// BMP180 sensor
Adafruit_BMP085 bmp;

// WiFi credentials
const char* ssid = "Iphone";     // Replace with your WiFi SSID
const char* password = "123456789"; // Replace with your WiFi password

// Web server port
WebServer server(80);

// NTP Client
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

// Colors
#define BLACK     0x0000
#define BLUE      0x001F
#define RED       0xF800
#define GREEN     0x07E0
#define YELLOW    0xFFE0
#define WHITE     0xFFFF
#define DARKGRAY  0x4208
#define CYAN      0x07FF
#define MAGENTA   0xF81F
#define ORANGE    0xFC00

// Initialize TFT
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);

// Global variables
unsigned long lastUpdateTime = 0;
const unsigned long UPDATE_INTERVAL = 2000; // Update every 2 seconds
bool layoutDrawn = false;
unsigned long wifiConnectionTimeout = 30000; // 30 seconds timeout for WiFi connection
String ipAddressStr = "";

// Days and months arrays
const char* days[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
const char* months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

// Function prototypes
void handleRoot();
void handleNotFound();
void connectToWiFi();
void printDebugInfo(const char* action, bool success);
void displayStartupSequence();
void drawLayout();
void updateDisplay();
void readSensorData(float &temp, float &pressure, float &altitude);

void setup() {
  Serial.begin(115200);
  Serial.println("\n========================================");
  Serial.println("ESP32 Weather Station with BMP180");
  Serial.println("========================================");

  // Initialize TFT display
  tft.initR(INITR_BLACKTAB);
  tft.setRotation(3);  // Landscape orientation
  tft.fillScreen(BLACK);

  // Display startup sequence
  displayStartupSequence();

  // Initialize BMP180 sensor
  if (!bmp.begin()) {
    Serial.println("Could not find a valid BMP180 sensor, check wiring!");
    while (1) {
      tft.fillScreen(RED);
      tft.setTextColor(WHITE);
      tft.setTextSize(1);
      tft.setCursor(10, 50);
      tft.println("BMP180 Sensor Error!");
      delay(1000);
    }
  }

  // Connect to WiFi
  connectToWiFi();

  // Initialize NTP client
  timeClient.begin();
  timeClient.setTimeOffset(19800); // Set your timezone offset in seconds (e.g., 19800 for UTC+5:30)

  // Setup web server routes
  server.on("/", handleRoot);
  server.onNotFound(handleNotFound);

  // Start server
  server.begin();
  Serial.println("HTTP server started");

  // Draw the initial display layout
  drawLayout();
}

void loop() {
  // Check WiFi connection and reconnect if needed
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi connection lost. Attempting to reconnect...");
    connectToWiFi();
  }

  // Update NTP time
  timeClient.update();

  // Update display every interval
  unsigned long currentMillis = millis();
  if (currentMillis - lastUpdateTime >= UPDATE_INTERVAL) {
    lastUpdateTime = currentMillis;
    updateDisplay();
  }

  // Handle web client requests
  server.handleClient();
}

void printDebugInfo(const char* action, bool success) {
  Serial.print(action);
  Serial.print(": ");
  Serial.println(success ? "SUCCESS" : "FAILED");
}

void displayStartupSequence() {
  tft.fillScreen(BLACK);

  // Display logo
  tft.setTextSize(2);
  tft.setTextColor(CYAN);
  tft.setCursor(25, 20);
  tft.println("ESP32");
  tft.setCursor(20, 40);
  tft.println("Weather");

  // Display loading bar
  tft.drawRect(20, 70, tft.width() - 40, 15, WHITE);

  for (int i = 0; i < tft.width() - 44; i++) {
    tft.fillRect(22, 72, i, 11, BLUE);
    delay(10);
  }

  tft.setTextSize(1);
  tft.setTextColor(GREEN);
  tft.setCursor(30, 100);
  tft.println("Initializing...");

  delay(500);
}

void connectToWiFi() {
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Already connected to WiFi");
    return;
  }

  tft.fillScreen(BLACK);
  tft.setTextSize(1);
  tft.setTextColor(WHITE);
  tft.setCursor(10, 10);
  tft.println("Connecting to WiFi...");

  tft.setCursor(10, 30);
  tft.print("SSID: ");
  tft.println(ssid);

  WiFi.begin(ssid, password);

  int dotCount = 0;
  tft.setCursor(10, 50);

  unsigned long startAttempt = millis();
  bool connected = false;

  while (millis() - startAttempt < wifiConnectionTimeout) {
    if (WiFi.status() == WL_CONNECTED) {
      connected = true;
      break;
    }

    delay(500);
    Serial.print(".");
    tft.print(".");
    dotCount++;

    if (dotCount >= 30) {
      tft.setCursor(10, tft.getCursorY() + 10);
      dotCount = 0;
    }
  }

  if (connected) {
    ipAddressStr = WiFi.localIP().toString();

    tft.fillScreen(BLACK);
    tft.setCursor(10, 20);
    tft.setTextColor(GREEN);
    tft.println("WiFi Connected!");
    tft.setCursor(10, 40);
    tft.println("IP Address:");
    tft.setCursor(10, 55);
    tft.setTextColor(YELLOW);
    tft.println(ipAddressStr);

    Serial.println("\nWiFi connected successfully!");
    Serial.print("IP address: ");
    Serial.println(ipAddressStr);
  } else {
    tft.fillScreen(BLACK);
    tft.setCursor(10, 20);
    tft.setTextColor(RED);
    tft.println("WiFi Connection Failed!");
    tft.setCursor(10, 40);
    tft.println("Will continue without WiFi");

    Serial.println("\nWiFi connection failed after timeout!");
    Serial.println("Continuing without WiFi connectivity");
  }

  delay(2000);
}

void drawLayout() {
  tft.fillScreen(BLACK);

  // Draw header bar
  tft.fillRect(0, 0, tft.width(), 14, BLUE);
  tft.setTextSize(1);
  tft.setTextColor(WHITE);
  tft.setCursor(5, 4);
  tft.print("ESP32 WEATHER STATION");

  // Draw time display area
  tft.fillRoundRect(2, 17, tft.width() - 4, 25, 3, DARKGRAY);
  tft.drawRoundRect(2, 17, tft.width() - 4, 25, 3, WHITE);

  // Draw date display area
  tft.fillRoundRect(2, 45, tft.width() - 4, 20, 3, DARKGRAY);
  tft.drawRoundRect(2, 45, tft.width() - 4, 20, 3, WHITE);

  // Draw temperature display area
  tft.fillRoundRect(2, 68, tft.width()/2 - 3, 25, 3, DARKGRAY);
  tft.drawRoundRect(2, 68, tft.width()/2 - 3, 25, 3, WHITE);
  tft.setTextSize(1);
  tft.setTextColor(WHITE, DARKGRAY);
  tft.setCursor(5, 73);
  tft.print("Temperature");

  // Draw pressure display area
  tft.fillRoundRect(tft.width()/2 + 2, 68, tft.width()/2 - 3, 25, 3, DARKGRAY);
  tft.drawRoundRect(tft.width()/2 + 2, 68, tft.width()/2 - 3, 25, 3, WHITE);
  tft.setCursor(tft.width()/2 + 5, 73);
  tft.print("Pressure");

  // Draw altitude display area
  tft.fillRoundRect(2, 96, tft.width() - 4, 25, 3, DARKGRAY);
  tft.drawRoundRect(2, 96, tft.width() - 4, 25, 3, WHITE);
  tft.setCursor(5, 101);
  tft.print("Altitude");

  // Draw WiFi status area
  tft.drawRect(2, 124, tft.width() - 4, 14, WHITE);

  layoutDrawn = true;
}

void readSensorData(float &temp, float &pressure, float &altitude) {
  temp = bmp.readTemperature();
  pressure = bmp.readPressure() / 100.0; // Convert to hPa
  altitude = bmp.readAltitude();
}

void updateDisplay() {
  if (!layoutDrawn) {
    drawLayout();
  }

  // Read sensor data
  float temperature, pressure, altitude;
  readSensorData(temperature, pressure, altitude);

  // Get current time
  time_t epochTime = timeClient.getEpochTime();
  struct tm *ptm = gmtime((time_t *)&epochTime);

  // Format time
  String timeStr = timeClient.getFormattedTime();

  // Format date
  String dateStr = String(days[ptm->tm_wday]) + ", " + 
                  String(months[ptm->tm_mon]) + " " + 
                  String(ptm->tm_mday) + ", " + 
                  String(ptm->tm_year + 1900);

  // Update time display
  tft.fillRect(5, 20, tft.width() - 10, 15, DARKGRAY);
  tft.setTextSize(1);
  tft.setTextColor(YELLOW, DARKGRAY);
  tft.setCursor(20, 22);
  tft.print(timeStr);

  // Update date display
  tft.fillRect(5, 48, tft.width() - 10, 15, DARKGRAY);
  tft.setTextSize(1);
  tft.setTextColor(CYAN, DARKGRAY);
  tft.setCursor(10, 50);
  tft.print(dateStr);

  // Update temperature display
  tft.fillRect(5, 80, tft.width()/2 - 10, 10, DARKGRAY);
  tft.setTextSize(1);
  tft.setTextColor(RED, DARKGRAY);
  tft.setCursor(10, 82);
  tft.print(temperature, 1);
  tft.print(" °C");

  // Update pressure display
  tft.fillRect(tft.width()/2 + 5, 80, tft.width()/2 - 10, 10, DARKGRAY);
  tft.setTextSize(1);
  tft.setTextColor(GREEN, DARKGRAY);
  tft.setCursor(tft.width()/2 + 10, 82);
  tft.print(pressure, 0);
  tft.print(" hPa");

  // Update altitude display
  tft.fillRect(5, 108, tft.width() - 10, 10, DARKGRAY);
  tft.setTextSize(1);
  tft.setTextColor(MAGENTA, DARKGRAY);
  tft.setCursor(10, 110);
  tft.print(altitude, 1);
  tft.print(" meters");

  // Update WiFi status
  tft.fillRect(3, 125, tft.width() - 6, 12, BLACK);
  tft.setTextSize(1);
  if (WiFi.status() == WL_CONNECTED) {
    tft.setTextColor(GREEN);
    tft.setCursor(5, 128);
    tft.print("WiFi: Connected");
  } else {
    tft.setTextColor(RED);
    tft.setCursor(5, 128);
    tft.print("WiFi: Disconnected");
  }
}

void handleRoot() {
  // Read sensor data
  float temperature, pressure, altitude;
  readSensorData(temperature, pressure, altitude);

  // Create JSON response
  String json = "{";
  json += "\"temperature\":" + String(temperature, 1) + ",";
  json += "\"pressure\":" + String(pressure, 0) + ",";
  json += "\"altitude\":" + String(altitude, 1) + ",";
  
  // Get current time
  time_t epochTime = timeClient.getEpochTime();
  struct tm *ptm = gmtime((time_t *)&epochTime);
  
  json += "\"time\":\"" + timeClient.getFormattedTime() + "\",";
  json += "\"date\":\"" + String(days[ptm->tm_wday]) + ", " + 
          String(months[ptm->tm_mon]) + " " + String(ptm->tm_mday) + ", " + 
          String(ptm->tm_year + 1900) + "\",";
  json += "\"ip\":\"" + ipAddressStr + "\"";
  json += "}";
  
  server.send(200, "application/json", json);
}

void handleNotFound() {
  server.send(404, "text/plain", "404: Not found");
}