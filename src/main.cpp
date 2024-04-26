#include <Arduino.h>        //Phải khai báo thêm nếu sử dụng Visual Studio Code + PlatformIO  á
#include <Wire.h>
#include <SPI.h>
#include <MPU6050_tockn.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <Preferences.h>
#include <WebServer.h>  
#include <iostream>
#include <chrono>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
//----------------------------------------------------------------
// CẤU HÌNH 
//----------------------------------------------------------------

/// Sử dụng thư viện SOICT_CORE_BOARD để có tính năng cấu hình và ghi nhớ cấu hình mạng WiFi
#define ENABLE_SIOT_WIFI
/// Sử dụng MQTT
#define ENABLE_MQTT
//các chân mặc định có sẵn trên board ở GPIO 22, 19

// bấm =0; nhả =1 hở
#define DOWN_BUTTON_PIN 14 
// = PIN VP còi reo báo hiệu 
#define BUZZER_BUTTON_PIN 33 
//màn led  0.91in white 4 in

//pin của mpu6050 và oled
#define SDA_PIN 13 
#define SCL_PIN 15 

#define OLED
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C // Địa chỉ I2C của màn hình OLED


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

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);


void setup() {
  Serial.begin(115200);
  Wire.begin(SDA_PIN, SCL_PIN);  // Ánh xạ chân Pin của CPU với các chân I2C của MPU và OLED
  pinMode(DOWN_BUTTON_PIN, INPUT_PULLUP);//nút bấm điện trở kéo lên vì đã nối đất GND ở chân còn lại
  pinMode(BUZZER_BUTTON_PIN, OUTPUT);
  delay(2000);

  // Khởi động cảm biến MPU6050
  mpu6050.begin();
  // Tính toán và hiệu chuẩn giá trị offset của con quay quanh các trục (gyroscope)
  mpu6050.calcGyroOffsets(true);

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
      // Make sure WiFi ssid/password is correct. Otherwise, raise the Adhoc AP Station with ssid = SOICT_CORE_BOARD and password =  12345678
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
        delay(2000);
    }
    Serial.println();
  #endif

  #if defined(OLED)
    if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) { 
      Serial.println(F("SSD1306 allocation failed"));
      for(;;); // Don't proceed, loop forever
    }
    // Khởi tạo màn hình LED
    display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS);
    
    // Xóa màn hình
    display.clearDisplay();
    
    // Cài đặt kích thước và kiểu font chữ
    display.setTextSize(1.5);
    display.setTextColor(SSD1306_WHITE);
    
    // Lấy ID bệnh nhân từ WiFi
    patientID = MyWiFi->GetPatientID();
    
    // Hiển thị ID bệnh nhân lên màn hình
    display.setCursor(0, 0);
    display.print("ID: ");
    display.println(patientID);
    display.print("Device:");
    display.println(deviceID);
    // Cập nhật màn hình
    display.display();
          
  #endif

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