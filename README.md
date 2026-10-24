# SmartLock – Khóa cửa thông minh nhập PIN trên Web (ESP32)

ESP32 tự phát WiFi và chạy Web Server với giao diện bàn phím số. Nhập đúng PIN thì relay mở khóa, LED xanh sáng; sai thì LED đỏ nháy, sai 3 lần bị tạm khóa 30 giây kèm còi báo động. Cảm biến cửa phát hiện đột nhập, PIR phát hiện người đến gần, OLED hiển thị trạng thái.

Chỉ dùng linh kiện trong bộ kit ESP32 Starter (ESP32, OLED 0.96", relay 2 kênh, PIR HC-SR501, module tránh vật cản, còi active/passive, nút nhấn, LED).

## Cấu trúc

- `SmartLock/SmartLock.ino` – mã nguồn Arduino cho ESP32
- `SmartLock_HuongDan.html` – tài liệu hướng dẫn đầy đủ (linh kiện, nối dây, lắp ráp, giải thích code, bảo mật, kiểm thử). Mở bằng trình duyệt, nhấn Ctrl+P để lưu PDF.

## Nối dây nhanh

| Linh kiện | Chân ESP32 | Nguồn |
|---|---|---|
| OLED SDA / SCL | GPIO 21 / GPIO 22 | 3V3 |
| Relay IN1 (khóa) / IN2 (còi báo động) | GPIO 26 / GPIO 27 | VIN 5V |
| LED xanh / vàng / đỏ (qua 220Ω) | GPIO 18 / 19 / 23 | – |
| Còi thụ động | GPIO 25 | – |
| Nút EXIT / LOCK (nối GND) | GPIO 32 / 33 | – |
| Cảm biến IR (cửa) OUT | GPIO 34 | 3V3 |
| PIR OUT | GPIO 35 | VIN 5V |

Còi báo động: relay COM2 → 5V, NO2 → còi (+), còi (−) → GND.

## Chạy thử

1. Arduino IDE → cài board **esp32** (Espressif) và thư viện **Adafruit SSD1306** (Install All).
2. Chọn board **ESP32 Dev Module**, nạp `SmartLock/SmartLock.ino`.
3. Điện thoại kết nối WiFi `SmartLock-ESP32` (mật khẩu `12345678`), mở `http://192.168.4.1`.
4. PIN mặc định: `1234`.
