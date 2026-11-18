#pragma once

#include <Arduino.h>
#include "config.h"

enum Motor2State {
    M2_STATE_IDLE,
    M2_STATE_FORWARD,
    M2_STATE_REVERSE
};

enum Motor1State {
    M1_STATE_STOP,
    M1_STATE_FORWARD,
    M1_STATE_BACKWARD
};

// Kiểu hàm callback thông báo trạng thái
typedef void (*StatusCallback)(const char* message);

class MotorController {
public:
    MotorController();
    void begin();
    
    // Điều khiển Động cơ 1 (Bánh xe)
    void motor1Forward(uint8_t speed = DEFAULT_MOTOR_SPEED);
    void motor1Backward(uint8_t speed = DEFAULT_MOTOR_SPEED);
    void motor1Stop();
    Motor1State getMotor1State() const { return _m1State; }

    // Điều khiển Động cơ 2 (Chu trình tự động 2s thuận -> 2s ngược -> Dừng)
    void motor2StartCycle(uint8_t speed = DEFAULT_MOTOR_SPEED);
    void motor2Stop();
    bool isMotor2Busy() const { return _m2State != M2_STATE_IDLE; }
    Motor2State getMotor2State() const { return _m2State; }

    // Cần gọi liên tục trong hàm loop() để xử lý non-blocking
    void update();

    // Dừng khẩn cấp toàn bộ động cơ
    void stopAll();

    // Đăng ký callback gửi log về BLE app
    void setCallback(StatusCallback cb) { _callback = cb; }

private:
    Motor1State _m1State;
    Motor2State _m2State;
    uint32_t _m2StartTime;
    uint8_t _m2Speed;
    StatusCallback _callback;

    void notify(const char* msg);
    void setMotor1Outputs(uint8_t in1Val, uint8_t in2Val);
    void setMotor2Outputs(uint8_t in3Val, uint8_t in4Val);
};

extern MotorController motors;
