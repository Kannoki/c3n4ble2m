#pragma once

#include <Arduino.h>

// ==============================================================================
// CẤU HÌNH GPIO CHUẨN XÁC TỪ SƠ ĐỒ NGUYÊN LÝ KICAD (Shield.kicad_sch)
// Bo mạch Shield: ESP32-C3 SuperMini + DRV8833 Dual H-Bridge
// ==============================================================================

// ------------------------------------------------------------------------------
// 1. DRIVER ĐỘNG CƠ CẦU H DRV8833 (IC U4)
// ------------------------------------------------------------------------------
// Chân kích hoạt Driver (Active HIGH -> nối chân ~SLEEP của DRV8833 qua Net EN)
#define MOTOR_ENABLE_PIN    21  // GPIO21 -> Net EN -> Pin 1 (~SLEEP)

// MOTOR 1 (Kênh B trên DRV8833 -> Giắc J3 -> Cọc AOUT1 / AOUT2 hoặc BOUT1 / BOUT2)
// BIN1 / BIN2 băm xung PWM điều khiển tốc độ & chiều quay
#define MOTOR_A_IN1         3   // GPIO3  -> Net BIN2 -> Pin 10 (BIN2)
#define MOTOR_A_IN2         1   // GPIO1  -> Net BIN1 -> Pin 9  (BIN1)

// MOTOR 2 (Kênh A trên DRV8833 -> Giắc J4 -> Cọc AOUT1 / AOUT2)
// AIN1 / AIN2 điều khiển động cơ 2 (AIN2 trên GPIO9 là chân BOOT)
#define MOTOR_B_IN1         9   // GPIO9  -> Net AIN2 -> Pin 15 (AIN2 - Chân BOOT)
#define MOTOR_B_IN2         20  // GPIO20 -> Net AIN1 -> Pin 16 (AIN1)

// Kênh phần cứng LEDC PWM (ESP32-C3)
#define MOTOR_A_IN1_CH      0
#define MOTOR_A_IN2_CH      1
#define MOTOR_B_IN2_CH      2

// Tần số & độ phân giải xung PWM
#define PWM_FREQ            1000
#define PWM_RESOLUTION      8   // 0 - 255
#define PWM_MAX             ((1 << PWM_RESOLUTION) - 1)

// ------------------------------------------------------------------------------
// 2. GIẮC CẮM ĐỘNG CƠ SERVO (Giắc J9 & J10 trên Shield)
// ------------------------------------------------------------------------------
#define SERVO_1_PIN         6   // GPIO6 -> Net SER1 -> Giắc J9 (Chân 5)
#define SERVO_2_PIN         8   // GPIO8 -> Net SER2 -> Giắc J9 (Chân 6)
#define SERVO_3_PIN         4   // GPIO4 -> Net SER3 -> Giắc J10 (Chân 5 - Móc cẩu Crane)
#define SERVO_4_PIN         0   // GPIO0 -> Net SER4 -> Giắc J10 (Chân 6)

// ------------------------------------------------------------------------------
// 3. CÒI BUZZER & LED RGB WS2812 (Giắc J7, J11 trên Shield)
// ------------------------------------------------------------------------------
#define BUZZER_PIN          7   // GPIO7  -> Net Buzz -> Giắc J11 (Chân 1)
#define RGB_LED1_PIN        2   // GPIO2  -> Net RGB1 -> Giắc J7  (Chân 1)
#define RGB_LED2_PIN        10  // GPIO10 -> Net RGB2 -> Giắc J5  (Chân 1)

// GPIO dự phòng / chưa định tuyến nhãn
#define SPARE_GPIO          5   // GPIO5

// ==============================================================================
// CẤU HÌNH LOGIC ĐIỀU KHIỂN & BLE (THEO DỰ ÁN STD_BLE_2M)
// ==============================================================================
#define DEFAULT_MOTOR_SPEED 255   // Tốc độ PWM mặc định (0 - 255)
#define M2_RUN_DURATION_MS  2000  // Chu trình Động cơ 2 (2000ms = 2.0 giây)

// Nordic UART Service (App Adafruit Bluefruit LE Connect)
#define DEVICE_NAME             "CyberBricked-lv4Crane"
#define SERVICE_UUID            "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_RX  "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX  "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"
