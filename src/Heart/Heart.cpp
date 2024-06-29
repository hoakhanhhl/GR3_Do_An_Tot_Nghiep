#include "Heart.h"
#include <Wire.h>
#include "MAX30105.h"
#include "heartRate.h"
#include "spo2_algorithm.h"
#include "../variables.h"
#include "./Oled/Oled.h"
#include "MPU6050/MPU6050.h" //MPU6050

MAX30105 particleSensor;
#define MAX_BRIGHTNESS 255
#define DOWN_BUTTON_PIN 14 // bấm =0; nhả =1 hở

void setup_heart()
{
  Serial.println("Initializing...");
  pinMode(DOWN_BUTTON_PIN, INPUT_PULLUP); // nút bấm điện trở kéo lên vì đã nối đất GND ở chân còn lại
  // Initialize sensor
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) // Use default I2C port, 400kHz speed
  {
    Serial.println("MAX30105 was not found. Please check wiring/power. ");
    while (1);
  }

  byte ledBrightness = 60;                                                                       // Options: 0=Off to 255=50mA
  byte sampleAverage = 4;                                                                        // Options: 1, 2, 4, 8, 16, 32
  byte ledMode = 2;                                                                              // Options: 1 = Red only, 2 = Red + IR, 3 = Red + IR + Green
  byte sampleRate = 100;                                                                         // Options: 50, 100, 200, 400, 800, 1000, 1600, 3200
  int pulseWidth = 411;                                                                          // Options: 69, 118, 215, 411
  int adcRange = 4096;                                                                           // Options: 2048, 4096, 8192, 16384
  particleSensor.setup(ledBrightness, sampleAverage, ledMode, sampleRate, pulseWidth, adcRange); // Configure sensor with these settings
}

bool buttonPressed = false;
unsigned long buttonPressStartTime = 0;
unsigned long buttonReleaseTime;
unsigned long buttonHoldTime = 8000;
bool buttonWasPressed = false;
unsigned long previousMillis = 1000;
String heartRate_payload;

void loop_heart()
{
  if (digitalRead(DOWN_BUTTON_PIN) == LOW && !buttonPressed)
  {// Nút DOWN_BUTTON_PIN được nhấn một lần
    buttonPressed = true; // Bật nút thành true
    buttonPressStartTime = millis();
    Serial.println("Button pressed.");
  }
  if (digitalRead(DOWN_BUTTON_PIN) == HIGH && !buttonPressed)
  {// Nút DOWN_BUTTON_PIN không được nhấn
    buttonWasPressed = false; // Đặt lại trạng thái của buttonWasPressed
  }
  if (digitalRead(DOWN_BUTTON_PIN) == HIGH && buttonPressed && !buttonWasPressed)
  {// Nút DOWN_BUTTON_PIN được thả và chưa cập nhật giá trị buttonReleaseTime trước đó
    buttonReleaseTime = millis();
    buttonPressed = false;   // Bật nút thành true
    buttonWasPressed = true; // Đánh dấu rằng giá trị buttonReleaseTime đã được cập nhật
    buttonHoldTime = buttonReleaseTime - buttonPressStartTime;
    printf("\n button pressed and end %ld\n", buttonHoldTime);
  }
  if (buttonHoldTime >= 5000 && buttonWasPressed)
  {// Đã giữ nút quá 2 giây
    Serial.println("Button held for more than 5 seconds. Stopping program.");
    heartRate_payload = "";
  }
  if (buttonPressed || buttonHoldTime < 5000)
  { 
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >1000) {
      // Read IR (infrared) and Red LED values 
      int32_t irValue = particleSensor.getIR();
      int32_t redValue = particleSensor.getRed();
      float spO2;
      float heartRate;
      float ratio = (float)redValue / (float)irValue;
      
      if (redValue <50000){
        Serial.print("No finger?\n");
        spO2 = 0;
        heartRate = 0;
      }else{
        spO2 = 100 - (17 * ratio/100);
        heartRate = irValue / 2000.0;
      }
      // Print the values to the Serial Monitor
      Serial.print("IR Value: ");
      Serial.print(irValue);
      Serial.print("\t");

      Serial.print("Red Value: ");
      Serial.print(redValue);
      Serial.print("\t");

      Serial.print("SpO2: ");
      Serial.print(spO2);
      Serial.print("%\t");

      Serial.print("Heart Rate: ");
      Serial.print(heartRate);
      Serial.print("bpm\n");
      Serial.println("=======================================================\n");

      heartRate_payload = "," + String(irValue) + "," + String(redValue) + "," + String(spO2) + "," + String(heartRate);
      update_Oled(irValue, spO2, heartRate);
      previousMillis = currentMillis;
      delay(1000);                     
    }
  }
}
