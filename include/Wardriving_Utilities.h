#ifndef WARDRIVING_UTILITIES_H
#define WARDRIVING_UTILITIES_H

#include <Config.h>
#include <Wardriving_Tasks.h>

size_t CountLines(String file) {
  File db = SD.open(file, FILE_READ);
  if (!db) {
    Serial.println("[ERROR] File open failed!");
    return 0;
  }
  size_t lineCount = 0;
  while (db.available()) {
    String line = db.readStringUntil('\n');
    lineCount++;
  }
  db.close();
  return lineCount;
}

int isBSSIDLogged(String db, String bssid) {
  File readFile = SD.open(db, FILE_READ);
  if (!readFile) {
    return -1;
  }

  Serial.printf("[DEBUG] Checking if BSSID %s is logged...\n", bssid.c_str());
  while (readFile.available()) {
    String line = readFile.readStringUntil('\n');
    line.trim();
    if (line == bssid) {
      readFile.close();
      return 1;
    }
  }
  readFile.close();
  return 0;
}

void addBSSIDToDB(String db, String bssid) {
  Serial.printf("[DEBUG] Adding BSSID %s to DB...\n", bssid.c_str());
  File dbFile = SD.open(db, FILE_APPEND);
  if (dbFile) {
    dbFile.println(bssid);
    dbFile.close();
  } else {
    Serial.println("[ERROR] BSSID_DB file open failed");
  }
}

String sanitizeCSVField(String field) {
  field.trim();

  if (field.length() == 0) {
    return "NaN";
  }

  bool needsQuotes = false;
  for (int i = 0; i < field.length(); i++) {
    char c = field.charAt(i);
    if (c == ',' || c == '\n' || c == '\r' || c == '\t') {
      needsQuotes = true;
      break;
    }
  }

  if (!needsQuotes) {
    return field;
  }

  String result = "\"" + field + "\"";
  return result;
}

String formatFixTime(unsigned long ms) {
  if (ms < 1000) {
    return String(ms) + "ms";
  }
  unsigned long seconds = ms / 1000;
  if (seconds < 60) {
    return String(seconds) + "s";
  }
  if (seconds < 3600) {
    return String(seconds / 60) + "m " + String(seconds % 60) + "s";
  }
  unsigned long hours = seconds / 3600;
  unsigned long minutes = (seconds % 3600) / 60;
  unsigned long remSec = seconds % 60;
  return String(hours) + "h " + String(minutes) + "m " + String(remSec) + "s";
}

String hdop_status(float hdop) {
  if (hdop <= 2) {
    return "High fix";
  } else if (hdop <= 5) {
    return "Med fix";
  } else {
    return "Low fix";
  }
}

void ListFiles(File dir, int depth = 0) {
  while (true) {
    File entry = dir.openNextFile();
    if (!entry)
      break;

    for (int i = 0; i < depth; i++)
      Serial.print("  ");
    Serial.print("- ");
    Serial.print(entry.name());
    if (entry.isDirectory()) {
      Serial.println(" [DIR]");
      ListFiles(entry, depth + 1);
    } else {
      Serial.printf(" (%d bytes)\n", entry.size());
    }
    entry.close();
  }
}

void Clearscreen() {
  tft.fillRect(0, 0, tft.width(), tft.height(), BACKGROUND_COLOR);
  tft.setCursor(0, 0);
}

bool GpsUART() {
  bool gps_detected = 0;
  while (gpser.available()) {
    gps.encode(gpser.read());
    gps_detected = 1;
  }
  return gps_detected;
}

void update_gps_data() {
  Serial.println("[DEBUG] Updating GPS data...");
  gps_data.latitude = gps.location.lat();
  gps_data.longitude = gps.location.lng();
  gps_data.altitude = gps.altitude.meters();
  gps_data.speed = gps.speed.kmph();
  gps_data.satellites = gps.satellites.value();

  // Validate and format time
  if (gps.time.isValid()) {
    sprintf(gps_data.timeString, "%02d:%02d:%02d", gps.time.hour(),
            gps.time.minute(), gps.time.second());
  } else {
    strcpy(gps_data.timeString, "--:--:--");
  }

  // Validate and format date
  if (gps.date.isValid()) {
    sprintf(gps_data.Date, "%04d/%02d/%02d", gps.date.year(), gps.date.month(),
            gps.date.day());
  } else {
    strcpy(gps_data.Date, "--/--/--");
  }
  if (gps.hdop.isValid()) {
    gps_data.hdop = gps.hdop.hdop();
  } else {
    gps_data.hdop = 404;
  }
}

void update_display() {
  String result = "";
  result = "Date: " + String(gps_data.Date) + "\n" +
           "Time: " + String(gps_data.timeString) + "\n" +
           "Lat : " + String(gps_data.latitude, 6) + "\n" +
           "Lon : " + String(gps_data.longitude, 6) + "\n" +
           "Alt : " + String(gps_data.altitude, 2) + " m\n" +
           "Sats: " + String(gps_data.satellites) + "\n" +
           "Accuracy: " + String(gps_data.hdop) + " (" +
           hdop_status(gps_data.hdop) + ")\n" +
           "First Fix Time: " + formatFixTime(fixTime) + "\n" +
           "Esp32 Uptime: " + String(formatFixTime(millis())) + "\n" +
           "Clients: " + String(WiFi.softAPgetStationNum()) + "\n" +
           "Total DETECTED AP: " + String(total_detected_AP) + "\n" +
           "Total ADDED AP: " + String(total_added_AP) + "\n" +
           "Total LOGGED AP: " + String(TotalLoggedAp) + "\n" +
           "\n    Have Fun with it =}";
  Clearscreen();
  tft.print(result);
  // Serial.println(result);
}

void Setup_SdCard_Gps() {
  Serial.println("[DEBUG] Initializing SD card...");
  if (!SD.begin(SD_CS)) {
    tft.println("SD FAIL");
    sdReady = 0;
    mode = 0;
    Serial.println("[ERROR] SD init failed!");
    tft.println("TOUCH ME NOW :D");
    while (1) {
      uint16_t x = 0, y = 0;
      unsigned long startTime = millis();
      bool t = tft.getTouch(&x, &y);
      unsigned long endTime = millis();
      unsigned long duration = endTime - startTime;

      if (t) {
        Clearscreen();
        tft.setCursor(100, 100);
        Serial.printf("x: %i, y: %i, took: %i\n", x, y, duration);
        tft.printf("X: %i, Y: %i\n", x, y);
      }
    }
  } else {
    Serial.println("[DEBUG] SD card OK.");
    tft.println("+ SD OK.");
    sdReady = 1;
    mode = 1;

    // CSV init
    if (!SD.exists(database)) {
      Serial.println("[DEBUG] Creating CSV header...");
      File header = SD.open(database, FILE_WRITE);
      if (header) {
        header.println(cvs_header);
        header.close();
      }
    } else {
      Serial.println("[DEBUG] CSV database found.");
    }

    if (!SD.exists(bssidDBFile)) {
      Serial.println("[DEBUG] Creating BSSID DB...");
      File bssidFile = SD.open(bssidDBFile, FILE_WRITE);
      if (bssidFile)
        bssidFile.close();
    } else {
      Serial.println("[DEBUG] BSSID database found.");
    }

    gpser.begin(GPS_BAUD, SERIAL_8N1, GPS_RX, GPS_TX);
    Serial.println("[INFO] GPS serial started.");
  }
  Serial.println("Starting counting csv lines");
  TotalLoggedAp = CountLines(bssidDBFile);
  Serial.println("completed");
}

void TFT_Setup() {
  ledcSetup(RST_CHANNEL, PWM_FREQ, PWM_RES);
  ledcAttachPin(RST_PIN, RST_CHANNEL);
  ledcWrite(RST_CHANNEL, 255);
  // turn screen on

  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(BACKGROUND_COLOR);
  tft.setTextColor(TEXT_COLOR);
  tft.setTextSize(2);
  tft.println("Initializing...");
  Serial.printf("[TFT] turn on screen rst_pin: %i, RST_CHANNEL: %i\n", RST_PIN,
                RST_CHANNEL);
}

void Setup_WifiScan_BlinkLed() {
  ledcSetup(LED_CHANNEL, PWM_FREQ, PWM_RES);
  ledcAttachPin(LED_PIN, LED_CHANNEL);
  xTaskCreate(BLINK_LED, "Blink LED Task", 1024, NULL, 0, NULL);
  xTaskCreate(AsyncWifiScanReplacement, "AWSR", 4096, NULL, 1, NULL);
}
#endif
