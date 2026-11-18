/**
 * VÍ DỤ TỔNG HỢP (LAB 5-03): HỆ THỐNG GIAO TIẾP VỆ TINH FULL DUPLEX (COMMAND & TELEMETRY)
 * 
 * Mô tả:
 * - Mã nguồn này được nạp vào Vệ Tinh (OBC ESP32).
 * - Trạm mặt đất (Ground Station) sử dụng module LoRa gắn USB-TTL hoặc bo ESP32 làm Receiver.
 * 
 * Luồng hoạt động:
 * 1. Vệ tinh lắng nghe lệnh từ trạm mặt đất qua sóng LoRa (Kênh 23, Chế độ Transparent).
 * 2. Khi nhận lệnh (ping, battery, sensor, imu, gps, led on/off, telemetry):
 *    - Thực thi tác vụ tương ứng.
 *    - In xác nhận ra Serial Monitor (CDH).
 *    - Phản hồi kết quả ngược về Trạm mặt đất qua sóng LoRa.
 */

#include <Arduino.h>
#include <ArduinoJson.h>
#include <PTITCube.h>

PTIT_CDH cdh;

// Khởi tạo các phân hệ
PTIT_COM lora;
PTIT_EPS eps;
PTIT_Sensor sensor;
PTIT_GPS gps;

#define LED_PIN 2

void setup() {
    cdh.begin(115200);
    while (!cdh) { delay(10); }

    cdh.println("\n=================================================");
    cdh.println("    VỆ TINH PTITCUBE - PHÂN HỆ TRUYỀN THÔNG COM   ");
    cdh.println("=================================================");
        
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

    // 1. Khởi tạo và cấu hình LoRa đồng bộ với Trạm mặt đất
    cdh.println("[INIT] Đang khởi tạo LoRa...");
    lora.init();
    lora.setChannel(23);           // Kênh 23 (433 MHz), khớp hoàn toàn với trạm mặt đất
    lora.setTransmissionMode(0);   // Chế độ Transparent (0) để truyền nhận 2 chiều
    
    // 2. Khởi tạo các phân hệ tải và cảm biến
    cdh.println("[INIT] Đang khởi tạo Cảm biến và Nguồn...");
    eps.init();
    sensor.init(); 
    gps.init();

    cdh.println("[READY] Vệ tinh đã sẵn sàng nhận lệnh từ trạm mặt đất!");
}

void loop() {
    // Luôn cập nhật cảm biến và GPS để có dữ liệu mới nhất
    sensor.update();
    gps.update();

    // Lắng nghe lệnh từ mặt đất qua sóng LoRa
    String incomingCmd = lora.update();
    
    if (incomingCmd.length() > 0) {
        incomingCmd.trim();
        incomingCmd.toLowerCase(); // Chuẩn hóa chữ thường
        
        cdh.println("\n[RX] Nhận được lệnh từ mặt đất: " + incomingCmd);

        // Chờ transceiver ổn định trạng thái chuyển mạch RX -> TX
        delay(20);

        String reply = "";

        // --- XỬ LÝ CÁC LỆNH (COMMANDS) & TẠO PHẢN HỒI ---
        
        if (incomingCmd.indexOf("ping") != -1) {
            reply = "pong - Vệ tinh đang hoạt động tốt!";
        } 
        else if (incomingCmd.indexOf("battery") != -1 || incomingCmd.indexOf("bat") != -1) {
            float v = eps.getBatteryVoltage();
            char buffer[64];
            snprintf(buffer, sizeof(buffer), "Điện áp Pin vệ tinh: %.2f V", v);
            reply = String(buffer);
        }
        else if (incomingCmd.indexOf("sensor") != -1 || incomingCmd.indexOf("env") != -1) {
            float t = sensor.getTemperature();
            float p = sensor.getPressure();
            char buffer[64];
            snprintf(buffer, sizeof(buffer), "Nhiệt độ: %.2f C | Áp suất: %.1f hPa", t, p);
            reply = String(buffer);
        }
        else if (incomingCmd.indexOf("imu") != -1) {
            char buffer[128];
            snprintf(buffer, sizeof(buffer), "Gia tốc (X:%.1f Y:%.1f Z:%.1f) | Gyro (X:%.1f Y:%.1f Z:%.1f)", 
                     sensor.getAccX(), sensor.getAccY(), sensor.getAccZ(),
                     sensor.getGyroX(), sensor.getGyroY(), sensor.getGyroZ());
            reply = String(buffer);
        }
        else if (incomingCmd.indexOf("gps") != -1) {
            char buffer[64];
            snprintf(buffer, sizeof(buffer), "GPS: Lat %f, Lng %f", gps.getLat(), gps.getLng());
            reply = String(buffer);
        }
        else if (incomingCmd.indexOf("led on") != -1) {
            digitalWrite(LED_PIN, HIGH);
            reply = "Đã bật LED trên OBC.";
        }
        else if (incomingCmd.indexOf("led off") != -1) {
            digitalWrite(LED_PIN, LOW);
            reply = "Đã tắt LED trên OBC.";
        }
        else if (incomingCmd.indexOf("telemetry") != -1) {
            JsonDocument doc;
            doc["battery_v"] = eps.getBatteryVoltage();
            doc["temp"] = sensor.getTemperature();
            doc["pressure"] = sensor.getPressure();
            doc["lat"] = gps.getLat();
            doc["lng"] = gps.getLng();
            
            String jsonOutput;
            serializeJson(doc, jsonOutput);
            reply = jsonOutput;
        }
        else {
            reply = "Lỗi: Lệnh [" + incomingCmd + "] không hợp lệ! Các lệnh: ping, bat, sensor, imu, gps, led on/off, telemetry";
        }

        // Phát phản hồi ngược về Trạm mặt đất qua sóng LoRa
        cdh.println("[TX] Gửi phản hồi về trạm mặt đất: " + reply);
        lora.sendMessage(reply);
    }
}
