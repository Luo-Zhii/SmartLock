/*
 * KHÓA CỬA THÔNG MINH - NHẬP PIN TRÊN WEB (ESP32)
 * Dùng đúng linh kiện trong bộ kit ESP32 starter:
 *   - Relay kênh 1 : chốt khóa cửa (relay đóng = mở khóa)
 *   - Relay kênh 2 : bật còi báo động (active buzzer)
 *   - OLED 0.96"   : hiển thị trạng thái
 *   - LED xanh/vàng/đỏ : mở khóa / cảnh báo / đang khóa
 *   - Còi thụ động : âm báo đúng/sai PIN
 *   - 2 nút nhấn   : EXIT (mở từ trong nhà), LOCK (khóa ngay + tắt báo động)
 *   - PIR HC-SR501 : phát hiện có người trước cửa -> bật màn hình
 *   - Cảm biến tránh vật cản (IR): cảm biến cửa đóng/mở
 *
 * Luồng chính: điện thoại kết nối WiFi "SmartLock-ESP32" -> mở http://192.168.4.1
 * -> nhập PIN -> đúng thì mở khóa, tự khóa lại khi hết 5s và cửa đã đóng.
 * Sai 3 lần -> tạm khóa 30s + báo động. Mở cửa khi đang khóa -> báo động đột nhập.
 *
 * Thư viện cần cài: Adafruit SSD1306, Adafruit GFX Library
 * Board: ESP32 Dev Module
 */

#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ===== WiFi do ESP32 tự phát =====
const char* AP_SSID = "SmartLock-ESP32";
const char* AP_PASS = "12345678";  // tối thiểu 8 ký tự

// ===== Chân kết nối =====
// OLED: SDA = GPIO 21, SCL = GPIO 22 (I2C mặc định)
const int PIN_RELAY_LOCK  = 26;  // Relay IN1
const int PIN_RELAY_ALARM = 27;  // Relay IN2
const int PIN_LED_GREEN   = 18;
const int PIN_LED_YELLOW  = 19;
const int PIN_LED_RED     = 23;
const int PIN_BUZZER      = 25;  // còi thụ động
const int PIN_BTN_EXIT    = 32;
const int PIN_BTN_LOCK    = 33;
const int PIN_DOOR_IR     = 34;  // OUT cảm biến tránh vật cản
const int PIN_PIR         = 35;  // OUT cảm biến PIR

const bool RELAY_ACTIVE_LOW = true;  // đa số module relay kích mức LOW
const bool USE_DOOR_SENSOR  = true;  // đặt false nếu chưa lắp cảm biến cửa

// ===== Thông số =====
const char* DEFAULT_PIN = "1234";
const int MAX_FAILS                   = 3;
const unsigned long LOCKOUT_MS        = 30000;
const unsigned long AUTO_LOCK_MS      = 5000;
const unsigned long ALARM_MS          = 15000;
const unsigned long SCREEN_TIMEOUT_MS = 30000;
const unsigned long OLED_MSG_MS       = 3000;

// ===== OLED =====
const int OLED_W = 128;
const int OLED_H = 64;
const uint8_t OLED_ADDR = 0x3C;  // một số màn hình dùng 0x3D

WebServer server(80);
Preferences prefs;
Adafruit_SSD1306 display(OLED_W, OLED_H, &Wire, -1);

// ===== Trạng thái =====
String currentPin;
bool isLocked = true;
unsigned long unlockedAt = 0;
int failCount = 0;
bool lockedOut = false;
unsigned long lockoutStart = 0;
bool alarmOn = false;
unsigned long alarmStart = 0;
bool doorOpen = false;
bool motion = false;

bool oledOk = false;
bool screenOn = true;
unsigned long lastActivity = 0;
String oledMsg;
unsigned long oledMsgAt = 0;

struct Button {
  int pin;
  bool last;
  unsigned long changedAt;
};
Button btnExit = {PIN_BTN_EXIT, HIGH, 0};
Button btnLock = {PIN_BTN_LOCK, HIGH, 0};

// ===== Nhật ký sự kiện (8 dòng gần nhất) =====
const int LOG_SIZE = 8;
String logs[LOG_SIZE];
int logCount = 0;

// ================= GIAO DIỆN WEB =================
const char PAGE_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="vi">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Smart Lock</title>
<style>
  body{font-family:sans-serif;background:#1e1e2e;color:#fff;text-align:center;margin:0;padding:16px}
  h2{margin:6px 0}
  .chips{display:grid;grid-template-columns:1fr 1fr;gap:8px;max-width:320px;margin:10px auto}
  .chip{padding:8px;border-radius:10px;background:#33334d;font-size:14px}
  .red{background:#8e2b20}.green{background:#1d7a44}.yellow{background:#9a6b00}
  #dots{font-size:32px;letter-spacing:8px;height:44px;margin:10px 0}
  .pad{display:grid;grid-template-columns:repeat(3,76px);gap:12px;justify-content:center}
  .pad button{height:68px;font-size:26px;border:none;border-radius:50%;background:#33334d;color:#fff}
  .pad button:active{background:#55557a}
  .pad .ok{background:#2e8b57}.pad .clr{background:#b03a2e}
  #msg{min-height:24px;margin:12px 0}
  .ok-msg{color:#4cd964}.err-msg{color:#ff5c5c}
  details{margin-top:18px}
  input{padding:10px;margin:6px;border-radius:8px;border:none;width:160px;font-size:16px}
  .btn{padding:10px 20px;border:none;border-radius:8px;background:#4a6cf7;color:#fff;font-size:16px;margin:6px}
  #log{list-style:none;padding:0;max-width:340px;margin:8px auto;text-align:left;font-size:13px;color:#c9c9d9}
  #log li{padding:4px 0;border-bottom:1px solid #33334d}
</style>
</head>
<body>
  <h2>🔐 Smart Lock</h2>
  <div class="chips">
    <div id="cLock" class="chip">...</div>
    <div id="cDoor" class="chip">...</div>
    <div id="cMotion" class="chip">...</div>
    <div id="cAlarm" class="chip">...</div>
  </div>
  <div id="dots">----</div>
  <div class="pad">
    <button onclick="press(1)">1</button><button onclick="press(2)">2</button><button onclick="press(3)">3</button>
    <button onclick="press(4)">4</button><button onclick="press(5)">5</button><button onclick="press(6)">6</button>
    <button onclick="press(7)">7</button><button onclick="press(8)">8</button><button onclick="press(9)">9</button>
    <button class="clr" onclick="clr()">C</button><button onclick="press(0)">0</button><button class="ok" onclick="send()">OK</button>
  </div>
  <div id="msg"></div>
  <button class="btn" onclick="lockNow()">Khóa ngay</button>

  <details>
    <summary>Đổi mã PIN</summary>
    <input id="oldPin" type="password" inputmode="numeric" placeholder="PIN cũ"><br>
    <input id="newPin" type="password" inputmode="numeric" placeholder="PIN mới (4-8 số)"><br>
    <button class="btn" onclick="changePin()">Lưu</button>
  </details>

  <details open>
    <summary>Nhật ký</summary>
    <ul id="log"></ul>
  </details>

<script>
let pin = '';
function show(){ document.getElementById('dots').textContent = pin.length ? '●'.repeat(pin.length) : '----'; }
function press(k){ if(pin.length < 8){ pin += k; show(); } }
function clr(){ pin = ''; show(); }
function msg(t, ok){ const m = document.getElementById('msg'); m.textContent = t; m.className = ok ? 'ok-msg' : 'err-msg'; }
function chip(id, text, cls){ const c = document.getElementById(id); c.textContent = text; c.className = 'chip ' + cls; }

async function post(url, data){
  const r = await fetch(url, {method:'POST',
    headers:{'Content-Type':'application/x-www-form-urlencoded'},
    body:new URLSearchParams(data)});
  return r.json();
}
async function send(){
  if(pin.length < 4){ msg('PIN phải có 4-8 chữ số', false); return; }
  const j = await post('/unlock', {pin});
  msg(j.msg, j.ok); clr(); refresh();
}
async function lockNow(){ const j = await post('/lock', {}); msg(j.msg, j.ok); refresh(); }
async function changePin(){
  const o = document.getElementById('oldPin'), n = document.getElementById('newPin');
  const j = await post('/changepin', {old:o.value, new:n.value});
  msg(j.msg, j.ok); o.value = ''; n.value = '';
}
async function refresh(){
  try{
    const j = await (await fetch('/status')).json();
    if(j.lockout > 0) chip('cLock', '⛔ Tạm khóa ' + j.lockout + 's', 'yellow');
    else chip('cLock', j.locked ? '🔒 Đang khóa' : '🔓 Đã mở', j.locked ? 'red' : 'green');
    chip('cDoor', j.door ? '🚪 Cửa mở' : '🚪 Cửa đóng', j.door ? 'yellow' : '');
    chip('cMotion', j.motion ? '🚶 Có người' : '🚶 Không có ai', j.motion ? 'yellow' : '');
    chip('cAlarm', j.alarm ? '🚨 BÁO ĐỘNG' : '🔕 Bình thường', j.alarm ? 'red' : '');
    document.getElementById('log').innerHTML = j.log.map(l => '<li>' + l + '</li>').join('');
  }catch(e){}
}
show(); refresh(); setInterval(refresh, 1000);
</script>
</body>
</html>
)rawliteral";

// ================= TIỆN ÍCH =================
String uptimeStr() {
  unsigned long s = millis() / 1000;
  char buf[12];
  snprintf(buf, sizeof(buf), "%02lu:%02lu:%02lu", s / 3600, (s / 60) % 60, s % 60);
  return buf;
}

void addLog(const String& msg) {
  for (int i = LOG_SIZE - 1; i > 0; i--) logs[i] = logs[i - 1];
  logs[0] = uptimeStr() + " " + msg;
  if (logCount < LOG_SIZE) logCount++;
  Serial.println(logs[0]);
}

void touch() {
  lastActivity = millis();
}

// Thông báo ngắn trên OLED (chỉ ASCII, không dấu)
void showMsg(const String& m) {
  oledMsg = m;
  oledMsgAt = millis();
  touch();
}

// ================= XỬ LÝ CHUỖI / BẢO MẬT =================
bool isValidPin(const String& p) {
  if (p.length() < 4 || p.length() > 8) return false;
  for (size_t i = 0; i < p.length(); i++) {
    if (!isDigit(p[i])) return false;
  }
  return true;
}

// So sánh thời gian cố định: không thoát sớm khi gặp ký tự sai
bool safeEquals(const String& a, const String& b) {
  if (a.length() != b.length()) return false;
  uint8_t diff = 0;
  for (size_t i = 0; i < a.length(); i++) diff |= a[i] ^ b[i];
  return diff == 0;
}

unsigned long lockoutRemainingSec() {
  if (!lockedOut) return 0;
  unsigned long elapsed = millis() - lockoutStart;
  if (elapsed >= LOCKOUT_MS) {
    lockedOut = false;
    return 0;
  }
  return (LOCKOUT_MS - elapsed) / 1000 + 1;
}

// ================= ĐIỀU KHIỂN PHẦN CỨNG =================
void setRelay(int pin, bool on) {
  digitalWrite(pin, (on ^ RELAY_ACTIVE_LOW) ? HIGH : LOW);
}

void playSuccess() {
  tone(PIN_BUZZER, 1047, 120); delay(140);
  tone(PIN_BUZZER, 1319, 120); delay(140);
  tone(PIN_BUZZER, 1568, 200); delay(220);
  noTone(PIN_BUZZER);
}

void playFail() {
  for (int i = 0; i < 2; i++) {
    digitalWrite(PIN_LED_RED, LOW);
    tone(PIN_BUZZER, 300, 180); delay(200);
    digitalWrite(PIN_LED_RED, HIGH);
    delay(120);
  }
  noTone(PIN_BUZZER);
}

void playClick() {
  tone(PIN_BUZZER, 880, 80); delay(100);
  noTone(PIN_BUZZER);
}

void startAlarm(const String& reason) {
  if (!alarmOn) addLog("BÁO ĐỘNG: " + reason);
  alarmOn = true;
  alarmStart = millis();
  setRelay(PIN_RELAY_ALARM, true);
  touch();
}

void stopAlarm() {
  if (!alarmOn) return;
  alarmOn = false;
  setRelay(PIN_RELAY_ALARM, false);
  addLog("Tắt báo động");
}

void lockDoor(const String& source) {
  isLocked = true;
  setRelay(PIN_RELAY_LOCK, false);
  digitalWrite(PIN_LED_GREEN, LOW);
  digitalWrite(PIN_LED_RED, HIGH);
  addLog("Khóa cửa (" + source + ")");
  showMsg("Da khoa cua");
}

void unlockDoor(const String& source) {
  isLocked = false;
  unlockedAt = millis();
  failCount = 0;
  stopAlarm();
  setRelay(PIN_RELAY_LOCK, true);
  digitalWrite(PIN_LED_GREEN, HIGH);
  digitalWrite(PIN_LED_RED, LOW);
  addLog("Mở khóa (" + source + ")");
  showMsg("Moi vao!");
}

// LED vàng: nháy nhanh khi báo động, nháy chậm khi tạm khóa, sáng khi có người
void updateYellowLed() {
  unsigned long t = millis();
  if (alarmOn) digitalWrite(PIN_LED_YELLOW, (t / 150) % 2);
  else if (lockoutRemainingSec() > 0) digitalWrite(PIN_LED_YELLOW, (t / 500) % 2);
  else digitalWrite(PIN_LED_YELLOW, motion ? HIGH : LOW);
}

// ================= CẢM BIẾN & NÚT NHẤN =================
bool readDoorOpen() {
  // Module tránh vật cản: OUT = LOW khi thấy vật cản (cánh cửa) -> cửa đóng
  return digitalRead(PIN_DOOR_IR) == HIGH;
}

void readSensors() {
  bool m = digitalRead(PIN_PIR) == HIGH;
  if (m && !motion) addLog("Phát hiện người trước cửa");
  motion = m;
  if (motion) touch();

  if (!USE_DOOR_SENSOR) return;
  bool open = readDoorOpen();
  if (open == doorOpen) return;
  doorOpen = open;
  addLog(open ? "Cửa mở" : "Cửa đóng");
  touch();
  if (open && isLocked) startAlarm("cửa bị mở khi đang khóa");
}

bool pressed(Button& b) {
  bool now = digitalRead(b.pin);
  if (now != b.last && millis() - b.changedAt > 50) {
    b.changedAt = millis();
    b.last = now;
    return now == LOW;
  }
  return false;
}

void handleButtons() {
  if (pressed(btnExit)) {
    unlockDoor("nút EXIT");
    playSuccess();
  }
  if (pressed(btnLock)) {
    stopAlarm();
    lockDoor("nút LOCK");
    playClick();
  }
}

// ================= OLED =================
void drawOled() {
  if (!oledOk) return;

  bool idle = isLocked && !alarmOn && lockoutRemainingSec() == 0;
  if (idle && millis() - lastActivity > SCREEN_TIMEOUT_MS) {
    if (screenOn) {
      display.ssd1306_command(SSD1306_DISPLAYOFF);
      screenOn = false;
    }
    return;
  }
  if (!screenOn) {
    display.ssd1306_command(SSD1306_DISPLAYON);
    screenOn = true;
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("SMART LOCK");
  if (USE_DOOR_SENSOR) {
    display.setCursor(80, 0);
    display.print(doorOpen ? "Cua:MO" : "Cua:DONG");
  }
  display.drawLine(0, 10, 127, 10, SSD1306_WHITE);

  display.setTextSize(2);
  display.setCursor(0, 16);
  unsigned long remain = lockoutRemainingSec();
  if (alarmOn) {
    display.print("! ALARM !");
  } else if (remain > 0) {
    display.print("WAIT ");
    display.print(remain);
    display.print("s");
  } else {
    display.print(isLocked ? "LOCKED" : "OPEN");
  }

  display.setTextSize(1);
  display.setCursor(0, 40);
  if (oledMsg.length() && millis() - oledMsgAt < OLED_MSG_MS) {
    display.print(oledMsg);
  } else if (motion && isLocked) {
    display.print("Moi nhap ma PIN...");
  } else {
    display.print("WiFi: ");
    display.print(AP_SSID);
  }
  display.setCursor(0, 54);
  display.print("http://");
  display.print(WiFi.softAPIP());
  display.display();
}

// ================= HTTP HANDLERS =================
void sendJson(bool ok, const String& msg) {
  server.send(200, "application/json", "{\"ok\":" + String(ok ? "true" : "false") + ",\"msg\":\"" + msg + "\"}");
}

String jsonBool(bool v) {
  return v ? "true" : "false";
}

void handleRoot() {
  server.send_P(200, "text/html", PAGE_HTML);
}

void handleStatus() {
  String json = "{\"locked\":" + jsonBool(isLocked) +
                ",\"lockout\":" + String(lockoutRemainingSec()) +
                ",\"door\":" + jsonBool(USE_DOOR_SENSOR && doorOpen) +
                ",\"motion\":" + jsonBool(motion) +
                ",\"alarm\":" + jsonBool(alarmOn) +
                ",\"log\":[";
  for (int i = 0; i < logCount; i++) {
    if (i) json += ",";
    json += "\"" + logs[i] + "\"";
  }
  json += "]}";
  server.send(200, "application/json", json);
}

void handleUnlock() {
  touch();
  unsigned long remain = lockoutRemainingSec();
  if (remain > 0) {
    sendJson(false, "Sai quá nhiều lần! Thử lại sau " + String(remain) + "s");
    return;
  }

  String pin = server.arg("pin");
  pin.trim();
  if (!isValidPin(pin)) {
    sendJson(false, "PIN không hợp lệ (chỉ 4-8 chữ số)");
    return;
  }

  if (safeEquals(pin, currentPin)) {
    unlockDoor("PIN trên web");
    sendJson(true, "Mở khóa thành công!");
    playSuccess();
    return;
  }

  failCount++;
  addLog("Sai PIN lần " + String(failCount));
  if (failCount >= MAX_FAILS) {
    failCount = 0;
    lockedOut = true;
    lockoutStart = millis();
    showMsg("Sai PIN qua 3 lan!");
    startAlarm("sai PIN " + String(MAX_FAILS) + " lần");
    sendJson(false, "Sai " + String(MAX_FAILS) + " lần! Tạm khóa " + String(LOCKOUT_MS / 1000) + "s");
  } else {
    showMsg("Sai PIN! " + String(failCount) + "/" + String(MAX_FAILS));
    sendJson(false, "Sai PIN! Còn " + String(MAX_FAILS - failCount) + " lần thử");
  }
  playFail();
}

// Khóa từ web KHÔNG tắt được báo động: chỉ PIN đúng hoặc nút trong nhà mới tắt được
void handleLock() {
  touch();
  lockDoor("web");
  sendJson(true, "Đã khóa cửa");
}

void handleChangePin() {
  touch();
  String oldPin = server.arg("old");
  String newPin = server.arg("new");
  oldPin.trim();
  newPin.trim();

  if (lockoutRemainingSec() > 0) {
    sendJson(false, "Đang bị tạm khóa");
    return;
  }
  if (!safeEquals(oldPin, currentPin)) {
    addLog("Đổi PIN thất bại (sai PIN cũ)");
    sendJson(false, "PIN cũ không đúng");
    return;
  }
  if (!isValidPin(newPin)) {
    sendJson(false, "PIN mới phải gồm 4-8 chữ số");
    return;
  }

  currentPin = newPin;
  prefs.putString("pin", currentPin);
  addLog("Đã đổi PIN");
  showMsg("Da doi PIN");
  sendJson(true, "Đổi PIN thành công!");
}

// ================= SETUP / LOOP =================
void setup() {
  Serial.begin(115200);

  pinMode(PIN_RELAY_LOCK, OUTPUT);
  pinMode(PIN_RELAY_ALARM, OUTPUT);
  setRelay(PIN_RELAY_LOCK, false);
  setRelay(PIN_RELAY_ALARM, false);
  pinMode(PIN_LED_GREEN, OUTPUT);
  pinMode(PIN_LED_YELLOW, OUTPUT);
  pinMode(PIN_LED_RED, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_BTN_EXIT, INPUT_PULLUP);
  pinMode(PIN_BTN_LOCK, INPUT_PULLUP);
  pinMode(PIN_DOOR_IR, INPUT);
  pinMode(PIN_PIR, INPUT);

  Wire.begin(21, 22);
  oledOk = display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);
  if (!oledOk) Serial.println("[WARN] Khong tim thay OLED (thu doi dia chi 0x3D)");

  prefs.begin("smartlock", false);
  currentPin = prefs.getString("pin", DEFAULT_PIN);

  if (USE_DOOR_SENSOR) doorOpen = readDoorOpen();
  lockDoor("khởi động");

  WiFi.softAP(AP_SSID, AP_PASS);
  Serial.print("WiFi: ");
  Serial.println(AP_SSID);
  Serial.print("Mo trinh duyet: http://");
  Serial.println(WiFi.softAPIP());

  server.on("/", HTTP_GET, handleRoot);
  server.on("/status", HTTP_GET, handleStatus);
  server.on("/unlock", HTTP_POST, handleUnlock);
  server.on("/lock", HTTP_POST, handleLock);
  server.on("/changepin", HTTP_POST, handleChangePin);
  server.begin();

  touch();
}

void loop() {
  server.handleClient();
  handleButtons();
  readSensors();

  // Tự khóa lại khi hết thời gian VÀ cửa đã đóng
  if (!isLocked && millis() - unlockedAt >= AUTO_LOCK_MS && !(USE_DOOR_SENSOR && doorOpen)) {
    lockDoor("tự động");
  }

  if (alarmOn && millis() - alarmStart >= ALARM_MS) stopAlarm();

  updateYellowLed();

  static unsigned long lastDraw = 0;
  if (millis() - lastDraw >= 200) {
    lastDraw = millis();
    drawOled();
  }
}
