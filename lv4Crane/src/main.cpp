#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#include "config.h"
#include "motor_driver.h"

// Biến quản lý trạng thái BLE
static BLEServer* pServer = nullptr;
static BLECharacteristic* pTxCharacteristic = nullptr;
static bool deviceConnected = false;
static bool oldDeviceConnected = false;

// Gửi phản hồi chuỗi ký tự về App Adafruit Bluefruit LE Connect qua BLE TX
void sendBleResponse(const char* text) {
    if (deviceConnected && pTxCharacteristic != nullptr) {
        String msg = String(text) + "\r\n";
        pTxCharacteristic->setValue((uint8_t*)msg.c_str(), msg.length());
        pTxCharacteristic->notify();
    }
}

// Xử lý dữ liệu nhận được từ ứng dụng Bluefruit
void handleRxData(const uint8_t* data, size_t length) {
    if (length == 0) return;

    // In thô ra Serial để debug
    Serial.printf("[BLE RX] Len=%d, Data: ", length);
    for (size_t i = 0; i < length; i++) {
        if (isprint(data[i])) Serial.print((char)data[i]);
        else Serial.printf("<0x%02X>", data[i]);
    }
    Serial.println();

    // 1. Xử lý gói phím bấm Control Pad của Bluefruit (Bắt đầu bằng '!B')
    // Định dạng: '!B' + Button_Num + State_Press ('1'=nhấn, '0'=nhả) + Checksum
    if (length >= 4 && data[0] == '!' && data[1] == 'B') {
        char button = (char)data[2];
        char state  = (char)data[3];

        switch (button) {
            case '5': // Mũi tên LÊN (Up arrow) -> Động cơ 1 Tiến (Hold-to-run)
                if (state == '1') {
                    motors.motor1Forward();
                } else {
                    motors.motor1Stop();
                }
                break;

            case '6': // Mũi tên XUỐNG (Down arrow) -> Động cơ 1 Lùi (Hold-to-run)
                if (state == '1') {
                    motors.motor1Backward();
                } else {
                    motors.motor1Stop();
                }
                break;

            case '7': // Mũi tên TRÁI (Left arrow) -> Động cơ 2 Hạ tời / Quay trái thủ công
                if (state == '1') {
                    motors.motor2Backward();
                } else {
                    motors.motor2Stop();
                }
                break;

            case '8': // Mũi tên PHẢI (Right arrow) -> Động cơ 2 Kéo tời / Quay phải thủ công
                if (state == '1') {
                    motors.motor2Forward();
                } else {
                    motors.motor2Stop();
                }
                break;

            case '1': // NÚT SỐ 1: Nhấn giữ quay trái -> Thả ra chờ 1s -> Kéo lên đúng thời gian đã nhấn
                if (state == '1') {
                    motors.btn1Pressed();
                } else {
                    motors.btn1Released();
                }
                break;

            case '2': // Nút số 2 (Button 2) -> Phanh dừng khẩn cấp toàn bộ động cơ
                if (state == '1') {
                    motors.stopAll();
                }
                break;

            case '3': // Nút số 3
            case '4': // Nút số 4
                if (state == '1') {
                    Serial.printf("Nut %c duoc nhan (Du phong)\n", button);
                    sendBleResponse("Nut chuc nang du phong");
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
    } else if (cmd.equalsIgnoreCase("1") || cmd.equalsIgnoreCase("TEST1") || cmd.equalsIgnoreCase("2") || cmd.equalsIgnoreCase("M2")) {
        // Mô phỏng chu trình Nút 1 với 2000ms
        motors.motor2StartCycle(2000);
    } else if (cmd.equalsIgnoreCase("UP") || cmd.equalsIgnoreCase("TOI_LEN") || cmd.equalsIgnoreCase("PHAI")) {
        motors.motor2Forward();
    } else if (cmd.equalsIgnoreCase("DOWN") || cmd.equalsIgnoreCase("TOI_XUONG") || cmd.equalsIgnoreCase("TRAI")) {
        motors.motor2Backward();
    } else if (cmd.equalsIgnoreCase("S2") || cmd.equalsIgnoreCase("DUNG2")) {
        motors.motor2Stop();
    } else if (cmd.equalsIgnoreCase("X") || cmd.equalsIgnoreCase("STOP")) {
        motors.stopAll();
    } else if (cmd.length() > 0) {
        Serial.printf("Lenh text chua ho tro: %s\n", cmd.c_str());
        sendBleResponse("Lenh hop le: F, B, S, 1 (Test Nut 1), UP, DOWN, S2, X (Dung toan bo)");
    }
}

// Callback sự kiện kết nối BLE Server
class MyServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer* server) override {
        deviceConnected = true;
        Serial.println(">> [BLE] Thiet bi da ket noi thanh cong!");
    }

    void onDisconnect(BLEServer* server) override {
        deviceConnected = false;
        Serial.println(">> [BLE] Mat ket noi! Dung toan bo dong co.");
        motors.stopAll();
    }
};

// Callback nhận dữ liệu ghi vào Characteristic RX
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

    Serial.println("\n=======================================================");
    Serial.println("   CYBERBRICKED - LV4 CRANE BLE CONTROLLER");
    Serial.println("   Hardware: ESP32-C3 + DRV8833 Dual H-Bridge");
    Serial.println("   Control & Logic: Adafruit Bluefruit NUS");
    Serial.println("   - M1 (Motor A): Banh xe di chuyen (Tien/Lui)");
    Serial.println("   - Nut 1 (Button 1):");
    Serial.println("       + Nhan giu: Quay trai & dem thoi gian");
    Serial.println("       + Tha ra: Dung, cho 1s");
    Serial.println("       + Sau 1s: Tu dong keo len dung thoi gian nhan");
    Serial.println("=======================================================\n");

    // 1. Khởi tạo phần cứng động cơ
    motors.begin();
    motors.setCallback(sendBleResponse);

    // 2. Khởi tạo BLE Device & Server
    BLEDevice::init(DEVICE_NAME);
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());

    // 3. Khởi tạo Nordic UART Service (NUS)
    BLEService* pService = pServer->createService(SERVICE_UUID);

    // Characteristic TX (ESP32 gửi trạng thái lên App điện thoại)
    pTxCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID_TX,
        BLECharacteristic::PROPERTY_NOTIFY
    );
    pTxCharacteristic->addDescriptor(new BLE2902());

    // Characteristic RX (App điện thoại gửi lệnh xuống ESP32)
    BLECharacteristic* pRxCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID_RX,
        BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR
    );
    pRxCharacteristic->setCallbacks(new MyRxCallbacks());

    pService->start();

    // 4. Bắt đầu quảng bá BLE (Advertising)
    BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);
    pAdvertising->setMinPreferred(0x12);
    BLEDevice::startAdvertising();

    Serial.printf(">> BLE san sang! Ten thiet bi quang ba: %s\n", DEVICE_NAME);
    Serial.println(">> Mo ung dung 'Adafruit Bluefruit LE Connect' de ket noi.\n");
}

void loop() {
    // Luôn cập nhật máy trạng thái non-blocking của động cơ (xử lý Nút 1 và đếm thời gian)
    motors.update();

    // Tự động kích hoạt lại Advertising khi bị ngắt kết nối
    if (!deviceConnected && oldDeviceConnected) {
        delay(500); // Đợi BLE stack ổn định
        pServer->startAdvertising();
        Serial.println(">> [BLE] Bat dau quang ba lai (Advertising)...");
        oldDeviceConnected = deviceConnected;
    }
    if (deviceConnected && !oldDeviceConnected) {
        oldDeviceConnected = deviceConnected;
    }

    // Delay 5ms nhường CPU cho BLE stack nền
    delay(5);
}
