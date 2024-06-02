#ifndef MPU6050_h
    #define MPU6050_h
    #if defined(ARDUINO_ARCH_ESP32)
        /// Các khai báo khi sử dụng CPU thuộc họ ESP32
        // Các hàm public được viết trong MPU6050.cpp
        /**
         * Cau hinh nhịp tim
        */
        void setup_MPU6050();
        void loop_MPU6050();
    #elif  defined(ARDUINO_ARCH_ESP8266)
        /// Các khai báo khi sử dụng CPU thuộc họ ESP8266
    #endif
#endif