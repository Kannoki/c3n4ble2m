#include "motor_driver.h"

MotorController motors;

MotorController::MotorController() 
    : _m1State(M1_STATE_STOP),
      _m2State(M2_STATE_IDLE),
      _m2StartTime(0),
      _m2Speed(DEFAULT_MOTOR_SPEED),
      _callback(nullptr) {
}

void MotorController::begin() {
    pinMode(PIN_M1_IN1, OUTPUT);
    pinMode(PIN_M1_IN2, OUTPUT);
    pinMode(PIN_M2_IN3, OUTPUT);
    pinMode(PIN_M2_IN4, OUTPUT);

    stopAll();
}

void MotorController::notify(const char* msg) {
    Serial.println(msg);
    if (_callback) {
        _callback(msg);
    }
}

void MotorController::setMotor1Outputs(uint8_t in1Val, uint8_t in2Val) {
    analogWrite(PIN_M1_IN1, in1Val);
    analogWrite(PIN_M1_IN2, in2Val);
}

void MotorController::setMotor2Outputs(uint8_t in3Val, uint8_t in4Val) {
    analogWrite(PIN_M2_IN3, in3Val);
    analogWrite(PIN_M2_IN4, in4Val);
}

void MotorController::motor1Forward(uint8_t speed) {
    _m1State = M1_STATE_FORWARD;
    setMotor1Outputs(speed, 0);
    notify("[M1] Tien (Forward)");
}

void MotorController::motor1Backward(uint8_t speed) {
    _m1State = M1_STATE_BACKWARD;
    setMotor1Outputs(0, speed);
    notify("[M1] Lui (Backward)");
}

void MotorController::motor1Stop() {
    _m1State = M1_STATE_STOP;
    setMotor1Outputs(0, 0);
    notify("[M1] Dung (Stop)");
}

void MotorController::motor2StartCycle(uint8_t speed) {
    _m2Speed = speed;
    _m2State = M2_STATE_FORWARD;
    _m2StartTime = millis();
    setMotor2Outputs(_m2Speed, 0);
    notify("[M2] Bat dau chu trinh: Quay thuan 2s...");
}

void MotorController::motor2Stop() {
    _m2State = M2_STATE_IDLE;
    setMotor2Outputs(0, 0);
    notify("[M2] Dung (Stop)");
}

void MotorController::update() {
    if (_m2State == M2_STATE_IDLE) {
        return;
    }

    uint32_t now = millis();
    uint32_t elapsed = now - _m2StartTime;

    if (_m2State == M2_STATE_FORWARD) {
        if (elapsed >= M2_RUN_DURATION_MS) {
            // Chuyển sang quay ngược
            _m2State = M2_STATE_REVERSE;
            _m2StartTime = now;
            setMotor2Outputs(0, _m2Speed);
            notify("[M2] Het 2s thuan -> Dao chieu quay nguoc 2s...");
        }
    } else if (_m2State == M2_STATE_REVERSE) {
        if (elapsed >= M2_RUN_DURATION_MS) {
            // Hoàn thành chu trình
            _m2State = M2_STATE_IDLE;
            setMotor2Outputs(0, 0);
            notify("[M2] Hoan thanh chu trinh -> Da dung.");
        }
    }
}

void MotorController::stopAll() {
    motor1Stop();
    motor2Stop();
    notify("[ALL] Dung toan bo dong co khancap.");
}
