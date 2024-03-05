#include <Arduino.h>        //Phải khai báo thêm nếu sử dụng Visual Studio Code + PlatformIO  á
#include <Wire.h>
#include <MPU6050_tockn.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <Preferences.h>
#include <WebServer.h>  

//----------------------------------------------------------------
// CẤU HÌNH 
//----------------------------------------------------------------

/// Sử dụng thư viện SOICT_CORE_BOARD để có tính năng cấu hình và ghi nhớ cấu hình mạng WiFi
#define ENABLE_SIOT_WIFI

/// Sử dụng MQTT
#define ENABLE_MQTT

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

#if defined(ENABLE_MQTT)
  // Thông tin kết nối MQTT  Mosquitto
  // PROGMEM để hằng kí tự nằm trong bộ nhớ chương trình flash, thay vì bộ nhớ dữ liệu ram
  const char* mqttServer = "192.168.42.105";
  const int mqttPort = 1883;
  const PROGMEM char* mqttTopic = "dulieu";
  const PROGMEM char* mqttUser = "hoaltk";
  const PROGMEM char* mqttPassword = "123456";
#endif

#if defined (ENABLE_MQTT)
  // Đối tượng MQTT
  WiFiClient espClient;
  PubSubClient client(espClient);
#endif

// Đối tượng cảm biến MPU6050
MPU6050 mpu6050(Wire);

#define MPU6050_SDA_PIN 19  
#define MPU6050_SCL_PIN 22

long timer = 1000;
// Đối tượng Preferences
Preferences preferences;

// Mã định danh thiết bị
String deviceID;

void setup() {
  Serial.begin(115200);

#if defined(ENABLE_SIOT_WIFI)
  // Make sure WiFi ssid/password is correct. Otherwise, raise the Adhoc AP Station with ssid = SOICT_CORE_BOARD and password =  12345678
  MyWiFi->setup("KHANHHOA_MEDTECH","12345678");
  // Release the memory allocated for WiFi Station Handler after finishing his work
  delete  MyWiFi;
  MyWiFi = _NULL;
#else
  //setup_wifi();
#endif  

  Wire.begin(MPU6050_SDA_PIN, MPU6050_SCL_PIN);  // Ánh xạ chân Pin của CPU với các chân I2C của MPU 
  delay(2000); 
  // Khởi động cảm biến MPU6050
  mpu6050.begin();
  // Tính toán và hiệu chuẩn giá trị offset của con quay quanh các trục (gyroscope)
  mpu6050.calcGyroOffsets(true);

#if defined(ENABLE_MQTT)
  // Kết nối MQTT
  client.setServer(mqttServer, mqttPort);
  while (!client.connected()) {
    if (client.connect("mqttServer, mqttUser, mqttPassword")) {
      Serial.println("Connected to MQTT Broker");
    } else {
      Serial.println("MQTT connection failed! Error code = ");
      delay(2000);
    }
  }
  Serial.println("You're connected to the MQTT broker!");
  Serial.println();
#endif

  // Lấy địa chỉ MAC của ESP32 làm mã định danh
  deviceID = WiFi.macAddress();

  // Khởi tạo Preferences
  preferences.begin("myApp", false);

  // Lưu trữ mã định danh vào bộ nhớ flash
  preferences.putString("deviceID", deviceID);
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
        String payload = deviceID + ","
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