#include <Arduino.h>        //Phải khai báo thêm nếu sử dụng Visual Studio Code + PlatformIO
#include <Wire.h>
#include <MPU9250.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <Preferences.h>
#include <WebServer.h>  


//----------------------------------------------------------------
// CẤU HÌNH 
//----------------------------------------------------------------

/// Sử dụng thư viện SOICT_CORE_BOARD để có tính năng cấu hình và ghi nhớ cấu hình mạng WiFi
#define ENABLE_SIOT_WIFI

//----------------------------------------------------------------
// Led mặc định có sẵn trên board ở GPIO 22
#define LED_BUILTIN 22

#if defined(ENABLE_SIOT_WIFI)
    #include "WiFiSelfEnroll.h"  // SIOT Core Lib - seft setup wifi network
    // Handler adhoc wifi station
    WiFiSelfEnroll * MyWiFi = new WiFiSelfEnroll();
#else
    //Lưu thông tin kết nối WiFi cố định trong code, nếu không sử dụng thư viện SOICT_CORE_BOARD
    const char* ssid = "P402";
    const char* password = "88888888"; 
    /// Kết nối WiFi
    void setup_wifi() {
      Serial.print("Connecting to ");
      Serial.println(ssid);
      WiFi.begin(ssid, password);
      while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
      }
      Serial.println("");
      Serial.println("WiFi connected");
      Serial.println("IP address: ");
      Serial.println(WiFi.localIP());
    }
#endif

// Thông tin kết nối MQTT  Mosquitto
const char* mqttServer = "sinno.soict.ai";
const int mqttPort = 1883;
const char* mqttTopic = "dulieu";
const char* mqttUser = "hoaltk";
const char* mqttPassword = "123456";

// Đối tượng MQTT
WiFiClient espClient;
PubSubClient client(espClient);

// Đối tượng cảm biến MPU9250
MPU9250 mpu;

// Đối tượng Preferences
Preferences preferences;

// void setup_wifi() {
//   //just for debug
//   Serial.begin(115200);
//   // Make sure WiFi ssid/password is correct. Otherwise, raise the Adhoc AP Station with ssid = SOICT_CORE_BOARD and password =  12345678
//   MyWiFi->setup();
//   // Release the memory allocated for WiFi Station Handler after finishing his work
//   delete  MyWiFi;
//   MyWiFi = _NULL;
//   // TODO something
// }

void setup() {
  Serial.begin(115200);

#if defined(ENABLE_SIOT_WIFI)
  // Make sure WiFi ssid/password is correct. Otherwise, raise the Adhoc AP Station with ssid = SOICT_CORE_BOARD and password =  12345678
  MyWiFi->setup("KHANHHOA_MEDTECH","12345678");
  // Release the memory allocated for WiFi Station Handler after finishing his work
  delete  MyWiFi;
  MyWiFi = _NULL;
#else
  setup_wifi();
#endif  

  Wire.begin();  // Khởi tạo giao tiếp I2C
  delay(2000);
  mpu.setup(0x68);  // Cấu hình MPU9250 với địa chỉ I2C là 0x68

  // while check (button =1) in 2 giay {{
  //   nhập định danh qua usb
  // }}

  // Kết nối MQTT
  client.setServer(mqttServer, mqttPort);
  while (!client.connected()) {
    if (client.connect("ESP32Client", mqttUser, mqttPassword)) {
      Serial.println("Connected to MQTT Broker");
    } else {
      Serial.println("Failed to connect to MQTT Broker, retrying...");
      delay(2000);
    }
  }

  // Lấy địa chỉ MAC của ESP32 làm mã định danh
  String deviceID = WiFi.macAddress();

  // Khởi tạo Preferences
  preferences.begin("myApp", false);

  // Lưu trữ mã định danh vào bộ nhớ flash
  preferences.putString("deviceID", deviceID);
}

void loop() {
  // Serial.println(MyWiFi->GetDeviceID());
  // Serial.printf("  %s / %s \n", MyWiFi->GetSSID(), MyWiFi->GetPassword());
  // delay(1000);
  // Đọc dữ liệu từ cảm biến
  mpu.update();
  // Lấy giá trị gia tốc, góc quay và cảm biến từ trường theo x,y,z
  float accx = mpu.getAccX();
  float accy = mpu.getAccY();
  float accz = mpu.getAccZ();
  float gyrox = mpu.getGyroX();
  float gyroy = mpu.getGyroY();
  float gyroz = mpu.getGyroZ();
  float magx = mpu.getMagX();
  float magy = mpu.getMagY();
  float magz = mpu.getMagZ();
 

  // Gửi dữ liệu lên MQTT
  String payload = preferences.getString("deviceID", "") + ","
                   + String(accx) + "," + String(accy) + "," + String(accz) + ","
                   + String(gyrox) + "," + String(gyroy) + "," + String(gyroz) + ","
                   + String(magx) + "," + String(magy) + "," + String(magz);

  client.publish(mqttTopic, payload.c_str());

  Serial.println("Data sent to MQTT");
  // Đợi 1 giây trước khi gửi dữ liệu tiếp theo
  delay(5000);
};