#include <OneWire.h>
#include <DallasTemperature.h>

// Chân kết nối với dây DATA của DS18B20
#define ONE_WIRE_BUS 19  // GPIO19

// Thiết lập giao tiếp OneWire
OneWire oneWire(ONE_WIRE_BUS);

// Truyền giao tiếp oneWire cho thư viện DallasTemperature
DallasTemperature sensors(&oneWire);

void setup() {
  Serial.begin(115200);
  Serial.println("Đang khởi động cảm biến DS18B20...");
  sensors.begin();  // Khởi động cảm biến DS18B20
  delay(1000);      // Chờ cảm biến sẵn sàng
}

void loop() {
  sensors.requestTemperatures(); // Gửi lệnh đọc nhiệt độ
  float tempC = sensors.getTempCByIndex(0); // Đọc nhiệt độ đầu tiên

  if (tempC == DEVICE_DISCONNECTED_C) {
    Serial.println("Không đọc được nhiệt độ! Cảm biến chưa sẵn sàng hoặc bị ngắt kết nối.");
  } else {
    Serial.print("Nhiệt độ: ");
    Serial.print(tempC);
    Serial.println(" °C");

    // Phân loại trạng thái nước dựa theo ngưỡng
    if (tempC <= 7.0) {
      Serial.println("=> Nước lạnh!");
    } else if (tempC >= 65.0) {
      Serial.println("=> Nước nóng!");
    } else {
      Serial.println("=> Nước bình thường.");
    }
  }

  delay(1000); // Đọc mỗi giây
}
