#include "motor_driver.h"
#include "driver/gpio.h"

MotorController motors;

// Hỗ trợ chân GPIO 9 (BOOT pin) trên ESP32-C3
static void pinHigh(int pin) {
    if (pin == 9) {
        pinMode(pin, INPUT_PULLUP);
    } else {
        pinMode(pin, OUTPUT);
        digitalWrite(pin, HIGH);
    }
}

static void pinLow(int pin) {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
}

static void setupPwmPin(int pin, int channel) {
    ledcSetup(channel, PWM_FREQ, PWM_RESOLUTION);
    ledcAttachPin(pin, channel);
    ledcWrite(channel, 0);
}

MotorController::MotorController()
    : _m1State(M1_STATE_STOP),
      _m2State(M2_STATE_IDLE),
      _m2StartTime(0),
      _m2Speed(DEFAULT_MOTOR_SPEED),
      _callback(nullptr) {
}

void MotorController::begin() {
    // 1. Kích hoạt chân enable DRV8833 (Active HIGH)
    pinMode(MOTOR_ENABLE_PIN, OUTPUT);
    digitalWrite(MOTOR_ENABLE_PIN, HIGH);

    // 2. Reset GPIO9 khỏi chế độ JTAG/Boot trước khi dùng
    gpio_reset_pin(GPIO_NUM_9);

    // 3. Khởi tạo PWM cho Động cơ 1 (Motor A)
    setupPwmPin(MOTOR_A_IN1, MOTOR_A_IN1_CH);
    setupPwmPin(MOTOR_A_IN2, MOTOR_A_IN2_CH);

    // 4. Khởi tạo chân Động cơ 2 (Motor B)
    pinLow(MOTOR_B_IN1);
    setupPwmPin(MOTOR_B_IN2, MOTOR_B_IN2_CH);

    // 5. Đưa toàn bộ động cơ về trạng thái dừng
    stopAll();
}

void MotorController::notify(const char* msg) {
    Serial.println(msg);
    if (_callback) {
        _callback(msg);
    }
}

void MotorController::setMotor1Outputs(int speed) {
    speed = constrain(speed, -PWM_MAX, PWM_MAX);
    if (speed > 0) {
        ledcWrite(MOTOR_A_IN1_CH, speed);
        ledcWrite(MOTOR_A_IN2_CH, 0);
    } else if (speed < 0) {
        ledcWrite(MOTOR_A_IN1_CH, 0);
        ledcWrite(MOTOR_A_IN2_CH, -speed);
    } else {
        ledcWrite(MOTOR_A_IN1_CH, 0);
        ledcWrite(MOTOR_A_IN2_CH, 0);
    }
}

void MotorController::setMotor2Outputs(int speed) {
    speed = constrain(speed, -PWM_MAX, PWM_MAX);
    if (speed > 0) {
        pinHigh(MOTOR_B_IN1);
        ledcWrite(MOTOR_B_IN2_CH, PWM_MAX - speed);
    } else if (speed < 0) {
        pinLow(MOTOR_B_IN1);
        ledcWrite(MOTOR_B_IN2_CH, -speed);
    } else {
        pinLow(MOTOR_B_IN1);
        ledcWrite(MOTOR_B_IN2_CH, 0);
    }
}

void MotorController::motor1Forward(uint8_t speed) {
    _m1State = M1_STATE_FORWARD;
    setMotor1Outputs(speed);
    notify("[M1] Tien (Forward)");
}

void MotorController::motor1Backward(uint8_t speed) {
    _m1State = M1_STATE_BACKWARD;
    setMotor1Outputs(-((int)speed));
    notify("[M1] Lui (Backward)");
}

void MotorController::motor1Stop() {
    _m1State = M1_STATE_STOP;
    setMotor1Outputs(0);
    notify("[M1] Dung (Stop)");
}

void MotorController::motor2Forward(uint8_t speed) {
    _m2State = M2_STATE_IDLE; // Thoát chu trình tự động nếu đang chạy
    setMotor2Outputs(speed);
    notify("[M2] Quay thuan / Keo toi (Forward)");
}

void MotorController::motor2Backward(uint8_t speed) {
    _m2State = M2_STATE_IDLE; // Thoát chu trình tự động nếu đang chạy
    setMotor2Outputs(-((int)speed));
    notify("[M2] Quay nghich / Ha toi (Backward)");
}

void MotorController::motor2Stop() {
    _m2State = M2_STATE_IDLE;
    setMotor2Outputs(0);
    notify("[M2] Dung (Stop)");
}

void MotorController::motor2StartCycle(uint8_t speed) {
    _m2Speed = speed;
    _m2State = M2_STATE_FORWARD;
    _m2StartTime = millis();
    setMotor2Outputs(_m2Speed);
    notify("[M2] Bat dau chu trinh: Quay thuan 2s...");
}

void MotorController::update() {
    if (_m2State == M2_STATE_IDLE) {
        return;
    }

    uint32_t now = millis();
    uint32_t elapsed = now - _m2StartTime;

    if (_m2State == M2_STATE_FORWARD) {
        if (elapsed >= M2_RUN_DURATION_MS) {
            // Chuyển sang quay nghịch
            _m2State = M2_STATE_REVERSE;
            _m2StartTime = now;
            setMotor2Outputs(-((int)_m2Speed));
            notify("[M2] Het 2s thuan -> Tu dong dao chieu quay nguoc 2s...");
        }
    } else if (_m2State == M2_STATE_REVERSE) {
        if (elapsed >= M2_RUN_DURATION_MS) {
            // Hoàn tất chu trình
            _m2State = M2_STATE_IDLE;
            setMotor2Outputs(0);
            notify("[M2] Hoan tat chu trinh -> Da dung tu dong.");
        }
    }
}

void MotorController::stopAll() {
    motor1Stop();
    motor2Stop();
    notify("[ALL] Dung toan bo dong co.");
}
