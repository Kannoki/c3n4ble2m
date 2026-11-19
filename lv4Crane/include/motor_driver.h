#pragma once

#include <Arduino.h>
#include "config.h"

// Trạng thái của Động cơ 1
enum Motor1State {
    M1_STATE_STOP = 0,
    M1_STATE_FORWARD,
    M1_STATE_BACKWARD
};

// Trạng thái của Động cơ 2 (Máy trạng thái non-blocking)
enum Motor2State {
    M2_STATE_IDLE = 0,
    M2_STATE_FORWARD,   // Pha 1: Quay thuận (2 giây)
    M2_STATE_REVERSE    // Pha 2: Quay ngược (2 giây)
};

typedef void (*BleResponseCallback)(const char* text);

class MotorController {
public:
    MotorController();
    void begin();

    // Điều khiển Động cơ 1 (Bánh xe / Trục chính)
    void motor1Forward(uint8_t speed = DEFAULT_MOTOR_SPEED);
    void motor1Backward(uint8_t speed = DEFAULT_MOTOR_SPEED);
    void motor1Stop();

    // Điều khiển Động cơ 2 (Tời nâng / Trục phụ)
    void motor2Forward(uint8_t speed = DEFAULT_MOTOR_SPEED);
    void motor2Backward(uint8_t speed = DEFAULT_MOTOR_SPEED);
    void motor2Stop();

    // Kích hoạt chu trình tự động của Động cơ 2: Thuận 2s -> Ngược 2s -> Dừng
    void motor2StartCycle(uint8_t speed = DEFAULT_MOTOR_SPEED);

    // Cập nhật máy trạng thái non-blocking (gọi liên tục trong loop)
    void update();

    // Phanh dừng khẩn cấp toàn bộ động cơ
    void stopAll();

    // Đăng ký callback gửi phản hồi qua BLE TX
    void setCallback(BleResponseCallback cb) { _callback = cb; }

    Motor1State getM1State() const { return _m1State; }
    Motor2State getM2State() const { return _m2State; }

private:
    Motor1State _m1State;
    Motor2State _m2State;
    uint32_t _m2StartTime;
    uint8_t _m2Speed;
    BleResponseCallback _callback;

    void notify(const char* msg);
    void setMotor1Outputs(int speed); // -255 đến +255
    void setMotor2Outputs(int speed); // -255 đến +255
};

extern MotorController motors;
