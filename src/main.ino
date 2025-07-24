#include <Ap_Dns_Webserver.h>
#include <Config.h>
#include <Wardriving_Tasks.h>
#include <Wardriving_Utilities.h>

gps_data_t gps_data;
HardwareSerial gpser(1);
TinyGPSPlus gps;
TFT_eSPI tft = TFT_eSPI();
DNSServer dnsServer;
AsyncWebServer server(80);

bool isScreenTouched = 0;
bool sdReady =  0;
bool stop_Waiting = 0;
bool OneShot1 = 0;
bool Enable_Led_Blink = 0;
bool EnableHiddenSSIDScan = 1;
bool Inside_Scanning_Wifi = 0;
int condition_to_start_scanning = 0;
bool Enable_WifiScanning = 0;
bool isGpsDetected = 0;
size_t TotalLoggedAp = 0;
int total_networks = 0 ;
int total_detected_AP = 0;
int total_added_AP = 0;
int Sd_card_failing = 0;
int lines = 0;
int mode = 0; 
unsigned long startTime;
unsigned long fixTime = 0;

String database = "/log_Wardriving.csv";
String bssidDBFile = "/bssids_db.db";
String cvs_header = "Date,Time_UTC,Latitude,Longitude,Altitude,Satellites,Accuracy,SSID,BSSID,Channel,RSSI,Encryption";

void setup() {
  randomSeed(esp_random());
  Serial.begin(115200);
  TFT_Setup();
  Setup_Ap_Dns_WebServer();
  Setup_SdCard_Gps();
  Setup_WifiScan_BlinkLed();
  // debug
  ListFiles(SD.open("/"));

  delay(1500);
  startTime = millis();
}

void loop() {
  Wardriving();
}

void Wardriving() {
  // Feed GPS data
  isGpsDetected = GpsUART();
  condition_to_start_scanning = gps.location.isValid() && 
    gps.satellites.value() >= MIN_SATELLITE && 
    gps.location.isUpdated() &&
    Sd_card_failing < SD_CARD_MAX_FAILING &&
    gps.location.age() < GPS_DATA_TIMEOUT;

  // Print all values in one Serial.printf with 1/0 representation
  Serial.printf("Location Valid: %i, Satellites: %i, Location Updated: %i, "
      "SD Card Failing: %i, Location Age: %i, Age Valid: %i, "
      "Condition to Start Scanning: %i, Gps Detected = %i\n", 
      gps.location.isValid() ? 1 : 0, 
      (gps.satellites.value() > MIN_SATELLITE) ? 1 : 0, 
      gps.location.isUpdated() ? 1 : 0, 
      (Sd_card_failing < SD_CARD_MAX_FAILING) ? 1 : 0, 
      gps.location.age(), 
      (gps.location.age() < GPS_DATA_TIMEOUT) ? 1 : 0, 
      condition_to_start_scanning ? 1 : 0,
      isGpsDetected);

  if (condition_to_start_scanning) {
    mode = 1;
    OneShot1 = 1;
    stop_Waiting = 1;
    update_gps_data();
    update_display();
    Inside_Scanning_Wifi = 1;
    Enable_Led_Blink = 1;
    scan_and_log_wifi();
    Inside_Scanning_Wifi = 0;
    Enable_Led_Blink = 0;

  } else {
    if (Sd_card_failing >= SD_CARD_MAX_FAILING) {
      sdReady = 0;
      mode = 0;
      Clearscreen();

      Serial.println("SD CARD FAILED\nReinsert SD card\nAND Reset your Esp32\n\n         ->|^p^|->");
      tft.println("SD CARD FAILED\nReinsert SD card\nAND Reset your Esp32\n\n         ->|^p^|->");

      while (1) {
        uint16_t x = 0 , y = 0; bool pressed = tft.getTouch(&x, &y);
        if (pressed) { 
          tft.setCursor(10,10);
          tft.println("SD CARD FAILED\nReinsert SD card\nAND Reset your Esp32\n\n         ->|^p^|->");
          tft.setCursor(100,150);
          Serial.printf("x: %i, y: %i \n", x, y); 
        }
      }
    }

    bool gpsSignalLost = !gps.location.isValid() || gps.location.age() > GPS_DATA_TIMEOUT || gps.satellites.value() < MIN_SATELLITE;
    if (gpsSignalLost){
      stop_Waiting = 0;

      if (OneShot1){
        startTime = millis();
        OneShot1 = 0;
      }
    }

    if (!stop_Waiting){
      vTaskDelay(250 / portTICK_PERIOD_MS);

      Clearscreen();
      if (!isGpsDetected) {
        mode = -1;
        tft.println("No GPS Serial data.\nCheck wiring & connection.");
        Serial.println("[ERROR] No GPS Serial data. Check wiring & connection.");
      } else {
        mode = 0 ;
        tft.println("Welcome to IFRIT UwU\n");
        tft.printf("Time taken; %s\n", formatFixTime(fixTime));
        tft.printf("Total Logged Ap: %i\n", TotalLoggedAp);
        tft.printf("SD_CARD mounted: %s\n", String(sdReady ? "True" : "False"));
        tft.print("IP: ");
        tft.println(WiFi.softAPIP());
        tft.println("Clients: " + String(WiFi.softAPgetStationNum()));

        tft.print("\nWaiting for GPS satellite data");
        Serial.print("[DEBUG] Waiting for GPS satellite data");
        for (int  i =0 ; i < 5 ; i++){
          fixTime = millis() - startTime;
          vTaskDelay(700 / portTICK_PERIOD_MS);
          Serial.print(".");
          tft.print(".");
        }
        Serial.println("");
      }
      fixTime = millis() - startTime;
    }
  }
}
