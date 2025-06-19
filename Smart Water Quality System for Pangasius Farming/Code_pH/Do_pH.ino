#include <U8g2lib.h>
#include <Wire.h>

// OLED SSD1306 128x64 I2C (phổ biến)
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

// Cảm biến pH gắn vào GPIO 34
#define PH_PIN 34
float offset = 0.0; // Điều chỉnh pH nếu cần

void setup() {
  Serial.begin(115200);
  u8g2.begin();

  // Màn hình khởi động
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB08_tr);
  u8g2.drawStr(10, 30, "Dang khoi dong...");
  u8g2.sendBuffer();
  delay(1500);
}

void loop() {
  int raw = analogRead(PH_PIN);
  float voltage = raw * (3.3 / 4095.0);
  float pH = 3.5 * voltage + offset;

  // Serial debug
  Serial.print("ADC: ");
  Serial.print(raw);
  Serial.print(" | Voltage: ");
  Serial.print(voltage, 3);
  Serial.print(" V | pH: ");
  Serial.println(pH, 2);

  // Hiển thị lên OLED
  u8g2.clearBuffer();

  u8g2.setFont(u8g2_font_fub11_tr);    // Font lớn hơn cho pH
  char buffer[32];
  sprintf(buffer, "pH: %.3f", pH);
  u8g2.drawStr(0, 30, buffer);

  sprintf(buffer, "V: %.4f V", voltage);
  u8g2.drawStr(0, 48, buffer);

  sprintf(buffer, "ADC: %d", raw);
  u8g2.setFont(u8g2_font_6x10_tr);     // Nhỏ hơn cho ADC
  u8g2.drawStr(0, 62, buffer);

  u8g2.sendBuffer();

  delay(1000);
}
