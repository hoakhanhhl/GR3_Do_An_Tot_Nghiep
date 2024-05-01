#ifndef Heart_h
    #define Heart_h
    #if defined(ARDUINO_ARCH_ESP32)
        /// Các khai báo khi sử dụng CPU thuộc họ ESP32
        // Các hàm public được viết trong Heart.cpp
        /**
         * Cau hinh nồng độ Oxy
        */
        void setup_heart();
        void loop_heart();
    #elif  defined(ARDUINO_ARCH_ESP8266)
        /// Các khai báo khi sử dụng CPU thuộc họ ESP8266
    #endif
#endif