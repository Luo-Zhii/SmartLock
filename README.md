# SmartLock – Khóa cửa thông minh nhập PIN trên Web (ESP32)

ESP32 tự phát WiFi và chạy Web Server với giao diện bàn phím số. Nhập đúng PIN thì Servo/Relay mở khóa và LED xanh sáng; nhập sai thì LED đỏ nháy, sai 3 lần bị tạm khóa 30 giây.

## Cấu trúc

- `SmartLock/SmartLock.ino` – mã nguồn Arduino cho ESP32
- `SmartLock_HuongDan.html` – tài liệu hướng dẫn đầy đủ (linh kiện, nối dây, cài đặt, giải thích code, bảo mật, kiểm thử). Mở bằng trình duyệt, nhấn Ctrl+P để lưu PDF.

## Nối dây nhanh

| Linh kiện | Chân ESP32 |
|---|---|
| Servo SG90 (tín hiệu / 5V / GND) | GPIO 13 / VIN / GND |
| LED xanh (qua 220Ω) | GPIO 26 |
| LED đỏ (qua 220Ω) | GPIO 27 |
| Buzzer (tùy chọn) | GPIO 25 |
| Relay IN (tùy chọn) | GPIO 14 |

## Chạy thử

1. Arduino IDE → cài board **esp32** (Espressif) và thư viện **ESP32Servo**.
2. Chọn board **ESP32 Dev Module**, nạp `SmartLock/SmartLock.ino`.
3. Điện thoại kết nối WiFi `SmartLock-ESP32` (mật khẩu `12345678`), mở `http://192.168.4.1`.
4. PIN mặc định: `1234`.
