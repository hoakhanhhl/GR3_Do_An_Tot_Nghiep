#include <Arduino.h>        //Phải khai báo thêm nếu sử dụng Visual Studio Code + PlatformIO  á
#include <Wire.h>
#include <SPI.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <Preferences.h>
#include <WebServer.h>  
#include <iostream>
#include <chrono>
#include "Heart/Heart.h" //Oxygen, heartrate
#include "Oled/Oled.h" // Oled
#include "MPU6050/MPU6050.h" //MPU6050
#include "variables.h"
//----------------------------------------------------------------
// CẤU HÌNH 
//----------------------------------------------------------------

/// Sử dụng thư viện SOICT_CORE_BOARD để có tính năng cấu hình và ghi nhớ cấu hình mạng WiFi
#define ENABLE_SIOT_WIFI
/// Sử dụng MQTT
#define ENABLE_MQTT
// = PIN VP còi reo báo hiệu 
#define BUZZER_BUTTON_PIN 33 
//pin của MPU6050 và OLED, MAX30102
#define SDA_PIN 13 
#define SCL_PIN 15 

#define OLED
#define MPU6050
#define HEARTRATE


#if defined(ENABLE_SIOT_WIFI)
    #include "WiFiSelfEnroll.h"  // SIOT Core Lib - seft setup wifi network
    // Handler adhoc wifi station
    WiFiSelfEnroll* MyWiFi = new WiFiSelfEnroll();
#endif

#if defined(ENABLE_MQTT)
  // Thông tin kết nối MQTT  Mosquitto
  // PROGMEM để hằng kí tự nằm trong bộ nhớ chương trình flash, thay vì bộ nhớ dữ liệu ram
  const char* mqttServer = "dev.techlinkvn.com";
  const int mqttPort = 1883;
  const PROGMEM char* mqttTopic = "dulieu";
  const PROGMEM char* mqttUser = "student";
  const PROGMEM char* mqttPassword = "sinhvien";
#endif

#if defined (ENABLE_MQTT)
  // Đối tượng MQTT
  WiFiClient espClient;
  PubSubClient client(espClient);
#endif

String deviceID;
String patientID;
extern String mpu6050_payload;
extern String heartRate_payload;
// Đối tượng Preferences
Preferences preferences;
// Mã định danh thiết bị
void setup() {
  Serial.begin(115200);
  Wire.begin(SDA_PIN, SCL_PIN);  // Ánh xạ chân Pin của CPU với các chân I2C của MPU6050 và OLED, MAX30102
  pinMode(BUZZER_BUTTON_PIN, OUTPUT); //chuông
  delay(2000);

  // Lấy địa chỉ MAC của ESP32 làm mã định danh
  deviceID = WiFi.macAddress();
  // Lấy địa chỉ ID của bệnh nhân
  patientID = MyWiFi->GetPatientID();
  // Khởi tạo Preferences
  preferences.begin("myApp", false);
  // Lưu trữ mã định danh vào bộ nhớ flash
  preferences.putString("deviceID", deviceID);
  preferences.putString("patientID", patientID);


    #if defined(ENABLE_SIOT_WIFI) 
  {
      // Make sure WiFi ssid/password is correct. Otherwise, raise the Adhoc AP Station with
      // ssid = KHANHHOA_MEDTECH and password =  12345678
      MyWiFi->setup("KHANHHOA_MEDTECH","12345678");
      // Release the memory allocated for WiFi Station Handler after finishing his work
      delete MyWiFi;
      MyWiFi = NULL;
    #endif
  }
   {  // Còi đã set wifi thành công
    digitalWrite(BUZZER_BUTTON_PIN, 1);
    delay(1000);
    digitalWrite(BUZZER_BUTTON_PIN, 0);
    delay(300);
  }

  #if defined(ENABLE_MQTT)
    // Kết nối MQTT
    client.setServer(mqttServer, mqttPort);
    if (!client.connect(mqttServer, mqttUser, mqttPassword)) {
      Serial.print("MQTT connection failed! Error code = ");
    } else {
        Serial.println("Connected to MQTT Broker");
        delay(1000);
    }
    Serial.println();
  #endif

  #if defined(OLED)
    patientID = MyWiFi->GetPatientID();
    setup_Oled();
  #endif

  #if defined(MPU6050)
    patientID = MyWiFi->GetPatientID();
    setup_MPU6050();
  #endif

  #if defined(HEARTRATE)
    setup_heart();
  #endif
}

void loop() {
  #pragma region MPU6050_READER  
    #if defined(MPU6050)
      loop_MPU6050(); // In dữ liệu ra senior
    #endif           
  #pragma endregion MPU6050_READER
   
  #pragma region HEART_RATE_READER
    #if defined(HEARTRATE)
      loop_heart(); // In dữ liệu ra senior
    #endif
  #pragma endregion HEART_RATE_READER

  #pragma region Oled_READER
    #if defined(OLED)
      patientID = MyWiFi->GetPatientID();
    #endif
  #pragma endregion Oled_READER

  #if defined(ENABLE_MQTT)
    String data_payload = mpu6050_payload + heartRate_payload;
    Serial.println(data_payload);
    client.publish(mqttTopic, data_payload.c_str()); //Gửi dữ liệu mpu6050 lên MQTT
  #endif
  
};
