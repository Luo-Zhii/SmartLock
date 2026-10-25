# 🔐 SmartLock – Khóa cửa thông minh nhập PIN trên Web (ESP32)

Đề tài IoT: **Khóa cửa thông minh kết hợp nhập mật khẩu (PIN Code) trên Web**, trọng tâm là *bảo mật & xử lý chuỗi*.

ESP32 tự phát WiFi và chạy một Web Server gồm **màn hình trạng thái** và **bàn phím số**. Người dùng kết nối điện thoại vào WiFi đó, nhập đúng PIN thì ESP32 kích relay để mở khóa và LED xanh sáng. Nhập sai thì LED đỏ nháy. Sai nhiều lần thì hệ thống báo động.

**Lắp không cần breadboard, không cần màn hình OLED, không cần còi.** Phần bắt buộc chỉ gồm ESP32, Relay 2 kênh, LED xanh, LED đỏ và 8 dây F-F, không phải cắt dây nào.

> 📘 **Hướng dẫn đầy đủ** gồm hình vẽ cách nối LED và nút, sơ đồ chân ESP32, cắm dây từng bước có kiểm tra, giải thích code, bảo mật, kiểm thử và xử lý lỗi: tải file [`SmartLock_HuongDan.html`](SmartLock_HuongDan.html) về, mở bằng trình duyệt, nhấn **Ctrl+P** để lưu thành PDF.

---

## Mục lục

- [Tính năng](#-tính-năng)
- [Cấu trúc thư mục](#-cấu-trúc-thư-mục)
- [Linh kiện](#-linh-kiện)
- [Nối dây không breadboard](#-nối-dây-không-breadboard)
- [Cắm dây từng bước](#-cắm-dây-từng-bước)
- [Cài đặt & nạp code](#-cài-đặt--nạp-code)
- [Sử dụng](#-sử-dụng)
- [Cấu hình](#-cấu-hình)
- [Web API](#-web-api)
- [Bảo mật & xử lý chuỗi](#-bảo-mật--xử-lý-chuỗi)
- [Kiểm thử nhanh](#-kiểm-thử-nhanh)
- [Xử lý lỗi thường gặp](#-xử-lý-lỗi-thường-gặp)
- [Hạn chế & hướng phát triển](#-hạn-chế--hướng-phát-triển)

---

## ✨ Tính năng

| Tính năng | Mô tả |
|---|---|
| Bàn phím số trên Web | Nhập PIN trên điện thoại. Màn hình chỉ hiện dấu `●`, PIN được gửi bằng POST |
| Màn hình trên Web (thay OLED) | Màn hình ảo hiện LOCKED / OPEN / WAIT / ALARM, thông báo, trạng thái cửa và người. Trang `/screen` hiển thị toàn màn hình để gắn điện thoại hoặc tablet cũ ở cửa |
| Mở / khóa bằng relay | Relay kênh 1 là chốt khóa. Tự khóa lại sau 5 giây (nếu có cảm biến cửa thì chờ đến khi cửa đóng) |
| LED thay còi | LED xanh nháy khi PIN đúng, LED đỏ nháy khi sai, LED đỏ nháy nhanh khi báo động |
| Chống dò mật khẩu | Sai 3 lần thì tạm khóa 30 giây và báo động 15 giây (relay kênh 2 bật) |
| Đổi PIN | Phải nhập đúng PIN cũ. PIN lưu trong flash nên mất điện không mất |
| Nhật ký | 8 sự kiện gần nhất hiển thị trên web và in ra Serial |
| *(Tùy chọn)* Nút EXIT / LOCK | EXIT mở cửa từ trong nhà. LOCK khóa ngay và tắt báo động |
| *(Tùy chọn)* Cảm biến cửa | Module tránh vật cản (IR) phát hiện cửa bị mở khi đang khóa và báo động đột nhập |
| *(Tùy chọn)* Cảm biến người | PIR bật LED vàng và hiện "Mời nhập mã PIN..." khi có người đến gần |

## 📁 Cấu trúc thư mục

```
SmartLock/
├── SmartLock/
│   └── SmartLock.ino        # Mã nguồn Arduino cho ESP32 (không cần thư viện ngoài)
├── SmartLock_HuongDan.html  # Tài liệu hướng dẫn đầy đủ (xuất được PDF)
└── README.md
```

## 🧰 Linh kiện

| Linh kiện | Mức | SL | Vai trò |
|---|---|---|---|
| ESP32 DevKit V1 + cáp Micro USB | bắt buộc | 1 | Bộ xử lý, phát WiFi, chạy Web Server |
| Relay 2 kênh 5V | bắt buộc | 1 | CH1: chốt khóa · CH2: đầu ra báo động |
| LED đỏ + điện trở 220Ω | bắt buộc | 1 + 1 | Đang khóa, sai PIN, báo động |
| LED xanh + điện trở 220Ω | bắt buộc | 1 + 1 | Đã mở, PIN đúng |
| Dây Dupont F-F | bắt buộc | 8–10 | Nối dây |
| LED vàng + điện trở 220Ω | tùy chọn | 1 + 1 | Có người, đang tạm khóa |
| Nút nhấn | tùy chọn | 2 | EXIT, LOCK |
| Module tránh vật cản (IR) | tùy chọn | 1 | Cảm biến cửa |
| PIR HC-SR501 | tùy chọn | 1 | Phát hiện người |
| Dây Dupont F-M | tùy chọn | ≤ 10 | Cắt thành "dây 1 đầu cái" |
| Kéo, băng dính điện, bìa carton | dụng cụ | — | Cắt và tuốt dây, cách điện, gắn mô hình |

Không dùng: breadboard, OLED, còi, DHT11, quang trở, chiết áp.

## 🔌 Nối dây không breadboard

### Ba kỹ thuật cần biết

1. **GND phụ**: ESP32 chỉ có 2 chân GND, nên code đặt thêm một số GPIO ở mức `LOW` để làm GND riêng cho từng LED, nút nhấn và PIR. Cách này chỉ dùng cho tải dưới 20mA: LED khoảng 6mA, nút khoảng 0.07mA, PIR dưới 0.1mA. **Không** dùng cho relay hay cảm biến IR.
2. **LED + điện trở**: xoắn 1 chân điện trở 220Ω vào **chân dài (+)** của LED rồi quấn băng dính. Chân còn lại của điện trở cắm vào đầu cái dây F-F nối tới GPIO. Chân ngắn (−) của LED cắm vào dây F-F nối tới chân GND phụ.
3. **Dây 1 đầu cái** *(chỉ cần cho phần tùy chọn)*: cắt bỏ đầu đực của dây F-M, tuốt 1.5cm vỏ, quấn lõi đồng vào chân linh kiện rồi quấn băng dính. Đầu cái còn lại cắm vào chân ESP32. **Dây chữ Y** (xoắn chung 3 đoạn dây có đầu cái) dùng khi chân VIN phải cấp cho cả Relay và PIR.

### Sơ đồ chân (ESP32 DevKit V1 30 chân, mặt chip quay lên, USB quay xuống)

```
                                 ┌──────────────┐
                                 │ ESP-WROOM-32 │
                            EN   ┤              ├ D23  ◄ LED đỏ (+) qua 220Ω
                            VP   ┤              ├ D22  ◄ LED đỏ (−) [GND phụ]
                            VN   ┤              ├ TX0 
        IR OUT (tùy chọn) ► D34  ┤              ├ RX0 
       PIR OUT (tùy chọn) ► D35  ┤              ├ D21 
          Nút EXIT chân 1 ► D32  ┤              ├ D19  ◄ LED xanh (−) [GND phụ]
Nút EXIT chân 2 [GND phụ] ► D33  ┤              ├ D18  ◄ LED xanh (+) qua 220Ω
                            D25  ┤              ├ D5  
                Relay IN1 ► D26  ┤              ├ TX2  ◄ LED vàng (+) qua 220Ω (tùy chọn)
                Relay IN2 ► D27  ┤              ├ RX2  ◄ LED vàng (−) [GND phụ]
Nút LOCK chân 2 [GND phụ] ► D14  ┤              ├ D4   ◄ PIR GND [GND phụ]
                            D12  ┤              ├ D2  
          Nút LOCK chân 1 ► D13  ┤              ├ D15 
                Relay GND ► GND  ┤              ├ GND  ◄ IR GND
    Relay VCC (+ PIR VCC) ► VIN  ┤              ├ 3V3  ◄ IR VCC
                                 └────[USB]─────┘
```

> Board 38 chân có thứ tự chân khác. Luôn cắm theo **nhãn in trên board**. `TX2` là GPIO17, `RX2` là GPIO16.

### Bảng nối dây

| Linh kiện | Chân linh kiện | Chân ESP32 | Dây |
|---|---|---|---|
| **Relay** | VCC · GND · IN1 · IN2 | VIN · GND · D26 · D27 | 4 × F-F |
| **LED đỏ** | (+) qua 220Ω · (−) | D23 · D22 *(GND phụ)* | 2 × F-F |
| **LED xanh** | (+) qua 220Ω · (−) | D18 · D19 *(GND phụ)* | 2 × F-F |
| LED vàng | (+) qua 220Ω · (−) | TX2 · RX2 *(GND phụ)* | 2 × 1 đầu cái |
| Nút EXIT | 2 chân chéo nhau | D32 · D33 *(GND phụ)* | 2 × 1 đầu cái |
| Nút LOCK | 2 chân chéo nhau | D13 · D14 *(GND phụ)* | 2 × 1 đầu cái |
| Cảm biến IR | VCC · GND · OUT | 3V3 · GND · D34 | 3 × F-F |
| PIR | VCC · GND · OUT | VIN *(dây Y)* · D4 *(GND phụ)* · D35 | Y + 2 × F-F |

### Kế hoạch dây

| Phương án | Gồm | F-F | F-M (cắt) |
|---|---|---|---|
| **A. Cơ bản** (đủ đề bài) | Relay + LED đỏ + LED xanh | 8 | 0 |
| A+ | A + LED vàng | 10 | 0 |
| B | A + LED vàng + 2 nút + IR | 7 | 10 |
| C. Đầy đủ | B + PIR | 10 (gồm 2 dây cắt làm dây Y) | 10 |

Quy tắc: dây F-F để dành cho module có chân đực (Relay, IR, PIR), còn chân LED và chân nút dùng dây 1 đầu cái.

## 🪛 Cắm dây từng bước

**Nạp code trước khi cắm dây.** Mỗi bước làm theo thứ tự: rút USB → cắm dây → cắm USB → kiểm tra.

| Bước | Việc làm | Kiểm tra |
|---|---|---|
| 0 | Đặt ESP32 mặt chip lên, USB quay về phía mình. Nạp code, mở web | Web hiện `LOCKED` |
| 1 | **LED đỏ**: điện trở xoắn vào chân dài → D23, chân ngắn → D22 | LED đỏ sáng. Nhập sai PIN thì nháy 3 lần |
| 2 | **LED xanh**: điện trở xoắn vào chân dài → D18, chân ngắn → D19 | Nhập `1234` thì LED xanh nháy rồi sáng, sau 5 giây tắt |
| 3 | **Relay**: VCC → VIN, GND → GND, IN1 → D26, IN2 → D27 | Mở khóa thì relay 1 kêu "tách". Sai 3 lần thì relay 2 đóng 15 giây. **Đủ đề bài** 🎉 |
| 4 | *(tùy chọn)* **LED vàng**: D17 (+) / D16 (−) | Sáng trong lúc tạm khóa |
| 5 | *(tùy chọn)* **Nút**: EXIT → D32 / D33, LOCK → D13 / D14 (chân chéo) | EXIT mở khóa. LOCK khóa và tắt báo động |
| 6 | *(tùy chọn)* **IR**: VCC → 3V3, GND → GND, OUT → D34. Đặt `USE_DOOR_SENSOR = true` | Che cảm biến thì "Cửa: ĐÓNG". Bỏ tay khi đang khóa thì báo động |
| 7 | *(tùy chọn)* **PIR**: VCC → VIN (dây Y), GND → D4, OUT → D35. Đặt `USE_PIR = true` | Chờ 60 giây, vẫy tay thì LED vàng sáng, web hiện "Có người" |
| 8 | Dán lên bìa carton, quấn băng dính mọi mối nối | Không có mối nối trần nào chạm nhau |

> ⚠️ Mọi mối xoắn phải quấn băng dính. Cảm biến IR dùng **3V3**, không dùng 5V. Không cắm vào TX0/RX0/D12.

## 🛠 Cài đặt & nạp code

1. Cài **Arduino IDE 2.x**.
2. Vào *File → Preferences → Additional boards manager URLs* và thêm đường dẫn:
   ```
   https://espressif.github.io/arduino-esp32/package_esp32_index.json
   ```
3. Vào *Boards Manager*, cài **esp32** của **Espressif Systems**. **Không cần cài thư viện nào.**
4. Mở `SmartLock/SmartLock.ino`, chọn board **ESP32 Dev Module** và chọn đúng cổng.
5. Bấm **Upload**. Nếu báo lỗi `Failed to connect`, hãy giữ nút **BOOT** cho đến khi thấy chữ `Writing...`.
6. Mở Serial Monitor ở **115200 baud** rồi bấm **EN**.

Driver: Windows cần cài CP210x hoặc CH340. Linux chạy lệnh `sudo usermod -aG dialout $USER` rồi đăng nhập lại.

Code đã được biên dịch thử với esp32 core **3.3.12**, không có cảnh báo, chiếm khoảng 71% flash.

## 📱 Sử dụng

1. Trên điện thoại, kết nối WiFi **`SmartLock-ESP32`**, mật khẩu **`12345678`**. Nên tắt 4G.
2. Mở **http://192.168.4.1** để thấy màn hình ảo và bàn phím. Nhập PIN mặc định **`1234`**.
3. *(Tùy chọn)* Trên một điện thoại hoặc tablet thứ 2 gắn ở cửa, mở **http://192.168.4.1/screen** để làm màn hình hiển thị (thay OLED).

| Trạng thái | LED đỏ | LED xanh | LED vàng | Relay 1 | Relay 2 | Màn hình web |
|---|---|---|---|---|---|---|
| Đang khóa | Sáng | Tắt | Tắt | Ngắt | Ngắt | LOCKED |
| PIN đúng | Tắt | Nháy → sáng | — | Đóng | Ngắt | OPEN · Mời vào! |
| PIN sai | Nháy 3 lần | Tắt | — | Ngắt | Ngắt | Sai PIN! 1/3 |
| Tạm khóa | Nháy chậm | Tắt | Sáng | Ngắt | — | WAIT 25s |
| Báo động | **Nháy nhanh** | Tắt | — | Ngắt | **Đóng** | ! ALARM ! |

**Tắt báo động:** nhập đúng PIN, bấm nút LOCK, hoặc chờ báo động tự tắt sau 15 giây. Nút "Khóa ngay" trên web **không** tắt được báo động.

## ⚙️ Cấu hình

Các hằng số ở đầu file `SmartLock.ino`:

| Hằng số | Mặc định | Ý nghĩa |
|---|---|---|
| `AP_SSID` / `AP_PASS` | `SmartLock-ESP32` / `12345678` | Tên và mật khẩu WiFi do ESP32 phát |
| `DEFAULT_PIN` | `"1234"` | PIN dùng lần đầu |
| `USE_DOOR_SENSOR` | `false` | Đổi thành `true` sau khi lắp cảm biến cửa (IR) |
| `USE_PIR` | `false` | Đổi thành `true` sau khi lắp PIR |
| `RELAY_ACTIVE_LOW` | `true` | Đổi thành `false` nếu relay hoạt động ngược |
| `GND_PINS[]` | `22, 19, 16, 33, 14, 4` | Các chân làm GND phụ |
| `MAX_FAILS` | `3` | Số lần được nhập sai trước khi tạm khóa |
| `LOCKOUT_MS` | `30000` | Thời gian tạm khóa (ms) |
| `AUTO_LOCK_MS` | `5000` | Thời gian chờ trước khi tự khóa lại (ms) |
| `ALARM_MS` | `15000` | Thời gian báo động (ms) |
| `SCREEN_MSG_MS` | `4000` | Thời gian thông báo ngắn hiện trên màn hình web (ms) |

## 🌐 Web API

| Đường dẫn | Phương thức | Tham số | Chức năng |
|---|---|---|---|
| `/` | GET | — | Màn hình ảo + bàn phím |
| `/screen` | GET | — | Chỉ màn hình, toàn trang (thay OLED) |
| `/status` | GET | — | Trạng thái dạng JSON |
| `/unlock` | POST | `pin` | Kiểm tra PIN và mở khóa |
| `/lock` | POST | — | Khóa ngay (không tắt báo động) |
| `/changepin` | POST | `old`, `new` | Đổi PIN và lưu vào flash |

Ví dụ phản hồi của `/status`:

```json
{"locked":true,"lockout":0,"hasDoor":false,"door":false,"hasPir":false,"motion":false,
 "alarm":false,"msg":"Sai PIN! 1/3","log":["00:01:20 Sai PIN lần 1","00:00:00 Khóa cửa (khởi động)"]}
```

## 🛡 Bảo mật & xử lý chuỗi

| Biện pháp | Cách làm | Chống lại |
|---|---|---|
| Kiểm tra đầu vào | `isValidPin()`: chỉ nhận 4–8 ký tự, tất cả là chữ số | Dữ liệu rác, chuỗi quá dài |
| Chuẩn hóa chuỗi | `trim()` trước khi so sánh | Khoảng trắng thừa |
| So sánh thời gian cố định | `safeEquals()` duyệt hết chuỗi bằng XOR, không dừng sớm ở ký tự sai | Timing attack |
| Giới hạn số lần thử | Sai 3 lần thì tạm khóa 30 giây và báo động | Brute-force |
| Gửi PIN bằng POST | PIN nằm trong body, không nằm trên URL | Lộ PIN qua lịch sử trình duyệt |
| Kiểm tra ở server | Chỉ ESP32 biết PIN đúng, `/status` không trả về PIN | Đọc mã nguồn trang web để lấy PIN |
| Đổi PIN cần PIN cũ | `/changepin` kiểm tra PIN cũ, lần thất bại được ghi nhật ký | Người lạ tự ý đổi PIN |
| Báo động đột nhập | Cửa mở khi đang khóa thì báo động *(IR)* | Phá cửa |
| Phân quyền tắt báo động | Chỉ PIN đúng hoặc nút LOCK trong nhà tắt được | Kẻ gian vào được WiFi rồi tắt báo động |
| Tự khóa lại | Khóa sau 5 giây (khi cửa đã đóng) | Quên khóa cửa |

Ước tính: dò hết PIN 4 số mất khoảng **27,8 giờ**, PIN 6 số mất khoảng **115 ngày**.

## ✅ Kiểm thử nhanh

| # | Thao tác | Kết quả mong đợi |
|---|---|---|
| 1 | Nhập `1234` | Relay 1 kêu "tách", LED xanh nháy rồi sáng, web hiện OPEN |
| 2 | Chờ 5 giây | Tự khóa, LED đỏ sáng |
| 3 | Nhập sai 3 lần | Tạm khóa 30 giây, LED đỏ nháy nhanh, relay 2 đóng, web hiện ! ALARM ! |
| 4 | Đang báo động, bấm "Khóa ngay" trên web | Báo động vẫn tiếp tục |
| 5 | Mở `/screen` trên máy thứ 2, mở khóa trên máy 1 | Máy 2 đổi sang OPEN trong vòng 1 giây |
| 6 | Đổi PIN thành `5678`, rút điện rồi cắm lại | `5678` mở được, `1234` bị từ chối |
| 7 | Nhập `12` hoặc đổi PIN mới thành `abcd` | Báo PIN không hợp lệ |

Bản đầy đủ 15 kịch bản kiểm thử nằm trong file hướng dẫn HTML.

## 🧯 Xử lý lỗi thường gặp

| Hiện tượng | Cách sửa |
|---|---|
| Máy tính không nhận cổng COM / ttyUSB | Dùng cáp USB có truyền dữ liệu, cài driver CP210x/CH340 |
| LED không sáng | Kiểm tra chiều LED (chân dài nối phía điện trở), xoắn chặt mối nối, gập đôi chân linh kiện trước khi cắm, nạp lại code mới |
| LED chập chờn khi chạm vào dây | Mối nối lỏng: xoắn lại, quấn băng dính, cố định dây |
| Relay hoạt động ngược | `RELAY_ACTIVE_LOW = false` |
| Relay không nhả | Rút jumper JD-VCC, nối JD-VCC với 5V và VCC với 3V3 |
| Báo động liên tục | Đã bật `USE_DOOR_SENSOR` khi chưa lắp hoặc chưa chỉnh cảm biến IR |
| Web báo "Có người" liên tục | Chờ 60 giây cho PIR ổn định, vặn Tx hết ngược chiều kim đồng hồ, kiểm tra cờ `USE_PIR` |
| Bấm nút không có tác dụng | Dùng 2 chân **chéo nhau** của nút |
| Web hiện "MẤT KẾT NỐI" | Tắt 4G, kết nối lại WiFi `SmartLock-ESP32`, kiểm tra nguồn ESP32 |

## 🚧 Hạn chế & hướng phát triển

**Hạn chế**
- Dùng HTTP nên dữ liệu không được mã hóa.
- PIN được lưu dạng chuỗi gốc trong flash.
- Relay chỉ mô phỏng chốt khóa. Báo động chỉ có đèn.
- Mối nối xoắn tay kém bền hơn so với breadboard hoặc hàn.

**Hướng phát triển**
- Chuyển sang breadboard hoặc hàn trên board đục lỗ.
- Gắn còi hoặc đèn chớp 5V vào relay kênh 2 để có âm thanh, không cần sửa code.
- Băm PIN bằng SHA-256 có salt (`mbedtls`). Mã hóa đường truyền bằng HTTPS hoặc dùng MQTT qua TLS.
- Chế độ STA để ESP32 vào WiFi nhà. Gửi thông báo Telegram khi có báo động. Mã OTP cho khách.
- Nối relay CH1 với khóa chốt điện 12V (dùng nguồn riêng, mắc thêm diode 1N4007).
