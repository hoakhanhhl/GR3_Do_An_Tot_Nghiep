#include <Wire.h>
#include <MPU9250.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <Preferences.h>
#if true
  #include <siot_core_lib.h>   // SIOT Core Lib - all packages or you could select each package manually.
#else
  #include <WiFiSelfEnroll.h>  // SIOT Core Lib - seft setup wifi network
#endif

// Thông tin kết nối WiFi
// const char* ssid = "P402";
// const char* password = "88888888";

// Thông tin kết nối MQTT  Mosquitto
const char* mqttServer = "192.168.42.105";
const int mqttPort = 1883;
const char* mqttTopic = "dulieu";

// Đối tượng MQTT
WiFiClient espClient;
PubSubClient client(espClient);

// Đối tượng cảm biến MPU9250
MPU9250 mpu;

// TODO ADV: kết nối wifi, su dung lib SOICT CORE LIB
WiFiSelfEnroll * MyWiFi = new WiFiSelfEnroll();

// Đối tượng Preferences
Preferences preferences;

// Kết nối WiFi
// void setup_wifi() {
//   Serial.print("Connecting to ");
//   Serial.println(ssid);
//   WiFi.begin(ssid, password);
//   while (WiFi.status() != WL_CONNECTED) {
//     delay(500);
//     Serial.print(".");
//   }
//   Serial.println("");
//   Serial.println("WiFi connected");
//   Serial.println("IP address: ");
//   Serial.println(WiFi.localIP());
// }
void setup_wifi() {
  //just for debug
  Serial.begin(115200);
  // Make sure WiFi ssid/password is correct. Otherwise, raise the Adhoc AP Station with ssid = SOICT_CORE_BOARD and password =  12345678
  MyWiFi->setup();
  // Release the memory allocated for WiFi Station Handler after finishing his work
  delete  MyWiFi;
  MyWiFi = _NULL;
  // TODO something
}

void setup() {
  Serial.begin(115200);
  Wire.begin();  // Khởi tạo giao tiếp I2C
  delay(2000);
  mpu.setup(0x68);  // Cấu hình MPU9250 với địa chỉ I2C là 0x68
  setup_wifi();
  // while check (button =1) in 2 giay {{
  //   nhập định danh qua usb
  // }}

  // Kết nối MQTT
  client.setServer(mqttServer, mqttPort);
  while (!client.connected()) {
    if (client.connect("ESP32Client")) {
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
  Serial.println(MyWiFi->GetDeviceID());
  Serial.printf("  %s / %s \n", MyWiFi->GetSSID(), MyWiFi->GetPassword());
  delay(1000);
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
                   + String(accx / 16384.0) + "," + String(accy / 16384.0) + "," + String(accz / 16384.0) + ","
                   + String(gyrox / 131.0) + "," + String(gyroy / 131.0) + "," + String(gyroz / 131.0) + ","
                   + String(magx) + "," + String(magy) + "," + String(magz);

  client.publish(mqttTopic, payload.c_str());

  Serial.println("Data sent to MQTT");
  // Đợi 1 giây trước khi gửi dữ liệu tiếp theo
  delay(1000);
};