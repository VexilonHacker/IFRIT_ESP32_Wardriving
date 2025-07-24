#ifndef AP_DNS_WEBSERVER_H
#define AP_DNS_WEBSERVER_H

#include "Config.h"
#include "Wardriving_Utilities.h"
#include "Html_code.h"

String update_webpage_html() {
    Serial.printf("[MODE] %i\n", mode);
    String data = "";
    if (mode == 1) {
        Serial.println("[INSIDE UPDATE COOL]");
        data = "Date: " + String(gps_data.Date) + "\n" +
            "Time: " + String(gps_data.timeString) + "\n" +
            "Lat : " + String(gps_data.latitude, 6) + "\n" +
            "Lon : " + String(gps_data.longitude, 6) + "\n" +
            "Alt : " + String(gps_data.altitude, 2) + " m\n" +
            "Sats: " + String(gps_data.satellites) + "\n" +
            "Accuracy: " + String(gps_data.hdop) + " (" + hdop_status(gps_data.hdop) + ")\n" +
            "First Fix Time: " + String(formatFixTime(fixTime)) + "\n" +
            "AP IP: " + WiFi.softAPIP().toString() + "\n"+
            "Clients: " + String(WiFi.softAPgetStationNum()) + "\n" +
            "IFRIT Uptime: " + String(formatFixTime(millis())) + "\n" +
            "Total DETECTED AP: " + String(total_detected_AP) + "\n" +
            "Total ADDED AP: " + String(total_added_AP) + "\n" +
            "Total LOGGED AP: " + String(TotalLoggedAp) + "\n" +
            "\n    Have Fun with it =}";


    } else if (mode == 0) {
        data =  "Welcome to IFRIT UwU\n"  
                "Time to First GPS Fix: " + formatFixTime(fixTime) + "\n" +
                "IFRIT Uptime: " + String(formatFixTime(millis())) + "\n" +
                "Clients: " + String(WiFi.softAPgetStationNum()) + "\n" +
                "Total Logged AP: " + String(TotalLoggedAp) + "\n";
        if (sdReady) {
            data += "SD Card Mounted: True\nWaiting for GPS satellite data...";
        } else {
            data += "SD Card Mounted: False\nSD Initialization Failed\n* Insert SD card and reset the device";
        }
        
    }else if (mode == -1){
        Serial.println("INSIDE MODE -1");
        data =  "Welcome to IFRIT UwU\n"  
                "Time to First GPS Fix: " + formatFixTime(fixTime) + "\n" +
                "IFRIT Uptime: " + String(formatFixTime(millis())) + "\n" +
                "Clients: " + String(WiFi.softAPgetStationNum()) + "\n" +
                "Total Logged AP: " + String(TotalLoggedAp) + "\n" +
                "SD Card Mounted: " + (sdReady ? "True" : "False") + "\n" +
                "No GPS Serial data.\nCheck GPS Module wiring & connections.";
    }
    
    return data;
}

int currentBrightness = MAX_BRIGHTNESS;
void setupServer() {
    Serial.println("[Inisde SETUP SERVER]");

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        String data = update_webpage_html();
        String htmc = BuildHtmlCode(data, currentBrightness);
        request->send(200, "text/html", htmc);
    });

    server.on("/statusData", HTTP_GET, [](AsyncWebServerRequest *request) {
        String data = update_webpage_html();
        request->send(200, "text/plain", data);
    });

    // Route: Handle brightness update
    server.on("/setBrightness", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (request->hasParam("value")) {
            String val = request->getParam("value")->value();
            int brightness = val.toInt();
            brightness = constrain(brightness, 0, 255);

            currentBrightness = brightness;
            ledcWrite(RST_CHANNEL, brightness);
            Serial.printf("[WEB] Brightness set to %d\n", brightness);

            request->send(200, "text/plain", "OK");
        } else {
            request->send(400, "text/plain", "Missing 'value' parameter");
        }
    });

    server.on("/reset", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "OK");
    delay(100);  // Give the response time to be sent
    ESP.restart();  // Soft reset the ESP32
    });

    server.onNotFound([&](AsyncWebServerRequest *request) {
        request->redirect("/");
    });
}

void Setup_Ap_Dns_WebServer(){
    randomSeed(esp_random());
    WiFi.mode(WIFI_MODE_APSTA);

    // Generate unique MAC addresses
    uint8_t bssid_ap[6], bssid_sta[6];
    do {
        for (int i = 0; i < 6; i++) {
            bssid_ap[i] = random(0, 256);
            bssid_sta[i] = random(0, 256);
        }
    } while (memcmp(bssid_ap, bssid_sta, 6) == 0);

    // Ensure unicast and locally administered
    bssid_ap[0] &= 0xFE; bssid_ap[0] |= 0x02;
    bssid_sta[0] &= 0xFE; bssid_sta[0] |= 0x02;

    esp_err_t result_ap = esp_wifi_set_mac(WIFI_IF_AP, bssid_ap);
    esp_err_t result_sta = esp_wifi_set_mac(WIFI_IF_STA, bssid_sta);

    if (result_ap == ESP_OK) {
        tft.println("+ MAC set for AP OK");
        Serial.println("[INFO] MAC set for AP successfully.");
    } else {
        tft.printf("- Failed to set MAC for AP. Err: %i\n", result_ap);
        Serial.printf("[ERROR] Failed to set MAC for AP. Error: %i\n", result_ap);
        while (1);
    }

    if (result_sta == ESP_OK) {
        tft.println("+ MAC set for STA OK");
        Serial.println("[INFO] MAC set for STA successfully.");
    } else {
        tft.printf("- Failed to set MAC for STA. Err: %i\n", result_sta);
        Serial.printf("[ERROR] Failed to set MAC for STA. Error: %i\n", result_sta);
        while (1);
    }

    // Setup network and AP
    int channel = random(1, 13);
    // int subnet = random(1, 255);
    int subnet = 1;
    IPAddress local_IP(192, 168, subnet, 1);
    IPAddress gateway(192, 168, subnet, 1);
    IPAddress subnetMask(255, 255, 255, 0);  // Typical subnet mask for 192.168.x.x networks
    if (!WiFi.softAPConfig(local_IP, gateway, subnetMask)) {
        tft.println("- AP config failed");
        Serial.println("[ERROR] Failed to configure softAP.");
        while (1);
    }

    Serial.printf("[INFO] Starting AP on channel %d...\n", channel);
    bool starting_ap = WiFi.softAP(SSID_AP, PASSWD, channel);
    if (!starting_ap) {
        tft.println("- Failed to start AP");
        Serial.println("[ERROR] Failed to start softAP.");
        while (1);
    } else {
        tft.println("+ AP started");
        Serial.println("[INFO] Access Point started successfully.");
        Serial.print("[INFO] AP IP address: ");
        Serial.println(WiFi.softAPIP());
    }
    // DNS and web server
    dnsServer.setTTL(86400);
    dnsServer.start(53, "*", WiFi.softAPIP());
    xTaskCreate(DNS_TASK, "Dns Server", 4096, NULL, 1, NULL);

    
    setupServer();
    server.begin();
    Serial.println("[INFO] Web server started.");
    tft.printf("+ Web server started:\n+ SSID = %s\n", SSID_AP);

}


void scan_and_log_wifi() {
    Serial.println("[DEBUG] Scanning for WiFi networks...");

    Enable_WifiScanning = 1;
    Serial.println("[909] after enableing wifi scan ");

    while (true){
        if (!Enable_WifiScanning) {
            for (int  i =  0 ; i < total_networks ; i++){
                Serial.printf("%-32.32s", WiFi.SSID(i).c_str());
            }
            break;
        }
        vTaskDelay(10 / portTICK_PERIOD_MS); 
    }

    File logFile = SD.open(database, FILE_APPEND);
    if (!logFile) {
        Clearscreen();
        Serial.println("[ERROR] Log file open failed");
        tft.println("[ERROR] Log file open failed");
        Sd_card_failing++;
        return;
    }


    total_detected_AP = total_networks;
    Clearscreen();
    tft.println("Scanning for WiFi networks\n\n         ->|^p^|->\n");
    Serial.printf("[INFO] Found %d networks\n", total_networks);
    tft.printf("[INFO] Found %d networks\n", total_networks);

    total_added_AP = 0;
    lines = 0;
    for (int i = 0; i < total_networks; i++) {
        String bssid = WiFi.BSSIDstr(i);
        int response = isBSSIDLogged(bssidDBFile, bssid);
        if (response) {
            Serial.printf("[SKIP] Already logged BSSID: %s\n", bssid.c_str());
            tft.printf("[SKP] Already loggedBSSID:%s\n", bssid.c_str());
            if (lines == BSSID_MAX_TFT_LINE) {
                Clearscreen();
                tft.println("Scanning for WiFi networks");
                lines = 0;
            }
            lines++;
            continue;
        }else if (response == -1){
            Clearscreen();
            Serial.println("[ERROR] BSSID_DB file open failed");
            tft.println("[ERROR] BSSID_DB file open failed");
            Sd_card_failing++;
            return;
        }

        String ssid = WiFi.SSID(i);
        if (ssid == "") {
            ssid = "[HIDDEN SSID]";
        } 

        total_added_AP++;
        TotalLoggedAp++;    
        String encType;
        switch (WiFi.encryptionType(i)) {
            case WIFI_AUTH_OPEN: encType = "OPEN"; break;
            case WIFI_AUTH_WEP: encType = "WEP"; break;
            case WIFI_AUTH_WPA_PSK: encType = "WPA"; break;
            case WIFI_AUTH_WPA2_PSK: encType = "WPA2"; break;
            case WIFI_AUTH_WPA_WPA2_PSK: encType = "WPA/WPA2"; break;
            default: encType = "UNKNOWN"; break;
        }

        String GPS_DATA = 
            sanitizeCSVField(String(gps_data.Date)) + "," +
            sanitizeCSVField(String(gps_data.timeString)) + "," +
            sanitizeCSVField(String(gps_data.latitude, 6)) + "," +
            sanitizeCSVField(String(gps_data.longitude, 6)) + "," +
            sanitizeCSVField(String(gps_data.altitude, 2)) + "," +
            sanitizeCSVField(String(gps_data.satellites)) + "," +
            sanitizeCSVField(String(gps_data.hdop));

        String WIFI_DATA = 
            sanitizeCSVField(ssid) + "," +
            sanitizeCSVField(bssid) + "," +
            sanitizeCSVField(String(WiFi.channel(i))) + "," +
            sanitizeCSVField(String(WiFi.RSSI(i))) + "," +
            sanitizeCSVField(encType);

        String LOG_DATA = GPS_DATA + "," + WIFI_DATA;

        logFile.println(LOG_DATA);
        Serial.println("[LOGGED] " + LOG_DATA);
        addBSSIDToDB(bssidDBFile, bssid);
    }

    logFile.close();
    WiFi.scanDelete();
    update_display();
}
#endif // AP_DNS_WEBSERVER_H

