#define TURBIDITY_PIN 35  // Cảm biến độ đục nối chân A0 (GPIO 35)

// Hàm chuyển đổi giá trị ADC thành NTU
float mapNTU(int adc_value) {
  // Giới hạn giá trị ADC để tránh sai số
  if (adc_value > 3029) adc_value = 3029;   // Tương ứng 2.44V → NTU = 0
  if (adc_value < 500) adc_value = 500;     // Tương ứng NTU = 200

  // Ánh xạ tuyến tính: ADC = 3029 (trong) → NTU = 0, ADC = 500 (đục) → NTU = 200
  float ntu = (3029.0 - adc_value) * 200.0 / (3029.0 - 500.0);
  return ntu;
}

// Hàm phân loại mức độ đục theo chuẩn WHO
String classifyNTU(float ntu) {
  if (ntu <= 1.0) return "Rất trong";
  else if (ntu <= 5.0) return "Hơi đục";
  else if (ntu <= 50.0) return "Đục vừa phải";
  else if (ntu <= 100.0) return "Rất đục";
  else return "Cực kỳ đục";
}

void setup() {
  Serial.begin(115200);
  analogReadResolution(12);  // Độ phân giải 12 bit (0–4095)
}

void loop() {
  int adc = analogRead(TURBIDITY_PIN);
  float voltage = adc * 3.3 / 4095.0;
  float ntu = mapNTU(adc);

  Serial.print("ADC: ");
  Serial.print(adc);
  Serial.print(" | Voltage: ");
  Serial.print(voltage, 2);
  Serial.print(" V | NTU: ");
  Serial.print(ntu, 2);
  Serial.print(" | Mức độ: ");
  Serial.println(classifyNTU(ntu));

  delay(1000);  // Đo mỗi 1 giây
}
