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
      _btn1PressStartTime(0),
      _btn1WaitStartTime(0),
      _btn1ReturnStartTime(0),
      _btn1HoldDurationMs(0),
      _m2Speed(DEFAULT_MOTOR_SPEED),
      _callback(nullptr) {
}

void MotorController::begin() {
    // 1. Kích hoạt chân enable DRV8833 (Active HIGH qua chân ~SLEEP)
    pinMode(MOTOR_ENABLE_PIN, OUTPUT);
    digitalWrite(MOTOR_ENABLE_PIN, HIGH);

    // 2. Reset GPIO9 khỏi chế độ JTAG/Boot trước khi dùng
    gpio_reset_pin(GPIO_NUM_9);

    // 3. Khởi tạo PWM cho Động cơ 1 (Motor A trên giắc J3 - Mâm xoay)
    setupPwmPin(MOTOR_A_IN1, MOTOR_A_IN1_CH);
    setupPwmPin(MOTOR_A_IN2, MOTOR_A_IN2_CH);

    // 4. Khởi tạo chân Động cơ 2 (Motor B trên giắc J4 - Tời cẩu)
    pinLow(MOTOR_B_IN1);
    setupPwmPin(MOTOR_B_IN2, MOTOR_B_IN2_CH);

    // 5. Đưa toàn bộ động cơ về trạng thái dừng ban đầu
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

// ------------------------------------------------------------------------------
// ĐỘNG CƠ 1: MÂM QUAY CẦN CẨU (NÚT LÊN -> QUA TRÁI / NÚT XUỐNG -> QUA PHẢI)
// ------------------------------------------------------------------------------
void MotorController::motor1Left(uint8_t speed) {
    _m1State = M1_STATE_LEFT;
    setMotor1Outputs(-((int)speed));
    notify("[M1] Qua Trai (Left)");
}

void MotorController::motor1Right(uint8_t speed) {
    _m1State = M1_STATE_RIGHT;
    setMotor1Outputs(speed);
    notify("[M1] Qua Phai (Right)");
}

void MotorController::motor1Stop() {
    _m1State = M1_STATE_STOP;
    setMotor1Outputs(0);
    notify("[M1] Dung (Stop)");
}

// ------------------------------------------------------------------------------
// ĐỘNG CƠ 2: ĐIỀU KHIỂN THỦ CÔNG (TỜI CẨU)
// ------------------------------------------------------------------------------
void MotorController::motor2Forward(uint8_t speed) {
    _m2State = M2_STATE_IDLE; // Hủy chu trình tự động nếu đang chạy
    setMotor2Outputs(speed);
    notify("[M2] Thu cong: Keo len / Quay thuan");
}

void MotorController::motor2Backward(uint8_t speed) {
    _m2State = M2_STATE_IDLE; // Hủy chu trình tự động nếu đang chạy
    setMotor2Outputs(-((int)speed));
    notify("[M2] Thu cong: Ha xuong / Quay trai");
}

void MotorController::motor2Stop() {
    _m2State = M2_STATE_IDLE;
    setMotor2Outputs(0);
    notify("[M2] Dung (Stop)");
}

// ------------------------------------------------------------------------------
// LOGIC NÚT 1:
// 1. Nhấn giữ: Quay trái và đếm thời gian
// 2. Thả ra: Dừng quay trái, chờ 1 giây
// 3. Sau 1s: Tự động kéo lên đúng bằng thời gian đã nhấn giữ -> Tự dừng
// ------------------------------------------------------------------------------
void MotorController::btn1Pressed(uint8_t speed) {
    _m2Speed = speed;
    _m2State = M2_STATE_BTN1_HELD;
    _btn1PressStartTime = millis();
    _btn1HoldDurationMs = 0;

    // Quay trái (chiều nghịch của Motor 2)
    setMotor2Outputs(-((int)_m2Speed));
    notify("[M2] Nut 1: Nhan giu -> Dang quay trai...");
}

void MotorController::btn1Released() {
    if (_m2State != M2_STATE_BTN1_HELD) {
        return;
    }

    // Dừng quay trái ngay lập tức khi nhả tay
    setMotor2Outputs(0);

    uint32_t now = millis();
    _btn1HoldDurationMs = now - _btn1PressStartTime;

    // Chống nhiễu phím (nếu thời gian nhấn quá ngắn dưới 50ms thì bỏ qua)
    if (_btn1HoldDurationMs < 50) {
        _m2State = M2_STATE_IDLE;
        notify("[M2] Nut 1: Thoi gian nhan qua ngan (<50ms) -> Bo qua.");
        return;
    }

    // Chuyển sang pha chờ 1 giây
    _m2State = M2_STATE_BTN1_WAIT;
    _btn1WaitStartTime = now;

    char buf[128];
    snprintf(buf, sizeof(buf), "[M2] Nut 1: Da tha (quay trai %lu ms) -> Cho 1s truoc khi keo len...", (unsigned long)_btn1HoldDurationMs);
    notify(buf);
}

void MotorController::motor2StartCycle(uint32_t durationMs, uint8_t speed) {
    _m2Speed = speed;
    _btn1HoldDurationMs = durationMs;
    _m2State = M2_STATE_BTN1_WAIT;
    _btn1WaitStartTime = millis();

    char buf[128];
    snprintf(buf, sizeof(buf), "[M2] Chu trinh dat truoc: Cho 1s roi tu dong keo len %lu ms...", (unsigned long)_btn1HoldDurationMs);
    notify(buf);
}

void MotorController::update() {
    if (_m2State == M2_STATE_IDLE) {
        return;
    }

    uint32_t now = millis();

    switch (_m2State) {
        case M2_STATE_BTN1_HELD: {
            // Giới hạn an toàn: Nếu giữ nút quá 60s (đề phòng rớt gói tin BLE nhả nút)
            if (now - _btn1PressStartTime >= BTN1_MAX_HOLD_DURATION_MS) {
                notify("[M2] Canh bao an toan: Dat gioi han nhan giu 60s -> Tu dong chuyen sang cho 1s.");
                btn1Released();
            }
            break;
        }

        case M2_STATE_BTN1_WAIT: {
            // Chờ đủ 1.0 giây (1000ms)
            if (now - _btn1WaitStartTime >= BTN1_WAIT_BEFORE_RETURN_MS) {
                _m2State = M2_STATE_BTN1_RETURNING;
                _btn1ReturnStartTime = now;

                // Bắt đầu kéo lên (chiều thuận của Motor 2)
                setMotor2Outputs(_m2Speed);

                char buf[128];
                snprintf(buf, sizeof(buf), "[M2] Het 1s cho -> Dang keo len dung %lu ms...", (unsigned long)_btn1HoldDurationMs);
                notify(buf);
            }
            break;
        }

        case M2_STATE_BTN1_RETURNING: {
            // Kéo lên đúng bằng khoảng thời gian đã nhấn giữ
            if (now - _btn1ReturnStartTime >= _btn1HoldDurationMs) {
                _m2State = M2_STATE_IDLE;
                setMotor2Outputs(0);

                char buf[128];
                snprintf(buf, sizeof(buf), "[M2] Da keo len hoan tat (%lu ms) -> Da tu dong dung.", (unsigned long)_btn1HoldDurationMs);
                notify(buf);
            }
            break;
        }

        default:
            break;
    }
}

void MotorController::stopAll() {
    motor1Stop();
    motor2Stop();
    _m2State = M2_STATE_IDLE;
    _btn1HoldDurationMs = 0;
    notify("[ALL] Dung toan bo dong co.");
}
