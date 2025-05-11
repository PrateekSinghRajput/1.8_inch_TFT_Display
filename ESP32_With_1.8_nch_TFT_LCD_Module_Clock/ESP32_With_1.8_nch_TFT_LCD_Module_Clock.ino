#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

// TFT display pins
#define TFT_CS    5
#define TFT_DC    2
#define TFT_RST   4
#define TFT_MOSI  23
#define TFT_SCLK  18

// WiFi credentials
const char* ssid = "Iphone"; // Replace with your WiFi SSID
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

// Global variables
unsigned long lastUpdateTime = 0;
const unsigned long UPDATE_INTERVAL = 1000; // Update time every second
bool layoutDrawn = false;
unsigned long wifiConnectionTimeout = 30000; // 30 seconds timeout for WiFi connection

// Initialize TFT
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);

// Store IP address as a string to avoid repeated calls
String ipAddressStr = "";

// Days and months arrays for date formatting
const char* days[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
const char* months[] = {"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};

// Function prototypes
void handleRoot();
void handleNotFound();
void connectToWiFi();
void printDebugInfo(const char* action, bool success);
void displayStartupSequence();
void drawLayout();
void updateDisplay();

void setup() {
  Serial.begin(115200);
  Serial.println("\n========================================");
  Serial.println("ESP32 Date & Time Display");
  Serial.println("========================================");

  // Initialize TFT display
  tft.initR(INITR_BLACKTAB);
  tft.setRotation(3);  // Landscape orientation
  tft.fillScreen(BLACK);

  // Display startup sequence
  displayStartupSequence();

  // Connect to WiFi
  connectToWiFi();

  // Initialize NTP client
   timeClient.begin();
  timeClient.setTimeOffset(19800 ); // Set your timezone offset in seconds (e.g., 3600 for UTC+1)

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

  // Update display every second
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
  tft.setCursor(30, 40);
  tft.println("Clock");

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
  // Only attempt to connect if not already connected
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

  // Set timeout for connection
  unsigned long startAttempt = millis();
  bool connected = false;

  while (millis() - startAttempt < wifiConnectionTimeout) {
    if (WiFi.status() == WL_CONNECTED) {
      connected = true;
      break;
    }

    delay(500);
    Serial.print(".");

    // Update dots on screen for connection progress
    tft.print(".");
    dotCount++;

    // Start a new line of dots if needed
    if (dotCount >= 30) {
      tft.setCursor(10, tft.getCursorY() + 10);
      dotCount = 0;
    }
  }

  if (connected) {
    // Store IP address as string to avoid repeated calls
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
  tft.print("ESP32 CLOCK");

  // Draw time display area
  tft.fillRoundRect(2, 17, tft.width() - 4, 40, 3, DARKGRAY);
  tft.drawRoundRect(2, 17, tft.width() - 4, 40, 3, WHITE);

  // Draw date display area
  tft.fillRoundRect(2, 60, tft.width() - 4, 30, 3, DARKGRAY);
  tft.drawRoundRect(2, 60, tft.width() - 4, 30, 3, WHITE);

  // Draw WiFi status area
  tft.drawRect(2, 93, tft.width() - 4, 14, WHITE);

  // Display IP address in footer
  tft.setTextSize(1);
  tft.setTextColor(GREEN);
  tft.setCursor(5, 110);
  tft.print("IP: ");

  // Use stored IP address string
  if (WiFi.status() == WL_CONNECTED) {
    tft.print(ipAddressStr);
  } else {
    tft.setTextColor(RED);
    tft.print("Not Connected");
  }

  layoutDrawn = true;
}

void updateDisplay() {
  if (!layoutDrawn) {
    drawLayout();
  }

  // Get current time
   time_t epochTime = timeClient.getEpochTime();
Serial.print("Epoch Time: ");
 Serial.println(epochTime); // Debug print to check epoch time

struct tm *ptm = gmtime((time_t *)&epochTime);

  // Format time
  String timeStr = timeClient.getFormattedTime();

  // Format date
  String dateStr = String(days[ptm->tm_wday]) + ", " + 
                  String(months[ptm->tm_mon]) + " " + 
                  String(ptm->tm_mday) + ", " + 
                  String(ptm->tm_year + 1900);

  // Update time display
  tft.fillRect(5, 20, tft.width() - 10, 30, DARKGRAY);
  tft.setTextSize(2);
  tft.setTextColor(YELLOW, DARKGRAY);
  tft.setCursor(20, 25);
  tft.print(timeStr);

  // Update date display
  tft.fillRect(5, 65, tft.width() - 10, 20, DARKGRAY);
  tft.setTextSize(1);
  tft.setTextColor(CYAN, DARKGRAY);
  tft.setCursor(10, 70);
  tft.print(dateStr);

  // Update WiFi status
  tft.fillRect(3, 94, tft.width() - 6, 12, BLACK);
  tft.setTextSize(1);
  if (WiFi.status() == WL_CONNECTED) {
    tft.setTextColor(GREEN);
    tft.setCursor(5, 97);
    tft.print("WiFi: Connected");
  } else {
    tft.setTextColor(RED);
    tft.setCursor(5, 97);
    tft.print("WiFi: Disconnected");
  }

  // Always update IP address in case WiFi status has changed
  tft.fillRect(5, 110, tft.width() - 10, 8, BLACK);
  tft.setTextSize(1);
  tft.setCursor(5, 110);
  tft.print("IP: ");

  if (WiFi.status() == WL_CONNECTED) {
    tft.setTextColor(GREEN);
    tft.print(ipAddressStr);
  } else {
    tft.setTextColor(RED);
    tft.print("Not Connected");
  }
}

void handleRoot() {
  // Create a simple webpage
  String html = "<!DOCTYPE html><html><head><title>ESP32 Clock</title>";
  html += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">";
  html += "<style>body { font-family: Arial, sans-serif; text-align: center; margin-top: 50px; }";
  html += "h1 { color: #2c3e50; } .time { font-size: 3em; color: #3498db; }";
  html += ".date { font-size: 1.5em; color: #7f8c8d; }</style></head><body>";
  html += "<h1>ESP32 Network Clock</h1>";
  
  // Get current time and date
  time_t epochTime = timeClient.getEpochTime();
  struct tm *ptm = gmtime((time_t *)&epochTime);
  
  html += "<div class=\"time\">" + timeClient.getFormattedTime() + "</div>";
  html += "<div class=\"date\">" + String(days[ptm->tm_wday]) + ", " + 
          String(months[ptm->tm_mon]) + " " + String(ptm->tm_mday) + ", " + 
          String(ptm->tm_year + 1900) + "</div>";
  
  html += "<p>IP Address: " + ipAddressStr + "</p>";
  html += "</body></html>";
  
  server.send(200, "text/html", html);
}

void handleNotFound() {
  server.send(404, "text/plain", "File Not Found");
}