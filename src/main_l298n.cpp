#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#include "config_l298n.h"
#include "motor_driver_l298n.h"

// Quản lý kết nối BLE
static BLEServer* pServer = nullptr;
static BLECharacteristic* pTxCharacteristic = nullptr;
static bool deviceConnected = false;
static bool oldDeviceConnected = false;

// Gửi phản hồi về App Bluefruit
void sendBleResponse(const char* text) {
    if (deviceConnected && pTxCharacteristic != nullptr) {
        String msg = String(text) + "\r\n";
        pTxCharacteristic->setValue((uint8_t*)msg.c_str(), msg.length());
        pTxCharacteristic->notify();
    }
}

// Xử lý dữ liệu nhận từ App Adafruit Bluefruit
void handleRxData(const uint8_t* data, size_t length) {
    if (length == 0) return;

    Serial.printf("[BLE RX L298N] Len=%d, Data: ", length);
    for (size_t i = 0; i < length; i++) {
        if (isprint(data[i])) Serial.print((char)data[i]);
        else Serial.printf("<0x%02X>", data[i]);
    }
    Serial.println();

    // 1. Gói Controller Control Pad (!B...)
    if (length >= 4 && data[0] == '!' && data[1] == 'B') {
        char button = (char)data[2];
        char state  = (char)data[3];

        switch (button) {
            case '5': // Mũi tên LÊN -> Động cơ 1 Tiến
                if (state == '1') {
                    motorsL298N.motor1Forward();
                } else {
                    motorsL298N.motor1Stop();
                }
                break;

            case '6': // Mũi tên XUỐNG -> Động cơ 1 Lùi
                if (state == '1') {
                    motorsL298N.motor1Backward();
                } else {
                    motorsL298N.motor1Stop();
                }
                break;

            case '1': // Nút số 1 -> Kích hoạt chu trình Động cơ 2 (2s Thuận -> 2s Ngược)
                if (state == '1') {
                    motorsL298N.motor2StartCycle();
                }
                break;

            case '2': // Nút số 2 -> Dừng khẩn cấp toàn bộ
                if (state == '1') {
                    motorsL298N.stopAll();
                }
                break;

            default:
                break;
        }
        return;
    }

    // 2. Giao diện UART dòng lệnh
    String cmd = "";
    for (size_t i = 0; i < length; i++) {
        char c = (char)data[i];
        if (c != '\r' && c != '\n') {
            cmd += c;
        }
    }
    cmd.trim();

    if (cmd.equalsIgnoreCase("F") || cmd.equalsIgnoreCase("TIEN")) {
        motorsL298N.motor1Forward();
    } else if (cmd.equalsIgnoreCase("B") || cmd.equalsIgnoreCase("LUI")) {
        motorsL298N.motor1Backward();
    } else if (cmd.equalsIgnoreCase("S") || cmd.equalsIgnoreCase("DUNG")) {
        motorsL298N.motor1Stop();
    } else if (cmd.equalsIgnoreCase("2") || cmd.equalsIgnoreCase("M2") || cmd.equalsIgnoreCase("ACTION")) {
        motorsL298N.motor2StartCycle();
    } else if (cmd.equalsIgnoreCase("X") || cmd.equalsIgnoreCase("STOP")) {
        motorsL298N.stopAll();
    } else if (cmd.length() > 0) {
        sendBleResponse("Lenh hop le: F (Tien), B (Lui), S (Dung), 2 (Chay M2), X (Dung het)");
    }
}

class L298NServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer* server) override {
        deviceConnected = true;
        Serial.println(">> [BLE] Connected to Bluefruit App!");
#if defined(PIN_STATUS_LED)
        digitalWrite(PIN_STATUS_LED, HIGH);
#endif
    }

    void onDisconnect(BLEServer* server) override {
        deviceConnected = false;
        Serial.println(">> [BLE] Disconnected! Stopping all L298N motors.");
        motorsL298N.stopAll();
#if defined(PIN_STATUS_LED)
        digitalWrite(PIN_STATUS_LED, LOW);
#endif
    }
};

class L298NRxCallbacks : public BLECharacteristicCallbacks {
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
    Serial.println("   ESP32-C3 DUAL MOTOR CONTROLLER (L298N)  ");
    Serial.println("   M1: 2 Banh xe (Tien/Lui)               ");
    Serial.println("   M2: Quay thuan 2s -> Quay nguoc 2s     ");
    Serial.println("   Driver: L298N Dual H-Bridge            ");
    Serial.println("==========================================\n");

#if defined(PIN_STATUS_LED)
    pinMode(PIN_STATUS_LED, OUTPUT);
    digitalWrite(PIN_STATUS_LED, LOW);
#endif

    // Khởi tạo điều khiển L298N
    motorsL298N.begin();
    motorsL298N.setCallback(sendBleResponse);

    // Khởi tạo BLE
    BLEDevice::init(DEVICE_NAME);
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new L298NServerCallbacks());

    // Nordic UART Service
    BLEService* pService = pServer->createService(SERVICE_UUID);

    pTxCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID_TX,
        BLECharacteristic::PROPERTY_NOTIFY
    );
    pTxCharacteristic->addDescriptor(new BLE2902());

    BLECharacteristic* pRxCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID_RX,
        BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR
    );
    pRxCharacteristic->setCallbacks(new L298NRxCallbacks());

    pService->start();

    // Bắt đầu quảng bá
    BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);
    pAdvertising->setMinPreferred(0x12);
    BLEDevice::startAdvertising();

    Serial.printf(">> BLE L298N san sang! Ten thiet bi: %s\n", DEVICE_NAME);
}

void loop() {
    // Cập nhật non-blocking cho Động cơ 2 (đếm 2s thuận -> 2s ngược)
    motorsL298N.update();

    // Tự động quảng bá lại khi mất kết nối
    if (!deviceConnected && oldDeviceConnected) {
        delay(500);
        pServer->startAdvertising();
        Serial.println(">> [BLE] Dang quang ba lai...");
        oldDeviceConnected = deviceConnected;
    }
    if (deviceConnected && !oldDeviceConnected) {
        oldDeviceConnected = deviceConnected;
    }

    delay(5);
}
