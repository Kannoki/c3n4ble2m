#include "motor_driver_l298n.h"

MotorControllerL298N motorsL298N;

MotorControllerL298N::MotorControllerL298N()
    : _m1State(L298N_M1_STOP),
      _m2State(L298N_M2_IDLE),
      _m2StartTime(0),
      _m2Speed(DEFAULT_MOTOR_SPEED),
      _callback(nullptr) {
}

void MotorControllerL298N::begin() {
    pinMode(PIN_L298N_IN1, OUTPUT);
    pinMode(PIN_L298N_IN2, OUTPUT);
    pinMode(PIN_L298N_IN3, OUTPUT);
    pinMode(PIN_L298N_IN4, OUTPUT);

#if L298N_USE_ENABLE_PINS
    pinMode(PIN_L298N_ENA, OUTPUT);
    pinMode(PIN_L298N_ENB, OUTPUT);
#endif

    stopAll();
}

void MotorControllerL298N::notify(const char* msg) {
    Serial.println(msg);
    if (_callback) {
        _callback(msg);
    }
}

void MotorControllerL298N::setMotor1Outputs(uint8_t in1Val, uint8_t in2Val, uint8_t speed) {
#if L298N_USE_ENABLE_PINS
    digitalWrite(PIN_L298N_IN1, in1Val);
    digitalWrite(PIN_L298N_IN2, in2Val);
    analogWrite(PIN_L298N_ENA, speed);
#else
    // Chế độ 4 chân (Jumper ENA gắn sẵn 5V)
    if (in1Val && !in2Val) {
        analogWrite(PIN_L298N_IN1, speed);
        analogWrite(PIN_L298N_IN2, 0);
    } else if (!in1Val && in2Val) {
        analogWrite(PIN_L298N_IN1, 0);
        analogWrite(PIN_L298N_IN2, speed);
    } else {
        analogWrite(PIN_L298N_IN1, 0);
        analogWrite(PIN_L298N_IN2, 0);
    }
#endif
}

void MotorControllerL298N::setMotor2Outputs(uint8_t in3Val, uint8_t in4Val, uint8_t speed) {
#if L298N_USE_ENABLE_PINS
    digitalWrite(PIN_L298N_IN3, in3Val);
    digitalWrite(PIN_L298N_IN4, in4Val);
    analogWrite(PIN_L298N_ENB, speed);
#else
    // Chế độ 4 chân (Jumper ENB gắn sẵn 5V)
    if (in3Val && !in4Val) {
        analogWrite(PIN_L298N_IN3, speed);
        analogWrite(PIN_L298N_IN4, 0);
    } else if (!in3Val && in4Val) {
        analogWrite(PIN_L298N_IN3, 0);
        analogWrite(PIN_L298N_IN4, speed);
    } else {
        analogWrite(PIN_L298N_IN3, 0);
        analogWrite(PIN_L298N_IN4, 0);
    }
#endif
}

void MotorControllerL298N::motor1Forward(uint8_t speed) {
    _m1State = L298N_M1_FORWARD;
    setMotor1Outputs(HIGH, LOW, speed);
    notify("[L298N - M1] Tien (Forward)");
}

void MotorControllerL298N::motor1Backward(uint8_t speed) {
    _m1State = L298N_M1_BACKWARD;
    setMotor1Outputs(LOW, HIGH, speed);
    notify("[L298N - M1] Lui (Backward)");
}

void MotorControllerL298N::motor1Stop() {
    _m1State = L298N_M1_STOP;
    setMotor1Outputs(LOW, LOW, 0);
    notify("[L298N - M1] Dung (Stop)");
}

void MotorControllerL298N::motor2StartCycle(uint8_t speed) {
    _m2Speed = speed;
    _m2State = L298N_M2_FORWARD;
    _m2StartTime = millis();
    setMotor2Outputs(HIGH, LOW, _m2Speed);
    notify("[L298N - M2] Bat dau chu trinh: Quay thuan 2s...");
}

void MotorControllerL298N::motor2Stop() {
    _m2State = L298N_M2_IDLE;
    setMotor2Outputs(LOW, LOW, 0);
    notify("[L298N - M2] Dung (Stop)");
}

void MotorControllerL298N::update() {
    if (_m2State == L298N_M2_IDLE) {
        return;
    }

    uint32_t now = millis();
    uint32_t elapsed = now - _m2StartTime;

    if (_m2State == L298N_M2_FORWARD) {
        if (elapsed >= M2_RUN_DURATION_MS) {
            // Chuyển sang quay ngược trong 2s
            _m2State = L298N_M2_REVERSE;
            _m2StartTime = now;
            setMotor2Outputs(LOW, HIGH, _m2Speed);
            notify("[L298N - M2] Het 2s thuan -> Dao chieu quay nguoc 2s...");
        }
    } else if (_m2State == L298N_M2_REVERSE) {
        if (elapsed >= M2_RUN_DURATION_MS) {
            // Hoàn thành chu trình
            _m2State = L298N_M2_IDLE;
            setMotor2Outputs(LOW, LOW, 0);
            notify("[L298N - M2] Hoan thanh chu trinh -> Da dung.");
        }
    }
}

void MotorControllerL298N::stopAll() {
    motor1Stop();
    motor2Stop();
    notify("[L298N - ALL] Dung toan bo dong co khancap.");
}
