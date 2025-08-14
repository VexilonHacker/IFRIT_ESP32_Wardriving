#ifndef WARDRIVING_TASKS_H
#define WARDRIVING_TASKS_H

#include <Config.h>

// Parallel tasks
void BLINK_LED(void *parameter){
    while (1) {
        if (Enable_Led_Blink){
            for (int i = 0; i <= 255; i = i+5) {
                ledcWrite(LED_PIN, i);
                vTaskDelay(LED_DELAY/ portTICK_PERIOD_MS);
            }
            for (int i = 255; i >= 0; i = i-5) {
                ledcWrite(LED_PIN, i);
                vTaskDelay(LED_DELAY/ portTICK_PERIOD_MS);
            }
        }else{
            vTaskDelay(10/ portTICK_PERIOD_MS);
        }
    }
}

void DNS_TASK(void *parameter) {
    while (true) {
        dnsServer.processNextRequest();
        vTaskDelay(2 / portTICK_PERIOD_MS); 
    }
}


void AsyncWifiScanReplacement(void *parameter) {
    while (1) {
        if (Enable_WifiScanning) {
            total_networks = WiFi.scanNetworks(false, true);
            Enable_WifiScanning = 0; 
        }
        vTaskDelay(50 / portTICK_PERIOD_MS);
    }
}

#endif 
