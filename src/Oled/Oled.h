#ifndef Oled_h
    #define Oled_h
    #if defined(ARDUINO_ARCH_ESP32)
        /// Các khai báo khi sử dụng CPU thuộc họ ESP32
        // Các hàm public được viết trong Oled.cpp
        /**
         * Cau hinh nhịp tim
        */
        void setup_Oled();
        void loop_Oled();
    #elif  defined(ARDUINO_ARCH_ESP8266)
        /// Các khai báo khi sử dụng CPU thuộc họ ESP8266
    #endif
#endif