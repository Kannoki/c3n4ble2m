#pragma once

#include <Arduino.h>
#include "config_l298n.h"

enum L298NMotor2State {
    L298N_M2_IDLE,
    L298N_M2_FORWARD,
    L298N_M2_REVERSE
};

enum L298NMotor1State {
    L298N_M1_STOP,
    L298N_M1_FORWARD,
    L298N_M1_BACKWARD
};

typedef void (*L298NStatusCallback)(const char* message);

class MotorControllerL298N {
public:
    MotorControllerL298N();
    void begin();

    // Điều khiển Động cơ 1 (Bánh xe)
    void motor1Forward(uint8_t speed = DEFAULT_MOTOR_SPEED);
    void motor1Backward(uint8_t speed = DEFAULT_MOTOR_SPEED);
    void motor1Stop();
    L298NMotor1State getMotor1State() const { return _m1State; }

    // Điều khiển Động cơ 2 (Chu trình tự động 2s thuận -> 2s ngược -> Dừng)
    void motor2StartCycle(uint8_t speed = DEFAULT_MOTOR_SPEED);
    void motor2Stop();
    bool isMotor2Busy() const { return _m2State != L298N_M2_IDLE; }
    L298NMotor2State getMotor2State() const { return _m2State; }

    // Gọi liên tục trong loop() để cập nhật non-blocking
    void update();

    // Dừng khẩn cấp toàn bộ động cơ
    void stopAll();

    // Đăng ký callback gửi log về BLE app
    void setCallback(L298NStatusCallback cb) { _callback = cb; }

private:
    L298NMotor1State _m1State;
    L298NMotor2State _m2State;
    uint32_t _m2StartTime;
    uint8_t _m2Speed;
    L298NStatusCallback _callback;

    void notify(const char* msg);
    void setMotor1Outputs(uint8_t in1Val, uint8_t in2Val, uint8_t speed);
    void setMotor2Outputs(uint8_t in3Val, uint8_t in4Val, uint8_t speed);
};

extern MotorControllerL298N motorsL298N;
