# lv4Crane - Điều Khiển Cần Cẩu CyberBricked Qua BLE (Logic std_ble_2m)

Dự án firmware dành cho robot cần cẩu **lv4Crane** tích hợp phần cứng của hệ sinh thái **CyberBricked** (vi điều khiển **ESP32-C3-DevKitM-1** kết hợp mạch công suất cầu H đôi **DRV8833**) nhưng sử dụng chuẩn giao thức điều khiển và logic máy trạng thái non-blocking từ dự án **std_ble_2m** qua ứng dụng **Adafruit Bluefruit LE Connect**.

---

## 🚀 Tính Năng Chính

1. **Phần cứng chuẩn CyberBricked (KiCad Shield):**
   - **Chip:** ESP32-C3 SuperMini / DevKitM-1 (kiến trúc RISC-V, BLE 5.0, Native USB CDC).
   - **Motor Driver:** DRV8833 Dual H-Bridge với chân kích hoạt `MOTOR_ENABLE_PIN = GPIO 21` (`~SLEEP`).
   - **Xử lý chân BOOT:** Chân `MOTOR_B_IN1` (GPIO 9) được thiết lập reset ngoại vi và dùng cơ chế pull-up an toàn, bảo đảm tương thích 100% với Shield CyberBricked.
   - **Hỗ trợ đầy đủ cổng Servo:** Tích hợp định nghĩa chân chuẩn cho 4 cổng servo (`GPIO 6`, `GPIO 8`, `GPIO 4`, `GPIO 0`), còi Buzzer (`GPIO 7`) và LED RGB WS2812 (`GPIO 2`, `GPIO 10`).
2. **Logic điều khiển chuẩn dự án `std_ble_2m` & Tính năng thông minh:**
   - **Động cơ 1 (Motor A - Bánh xe di chuyển / Trục chính):**
     - Phím **Mũi tên Lên (Up)**: Nhấn giữ xe chạy **Tiến**, nhả tay xe tự **Dừng** (Hold-to-run).
     - Phím **Mũi tên Xuống (Down)**: Nhấn giữ xe chạy **Lùi**, nhả tay xe tự **Dừng** (Hold-to-run).
   - **Động cơ 2 (Motor B - Tời cẩu / Cơ cấu tác vụ):**
     - **Tính năng Button 1 (Nhấn giữ quay trái $\rightarrow$ Sau 1s tự động kéo lên đúng thời gian đã nhấn):**
       - **Khi nhấn giữ Button 1:** Động cơ 2 quay trái (nghịch), đồng thời bộ đếm thời gian `millis()` ghi nhận thời gian nhấn liên tục.
       - **Khi nhả Button 1:** Động cơ 2 dừng ngay lập tức; hệ thống tạm nghỉ trong **1.0 giây** (1000ms).
       - **Sau 1.0 giây:** Động cơ 2 tự động quay phải / kéo lên (thuận) với thời gian chạy **chính xác bằng thời gian đã nhấn giữ**, sau đó tự dừng hẳn.
       - **An toàn & Lọc nhiễu:** Bỏ qua thao tác nhấp nhả ngẫu nhiên (< 50ms) và tự động ngắt nếu giữ quá 60s để phòng trường hợp rớt gói tin BLE.
     - **Chế độ thủ công (Mũi tên Trái / Phải hoặc lệnh `UP` / `DOWN` / `S2`):** Nhấn giữ để kéo hoặc hạ tời cẩu chủ động.
     - **Chu trình mẫu tự động 2s (Lệnh UART `2` hoặc `M2`):** Quay thuận 2s $\rightarrow$ Quay ngược 2s $\rightarrow$ Tự dừng.
     - **Thuật toán Non-blocking 100%:** Toàn bộ chu trình sử dụng máy trạng thái với `millis()`, hoàn toàn không dùng hàm `delay()`. Do đó Động cơ 1 vẫn tiến/lùi mượt mà trong khi Động cơ 2 đang thực thi chu trình tự động.
   - **Cơ chế an toàn (Fail-safe):** Khi ngắt kết nối BLE hoặc điện thoại ra khỏi vùng phát sóng, hệ thống lập tức ngắt toàn bộ động cơ (`stopAll()`) và tự động bật lại chế độ quảng bá (Advertising) để chờ kết nối lại.
   - **Phản hồi thời gian thực (BLE TX Feedback):** Mọi thao tác đều được gửi thông báo trạng thái tức thời về điện thoại qua BLE Notify.

---

## 📊 Biểu Đồ Hệ Thống & Lưu Đồ Hoạt Động (Mermaid Diagrams)

### 1. Kiến Trúc Tổng Quan (System Architecture)

```mermaid
flowchart TD
    subgraph Client ["📱 ỨNG DỤNG ĐIỀU KHIỂN (ADAFRUIT BLUEFRUIT APP)"]
        PAD["Giao Diện Controller Pad (!B Buttons)"]
        UART["Giao Diện UART Terminal (Text Commands)"]
    end

    subgraph BLE_Stack ["📶 KẾT NỐI BLUETOOTH LOW ENERGY (NUS)"]
        RX["Characteristic RX (Gửi lệnh xuống ESP32)"]
        TX["Characteristic TX (Gửi phản hồi lên điện thoại)"]
    end

    subgraph Firmware ["💻 FIRMWARE ESP32-C3 (LV4CRANE)"]
        DISPATCHER["Bộ Phân Tích Gói Tin (handleRxData)"]
        CONTROLLER["Bộ Điều Khiển Máy Trạng Thái (MotorController)"]
        PWM_DRIVER["Trình Điều Khiển Xung (LEDC PWM Driver)"]
    end

    subgraph Hardware ["⚙️ MẠCH CÔNG SUẤT VÀ ĐỘNG CƠ (CYBERBRICKED SHIELD)"]
        DRV["IC Cầu H Đôi DRV8833"]
        M1["Động cơ 1: Di Chuyển (Tiến / Lùi)"]
        M2["Động cơ 2: Tời Cẩu (Quay Trái / Chờ 1s / Kéo Lên)"]
    end

    PAD & UART ==>|Bluetooth BLE 5.0| RX
    RX --> DISPATCHER
    DISPATCHER --> CONTROLLER
    CONTROLLER --> PWM_DRIVER
    PWM_DRIVER --> DRV
    DRV ==> M1
    DRV ==> M2

    CONTROLLER -.->|Thông báo trạng thái| TX
    TX -.->|BLE Notify| Client
```

---

### 2. Máy Trạng Thái Non-Blocking Động Cơ 2 (State Machine Diagram)

```mermaid
stateDiagram-v2
    [*] --> CHỜ_LỆNH: Khởi động hệ thống
    
    CHỜ_LỆNH --> NHẤN_GIỮ_QUAY_TRÁI: Nhấn Button 1 (!B11)
    CHỜ_LỆNH --> CHU_TRÌNH_CŨ_2S: Gửi lệnh '2' hoặc 'M2' qua UART
    CHỜ_LỆNH --> THỦ_CÔNG_THUẬN: Nhấn giữ Mũi tên Phải (Right) / gửi 'UP'
    CHỜ_LỆNH --> THỦ_CÔNG_NGHỊCH: Nhấn giữ Mũi tên Trái (Left) / gửi 'DOWN'
    
    state NHẤN_GIỮ_QUAY_TRÁI {
        [*] --> Quay_Trai: Động cơ 2 quay trái (nghịch)
        Quay_Trai --> Dem_Thoi_Gian: Đếm liên tục thời gian nhấn (millis)
    }
    
    NHẤN_GIỮ_QUAY_TRÁI --> CHỜ_1_GIÂY: Thả tay khỏi Button 1 (!B10)
    
    state CHỜ_1_GIÂY {
        [*] --> Dung_Dong_Co_2: Dừng M2 ngay lập tức
        Dung_Dong_Co_2 --> Dem_1000ms: Chờ đủ 1000ms (Non-blocking)
    }
    
    CHỜ_1_GIÂY --> TỰ_ĐỘNG_KÉO_LÊN: Hết 1.0 giây chờ
    
    state TỰ_ĐỘNG_KÉO_LÊN {
        [*] --> Quay_Thuan_Keo_Len: Động cơ 2 quay thuận kéo lên
        Quay_Thuan_Keo_Len --> Chay_Dung_Thoi_Gian: Chạy đúng bằng thời gian đã nhấn
    }
    
    TỰ_ĐỘNG_KÉO_LÊN --> CHỜ_LỆNH: Hết thời gian kéo lên -> Tự dừng

    state CHU_TRÌNH_CŨ_2S {
        [*] --> Thuan_2s: Thuận 2s
        Thuan_2s --> Nghich_2s: Nghịch 2s
        Nghich_2s --> [*]
    }
    CHU_TRÌNH_CŨ_2S --> CHỜ_LỆNH: Hoàn thành

    THỦ_CÔNG_THUẬN --> CHỜ_LỆNH: Nhả tay khỏi nút / gửi 'S2'
    THỦ_CÔNG_NGHỊCH --> CHỜ_LỆNH: Nhả tay khỏi nút / gửi 'S2'

    state Phanh_Khan_Cap {
        [*] --> Dừng_Toàn_Bộ
    }
    CHỜ_LỆNH --> Phanh_Khan_Cap: Nhấn Button 2 hoặc gửi 'X' / Mất kết nối BLE
    NHẤN_GIỮ_QUAY_TRÁI --> Phanh_Khan_Cap: Nhấn Button 2 hoặc gửi 'X' / Mất kết nối BLE
    CHỜ_1_GIÂY --> Phanh_Khan_Cap: Nhấn Button 2 hoặc gửi 'X' / Mất kết nối BLE
    TỰ_ĐỘNG_KÉO_LÊN --> Phanh_Khan_Cap: Nhấn Button 2 hoặc gửi 'X' / Mất kết nối BLE
    Phanh_Khan_Cap --> CHỜ_LỆNH: Đã ngắt điện an toàn
```

---

### 3. Trình Tự Giao Tiếp BLE - Nút 1 (Sequence Diagram)

```mermaid
sequenceDiagram
    autonumber
    actor User as Người Dùng (Điện Thoại)
    participant App as Adafruit Bluefruit App
    participant BLE as ESP32-C3 BLE Stack
    participant Core as Firmware (lv4Crane)
    participant Driver as DRV8833 & Motors

    User->>App: Mở app & Quét tìm Bluetooth
    App->>BLE: Kết nối thiết bị "CyberBricked-lv4Crane"
    BLE->>Core: Gọi onConnect()
    Core-->>App: Kết nối thành công (Notify TX)

    rect rgb(235, 245, 255)
        note right of User: Điều khiển Động cơ 1 di chuyển (Hold-to-run)
        User->>App: Nhấn giữ Mũi tên Lên (Up)
        App->>BLE: Gửi gói tin '!B51' (Button 5 pressed)
        BLE->>Core: handleRxData("!B51")
        Core->>Driver: motor1Forward(255)
        Driver-->>Core: Động cơ 1 chạy Tiến
        Core-->>App: Thông báo "[M1] Tien (Forward)"

        User->>App: Nhả tay khỏi Mũi tên Lên
        App->>BLE: Gửi gói tin '!B50' (Button 5 released)
        BLE->>Core: handleRxData("!B50")
        Core->>Driver: motor1Stop()
        Driver-->>Core: Động cơ 1 Dừng
        Core-->>App: Thông báo "[M1] Dung (Stop)"
    end

    rect rgb(255, 245, 235)
        note right of User: Chu trình Button 1: Giữ quay trái -> Sau 1s tự kéo lên
        User->>App: Nhấn giữ Button 1
        App->>BLE: Gửi gói tin '!B11' (Button 1 pressed)
        BLE->>Core: handleRxData("!B11") -> motors.btn1Pressed()
        Core->>Driver: Quay trái M2 & Bắt đầu tính thời gian T_hold
        Driver-->>Core: Động cơ 2 quay trái
        Core-->>App: "[M2] Nut 1: Dang quay trai & dem thoi gian..."

        loop Trong lúc nhấn giữ (T_hold ms)
            Core->>Core: motors.update() đếm T_hold = millis() - T_start
            note over Core,Driver: Động cơ 1 vẫn có thể tiến/lùi tự do
        end

        User->>App: Thả tay khỏi Button 1
        App->>BLE: Gửi gói tin '!B10' (Button 1 released)
        BLE->>Core: handleRxData("!B10") -> motors.btn1Released()
        Core->>Driver: Dừng Động cơ 2 & Bắt đầu đếm 1s chờ
        Driver-->>Core: Động cơ 2 dừng
        Core-->>App: "[M2] Nut 1: Da tha sau X ms. Cho 1s..."

        loop Trong 1.0 giây chờ (1000ms)
            Core->>Core: motors.update() kiểm tra thời gian chờ
        end

        Core->>Driver: Tự động đảo chiều kéo lên M2 trong X ms
        Driver-->>Core: Động cơ 2 quay thuận kéo lên
        Core-->>App: "[M2] Nut 1: Tu dong keo len trong X ms..."

        loop Chạy đúng X ms kéo lên
            Core->>Core: motors.update() đếm thời gian kéo lên
        end

        Core->>Driver: motor2Stop()
        Driver-->>Core: Động cơ 2 dừng hẳn
        Core-->>App: "[M2] Nut 1: Hoan tat chu trinh tu dong keo len."
    end

    rect rgb(255, 235, 235)
        note right of User: Cơ chế an toàn (Fail-safe)
        User->>App: Mất sóng / Điện thoại tắt Bluetooth
        BLE->>Core: Gọi onDisconnect()
        Core->>Driver: stopAll() (Ngắt toàn bộ động cơ)
        Core->>BLE: Tự động kích hoạt lại Advertising
    end
```

---

## 🔌 Bảng Đấu Nối Phần Cứng Chuẩn KiCad (`Shield.kicad_sch`)

Chi tiết mô hình mạch điện xem tại: [CIRCUIT_DIAGRAM.md](file:///d:/CyberBricked/lv4Crane/CIRCUIT_DIAGRAM.md)

| Chân ESP32-C3 | Nhãn Net KiCad | Chân IC / Đầu Nối | Chức Năng Trên Bo Mạch | Ghi Chú Kỹ Thuật |
| :--- | :--- | :--- | :--- | :--- |
| **GPIO 21** | `EN` | Pin 1 (`~SLEEP`) của DRV8833 | Bo Shield | **Kích hoạt công suất:** Kéo mức `HIGH` để đánh thức driver DRV8833. |
| **GPIO 3** | `BIN2` | Pin 10 (`BIN2`) của DRV8833 | Giắc Motor J3 | **Động cơ 1 - Chiều tiến:** Xung PWM LEDC Kênh 0 (1kHz, 8-bit). |
| **GPIO 1** | `BIN1` | Pin 9 (`BIN1`) của DRV8833 | Giắc Motor J3 | **Động cơ 1 - Chiều lùi:** Xung PWM LEDC Kênh 1 (1kHz, 8-bit). |
| **GPIO 9** | `AIN2` | Pin 15 (`AIN2`) của DRV8833 | Giắc Motor J4 | **Động cơ 2 - Chiều thuận:** Chân BOOT trên ESP32-C3; kéo pull-up an toàn. |
| **GPIO 20** | `AIN1` | Pin 16 (`AIN1`) của DRV8833 | Giắc Motor J4 | **Động cơ 2 - Chiều nghịch:** Xung PWM LEDC Kênh 2 (1kHz, 8-bit). |
| **GPIO 4** | `SER3` | Chân 5 Giắc J10 | Giắc Servo J10 | **Cơ cấu móc cẩu:** Tín hiệu điều khiển Servo 50Hz (Chân 5 Giắc J10). |
| **GPIO 0** | `SER4` | Chân 6 Giắc J10 | Giắc Servo J10 | **Servo 4 mở rộng:** Tín hiệu Servo 4. |
| **GPIO 6** | `SER1` | Chân 5 Giắc J9 | Giắc Servo J9 | **Servo 1 mở rộng:** Tín hiệu Servo 1. |
| **GPIO 8** | `SER2` | Chân 6 Giắc J9 | Giắc Servo J9 | **Servo 2 mở rộng:** Tín hiệu Servo 2. |
| **GPIO 7** | `Buzz` | Chân 1 Giắc J11 | Giắc Còi J11 | **Còi Buzzer:** Âm thanh bíp / còi xe. |
| **GPIO 2** | `RGB1` | Chân 1 Giắc J7 | Giắc LED J7 | **Đèn LED RGB 1:** Chuẩn tín hiệu WS2812. |
| **GPIO 10** | `RGB2` | Chân 1 Giắc J5 | Giắc LED J5 | **Đèn LED RGB 2:** Chuẩn tín hiệu WS2812. |
| **GPIO 18 / 19** | `D-` / `D+` | Chân D- / D+ Cổng Type-C | Cổng Type-C | Giao tiếp nạp code và USB Serial CDC. |
| **GPIO 5** | — | Chân tự do | Bo Shield | Dự phòng (`SPARE`). |

---

## 📱 Hướng Dẫn Điều Khiển Trên App Adafruit Bluefruit LE Connect

1. Tải ứng dụng **Adafruit Bluefruit LE Connect** trên [App Store (iOS)](https://apps.apple.com/app/adafruit-bluefruit-le-connect/id830125974) hoặc [Google Play (Android)](https://play.google.com/store/apps/details?id=com.adafruit.bluefruit.le.connect).
2. Bật Bluetooth trên điện thoại, mở app và kết nối với thiết bị: **`CyberBricked-lv4Crane`**.
3. **Cách 1: Giao diện Controller -> Control Pad (Phím bấm):**
   - ⬆️ **Mũi tên Lên (Up):** Nhấn giữ Động cơ 1 **Tiến**, nhả tay xe tự **Dừng**.
   - ⬇️ **Mũi tên Xuống (Down):** Nhấn giữ Động cơ 1 **Lùi**, nhả tay xe tự **Dừng**.
   - ⬅️ **Mũi tên Trái (Left):** Nhấn giữ Động cơ 2 **Hạ tời cẩu**, nhả tay tự **Dừng**.
   - ➡️ **Mũi tên Phải (Right):** Nhấn giữ Động cơ 2 **Kéo tời cẩu**, nhả tay tự **Dừng**.
   - 🔘 **Button 1:** Nhấn giữ để quay trái và đếm thời gian; khi thả ra, sau 1 giây sẽ tự động kéo lên đúng thời gian đã nhấn giữ.
   - 🔘 **Button 2:** Phanh dừng khẩn cấp toàn bộ động cơ (`stopAll`).
4. **Cách 2: Giao diện UART Terminal (Dòng lệnh):**
   - Gửi `F` hoặc `TIEN`: Động cơ 1 Tiến.
   - Gửi `B` hoặc `LUI`: Động cơ 1 Lùi.
   - Gửi `S` hoặc `DUNG`: Động cơ 1 Dừng.
   - Gửi `2` hoặc `M2`: Chạy chu trình tự động Động cơ 2 (Thuận 2s -> Nghịch 2s).
   - Gửi `UP` hoặc `TOI_LEN`: Động cơ 2 Kéo tời.
   - Gửi `DOWN` hoặc `TOI_XUONG`: Động cơ 2 Hạ tời.
   - Gửi `S2`: Động cơ 2 Dừng.
   - Gửi `X` hoặc `STOP`: Dừng khẩn cấp toàn bộ hệ thống.

---

## 🛠️ Biên Dịch & Nạp Code Với PlatformIO

```powershell
# 1. Biên dịch mã nguồn:
pio run -d d:\CyberBricked\lv4Crane

# 2. Cắm cáp USB-C và nạp code vào ESP32-C3:
pio run -d d:\CyberBricked\lv4Crane -t upload

# 3. Mở Serial Monitor theo dõi log hệ thống (115200 baud):
pio device monitor
```
