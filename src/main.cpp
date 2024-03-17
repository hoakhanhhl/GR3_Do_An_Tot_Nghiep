#include <Arduino.h>        //Phải khai báo thêm nếu sử dụng Visual Studio Code + PlatformIO  á
#include <Wire.h>
#include <MPU6050_tockn.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <Preferences.h>
#include <WebServer.h>  
#include <iostream>
#include <chrono>
//----------------------------------------------------------------
// CẤU HÌNH 
//----------------------------------------------------------------

/// Sử dụng thư viện SOICT_CORE_BOARD để có tính năng cấu hình và ghi nhớ cấu hình mạng WiFi
#define ENABLE_SIOT_WIFI
/// Sử dụng MQTT
#define ENABLE_MQTT
//các chân mặc định có sẵn trên board ở GPIO 22, 19
#define MPU6050_SDA_PIN 19  
#define MPU6050_SCL_PIN 22


#if defined(ENABLE_SIOT_WIFI)
    #include "WiFiSelfEnroll.h"  // SIOT Core Lib - seft setup wifi network
    // Handler adhoc wifi station
    WiFiSelfEnroll * MyWiFi = new WiFiSelfEnroll();
#endif

#if defined(ENABLE_MQTT)
  // Thông tin kết nối MQTT  Mosquitto
  // PROGMEM để hằng kí tự nằm trong bộ nhớ chương trình flash, thay vì bộ nhớ dữ liệu ram
  const char* mqttServer = "sinno.soict.ai";
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


// Đối tượng cảm biến MPU6050
MPU6050 mpu6050(Wire);
long timer = 1000;
// Đối tượng Preferences
Preferences preferences;
// Mã định danh thiết bị
String deviceID;
String patientID;

void setup() {
  Serial.begin(115200);
  Wire.begin(MPU6050_SDA_PIN, MPU6050_SCL_PIN);  // Ánh xạ chân Pin của CPU với các chân I2C của MPU 
  delay(2000); 
  // Khởi động cảm biến MPU6050
  mpu6050.begin();
  // Tính toán và hiệu chuẩn giá trị offset của con quay quanh các trục (gyroscope)
  mpu6050.calcGyroOffsets(true);

  #if defined(ENABLE_SIOT_WIFI)
    // Make sure WiFi ssid/password is correct. Otherwise, raise the Adhoc AP Station with ssid = SOICT_CORE_BOARD and password =  12345678
    MyWiFi->setup("KHANHHOA_MEDTECH","12345678");
    // Release the memory allocated for WiFi Station Handler after finishing his work
    delete  MyWiFi;
    MyWiFi = _NULL;
  #else
    //setup_wifi();
  #endif  

  #if defined(ENABLE_MQTT)
    // Kết nối MQTT
    client.setServer(mqttServer, mqttPort);
    if (!client.connect(mqttServer, mqttUser, mqttPassword)) {
      Serial.print("MQTT connection failed! Error code = ");
    } else {
        Serial.println("Connected to MQTT Broker");
        delay(2000);
    }
    Serial.println();
  #endif

  // Lấy địa chỉ MAC của ESP32 làm mã định danh
  deviceID = WiFi.macAddress();
  // Lấy địa chỉ ID của bệnh nhân
  patientID = MyWiFi->GetPatientID();
  // Khởi tạo Preferences
  preferences.begin("myApp", false);
  // Lưu trữ mã định danh vào bộ nhớ flash
  preferences.putString("deviceID", deviceID);
  preferences.putString("patientID", patientID);
}

void loop() {
  #pragma region MPU6050_READER  
    mpu6050.update();
    if(millis() - timer > 1000){
          // Lấy giá trị gia tốc, góc quay và cảm biến từ trường theo x,y,z
          float accx = mpu6050.getAccX();
          float accy = mpu6050.getAccY();
          float accz = mpu6050.getAccZ();
          float gyrox = mpu6050.getGyroX();
          float gyroy = mpu6050.getGyroY();
          float gyroz = mpu6050.getGyroZ();
          float angx = mpu6050.getGyroAngleX();
          float angy = mpu6050.getGyroAngleY();
          float angz = mpu6050.getGyroAngleZ();

          // Gửi dữ liệu lên MQTT
          String payload = deviceID + "," + patientID + ","
                        + String(accx) + "," + String(accy) + "," + String(accz) + ","
                        + String(gyrox) + "," + String(gyroy) + "," + String(gyroz) + ","
                        + String(angx) + "," + String(angy) + "," + String(angz);

          #if defined(ENABLE_MQTT)
            client.publish(mqttTopic, payload.c_str());
          #endif
          Serial.println(payload);
          timer = millis();
          // Đợi 1 giây trước khi gửi dữ liệu tiếp theo
          delay(1000);
        }           

  #pragma endregion MPU6050_READER
};