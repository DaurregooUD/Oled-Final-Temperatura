#include "mbed.h"
#include "Display.h"
#include "I2C.h"
#include "ThisThread.h"
#include "Adafruit_GFX.h"
#include "Adafruit_SSD1306.h" 
#include <cstring>
#include <cstdio>
#include <stdio.h>


// Definiciones
#define tiempo_muestreo   2s
#define AHT15_ADDRESS 0x38 << 1

// Prototipos

void condicion_start(void);
void condicion_stop(void);
void aht15_init();
bool read_aht15(float &temperature);
float read_ntc_temperature();
float read_lm35_temperature();
void display_oled(const char* message, int row);
void capture_temperatures(float ntc[], float lm35[], float aht[], int samples);
void calculate_and_display_errors(float avg_ntc, float avg_aht);
void graphic_oled(void);

// Pines y puertos
BufferedSerial serial(USBTX, USBRX);
I2C i2c (D14,D15);
Adafruit_SSD1306_I2c oled (i2c, D0);

DigitalOut led(LED1); // Led para saber que esta funcionando el codigo
AnalogIn ntc_pin(A0);
AnalogIn lm35_pin(A1);

// Variables globales

const float VADC = 3.3; 
const float RRef = 10000.0;  // Resistencia de referencia
const float A = 1.009249522e-03; // Coeficientes Steinhart-Hart
const float B = 2.378405444e-04;
const float C = 2.019202697e-07;
const char *mensaje_inicio = "Arranque del programa\n\r";
int ent = 0, dec = 0;
char men[40];
char data[6];

// Inicializar el sensor AHT15
void aht15_init() {
    char cmd[3] = {0xE1, 0x08, 0x00};
    i2c.write(AHT15_ADDRESS, cmd, 3);
    ThisThread::sleep_for(20ms);
}

// Leer temperatura del sensor AHT15
bool read_aht15(float &temperature) {
    char cmd = 0xAC;
    i2c.write(AHT15_ADDRESS, &cmd, 1);
    ThisThread::sleep_for(80ms);
    if (i2c.read(AHT15_ADDRESS, data, 6) == 0) {
        uint32_t raw_temp = ((data[3] & 0x0F) << 16) | (data[4] << 8) | data[5];
        temperature = ((float)raw_temp / 1048576.0) * 200.0 - 50.0;
        return true;
    }
    return false;
}

// Leer temperatura del NTC
float read_ntc_temperature() {
    float analog_value = ntc_pin.read();
    float voltage = analog_value * 3.3;
    float resistance = RRef * (3.3 / voltage - 1.0);
    float temperature = 1.0f / (1.0f / 298.15 + (1.0f / 32596) * log(resistance / RRef)) - 273.15;
    return temperature;
}

// Leer temperatura del LM35
float read_lm35_temperature() {
    float voltage = lm35_pin.read() * 3.3f;
    return voltage * 100.0f;
}

// Ordenar un array usando burbuja
void bubble_sort(float arr[], int n) {
    for (int i = 0; i < n - 1; i++) {
        for (int j = 0; j < n - i - 1; j++) {
            if (arr[j] > arr[j + 1]) {
                float temp = arr[j];
                arr[j] = arr[j + 1];
                arr[j + 1] = temp;
            }
        }
    }
}

// Mostrar mensaje en la pantalla OLED
void display_oled(const char* message, int row) {
    oled.clearDisplay();
    oled.setTextCursor(0, 0);
    oled.printf("%s", message);
    oled.display();
    ThisThread::sleep_for(tiempo_muestreo);
}

// Capturar 10 muestras de temperatura de cada sensor
void capture_temperatures(float ntc[], float lm35[], float aht[], int samples) {
    for (int i = 0; i < samples; i++) {
        ntc[i] = read_ntc_temperature();
        lm35[i] = read_lm35_temperature();
        read_aht15(aht[i]);
        ThisThread::sleep_for(100ms);
    }
}

// Calcular y mostrar errores de promedio
void calculate_and_display_errors(float avg_ntc, float avg_aht) {
    float combined_avg = (avg_ntc + avg_aht) / 2.0;
    float abs_error = fabs(avg_ntc - avg_aht);
    int entero_abs_error = floor(abs_error);
    int decimal_abs_error = abs((abs_error-entero_abs_error)*100);
    float rel_error_ntc = (abs_error / avg_ntc) * 100;
    int entero_rel_er_NTC = floor(rel_error_ntc);
    int decimal_rel_er_NTC = abs((rel_error_ntc-entero_rel_er_NTC)*100);
    float rel_error_aht = (abs_error / avg_aht) * 100;
    int entero_error_Aht = floor(rel_error_aht);
    int decimal_error_Aht = abs((rel_error_aht-entero_error_Aht)*100);
    send_data(combined_avg*100);
    char error_msg[40];
    sprintf(error_msg, "Error Absoluto:\n\r %01u.%04u Celsius\n\r", entero_abs_error,decimal_abs_error);
    display_oled(error_msg, 0);
    printf("Error Absoluto:\n\r %01u.%04u Celsius\n\r", entero_abs_error,decimal_abs_error);
    ThisThread::sleep_for(1s);
    sprintf(error_msg, "Error Relativo NTC: \n\r %01u.%04u Celsius\n\r", entero_rel_er_NTC,decimal_rel_er_NTC);
    display_oled(error_msg, 0);
    printf("Error Relativo NTC: \n\r %01u.%04u Celsius\n\r", entero_rel_er_NTC,decimal_rel_er_NTC);
    ThisThread::sleep_for(1s);
    sprintf(error_msg, "Error Relativo AHT: \n\r %01u.%04u Celsius\n\r", entero_error_Aht,decimal_error_Aht);
    display_oled(error_msg, 0);
    printf("Error Relativo AHT: \n\r %01u.%04u Celsius\n\r", entero_error_Aht,decimal_error_Aht);
    ThisThread::sleep_for(1s);
    
    
}


// Función principal
int main() {
    oled.begin();
    oled.clearDisplay();
    display_oled("Test", 2);
    aht15_init();
    
    float temperature = 0.0;
    const int num_samples = 15;
    float ntc_temperatures[num_samples];
    float lm35_temperatures[num_samples];
    float aht_temperatures[num_samples];

    
    while (true) {
        capture_temperatures(ntc_temperatures, lm35_temperatures, aht_temperatures, num_samples);

        bubble_sort(ntc_temperatures, num_samples);
        bubble_sort(lm35_temperatures, num_samples);
        bubble_sort(aht_temperatures, num_samples);
        


        float avg_ntc = 0.0, avg_lm35 = 0.0, avg_aht = 0.0;
        for (int i = 0; i < num_samples; i++) {
            avg_ntc += ntc_temperatures[i];
            avg_lm35 += lm35_temperatures[i];
            avg_aht += aht_temperatures[i];
        }
        avg_ntc /= num_samples;
        avg_lm35 /= num_samples;
        avg_aht /= num_samples;

        calculate_and_display_errors(avg_ntc, avg_aht);
        int entero_ntc= floor(avg_ntc);
        int decimal_ntc= abs((avg_ntc-entero_ntc)*100);
        int entero_aht= floor(avg_aht);
        int decimal_aht= abs((avg_aht-entero_aht)*100);
        int entero_lm35= floor(avg_lm35);
        int decimal_lm35= abs((avg_lm35-entero_lm35)*100);


        char message[40];
        sprintf(message, "Temp. Prom. NTC: \n\r %01u.%04u Celsius\n\r", entero_ntc,decimal_ntc);
        display_oled(message, 0);
        printf("Temp. Prom. NTC: \n\r %01u.%04u Celsius\n\r", entero_ntc,decimal_ntc);
        ThisThread::sleep_for(1s);
        sprintf(message, "Temp. Prom. LM35: \n\r %01u.%04u Celsius\n\r", entero_lm35,decimal_lm35);
        display_oled(message, 0);
        printf("Temp. Prom. LM35: \n\r %01u.%04u Celsius\n\r", entero_lm35,decimal_lm35);
        ThisThread::sleep_for(1s);
        sprintf(message, "Temp. Prom. AHT: \n\r %01u.%04u Celsius\n\r", entero_aht,decimal_aht);
        display_oled(message, 0);
        printf("Temp. Prom. AHT: \n\r %01u.%04u Celsius\n\r", entero_aht,decimal_aht);
        ThisThread::sleep_for(1s);
        


        
        
        

        
    }
}


