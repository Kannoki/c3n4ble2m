#pragma once

#include <Arduino.h>

// ==========================================
// CẤU HÌNH CHÂN KẾT NỐI ESP32-C3 VỚI MODULE L298N
// ==========================================

// Chọn chế độ điều khiển L298N:
// 1 = Dùng 6 chân (Có điều tốc PWM qua ENA, ENB - Cần rút 2 jumper 5V trên L298N)
// 0 = Dùng 4 chân (Gắn sẵn 2 jumper ENA, ENB vào 5V trên L298N - PWM trực tiếp vào IN1-IN4)
#define L298N_USE_ENABLE_PINS   1

// Động cơ 1 (2 bánh xe - Tiến / Lùi)
#define PIN_L298N_ENA           3   // GPIO3 -> ENA (Điều tốc PWM Kênh A)
#define PIN_L298N_IN1           4   // GPIO4 -> IN1 (L298N)
#define PIN_L298N_IN2           5   // GPIO5 -> IN2 (L298N)

// Động cơ 2 (Quay 2s thuận -> 2s ngược)
#define PIN_L298N_IN3           6   // GPIO6 -> IN3 (L298N)
#define PIN_L298N_IN4           7   // GPIO7 -> IN4 (L298N)
#define PIN_L298N_ENB           10  // GPIO10 -> ENB (Điều tốc PWM Kênh B)

// Đèn LED báo trạng thái BLE
#define PIN_STATUS_LED          2   // GPIO2

// ==========================================
// CẤU HÌNH TỐC ĐỘ VÀ THỜI GIAN
// ==========================================
#define DEFAULT_MOTOR_SPEED     255  // Tốc độ PWM mặc định (0 - 255)
#define M2_RUN_DURATION_MS      2000 // Thời gian quay mỗi chiều của Động cơ 2 (2000 ms = 2 giây)

// ==========================================
// BLE NORDIC UART SERVICE (ADAFRUIT BLUEFRUIT)
// ==========================================
#define DEVICE_NAME             "ESP32C3-L298N-Motor"
#define SERVICE_UUID            "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_RX  "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX  "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"
