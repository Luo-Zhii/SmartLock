/*
 * KHÓA CỬA THÔNG MINH - NHẬP PIN TRÊN WEB (ESP32)
 * - ESP32 tự phát WiFi, điện thoại kết nối rồi mở http://192.168.4.1
 * - Nhập đúng PIN -> Servo quay mở + Relay đóng + LED xanh, tự khóa lại sau 5s
 * - Sai 3 lần -> khóa tạm 30s (chống dò mật khẩu)
 * - Đổi PIN trên web, PIN lưu vào bộ nhớ flash (mất điện không mất)
 *
 * Thư viện cần cài: ESP32Servo (Kevin Harrington)
 * Board: ESP32 Dev Module
 */

#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <ESP32Servo.h>

// ===== WiFi do ESP32 tự phát =====
const char* AP_SSID = "SmartLock-ESP32";
const char* AP_PASS = "12345678";  // tối thiểu 8 ký tự

// ===== Chân kết nối =====
const int SERVO_PIN  = 13;
const int RELAY_PIN  = 14;
const int LED_GREEN  = 26;
const int LED_RED    = 27;
const int BUZZER_PIN = 25;
const bool RELAY_ACTIVE_LOW = true;  // đa số module relay kích mức LOW

// ===== Thông số =====
const int SERVO_LOCKED   = 0;   // góc khi khóa
const int SERVO_UNLOCKED = 90;  // góc khi mở
const char* DEFAULT_PIN  = "1234";
const int MAX_FAILS              = 3;
const unsigned long LOCKOUT_MS   = 30000;
const unsigned long AUTO_LOCK_MS = 5000;

WebServer server(80);
Preferences prefs;
Servo lockServo;

String currentPin;
bool isLocked = true;
unsigned long unlockedAt = 0;
int failCount = 0;
bool lockedOut = false;
unsigned long lockoutStart = 0;

// ================= GIAO DIỆN WEB =================
const char PAGE_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="vi">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Smart Lock</title>
<style>
  body{font-family:sans-serif;background:#1e1e2e;color:#fff;text-align:center;margin:0;padding:20px}
  h2{margin:8px 0}
  #status{font-size:18px;font-weight:bold;margin:10px 0}
  .locked{color:#ff5c5c}.unlocked{color:#4cd964}
  #dots{font-size:32px;letter-spacing:8px;height:44px;margin:12px 0}
  .pad{display:grid;grid-template-columns:repeat(3,80px);gap:12px;justify-content:center}
  .pad button{height:70px;font-size:26px;border:none;border-radius:50%;background:#33334d;color:#fff}
  .pad button:active{background:#55557a}
  .pad .ok{background:#2e8b57}.pad .clr{background:#b03a2e}
  #msg{min-height:24px;margin:12px 0}
  .ok-msg{color:#4cd964}.err-msg{color:#ff5c5c}
  details{margin-top:24px}
  input{padding:10px;margin:6px;border-radius:8px;border:none;width:160px;font-size:16px}
  .btn{padding:10px 20px;border:none;border-radius:8px;background:#4a6cf7;color:#fff;font-size:16px;margin:6px}
</style>
</head>
<body>
  <h2>🔐 Smart Lock</h2>
  <div id="status">...</div>
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

<script>
let pin = '';
function show(){ document.getElementById('dots').textContent = pin.length ? '●'.repeat(pin.length) : '----'; }
function press(k){ if(pin.length < 8){ pin += k; show(); } }
function clr(){ pin = ''; show(); }
function msg(t, ok){ const m = document.getElementById('msg'); m.textContent = t; m.className = ok ? 'ok-msg' : 'err-msg'; }

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
    const s = document.getElementById('status');
    if(j.lockout > 0){ s.textContent = '⛔ Tạm khóa ' + j.lockout + 's'; s.className = 'locked'; }
    else if(j.locked){ s.textContent = '🔒 ĐANG KHÓA'; s.className = 'locked'; }
    else { s.textContent = '🔓 ĐÃ MỞ'; s.className = 'unlocked'; }
  }catch(e){}
}
show(); refresh(); setInterval(refresh, 1000);
</script>
</body>
</html>
)rawliteral";

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
void setRelay(bool on) {
  digitalWrite(RELAY_PIN, (on ^ RELAY_ACTIVE_LOW) ? HIGH : LOW);
}

void beep(int times, int ms) {
  for (int i = 0; i < times; i++) {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(ms);
    digitalWrite(BUZZER_PIN, LOW);
    delay(ms);
  }
}

void lockDoor() {
  isLocked = true;
  lockServo.write(SERVO_LOCKED);
  setRelay(false);
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_RED, HIGH);
  Serial.println("[LOCK] Da khoa");
}

void unlockDoor() {
  isLocked = false;
  unlockedAt = millis();
  lockServo.write(SERVO_UNLOCKED);
  setRelay(true);
  digitalWrite(LED_GREEN, HIGH);
  digitalWrite(LED_RED, LOW);
  beep(1, 150);
  Serial.println("[LOCK] Da mo");
}

void wrongPinAlert() {
  for (int i = 0; i < 3; i++) {
    digitalWrite(LED_RED, LOW);
    digitalWrite(BUZZER_PIN, HIGH);
    delay(100);
    digitalWrite(LED_RED, HIGH);
    digitalWrite(BUZZER_PIN, LOW);
    delay(100);
  }
}

// ================= HTTP HANDLERS =================
void sendJson(bool ok, const String& msg) {
  server.send(200, "application/json", "{\"ok\":" + String(ok ? "true" : "false") + ",\"msg\":\"" + msg + "\"}");
}

void handleRoot() {
  server.send_P(200, "text/html", PAGE_HTML);
}

void handleStatus() {
  String json = "{\"locked\":" + String(isLocked ? "true" : "false") +
                ",\"lockout\":" + String(lockoutRemainingSec()) + "}";
  server.send(200, "application/json", json);
}

void handleUnlock() {
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
    failCount = 0;
    unlockDoor();
    sendJson(true, "Mở khóa thành công!");
  } else {
    failCount++;
    Serial.printf("[WARN] Sai PIN lan %d\n", failCount);
    wrongPinAlert();
    if (failCount >= MAX_FAILS) {
      failCount = 0;
      lockedOut = true;
      lockoutStart = millis();
      sendJson(false, "Sai " + String(MAX_FAILS) + " lần! Tạm khóa " + String(LOCKOUT_MS / 1000) + "s");
    } else {
      sendJson(false, "Sai PIN! Còn " + String(MAX_FAILS - failCount) + " lần thử");
    }
  }
}

void handleLock() {
  lockDoor();
  sendJson(true, "Đã khóa cửa");
}

void handleChangePin() {
  String oldPin = server.arg("old");
  String newPin = server.arg("new");
  oldPin.trim();
  newPin.trim();

  if (lockoutRemainingSec() > 0) {
    sendJson(false, "Đang bị tạm khóa");
    return;
  }
  if (!safeEquals(oldPin, currentPin)) {
    sendJson(false, "PIN cũ không đúng");
    return;
  }
  if (!isValidPin(newPin)) {
    sendJson(false, "PIN mới phải gồm 4-8 chữ số");
    return;
  }

  currentPin = newPin;
  prefs.putString("pin", currentPin);
  Serial.println("[INFO] Da doi PIN");
  sendJson(true, "Đổi PIN thành công!");
}

// ================= SETUP / LOOP =================
void setup() {
  Serial.begin(115200);

  pinMode(RELAY_PIN, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  lockServo.setPeriodHertz(50);
  lockServo.attach(SERVO_PIN, 500, 2400);

  prefs.begin("smartlock", false);
  currentPin = prefs.getString("pin", DEFAULT_PIN);

  lockDoor();

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
}

void loop() {
  server.handleClient();

  if (!isLocked && millis() - unlockedAt >= AUTO_LOCK_MS) {
    lockDoor();
  }
}
