/**
 * VÍ DỤ 9: MÔ PHỎNG TRẠM MẶT ĐẤT (GROUND STATION)
 * 
 * Mô tả:
 * Đoạn code này dành cho board ESP32 đóng vai trò là Trạm Mặt Đất (Receiver).
 * Nhiệm vụ:
 * 1. Lắng nghe liên tục từ module LoRa.
 * 2. Khi nhận được JSON Telemetry từ Vệ Tinh, dùng ArduinoJson để dịch và hiển thị đẹp.
 * 3. Cho phép gõ lệnh từ Serial Monitor (như 'pause', 'ping', 'ls') để gửi lên vệ tinh.
 */

#include <Arduino.h>
#include <ArduinoJson.h>
#include <PTITCube.h>

// Khởi tạo LoRa cho trạm mặt đất
PTIT_COM lora;

void setup() {
    Serial.begin(115200);
    while (!Serial) { delay(10); }

    Serial.println("\n==========================================");
    Serial.println("   TRẠM MẶT ĐẤT (GROUND STATION) SẴN SÀNG");
    Serial.println("==========================================");
    Serial.println("Đang kết nối LoRa...");
    
    lora.init();
    lora.setChannel(23);           // Kênh 23, phải khớp với vệ tinh
    lora.setTransmissionMode(0);   // Chế độ Transparent

    Serial.println("Bạn có thể gõ lệnh (ping, status, pause, resume...) và nhấn Enter để gửi.");
}

void loop() {
    // 1. NGHE DỮ LIỆU TỪ VỆ TINH
    String incoming = lora.update();
    if (incoming.length() > 0) {
        incoming.trim();

        // Xử lý chuỗi JSON
        if (incoming.startsWith("{") && incoming.endsWith("}")) {
            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, incoming);
            
            if (!error) {
                // Phân loại JSON dựa trên các key
                if (doc.containsKey("battery_v")) {
                    float bat = doc["battery_v"];
                    Serial.printf("[TELEMETRY] 🔋 Pin vệ tinh: %.2f V\n", bat);
                }
                else if (doc.containsKey("temp")) {
                    float t = doc["temp"];
                    float p = doc["pressure"];
                    Serial.printf("[TELEMETRY] 🌤️ Môi trường: %.2f °C | Áp suất: %.1f hPa\n", t, p);
                }
                else if (doc.containsKey("lat")) {
                    float lat = doc["lat"];
                    float lng = doc["lng"];
                    Serial.printf("[TELEMETRY] 🌍 Tọa độ GPS: %f, %f\n", lat, lng);
                }
                else {
                    Serial.println("[TELEMETRY] Dữ liệu Cảm biến IMU/MAG: " + incoming);
                }
            } else {
                Serial.println("[LORA] JSON Lỗi: " + incoming);
            }
        } 
        // Xử lý chuỗi văn bản thuần (vd: phản hồi lệnh ls, pwd, ping)
        else {
            Serial.println("\n[SATELLITE REPLY] ➔ " + incoming);
        }
    }

    // 2. GỬI LỆNH LÊN VỆ TINH
    if (Serial.available()) {
        String cmd = Serial.readStringUntil('\n');
        cmd.trim();
        if (cmd.length() > 0) {
            Serial.println("\n[TX] Đang phát lệnh: " + cmd);
            lora.sendMessage(cmd);
        }
    }
}
