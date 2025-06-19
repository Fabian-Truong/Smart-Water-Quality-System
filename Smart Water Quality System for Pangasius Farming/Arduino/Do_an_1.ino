#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>
#include <U8g2lib.h>

// ==== WiFi & Firebase Info ====
#define WIFI_SSID "1412"
#define WIFI_PASSWORD "22224444"
#define API_KEY "AIzaSyCbf1-kAHwLadyC5Rvvzs_LfxZVeLw4_mg"
#define DATABASE_URL "https://tmh28-2f86d-default-rtdb.firebaseio.com/"
#define USER_EMAIL "tranminhhuua2@gmail.com"
#define USER_PASSWORD "01686909659n"

// ==== Firebase Setup ====
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// ==== OLED (U8g2) ====
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

// ==== Sensor Pins ====
#define ONE_WIRE_BUS 19
#define PH_SENSOR_PIN 34
#define TURBIDITY_PIN 35

// ==== Relay & LED ====
#define RELAY_FAN    32
#define RELAY_PUMP   33
#define RELAY_BUBBLE 27
#define STATUS_LED   23

// ==== DS18B20 ====
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature ds18b20(&oneWire);

// --------- GIÁ TRỊ HIỆU CHUẨN MỚI ---------
float voltage_acid   = 0.810;  float pH_acid   = 4.00;
float voltage_neutral= 0.778;  float pH_neutral= 7.50;
float voltage_base   = 0.720;  float pH_base   = 9.05;
// ------------------------------------------

// ==== Error Count ====
static int errorCount = 0;

// ==== Function: Đọc trung bình ADC ====
int readAverageADC(int pin, int samples = 10) {
  long total = 0;
  for (int i = 0; i < samples; i++) {
    total += analogRead(pin);
    delay(10);
  }
  return total / samples;
}

// ==== Function: Nội suy tuyến tính ====
float interpolate(float x, float x0, float y0, float x1, float y1) {
  return y0 + (x - x0) * (y1 - y0) / (x1 - x0);
}

// Hàm chuyển đổi giá trị ADC thành NTU (chuẩn hóa theo bảng WHO)
float mapNTU(int adc_value) {
  if (adc_value > 4095) adc_value = 4095;
  if (adc_value < 500) adc_value = 500;
  float ntu = (4095.0 - adc_value) * 200.0 / (4095.0 - 500.0);
  return ntu;
}

// Hàm phân loại mức độ đục theo chuẩn WHO
String classifyNTU(float ntu) {
  if (ntu <= 1.0) return "Rất trong ";
  else if (ntu <= 5.0) return "Hơi đục ";
  else if (ntu <= 50.0) return "Đục vừa ";
  else if (ntu <= 100.0) return "Rất đục ";
  else return "Cực kỳ đục";
}


void setup() {
  Serial.begin(115200);
  analogReadResolution(12);
  delay(1000);

  // WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Dang ket noi WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); Serial.print(".");
  }
  Serial.println("\nDa ket noi WiFi");

  // Firebase Config
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  // Check Firebase Connection
  Serial.print("Dang kiem tra ket noi Firebase");
  int retry = 0;
  while (!Firebase.ready()) {
    delay(500);
    Serial.print(".");
    retry++;
    if (retry > 20) {
      Serial.println("\nKhong the ket noi Firebase, reset ESP32!");
      delay(1000);
      ESP.restart();
    }
  }
  Serial.println("\nDa ket noi Firebase");

  // DS18B20
  ds18b20.begin();

  // OLED
  u8g2.begin();

  // I/O
  pinMode(RELAY_FAN, OUTPUT);
  pinMode(RELAY_PUMP, OUTPUT);
  pinMode(RELAY_BUBBLE, OUTPUT);
  pinMode(STATUS_LED, OUTPUT);
  digitalWrite(RELAY_FAN, HIGH);
  digitalWrite(RELAY_PUMP, HIGH);
  digitalWrite(RELAY_BUBBLE, HIGH);
  digitalWrite(STATUS_LED, LOW);
}

void loop() {
  bool isAuto = true;

  // 1. Read mode
  if (Firebase.RTDB.getString(&fbdo, "/basafish/mode")) {
    String mode = fbdo.stringData();
    mode.trim(); mode.toLowerCase();
    isAuto = (mode == "auto");
    Serial.println("Doc che do thanh cong: " + mode);
  } else {
    Serial.println("Loi doc mode");
    errorCount++;
  }

  // 2. Read sensors
  ds18b20.requestTemperatures();
  float tempC = ds18b20.getTempCByIndex(0)-3;
  delay(1000);

  int rawADC = readAverageADC(PH_SENSOR_PIN);
  float voltage = rawADC * 3.3 / 4095.0;
  float pHValue;

  // Dùng nội suy giữa các cặp điểm gần nhất
  if (voltage >= voltage_base && voltage <= voltage_neutral) {
    pHValue= interpolate(voltage, voltage_base, pH_base, voltage_neutral, pH_neutral);
  } else if (voltage > voltage_neutral && voltage <= voltage_acid) {
    pHValue = interpolate(voltage, voltage_neutral, pH_neutral, voltage_acid, pH_acid);
  } else {
    // Ngoài vùng hiệu chuẩn → nội suy toàn khoảng (ít chính xác hơn)
    pHValue = interpolate(voltage, voltage_base, pH_base, voltage_acid, pH_acid);
  }
  if (voltage < 0.05 || voltage > 3.2) {
    Serial.println("⚠️ Dien ap bat thuong! Kiem tra cam bien.");
  }
  if (pHValue>10.0){
    pHValue-=4;
  }else if (pHValue>7.0){
    pHValue+=2;
  }

  // Đọc giá trị ADC từ cảm biến độ đục
  int rawTurb = analogRead(TURBIDITY_PIN);
  float voltageTurb = rawTurb * 3.3 / 4095.0;
  float turbidityNTU = mapNTU(rawTurb);
  if (turbidityNTU < 0) turbidityNTU = 0;

  Serial.print("Nhiet do: "); Serial.println(tempC);
  Serial.print("pH: "); Serial.println(pHValue, 2);
  Serial.print("NTU: "); Serial.println(turbidityNTU, 2);

  // 3. Control logic
  bool fan_on = false, pump_on = false, bubble_on = false;
  if (isAuto) {
    fan_on = tempC > 30;
    pump_on = turbidityNTU > 50;
    bubble_on = (pHValue < 6.5 || pHValue > 8.5);
  } else {
    if (Firebase.RTDB.getBool(&fbdo, "/basafish/override/fan"))
      fan_on = fbdo.boolData();
    else { Serial.println("Loi doc override fan"); errorCount++; }

    if (Firebase.RTDB.getBool(&fbdo, "/basafish/override/pump"))
      pump_on = fbdo.boolData();
    else { Serial.println("Loi doc override pump"); errorCount++; }

    if (Firebase.RTDB.getBool(&fbdo, "/basafish/override/bubble"))
      bubble_on = fbdo.boolData();
    else { Serial.println("Loi doc override bubble"); errorCount++; }
  }

  // 4. Device control
  digitalWrite(RELAY_FAN, fan_on ? HIGH : LOW);
  digitalWrite(RELAY_PUMP, pump_on ? HIGH : LOW);
  digitalWrite(RELAY_BUBBLE, bubble_on ? HIGH : LOW);

  // LED cảnh báo khi thông số vượt ngưỡng
  bool warning = (tempC > 30) || (pHValue < 6.5 || pHValue > 8.5) || (turbidityNTU > 50);
  digitalWrite(STATUS_LED, warning ? HIGH : LOW);

  // 5. Send data to Firebase
  if (!Firebase.RTDB.setFloat(&fbdo, "/basafish/temp", tempC)) errorCount++;
  if (!Firebase.RTDB.setFloat(&fbdo, "/basafish/pH", pHValue)) errorCount++;
  if (!Firebase.RTDB.setFloat(&fbdo, "/basafish/turbidity", turbidityNTU)) errorCount++;

  if (!Firebase.RTDB.setBool(&fbdo, "/basafish/devices/fan", fan_on)) errorCount++;
  if (!Firebase.RTDB.setBool(&fbdo, "/basafish/devices/pump", pump_on)) errorCount++;
  if (!Firebase.RTDB.setBool(&fbdo, "/basafish/devices/bubble", bubble_on)) errorCount++;

  // 5.5. Log history
  unsigned long timestamp = millis() / 1000;
  String path = "/basafish/history/" + String(timestamp);
  FirebaseJson json;
  json.set("temp", tempC);
  json.set("pH", pHValue);
  json.set("turbidity", turbidityNTU);
  if (!Firebase.RTDB.setJSON(&fbdo, path, &json)) errorCount++;

  // Reset if too many errors
  if (errorCount >= 5) {
    Serial.println("Qua nhieu loi, reset ESP32!");
    delay(1000);
    ESP.restart();
  }

  // 6. OLED Display
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.setCursor(0, 10);
  u8g2.print("Mode: "); u8g2.print(isAuto ? "Auto" : "Manual");
  u8g2.setCursor(0, 22);
  u8g2.print("Temp: "); u8g2.print(tempC, 1); u8g2.print(" C");
  u8g2.setCursor(0, 34);
  u8g2.print("pH: "); u8g2.print(pHValue, 2);
  u8g2.setCursor(0, 46);
  u8g2.print("Turb: "); u8g2.print(turbidityNTU, 0);
  u8g2.sendBuffer();

  delay(2000);
}
