#pragma once

#include <Arduino.h>
#include "config.h"

// Trạng thái của Động cơ 1 (Mâm xoay cần cẩu: Qua Trái / Qua Phải)
enum Motor1State {
    M1_STATE_STOP = 0,
    M1_STATE_LEFT,
    M1_STATE_RIGHT,
    M1_STATE_FORWARD = M1_STATE_LEFT,    // Alias tương thích ngược
    M1_STATE_BACKWARD = M1_STATE_RIGHT   // Alias tương thích ngược
};

// Trạng thái của Động cơ 2 (Máy trạng thái non-blocking)
enum Motor2State {
    M2_STATE_IDLE = 0,
    M2_STATE_BTN1_HELD,       // Nhấn giữ Nút 1: Đang quay trái & đếm thời gian nhấn
    M2_STATE_BTN1_WAIT,       // Thả Nút 1: Chờ 1 giây trước khi kéo lên
    M2_STATE_BTN1_RETURNING   // Hết 1s: Tự động kéo lên đúng bằng thời gian đã nhấn giữ
};

typedef void (*BleResponseCallback)(const char* text);

class MotorController {
public:
    MotorController();
    void begin();

    // Điều khiển Động cơ 1 (Mâm quay / Trục chính - Hold-to-run)
    // Nút Lên -> Qua Trái; Nút Xuống -> Qua Phải
    void motor1Left(uint8_t speed = DEFAULT_MOTOR_SPEED);
    void motor1Right(uint8_t speed = DEFAULT_MOTOR_SPEED);
    void motor1Forward(uint8_t speed = DEFAULT_MOTOR_SPEED) { motor1Left(speed); }
    void motor1Backward(uint8_t speed = DEFAULT_MOTOR_SPEED) { motor1Right(speed); }
    void motor1Stop();

    // Điều khiển Động cơ 2 thủ công (Tời cẩu)
    void motor2Forward(uint8_t speed = DEFAULT_MOTOR_SPEED);
    void motor2Backward(uint8_t speed = DEFAULT_MOTOR_SPEED);
    void motor2Stop();

    // Logic Nút 1: Nhấn giữ quay trái -> Thả ra chờ 1s -> Kéo lên đúng thời gian nhấn
    void btn1Pressed(uint8_t speed = DEFAULT_MOTOR_SPEED);
    void btn1Released();

    // Mô phỏng chu trình Nút 1 (cho lệnh text UART)
    void motor2StartCycle(uint32_t durationMs = 2000, uint8_t speed = DEFAULT_MOTOR_SPEED);

    // Cập nhật máy trạng thái non-blocking (gọi liên tục trong loop)
    void update();

    // Phanh dừng khẩn cấp toàn bộ động cơ
    void stopAll();

    // Đăng ký callback gửi phản hồi qua BLE TX
    void setCallback(BleResponseCallback cb) { _callback = cb; }

    Motor1State getM1State() const { return _m1State; }
    Motor2State getM2State() const { return _m2State; }
    uint32_t getBtn1HoldDurationMs() const { return _btn1HoldDurationMs; }

private:
    Motor1State _m1State;
    Motor2State _m2State;

    // Biến thời gian máy trạng thái Nút 1
    uint32_t _btn1PressStartTime;
    uint32_t _btn1WaitStartTime;
    uint32_t _btn1ReturnStartTime;
    uint32_t _btn1HoldDurationMs;

    uint8_t _m2Speed;
    BleResponseCallback _callback;

    void notify(const char* msg);
    void setMotor1Outputs(int speed); // -255 đến +255
    void setMotor2Outputs(int speed); // -255 đến +255
};

extern MotorController motors;
