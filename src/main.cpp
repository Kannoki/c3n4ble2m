#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#include "config.h"
#include "motor_driver.h"

// Biến quản lý BLE
static BLEServer* pServer = nullptr;
static BLECharacteristic* pTxCharacteristic = nullptr;
static bool deviceConnected = false;
static bool oldDeviceConnected = false;

// Hàm gửi phản hồi chuỗi ký tự về App Bluefruit qua BLE TX
void sendBleResponse(const char* text) {
    if (deviceConnected && pTxCharacteristic != nullptr) {
        String msg = String(text) + "\r\n";
        pTxCharacteristic->setValue((uint8_t*)msg.c_str(), msg.length());
        pTxCharacteristic->notify();
    }
}

// Xử lý dữ liệu nhận được từ Adafruit Bluefruit LE Connect App
void handleRxData(const uint8_t* data, size_t length) {
    if (length == 0) return;

    // In thô ra Serial để debug
    Serial.printf("[BLE RX] Len=%d, Data: ", length);
    for (size_t i = 0; i < length; i++) {
        if (isprint(data[i])) Serial.print((char)data[i]);
        else Serial.printf("<0x%02X>", data[i]);
    }
    Serial.println();

    // 1. Kiểm tra gói Controller Pad của Bluefruit (bắt đầu bằng '!B')
    // Gói dạng: '!B' + Button_Num + State_Press ('1'=nhấn, '0'=nhả) + Checksum
    if (length >= 4 && data[0] == '!' && data[1] == 'B') {
        char button = (char)data[2];
        char state  = (char)data[3];

        switch (button) {
            case '5': // Mũi tên LÊN (Up arrow) -> Động cơ 1 Tiến
                if (state == '1') {
                    motors.motor1Forward();
                } else {
                    motors.motor1Stop();
                }
                break;

            case '6': // Mũi tên XUỐNG (Down arrow) -> Động cơ 1 Lùi
                if (state == '1') {
                    motors.motor1Backward();
                } else {
                    motors.motor1Stop();
                }
                break;

            case '1': // Nút số 1 (Button 1) -> Kích hoạt chu trình Động cơ 2
                if (state == '1') {
                    motors.motor2StartCycle();
                }
                break;

            case '2': // Nút số 2 (Button 2) -> Dừng khẩn cấp toàn bộ
                if (state == '1') {
                    motors.stopAll();
                }
                break;

            case '7': // Mũi tên TRÁI (Left) - dự phòng
            case '8': // Mũi tên PHẢI (Right) - dự phòng
            case '3': // Nút 3
            case '4': // Nút 4
                if (state == '1') {
                    Serial.printf("Nut %c duoc nhan (Chua gan tinh nang)\n", button);
                }
                break;

            default:
                break;
        }
        return;
    }

    // 2. Xử lý lệnh dạng Text (khi dùng tab UART Terminal của Bluefruit)
    String cmd = "";
    for (size_t i = 0; i < length; i++) {
        char c = (char)data[i];
        if (c != '\r' && c != '\n') {
            cmd += c;
        }
    }
    cmd.trim();

    if (cmd.equalsIgnoreCase("F") || cmd.equalsIgnoreCase("TIEN")) {
        motors.motor1Forward();
    } else if (cmd.equalsIgnoreCase("B") || cmd.equalsIgnoreCase("LUI")) {
        motors.motor1Backward();
    } else if (cmd.equalsIgnoreCase("S") || cmd.equalsIgnoreCase("DUNG")) {
        motors.motor1Stop();
    } else if (cmd.equalsIgnoreCase("2") || cmd.equalsIgnoreCase("M2") || cmd.equalsIgnoreCase("ACTION")) {
        motors.motor2StartCycle();
    } else if (cmd.equalsIgnoreCase("X") || cmd.equalsIgnoreCase("STOP")) {
        motors.stopAll();
    } else if (cmd.length() > 0) {
        Serial.printf("Lenh text khong xac dinh: %s\n", cmd.c_str());
        sendBleResponse("Lenh hop le: F (Tien), B (Lui), S (Dung), 2 (Chay M2), X (Dung het)");
    }
}

// Callback sự kiện kết nối BLE Server
class MyServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer* server) override {
        deviceConnected = true;
        Serial.println(">> [BLE] Thiet bi da ket noi!");
#if defined(PIN_STATUS_LED)
        digitalWrite(PIN_STATUS_LED, HIGH);
#endif
    }

    void onDisconnect(BLEServer* server) override {
        deviceConnected = false;
        Serial.println(">> [BLE] Mat ket noi! Dung toan bo dong co.");
        motors.stopAll();
#if defined(PIN_STATUS_LED)
        digitalWrite(PIN_STATUS_LED, LOW);
#endif
    }
};

// Callback khi App gửi dữ liệu xuống qua Characteristic RX
class MyRxCallbacks : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic* pCharacteristic) override {
        std::string rxValue = pCharacteristic->getValue();
        if (rxValue.length() > 0) {
            handleRxData((const uint8_t*)rxValue.data(), rxValue.length());
        }
    }
};

void setup() {
    Serial.begin(115200);
    delay(500);

    Serial.println("\n==========================================");
    Serial.println("   ESP32-C3 DUAL MOTOR BLUEFRUIT CONTROLLER");
    Serial.println("   M1: 2 Banh xe (Tien/Lui)");
    Serial.println("   M2: Quay thuan 2s -> Quay nguoc 2s -> Dung");
    Serial.println("   Driver: MX1508");
    Serial.println("==========================================\n");

#if defined(PIN_STATUS_LED)
    pinMode(PIN_STATUS_LED, OUTPUT);
    digitalWrite(PIN_STATUS_LED, LOW);
#endif

    // Khởi tạo điều khiển động cơ
    motors.begin();
    motors.setCallback(sendBleResponse);

    // Khởi tạo BLE
    BLEDevice::init(DEVICE_NAME);
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());

    // Tạo Service Nordic UART
    BLEService* pService = pServer->createService(SERVICE_UUID);

    // Characteristic TX (Gửi trạng thái về điện thoại)
    pTxCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID_TX,
        BLECharacteristic::PROPERTY_NOTIFY
    );
    pTxCharacteristic->addDescriptor(new BLE2902());

    // Characteristic RX (Nhận lệnh từ điện thoại)
    BLECharacteristic* pRxCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID_RX,
        BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR
    );
    pRxCharacteristic->setCallbacks(new MyRxCallbacks());

    pService->start();

    // Bắt đầu quảng bá (Advertising)
    BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);
    pAdvertising->setMinPreferred(0x12);
    BLEDevice::startAdvertising();

    Serial.printf(">> BLE san sang! Ten thiet bi: %s\n", DEVICE_NAME);
    Serial.println(">> Hay mo app 'Adafruit Bluefruit LE Connect' de ket noi.");
}

void loop() {
    // Luôn cập nhật trạng thái non-blocking của động cơ (đặc biệt là M2 đếm thời gian 2s)
    motors.update();

    // Tự động kích hoạt lại Advertising khi bị ngắt kết nối
    if (!deviceConnected && oldDeviceConnected) {
        delay(500); // Đợi bluetooth stack ổn định
        pServer->startAdvertising();
        Serial.println(">> [BLE] Bat dau quang ba lai (Advertising)...");
        oldDeviceConnected = deviceConnected;
    }
    if (deviceConnected && !oldDeviceConnected) {
        oldDeviceConnected = deviceConnected;
    }

    // Delay nhỏ để nhường CPU cho background BLE stack và tránh nóng chip
    delay(5);
}
