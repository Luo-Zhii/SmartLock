# 🔐 SmartLock – Khóa cửa thông minh nhập PIN trên Web (ESP32)

Đề tài IoT: **Khóa cửa thông minh kết hợp nhập mật khẩu (PIN Code) trên Web**, trọng tâm là *bảo mật & xử lý chuỗi*.

ESP32 tự phát WiFi và chạy một Web Server có giao diện bàn phím số. Người dùng kết nối điện thoại vào WiFi đó, nhập đúng PIN thì ESP32 kích relay để mở khóa. Nhập sai nhiều lần hoặc cửa bị mở khi đang khóa thì hệ thống báo động.

Dự án chỉ dùng linh kiện có trong **bộ kit ESP32 Starter**, không cần mua thêm.

> 📘 **Hướng dẫn đầy đủ** gồm sơ đồ nối dây có hình, lắp ráp từng bước, giải thích code, bảo mật, kiểm thử và xử lý lỗi: tải file [`SmartLock_HuongDan.html`](SmartLock_HuongDan.html) về, mở bằng trình duyệt, nhấn **Ctrl+P** để lưu thành PDF.

---

## Mục lục

- [Tính năng](#-tính-năng)
- [Cấu trúc thư mục](#-cấu-trúc-thư-mục)
- [Linh kiện](#-linh-kiện)
- [Sơ đồ nối dây](#-sơ-đồ-nối-dây)
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
| Mở / khóa bằng relay | Relay kênh 1 đóng vai trò chốt khóa điện. Mở xong tự khóa lại sau 5 giây, nhưng chỉ khi cửa đã đóng |
| Chống dò mật khẩu | Sai 3 lần thì tạm khóa 30 giây và bật còi báo động |
| Phát hiện đột nhập | Cảm biến cửa (module hồng ngoại) báo động nếu cửa bị mở khi đang khóa |
| Còi báo động | Relay kênh 2 cấp nguồn cho còi active, tự tắt sau 15 giây |
| Màn hình OLED | Hiển thị LOCKED / OPEN / WAIT / ALARM, thông báo và IP. Tự tắt sau 30 giây không có ai |
| Phát hiện người | Cảm biến PIR bật màn hình và LED vàng khi có người đến gần cửa |
| Âm báo | Còi thụ động phát giai điệu khi PIN đúng, "tè tè" khi PIN sai |
| Nút trong nhà | **EXIT**: mở cửa không cần PIN. **LOCK**: khóa ngay và tắt báo động |
| Đổi PIN | Đổi trên web (phải nhập đúng PIN cũ). PIN lưu trong flash nên mất điện không mất |
| Nhật ký | 8 sự kiện gần nhất hiển thị trên web và in ra Serial |

## 📁 Cấu trúc thư mục

```
SmartLock/
├── SmartLock/
│   └── SmartLock.ino        # Mã nguồn Arduino cho ESP32
├── SmartLock_HuongDan.html  # Tài liệu hướng dẫn đầy đủ (xuất được PDF)
└── README.md
```

## 🧰 Linh kiện

| Linh kiện | SL | Vai trò |
|---|---|---|
| ESP32 DevKit V1 + cáp Micro USB | 1 | Bộ xử lý, phát WiFi, chạy Web Server |
| Breadboard 830 lỗ | 1 | Cắm LED, nút, còi, làm đường nguồn chung |
| Relay 2 kênh 5V | 1 | CH1: chốt khóa · CH2: bật còi báo động |
| OLED 0.96" I2C (SSD1306) | 1 | Hiển thị trạng thái |
| LED xanh / vàng / đỏ + điện trở 220Ω | 3 + 3 | Đèn báo trạng thái |
| Còi thụ động (passive) | 1 | Âm báo đúng / sai |
| Còi báo động (active) | 1 | Báo động (qua relay CH2) |
| Nút nhấn | 2 | EXIT, LOCK |
| PIR HC-SR501 | 1 | Phát hiện người |
| Module tránh vật cản (IR) | 1 | Cảm biến cửa đóng / mở |
| Dây Dupont F-F, F-M, M-M | ~28 | Nối dây |

Không dùng: DHT11, quang trở, chiết áp 10k, điện trở 1k/10k. Gợi ý cách tận dụng chúng nằm ở phần [hướng phát triển](#-hạn-chế--hướng-phát-triển).

> **Phân biệt 2 loại còi:** còi **active** có mặt dưới bịt kín, cấp 5V vào là kêu liên tục. Còi **passive** có mặt dưới hở (thấy mạch xanh), cấp 5V vào chỉ kêu "tạch" một cái.

## 🔌 Sơ đồ nối dây

| Linh kiện | Chân linh kiện | Nối tới |
|---|---|---|
| OLED | VCC · GND · SCL · SDA | 3V3 · GND · **GPIO 22** · **GPIO 21** |
| Relay 2 kênh | VCC · GND · IN1 · IN2 | VIN (5V) · GND · **GPIO 26** · **GPIO 27** |
| Relay CH2 (cọc vít) | COM2 · NO2 | 5V · chân (+) còi báo động |
| Còi báo động | (−) | GND |
| LED xanh | chân dài (+) qua 220Ω · chân ngắn | **GPIO 18** · GND |
| LED vàng | chân dài (+) qua 220Ω · chân ngắn | **GPIO 19** · GND |
| LED đỏ | chân dài (+) qua 220Ω · chân ngắn | **GPIO 23** · GND |
| Còi thụ động | (+) · (−) | **GPIO 25** · GND |
| Nút EXIT | 2 chân chéo nhau | **GPIO 32** · GND |
| Nút LOCK | 2 chân chéo nhau | **GPIO 33** · GND |
| Cảm biến IR | VCC · GND · OUT | 3V3 · GND · **GPIO 34** |
| PIR HC-SR501 | VCC · GND · OUT | VIN (5V) · GND · **GPIO 35** |

Cách chia đường nguồn trên breadboard:

```
Mép trên :  (+) = 3V3  ← chân 3V3 ESP32   → OLED, cảm biến IR
            (−) = GND  ← chân GND ESP32
Mép dưới :  (+) = 5V   ← chân VIN ESP32   → Relay, PIR, còi báo động
            (−) = GND  ← nối sang (−) mép trên
```

> ⚠️ Luôn rút cáp USB trước khi cắm hoặc tháo dây. Không để đường 3V3 và 5V chạm nhau. Tất cả GND phải nối chung. OLED và cảm biến IR dùng **3V3**.

**Nên lắp và thử từng phần:** nguồn + 3 LED → OLED → còi thụ động + nút → relay → còi báo động → cảm biến IR → PIR. Thiếu linh kiện nào thì chỉ phần đó không chạy, các phần khác vẫn hoạt động.

## 🛠 Cài đặt & nạp code

1. Cài **Arduino IDE 2.x**.
2. Vào *File → Preferences → Additional boards manager URLs* và thêm đường dẫn:
   ```
   https://espressif.github.io/arduino-esp32/package_esp32_index.json
   ```
3. Vào *Boards Manager*, cài **esp32** của **Espressif Systems**.
4. Vào *Library Manager*, cài **Adafruit SSD1306** và chọn **Install All** để cài kèm Adafruit GFX và BusIO.
5. Mở `SmartLock/SmartLock.ino`, chọn board **ESP32 Dev Module** và chọn đúng cổng.
6. Bấm **Upload**. Nếu báo lỗi `Failed to connect`, hãy giữ nút **BOOT** cho đến khi thấy chữ `Writing...`.
7. Mở Serial Monitor ở **115200 baud** rồi bấm **EN** để xem địa chỉ IP.

Driver: Windows cần cài CP210x hoặc CH340, tùy chip USB trên board. Linux chạy lệnh `sudo usermod -aG dialout $USER` rồi đăng nhập lại.

Code đã được biên dịch thử với esp32 core **3.3.12**, Adafruit SSD1306 **2.5.17** và Adafruit GFX **1.12.6**. Chương trình chiếm khoảng 75% flash.

## 📱 Sử dụng

1. Trên điện thoại, kết nối WiFi **`SmartLock-ESP32`**, mật khẩu **`12345678`**. Nên tắt 4G trong lúc dùng.
2. Mở trình duyệt và vào **http://192.168.4.1**. Địa chỉ này cũng hiện trên OLED.
3. Nhập PIN mặc định **`1234`** rồi bấm **OK**.

| Trạng thái | LED | OLED | Relay 1 | Âm thanh |
|---|---|---|---|---|
| Đang khóa | Đỏ | LOCKED | Ngắt | — |
| Có người trước cửa | Đỏ + Vàng | Moi nhap ma PIN... | Ngắt | — |
| Mở khóa | Xanh | OPEN | Đóng | Giai điệu |
| Sai PIN | Đỏ nháy | Sai PIN! 1/3 | Ngắt | "Tè tè" |
| Tạm khóa | Vàng nháy chậm | WAIT 25s | Ngắt | — |
| Báo động | Vàng nháy nhanh | ! ALARM ! | Ngắt | Còi báo động |

**Tắt báo động:** nhập đúng PIN, bấm nút LOCK, hoặc chờ báo động tự tắt sau 15 giây. Nút "Khóa ngay" trên web **không** tắt được báo động.

## ⚙️ Cấu hình

Các hằng số ở đầu file `SmartLock.ino`:

| Hằng số | Mặc định | Ý nghĩa |
|---|---|---|
| `AP_SSID` / `AP_PASS` | `SmartLock-ESP32` / `12345678` | Tên và mật khẩu WiFi do ESP32 phát |
| `DEFAULT_PIN` | `"1234"` | PIN dùng lần đầu, khi flash chưa lưu PIN nào |
| `MAX_FAILS` | `3` | Số lần được nhập sai trước khi tạm khóa |
| `LOCKOUT_MS` | `30000` | Thời gian tạm khóa (ms) |
| `AUTO_LOCK_MS` | `5000` | Thời gian chờ trước khi tự khóa lại (ms) |
| `ALARM_MS` | `15000` | Thời gian còi báo động kêu (ms) |
| `SCREEN_TIMEOUT_MS` | `30000` | Thời gian không có hoạt động trước khi tắt OLED (ms) |
| `RELAY_ACTIVE_LOW` | `true` | Đổi thành `false` nếu relay hoạt động ngược |
| `USE_DOOR_SENSOR` | `true` | Đổi thành `false` nếu **chưa lắp cảm biến cửa**, tránh báo động giả |
| `OLED_ADDR` | `0x3C` | Đổi thành `0x3D` nếu OLED không hiện |

## 🌐 Web API

| Đường dẫn | Phương thức | Tham số | Chức năng |
|---|---|---|---|
| `/` | GET | — | Trang bàn phím số |
| `/status` | GET | — | JSON: `locked`, `lockout`, `door`, `motion`, `alarm`, `log[]` |
| `/unlock` | POST | `pin` | Kiểm tra PIN và mở khóa |
| `/lock` | POST | — | Khóa ngay (không tắt báo động) |
| `/changepin` | POST | `old`, `new` | Đổi PIN và lưu vào flash |

Ví dụ phản hồi của `/status`:

```json
{"locked":true,"lockout":0,"door":false,"motion":true,"alarm":false,
 "log":["00:01:12 Phát hiện người trước cửa","00:00:00 Khóa cửa (khởi động)"]}
```

## 🛡 Bảo mật & xử lý chuỗi

| Biện pháp | Cách làm | Chống lại |
|---|---|---|
| Kiểm tra đầu vào | `isValidPin()`: chỉ nhận 4–8 ký tự, tất cả là chữ số | Dữ liệu rác, chuỗi quá dài |
| Chuẩn hóa chuỗi | `trim()` trước khi so sánh | Khoảng trắng thừa |
| So sánh thời gian cố định | `safeEquals()` duyệt hết chuỗi bằng XOR, không dừng sớm ở ký tự sai | Timing attack |
| Giới hạn số lần thử | Sai 3 lần thì tạm khóa 30 giây và báo động | Brute-force |
| Gửi PIN bằng POST | PIN nằm trong body, không nằm trên URL | Lộ PIN qua lịch sử trình duyệt |
| Kiểm tra ở server | Chỉ ESP32 biết PIN đúng | Đọc mã nguồn trang web để lấy PIN |
| Đổi PIN cần PIN cũ | `/changepin` kiểm tra PIN cũ, lần thất bại được ghi nhật ký | Người lạ tự ý đổi PIN |
| Báo động đột nhập | Cửa mở khi đang khóa thì bật còi | Phá cửa |
| Phân quyền tắt còi | Chỉ PIN đúng hoặc nút LOCK trong nhà tắt được còi | Kẻ gian vào được WiFi rồi tắt còi |
| Tự khóa lại | Khóa sau 5 giây khi cửa đã đóng | Quên khóa cửa |

Ước tính: dò hết PIN 4 số mất khoảng **27,8 giờ**, PIN 6 số mất khoảng **115 ngày**, và còi sẽ kêu sau mỗi 3 lần sai.

## ✅ Kiểm thử nhanh

| # | Thao tác | Kết quả mong đợi |
|---|---|---|
| 1 | Nhập `1234` | Mở khóa, LED xanh, relay kêu "tách", nghe giai điệu |
| 2 | Che cảm biến IR, chờ 5 giây | Tự khóa lại |
| 3 | Nhập sai 3 lần | Tạm khóa 30 giây, còi báo động kêu |
| 4 | Đang khóa thì bỏ tay khỏi IR | Báo động đột nhập |
| 5 | Đang báo động, bấm "Khóa ngay" trên web | Còi vẫn kêu |
| 6 | Đang báo động, bấm nút LOCK | Còi tắt |
| 7 | Đổi PIN thành `5678`, rút điện rồi cắm lại | `5678` mở được, `1234` bị từ chối |

Bản đầy đủ 15 kịch bản kiểm thử nằm trong file hướng dẫn HTML.

## 🧯 Xử lý lỗi thường gặp

| Hiện tượng | Cách sửa |
|---|---|
| Máy tính không nhận cổng COM / ttyUSB | Dùng cáp USB có truyền dữ liệu, cài driver CP210x/CH340 |
| OLED không hiện | Kiểm tra SDA=21, SCL=22, VCC=3V3. Thử `OLED_ADDR = 0x3D` |
| Báo động ngay khi khởi động | Chỉnh biến trở trên module IR, hoặc đặt `USE_DOOR_SENSOR = false` |
| Relay hoạt động ngược | `RELAY_ACTIVE_LOW = false` |
| Relay không nhả | Rút jumper JD-VCC, nối JD-VCC với 5V và VCC với 3V3 |
| PIR báo có người liên tục | Chờ 60 giây cho PIR ổn định, vặn biến trở Tx hết ngược chiều kim đồng hồ |
| Còi thụ động chỉ kêu "tạch" | Lắp nhầm 2 loại còi, đổi chỗ chúng cho nhau |
| Không vào được 192.168.4.1 | Tắt 4G, chọn "Vẫn kết nối" khi điện thoại báo WiFi không có Internet |

## 🚧 Hạn chế & hướng phát triển

**Hạn chế**
- Dùng HTTP nên dữ liệu không được mã hóa.
- PIN được lưu dạng chuỗi gốc trong flash.
- Relay chỉ mô phỏng chốt khóa, chưa nối khóa thật.

**Hướng phát triển**
- Băm PIN bằng SHA-256 có salt (thư viện `mbedtls` có sẵn trên ESP32).
- Mã hóa đường truyền bằng HTTPS, hoặc dùng MQTT qua TLS.
- Chế độ STA để ESP32 vào WiFi nhà. Gửi thông báo Telegram khi có báo động.
- Hiển thị nhiệt độ/độ ẩm (DHT11) lên OLED, chỉnh độ sáng màn hình theo quang trở, chỉnh thời gian tự khóa bằng chiết áp.
- Nối relay CH1 với khóa chốt điện 12V (dùng nguồn riêng, mắc thêm diode 1N4007).
- Thêm cách mở bằng thẻ RFID RC522 hoặc vân tay.
