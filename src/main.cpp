/**
 * VÍ DỤ TỔNG HỢP (LAB 5-03): HỆ THỐNG GIAO TIẾP VỆ TINH HAI CHIỀU (COMMAND & TELEMETRY)
 */
#include <Arduino.h>
#include <ArduinoJson.h>
#include <PTITCube.h>

PTIT_CDH cdh;

PTIT_COM lora;
PTIT_EPS eps;
PTIT_Sensor sensor;
PTIT_GPS gps;

#define LED_PIN 2

void setup() {
    cdh.begin();

    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

    lora.init();
    lora.setChannel(23);           // Kênh 23 (433 MHz)
    lora.setTransmissionMode(0);   // Transparent Mode

    eps.init();
    sensor.init();
    gps.init();

}

void loop() {
    sensor.update();
    gps.update();

    String incomingCmd = lora.update();
    
    if (incomingCmd.length() > 0) {
        incomingCmd.trim();
        incomingCmd.toLowerCase();
        
        delay(20);

        String reply = "";

        if (incomingCmd.indexOf("ping") != -1) {
            reply = "pong - CubeSat OK";
        } 
        else if (incomingCmd.indexOf("battery") != -1 || incomingCmd.indexOf("bat") != -1) {
            char buffer[64];
            snprintf(buffer, sizeof(buffer), "Bat: %.2fV (%.1f%%)", eps.getBatteryVoltage(), eps.getBatteryPercent());
            reply = String(buffer);
        }
        else if (incomingCmd.indexOf("sensor") != -1 || incomingCmd.indexOf("env") != -1) {
            char buffer[64];
            snprintf(buffer, sizeof(buffer), "Temp: %.1fC | Press: %.1fhPa", sensor.getTemperature(), sensor.getPressure());
            reply = String(buffer);
        }
        else if (incomingCmd.indexOf("imu") != -1) {
            char buffer[128];
            snprintf(buffer, sizeof(buffer), "Roll: %.1f | Pitch: %.1f | Head: %.1f", 
                     sensor.getRoll(), sensor.getPitch(), sensor.getHeading());
            reply = String(buffer);
        }
        else if (incomingCmd.indexOf("gps") != -1) {
            char buffer[64];
            snprintf(buffer, sizeof(buffer), "GPS: Lat %.6f, Lng %.6f", gps.getLat(), gps.getLng());
            reply = String(buffer);
        }
        else if (incomingCmd.indexOf("led on") != -1) {
            digitalWrite(LED_PIN, HIGH);
            reply = "LED ON";
        }
        else if (incomingCmd.indexOf("led off") != -1) {
            digitalWrite(LED_PIN, LOW);
            reply = "LED OFF";
        }
        else if (incomingCmd.indexOf("telemetry") != -1) {
            JsonDocument doc;
            doc["v"] = eps.getBatteryVoltage();
            doc["t"] = sensor.getTemperature();
            doc["p"] = sensor.getPressure();
            doc["lat"] = gps.getLat();
            doc["lng"] = gps.getLng();
            
            String jsonOutput;
            serializeJson(doc, jsonOutput);
            reply = jsonOutput;
        }
        else {
            reply = "ERR: Unknown cmd [" + incomingCmd + "]";
        }

        cdh.println("CMD: " + incomingCmd + " -> " + reply);
        lora.sendMessage(reply);
    }
}
