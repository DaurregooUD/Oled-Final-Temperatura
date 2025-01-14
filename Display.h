#include "mbed.h"
#include "I2C.h"
#include "ThisThread.h"
#include "Adafruit_GFX.h"
#include "Adafruit_SSD1306.h" 
#include <cstring>
#include <cstdio>
#include <stdio.h>

#define poner_brillo  0x88
#define dir_display   0xC0
#define escritura     0x40

const char digitToSegment[10] = { 0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F };
void send_byte(char data);
void send_data(int number);
// Funciones de manejo de display y envío de datos
DigitalOut sclk(D2);  // Clock pin
DigitalInOut dio(D3); // Data pin

void condicion_start() {
    sclk = 1;
    dio.output();
    dio = 1;
    ThisThread::sleep_for(1ms);
    dio = 0;
    sclk = 0;
}

void condicion_stop() {
    sclk = 0;
    dio.output();
    dio = 0;
    ThisThread::sleep_for(1ms);
    sclk = 1;
    dio = 1;
}

void send_byte(char data) {
    dio.output();
    for (int i = 0; i < 8; i++) {
        sclk = 0;
        dio = (data & 0x01) ? 1 : 0;
        data >>= 1;
        sclk = 1;
    }
    dio.input();
    sclk = 0;
    ThisThread::sleep_for(1ms);
    if (dio == 0) {
        sclk = 1;
        sclk = 0;
    }
}

void send_data(int number) {
    condicion_start();
    send_byte(escritura);
    condicion_stop();
    condicion_start();
    send_byte(dir_display);

    int digit[4] = {0};
    for (int i = 0; i < 4; i++) {
        digit[i] = number % 10;
        number /= 10;
    }

    for(int i = 3; i >= 0; i--) {
        send_byte(digitToSegment[digit[i]]);
    }
    condicion_stop();
    condicion_start();
    send_byte(poner_brillo);
    condicion_stop();
}