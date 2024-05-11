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

#if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega168__)
// Arduino Uno doesn't have enough SRAM to store 100 samples of IR led data and red led data in 32-bit format
// To solve this problem, 16-bit MSB of the sampled data will be truncated. Samples become 16-bit data.
uint16_t irBuffer[100];  // infrared LED sensor data
uint16_t redBuffer[100]; // red LED sensor data
#else
uint32_t irBuffer[100];  // infrared LED sensor data
uint32_t redBuffer[100]; // red LED sensor data
#endif

int32_t bufferLength;  // data length
int32_t spo2;          // SPO2 value
int8_t validSPO2;      // indicator to show if the SPO2 calculation is valid
int32_t heartRate;     // heart rate value
int8_t validHeartRate; // indicator to show if the heart rate calculation is valid

void setup_heart()
{
  Serial.println("Initializing...");
  pinMode(DOWN_BUTTON_PIN, INPUT_PULLUP); // nút bấm điện trở kéo lên vì đã nối đất GND ở chân còn lại
  // Initialize sensor
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) // Use default I2C port, 400kHz speed
  {
    Serial.println("MAX30105 was not found. Please check wiring/power. ");
    while (1)
      ;
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

void loop_heart()
{
  const PROGMEM char *mqttTopic = "dulieu";
  if (digitalRead(DOWN_BUTTON_PIN) == LOW && !buttonPressed)
  {
    // Nút DOWN_BUTTON_PIN được nhấn một lần
    buttonPressed = true; // Bật nút thành true
    buttonPressStartTime = millis();
    Serial.println("Button pressed. Starting program.");
  }
  // if (digitalRead(DOWN_BUTTON_PIN) == HIGH && buttonPressed && !buttonWasPressed)
  // {
  //   // Nút DOWN_BUTTON_PIN được thả và chưa cập nhật giá trị buttonReleaseTime trước đó
  //   buttonReleaseTime = millis();
  //   buttonPressed = false;   // Bật nút thành true
  //   buttonWasPressed = true; // Đánh dấu rằng giá trị buttonReleaseTime đã được cập nhật
  //   buttonHoldTime = buttonReleaseTime - buttonPressStartTime;
  // }

  if (digitalRead(DOWN_BUTTON_PIN) == HIGH && !buttonPressed)
  {
    // Nút DOWN_BUTTON_PIN không được nhấn
    buttonWasPressed = false; // Đặt lại trạng thái của buttonWasPressed
  }

  if (buttonPressed || buttonHoldTime < 5000)
  {
    bufferLength = 50; // buffer length of 100 stores 4 seconds of samples running at 25sps
    // read the first 100 samples, and determine the signal range
    for (byte i = 0; i < bufferLength; i++)
    {
      while (particleSensor.available() == false) // do we have new data?
        particleSensor.check();                   // Check the sensor for new data

      redBuffer[i] = particleSensor.getRed();
      irBuffer[i] = particleSensor.getIR();
      particleSensor.nextSample(); // We're finished with this sample so move to next sample

      Serial.print(F("red="));
      Serial.print(redBuffer[i], DEC);
      Serial.print(F(", ir="));
      Serial.println(irBuffer[i], DEC);
      if (irBuffer[i] < 50000)
        Serial.print(" No finger?");
    }

    // calculate heart rate and SpO2 after first 100 samples (first 4 seconds of samples)
    maxim_heart_rate_and_oxygen_saturation(irBuffer, bufferLength, redBuffer, &spo2, &validSPO2, &heartRate, &validHeartRate);
    // dat lai bo dem thoi gian
    buttonPressed = false;
    buttonPressStartTime = 0;
    buttonReleaseTime = 0;
    buttonHoldTime = 0;
    buttonWasPressed = false;
    byte i = 25;
    byte j = 75;
    // Continuously taking samples from MAX30102.  Heart rate and SpO2 are calculated every 1 second
    while (1)
    { // In dữ liệu ra senior
      if (digitalRead(DOWN_BUTTON_PIN) == HIGH && !buttonPressed)
      {
        // Nút DOWN_BUTTON_PIN không được nhấn
        buttonWasPressed = false; // Đặt lại trạng thái của buttonWasPressed
      }
      if (digitalRead(DOWN_BUTTON_PIN) == LOW && !buttonPressed)
      {
        // Nút DOWN_BUTTON_PIN được nhấn một lần
        buttonPressed = true; // Bật nút thành true
        buttonPressStartTime = millis();
      }

      if (digitalRead(DOWN_BUTTON_PIN) == HIGH && buttonPressed && !buttonWasPressed)
      {
        // Nút DOWN_BUTTON_PIN được thả và chưa cập nhật giá trị buttonReleaseTime trước đó
        buttonReleaseTime = millis();
        buttonPressed = false;   // Bật nút thành true
        buttonWasPressed = true; // Đánh dấu rằng giá trị buttonReleaseTime đã được cập nhật
        buttonHoldTime = buttonReleaseTime - buttonPressStartTime;
        printf("\n button pressed and end %ld\n", buttonHoldTime);
      }
      if (buttonHoldTime >= 5000)
      {
        // Đã giữ nút quá 2 giây
        Serial.println("Button held for more than 5 seconds. Stopping program.");
        break;
      }
      // dumping the first 25 sets of samples in the memory and shift the last 75 sets of samples to the top
      if (i < 100)
      {
        redBuffer[i - 25] = redBuffer[i];
        irBuffer[i - 25] = irBuffer[i];
      }

      while (particleSensor.available() == false) // do we have new data?
        particleSensor.check();                   // Check the sensor for new data

      redBuffer[j] = particleSensor.getRed();
      irBuffer[j] = particleSensor.getIR();
      particleSensor.nextSample(); // We're finished with this sample so move to next sample
      update_Oled(irBuffer[j], heartRate, spo2);
      delay(1000);
      // send samples and calculation result to terminal program through UART
      Serial.print(F("red="));
      Serial.print(redBuffer[j], DEC);
      Serial.print(F(", ir="));
      Serial.print(irBuffer[j], DEC);

      Serial.print(F(", HR="));
      Serial.print(heartRate, DEC);

      Serial.print(F(", HRvalid="));
      Serial.print(validHeartRate, DEC);

      Serial.print(F(", SPO2="));
      Serial.print(spo2, DEC);

      Serial.print(F(", SPO2Valid="));
      Serial.println(validSPO2, DEC);

      if (irBuffer[j] < 50000)
        Serial.print(" No finger?");
      String uploadValue = loop_MPU6050();
      // uploadValue = uploadValue + "," + String(irBuffer[j]) + "," + String(heartRate) + "," + String(spo2);
      // Serial.println(uploadValue.c_str());
       // client.publish(mqttTopic, uploadValue.c_str());

      j++;
      i++;
      // take 25 sets of samples before calculating the heart rate.
      if (j == 100)
      {
        Serial.println("Xu ly du lieu sau 25 lan.");
        maxim_heart_rate_and_oxygen_saturation(irBuffer, bufferLength, redBuffer, &spo2, &validSPO2, &heartRate, &validHeartRate);
        j = 75;
      }
      if (i == 100)
      {
        i = 25;
      }
    }
  }
}
