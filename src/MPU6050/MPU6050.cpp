#include <MPU6050_tockn.h> //Mpu6050
#include <Wire.h>
#include "MPU6050.h"
#include "../variables.h"

// Đối tượng cảm biến MPU6050
MPU6050 mpu6050(Wire);
long timer = 1000;

String mpu6050_payload; // Khai báo biến mpu6050_payload
extern String deviceID;
extern String patientID;
void setup_MPU6050(){
  // Khởi động cảm biến MPU6050, địa chỉ mặc định MPU6050_ADDR là 0x68
  mpu6050.begin();
  // Tính toán và hiệu chuẩn giá trị offset của con quay quanh các trục (gyroscope)
  mpu6050.calcGyroOffsets(true);
}

void loop_MPU6050(){
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

          // Dữ liệu của MPU6050
          mpu6050_payload = String(deviceID) + "," + String(patientID) + ","
                        + String(accx) + "," + String(accy) + "," + String(accz) + ","
                        + String(gyrox) + "," + String(gyroy) + "," + String(gyroz) + ","
                        + String(angx) + "," + String(angy) + "," + String(angz);
          // In dữ liệu
          Serial.println(mpu6050_payload);
          timer = millis();
          // Đợi 1 giây trước khi gửi dữ liệu tiếp theo
          delay(1000);
    }   
};
