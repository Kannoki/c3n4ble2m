#pragma once

#include <Arduino.h>

// ==========================================
// CẤU HÌNH CHÂN KẾT NỐI ESP32-C3 VỚI MX1508
// ==========================================
// Động cơ 1 (2 bánh xe - Tiến / Lùi)
#define PIN_M1_IN1          4   // GPIO4 -> IN1 (MX1508)
#define PIN_M1_IN2          5   // GPIO5 -> IN2 (MX1508)

// Động cơ 2 (Quay 2s rồi quay ngược lại)
#define PIN_M2_IN3          6   // GPIO6 -> IN3 (MX1508)
#define PIN_M2_IN4          7   // GPIO7 -> IN4 (MX1508)

// Đèn LED báo trạng thái BLE (Tùy chọn: GPIO 8 hoặc GPIO 2 tùy board)
#define PIN_STATUS_LED      2   // GPIO2 (hoặc 8 cho DevKitM-1 RGB/LED)

// ==========================================
// CẤU HÌNH TỐC ĐỘ VÀ THỜI GIAN
// ==========================================
#define DEFAULT_MOTOR_SPEED 255 // Tốc độ PWM (0 - 255)
#define M2_RUN_DURATION_MS  2000 // Thời gian quay mỗi chiều của Động cơ 2 (2000 ms = 2 giây)

// ==========================================
// BLE NORDIC UART SERVICE (ADAFRUIT BLUEFRUIT)
// ==========================================
#define DEVICE_NAME         "ESP32C3-Motor"
#define SERVICE_UUID        "6E400001-B5A3-F393-E0A9-E50E24DCCA9E" // UART Service
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E" // App gửi xuống ESP32
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E" // ESP32 gửi lên App
