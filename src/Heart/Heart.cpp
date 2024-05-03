#include "Heart.h"
#include <Wire.h>
#include "MAX30105.h"
#include "heartRate.h"
#include "spo2_algorithm.h"
// bấm =0; nhả =1 hở
#define DOWN_BUTTON_PIN 14 

MAX30105 particleSensor;

int32_t spo2; //SPO2 value
int8_t validSPO2; //indicator to show if the SPO2 calculation is valid
int32_t heartRate; //heart rate value
int8_t validHeartRate; //indicator to show if the heart rate calculation is valid

void setup_heart()
{
  Serial.begin(115200);
  Serial.println("Initializing...");
  pinMode(DOWN_BUTTON_PIN, INPUT_PULLUP);//nút bấm điện trở kéo lên vì đã nối đất GND ở chân còn lại

  // Initialize sensor
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) //Use default I2C port, 400kHz speed
  {
    Serial.println("MAX30105 was not found. Please check wiring/power. ");
    while (1);
  }
  //Ấn nút 14
  // Serial.println(F("Attach sensor to finger with rubber band. Press any key to start conversion"));
  // while (Serial.available() == 0) ; //wait until user presses a key
  // Serial.read();

  particleSensor.setup(); //Configure sensor with default settings
  particleSensor.setPulseAmplitudeRed(0x0A); //Turn Red LED to low to indicate sensor is running
  particleSensor.setPulseAmplitudeGreen(0); //Turn off Green LED

}

bool buttonPressed = false;
unsigned long buttonPressStartTime = 0;
unsigned long buttonReleaseTime;
unsigned long buttonHoldTime = 8000;
bool buttonWasPressed = false;

void loop_heart() {
  if (digitalRead(DOWN_BUTTON_PIN) == LOW && !buttonPressed) {
    // Nút DOWN_BUTTON_PIN được nhấn một lần
    buttonPressed = true; // Bật nút thành true
    buttonPressStartTime = millis();
    Serial.println("Button pressed. Starting program.");
    Serial.print("buttonPressStartTime: ");
    Serial.println(buttonPressStartTime);
  }
  if (digitalRead(DOWN_BUTTON_PIN) == HIGH && buttonPressed && !buttonWasPressed) {
    // Nút DOWN_BUTTON_PIN được thả và chưa cập nhật giá trị buttonReleaseTime trước đó
    buttonReleaseTime = millis();
    Serial.print("buttonReleaseTime: ");
    Serial.println(buttonReleaseTime);
    buttonPressed = false; // Bật nút thành true
    buttonWasPressed = true; // Đánh dấu rằng giá trị buttonReleaseTime đã được cập nhật
    buttonHoldTime = buttonReleaseTime - buttonPressStartTime;
    Serial.print("buttonHoldTime: ");
    Serial.println(buttonHoldTime);
  }

  if (digitalRead(DOWN_BUTTON_PIN) == HIGH && !buttonPressed) {
    // Nút DOWN_BUTTON_PIN không được nhấn
    buttonWasPressed = false; // Đặt lại trạng thái của buttonWasPressed
  }

  if (buttonPressed ||buttonHoldTime < 5000) {
      // Chương trình đang chạy, thực hiện các hành động mong muốn
      // Đọc dữ liệu từ cảm biến và tính toán HR và SPO2
      while (particleSensor.available() == false) { // Có dữ liệu mới?
        particleSensor.check(); // Kiểm tra cảm biến có dữ liệu mới hay không
      }

      long redBuffer = particleSensor.getRed();
      long irBuffer = particleSensor.getIR();
      particleSensor.nextSample(); // Hoàn tất mẫu này, chuyển sang mẫu kế tiếp

      // Gửi các mẫu và tính toán HR và SPO2
      Serial.print(F("red="));
      Serial.print(redBuffer, DEC);
      Serial.print(F(", ir="));
      Serial.print(irBuffer, DEC);
      if (irBuffer < 50000) {
        Serial.print(" No finger?");
      }

      Serial.print(F(", HR="));
      Serial.print(heartRate, DEC);

      Serial.print(F(", HRvalid="));
      Serial.print(validHeartRate, DEC);

      Serial.print(F(", SPO2="));
      Serial.print(spo2, DEC);

      Serial.print(F(", SPO2Valid="));
      Serial.println(validSPO2, DEC);
      delay(1000);
  }
  if (buttonHoldTime >= 5000) {
    // Đã giữ nút quá 2 giây
    Serial.println("Button held for more than 5 seconds. Stopping program.");
    delay(1000);
  }
}