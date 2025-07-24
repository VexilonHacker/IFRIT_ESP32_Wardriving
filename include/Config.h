#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <WiFi.h>
#include <DNSServer.h>
#include <ESPAsyncWebServer.h>
#include <TinyGPSPlus.h>
#include <TFT_eSPI.h>
#include <FS.h>
#include <SD.h>
#include <esp_wifi.h>

#define SD_CS 5
#define GPS_RX 16
#define GPS_TX 17
#define GPS_BAUD 9600
#define GPS_DATA_TIMEOUT 3000
#define MIN_SATELLITE 4 

#define TIMEOUT_MS 120 // scanning 13 channel will takes  1.56 second  // 120 ms BTW
#define BSSID_MAX_TFT_LINE 5

#define SD_CARD_MAX_FAILING 4
#define BACKGROUND_COLOR TFT_BLACK
#define TEXT_COLOR TFT_GREEN

#define PWM_FREQ     5000
#define PWM_RES      8
#define LED_PIN 12 
#define LED_CHANNEL 0
#define LED_DELAY 30

#define RST_PIN 27 
#define RST_CHANNEL 1

#define MAX_BRIGHTNESS   255
#define MIN_BRIGHTNESS   0


#define SSID_AP "iPhone 16 Pro Max"
#define PASSWD "!@DoIt1984"

#define WEBPAGE_DELAY 1500
#define WEBPAGE_TITLE "IFRIT Wardriving v 1.7.2"

typedef struct {
  double latitude;
  double longitude;
  double altitude;
  double speed;
  int satellites;
  char timeString[9];
  char Date[15];
  int hdop;
} gps_data_t;

// Global object declarations 
extern AsyncWebServer server;
extern DNSServer dnsServer;
extern gps_data_t gps_data;
extern HardwareSerial gpser;
extern TinyGPSPlus gps;
extern TFT_eSPI tft;

// Global flags and variables
extern bool sdReady;
extern bool stop_Waiting;
extern bool OneShot1;
extern bool Enable_Led_Blink;
extern bool EnableHiddenSSIDScan;
extern bool Inside_Scanning_Wifi;
extern int condition_to_start_scanning;
extern bool Enable_WifiScanning;

extern int total_networks;
extern int total_detected_AP;
extern int total_added_AP;
extern int Sd_card_failing;
extern int lines;
extern int mode;

extern size_t TotalLoggedAp;
extern unsigned long startTime;
extern unsigned long fixTime;

extern String database;
extern String bssidDBFile;
extern String cvs_header;

#endif // CONFIG_H

