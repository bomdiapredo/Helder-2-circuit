#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>
#include <TinyGPS++.h>
#include <HardwareSerial.h>
#include <SPI.h>
#include <SD.h>
#include <LoRa.h>
#include "board_def.h"

// GY-91 MPU9250 Configuration
#define GYRO_CONFIG_MODE 0
#define ACC_CONFIG_MODE 1
#define MPU_POWER_REGISTER 0x6B
#define MPU_MODE 0x6C
#define GYRO_MODE 0x1A
#define GYRO_CONFIG 0x1B
#define ACCEL_INIT 0x1C
#define ACCEL_CONFIG 0x1D
#define accX_H 0x3B
#define accY_H 0x3D
#define accZ_H 0x3F
#define gyroX_H 0x43
#define gyroY_H 0x45
#define gyroZ_H 0x47
#define TEMP_H 0x41
#define TEMP_L 0x42

const uint8_t MPU = 0x68;
unsigned long delay_cal = 3000;

float rawaccX = 0, rawaccY = 0, rawaccZ = 0, rawgyroX = 0, rawgyroY = 0, rawgyroZ = 0;
float tempC = 0, gyroX = 0, gyroY = 0, gyroZ = 0, accX = 0, accY = 0, accZ = 0;
float calgyroX = 0, calgyroY = 0, calgyroZ = 0, calaccX = 0, calaccY = 0, calaccZ = 0;

// BMP280 sensor instance
Adafruit_BMP280 bmp;
bool bmpAvailable = false;

// GPS Configuration
TinyGPSPlus gps;
HardwareSerial gpsSerial(1);  // Using UART1
#define GPS_RX 34
#define GPS_TX 12

bool mpuAvailable = false;

// SD Card Configuration
#define SD_CS 13  // Pino CS do cartão SD
#define SPI_MOSI 15
#define SPI_MISO 2
#define SPI_SCK 14

SPIClass spiSD(VSPI);  // Utiliza o VSPI
File dataFile;

void i2cWrite(uint8_t address, uint8_t reg, uint8_t val) {
    Wire.beginTransmission(address);
    Wire.write(reg);
    Wire.write(val);
    Wire.endTransmission();
}

int16_t i2cRead(uint8_t address, int8_t reg) {
    int16_t temp = 0;
    Wire.beginTransmission(address);
    Wire.write(reg);
    if (Wire.endTransmission(false) != 0) {
        // Device not responding
        return 0;
    }
    Wire.requestFrom(address, 2, true);
    if (Wire.available() >= 2) {
        temp = (Wire.read() << 8 | Wire.read());
    }
    return temp;
}

void sep(int a) {
    for (int i = 0; i < a; i++) {
        Serial.print("_");
    }
    Serial.println();
}

void readTemperature() {
    if (!mpuAvailable) {
        tempC = 0;
        return;
    }
    int16_t rawTemp = i2cRead(MPU, TEMP_H);
    tempC = (rawTemp / 340.0) + 36.53; // Ajuste para temperatura
}

void printAcc() {
    sep(30);
    Serial.println("Aceleração (g):");
    Serial.printf("AccX = %.3f\n", mpuAvailable ? accX : 0.0);
    Serial.printf("AccY = %.3f\n", mpuAvailable ? accY : 0.0);
    Serial.printf("AccZ = %.3f\n", mpuAvailable ? accZ : 0.0);
}

void printGyro() {
    sep(30);
    Serial.println("Giroscópio (deg/s):");
    Serial.printf("GyroX = %.3f\n", mpuAvailable ? gyroX : 0.0);
    Serial.printf("GyroY = %.3f\n", mpuAvailable ? gyroY : 0.0);
    Serial.printf("GyroZ = %.3f\n", mpuAvailable ? gyroZ : 0.0);
}

void printBMP280() {
    sep(30);
    Serial.println("Sensor BMP280:");
    if (bmpAvailable) {
        float pressure = bmp.readPressure() / 100.0F; // hPa
        float altitude = bmp.readAltitude(1013.25);
        float temperature = bmp.readTemperature();
        Serial.printf("Temperatura: %.2f °C\n", temperature);
        Serial.printf("Pressão: %.2f hPa\n", pressure);
        Serial.printf("Altitude: %.2f m\n", altitude);
    } else {
        Serial.println("Temperatura: 0.00 °C");
        Serial.println("Pressão: 0.00 hPa");
        Serial.println("Altitude: 0.00 m");
    }
}

void printGPS() {
    sep(30);
    Serial.println("Dados do GPS:");
    if (gps.location.isValid()) {
        Serial.printf("Latitude: %.6f\n", gps.location.lat());
        Serial.printf("Longitude: %.6f\n", gps.location.lng());
        Serial.printf("Satélites: %d\n", gps.satellites.value());
        
        // Corrigindo a altitude
        float altitudeGPS = gps.altitude.meters() * -1; // Multiplicando por -1 para corrigir negativo
        Serial.printf("Altitude GPS: %.2f m\n", altitudeGPS);
        
        Serial.printf("Velocidade (km/h): %.2f\n", gps.speed.kmph());
        Serial.printf("Data: %02d/%02d/%04d\n", gps.date.day(), gps.date.month(), gps.date.year());
        
        // Ajustando horário para Brasília (-3 horas)
        int adjustedHour = (gps.time.hour() - 3 + 24) % 24;
        Serial.printf("Hora: %02d:%02d:%02d\n", adjustedHour, gps.time.minute(), gps.time.second());
    } else {
        Serial.println("Latitude: 0.000000");
        Serial.println("Longitude: 0.000000");
        Serial.println("Satélites: 0");
        Serial.println("Altitude GPS: 0.00 m");
        Serial.println("Velocidade (km/h): 0.00");
        Serial.println("Data: 00/00/0000");
        Serial.println("Hora: 00:00:00");
    }
}

void powMan() {
    if (!mpuAvailable) return;
    i2cWrite(MPU, MPU_POWER_REGISTER, 0);
    i2cWrite(MPU, MPU_MODE, 0);
}

void gyroConfig(uint8_t choice) {
    if (!mpuAvailable) return;
    uint8_t val;
    switch (choice) {
        case 0: val = 0b00000001; break;
        case 1: val = 0b00001001; break;
        case 2: val = 0b00010001; break;
        case 3: val = 0b00011001; break;
        default:
            Serial.println("Entrada inválida! Selecionado padrão Case 0");
            val = 0b00000001;
            break;
    }
    i2cWrite(MPU, GYRO_MODE, 0x00);
    i2cWrite(MPU, GYRO_CONFIG, val);
}

void accConfig(uint8_t choice) {
    if (!mpuAvailable) return;
    uint8_t val;
    switch (choice) {
        case 0: val = 0b00000000; break;
        case 1: val = 0b00001000; break;
        case 2: val = 0b00010000; break;
        case 3: val = 0b00011000; break;
        default:
            Serial.println("Entrada inválida! Selecionado padrão Case 0");
            val = 0b00000000;
            break;
    }
    i2cWrite(MPU, ACCEL_INIT, val);
    i2cWrite(MPU, ACCEL_CONFIG, 0b00001000);
}

void accData(bool status, int choice) {
    if (!mpuAvailable) {
        accX = accY = accZ = 0;
        return;
    }
    float rawVal;
    switch (choice) {
        case 0: rawVal = 16384.0; break;
        case 1: rawVal = 8192.0; break;
        case 2: rawVal = 4096.0; break;
        case 3: rawVal = 2048.0; break;
        default: rawVal = 8192.0; break;
    }

    if (status) {
        unsigned int count = 0;
        unsigned long initTime = millis();
        rawaccX = rawaccY = rawaccZ = 0; // reset calibration sums
        Serial.printf("Não se mova! Iniciando calibração por %d segundos\n", (delay_cal / 1000));
        while (millis() - initTime <= delay_cal) {
            rawaccX += (i2cRead(MPU, accX_H)) / rawVal;
            rawaccY += (i2cRead(MPU, accY_H)) / rawVal;
            rawaccZ += (i2cRead(MPU, accZ_H)) / rawVal;
            count++;
            delay(5);
        }
        Serial.printf("Contagem = %d\n", count);
        Serial.println("Calibração concluída!");
        calaccX = (float)rawaccX / count;
        calaccY = (float)rawaccY / count;
        calaccZ = (float)rawaccZ / count;
    } else {
        accX = (i2cRead(MPU, accX_H) / rawVal) - calaccX;
        accY = (i2cRead(MPU, accY_H) / rawVal) - calaccY;
        accZ = (i2cRead(MPU, accZ_H) / rawVal) - calaccZ;
    }
}

void gyroData(bool status, int choice) {
    if (!mpuAvailable) {
        gyroX = gyroY = gyroZ = 0;
        return;
    }
    float rawVal;
    switch (choice) {
        case 0: rawVal = 131.0; break;
        case 1: rawVal = 65.5; break;
        case 2: rawVal = 32.8; break;
        case 3: rawVal = 16.4; break;
        default: rawVal = 131.0; break;
    }
    if (status) {
        unsigned int count = 0;
        unsigned long initTime = millis();
        rawgyroX = rawgyroY = rawgyroZ = 0;
        Serial.printf("Não se mova! Iniciando calibração por %d segundos\n", (delay_cal / 1000));
        while (millis() - initTime <= delay_cal) {
            rawgyroX += (i2cRead(MPU, gyroX_H)) / rawVal;
            rawgyroY += (i2cRead(MPU, gyroY_H)) / rawVal;
            rawgyroZ += (i2cRead(MPU, gyroZ_H)) / rawVal;
            count++;
            delay(5);
        }
        Serial.printf("Contagem = %d\n", count);
        Serial.println("Calibração concluída!");
        calgyroX = (float)rawgyroX / count;
        calgyroY = (float)rawgyroY / count;
        calgyroZ = (float)rawgyroZ / count;
    } else {
        gyroX = (i2cRead(MPU, gyroX_H)) / rawVal - calgyroX;
        gyroY = (i2cRead(MPU, gyroY_H)) / rawVal - calgyroY;
        gyroZ = (i2cRead(MPU, gyroZ_H)) / rawVal - calgyroZ;
    }
}

// Check if device at I2C address responds successfully
bool checkI2CDevice(uint8_t address) {
    Wire.beginTransmission(address);
    uint8_t error = Wire.endTransmission();
    return (error == 0);
}

void setup() {
    Serial.begin(115200);
    Wire.begin();
    Wire.setClock(400000);

    // Inicializa o SPI com os pinos definidos do cartão SD
    spiSD.begin(SPI_SCK, SPI_MISO, SPI_MOSI, SD_CS);

    // Inicializa o cartão SD
    if (!SD.begin(SD_CS, spiSD)) {
        Serial.println("Falha ao montar o cartão SD");
        while (1) { delay(1000); } // Trava aqui
    }
    Serial.println("Cartão SD montado com sucesso");

    // Cria ou abre o arquivo para escrita
    dataFile = SD.open("/dados.txt", FILE_APPEND);
    if (!dataFile) {
        Serial.println("Erro ao abrir o arquivo");
        while(1) { delay(1000); }
    }

    // Check MPU availability (address 0x68)
    mpuAvailable = checkI2CDevice(MPU);
    if (mpuAvailable) {
        Serial.println("MPU9250 detectado.");
    } else {
        Serial.println("MPU9250 não detectado, dados do sensor serão zeros.");
    }

    // Initialize BMP280, if present
    bmpAvailable = bmp.begin(0x76);
    if (bmpAvailable) {
        Serial.println("Sensor BMP280 detectado.");
    } else {
        Serial.println("Sensor BMP280 não detectado, dados de pressão/altitude serão zeros.");
    }

    // Initialize GPS UART
    gpsSerial.begin(9600, SERIAL_8N1, GPS_RX, GPS_TX);
    Serial.println("GPS NEO-6M inicializado.");

    // Initialize LoRa
    SPI.begin(CONFIG_CLK, CONFIG_MISO, CONFIG_MOSI, CONFIG_NSS);
    LoRa.setPins(CONFIG_NSS, CONFIG_RST, CONFIG_DIO0);
    if (!LoRa.begin(BAND)) {
        Serial.println("Inicialização do LoRa falhou!");
        while (1);
    }
    Serial.println("LoRa inicializado.");

    if (mpuAvailable) {
        powMan();
        sep(60);
        gyroConfig(GYRO_CONFIG_MODE);
        accConfig(ACC_CONFIG_MODE);
        // Calibrate sensors
        accData(true, ACC_CONFIG_MODE);
        gyroData(true, GYRO_CONFIG_MODE);
        delay(3000);
        sep(60);
    }
}

void loop() {
    // Read data from MPU sensors if available
    if (mpuAvailable) {
        accData(false, ACC_CONFIG_MODE);
        gyroData(false, GYRO_CONFIG_MODE);
        readTemperature();
    } else {
        accX = accY = accZ = 0;
        gyroX = gyroY = gyroZ = 0;
        tempC = 0;
    }

    // Read pressure and altitude if BMP280 available
    float pressure = 0, altitude = 0, bmpTemp = 0;
    if (bmpAvailable) {
        pressure = bmp.readPressure() / 100.0F;
        altitude = bmp.readAltitude(1013.25);
        bmpTemp = bmp.readTemperature();
    }

    // Read GPS data
    while (gpsSerial.available() > 0) {
        gps.encode(gpsSerial.read());
    }

    // Print sensor data to serial monitor
    printGyro();
    Serial.printf("Temperatura MPU (°C): %.2f\n", mpuAvailable ? tempC : 0.0);
    printAcc();

    sep(30);
    Serial.println("Sensor BMP280:");
    if (bmpAvailable) {
        Serial.printf("Temperatura BMP280: %.2f °C\n", bmpTemp);
        Serial.printf("Pressão: %.2f hPa\n", pressure);
        Serial.printf("Altitude: %.2f m\n", altitude);
    } else {
        Serial.println("Temperatura BMP280: 0.00 °C");
        Serial.println("Pressão: 0.00 hPa");
        Serial.println("Altitude: 0.00 m");
    }

    // Corrigir altitude GPS para salvar e enviar
    float correctedAltitudeGPS = gps.altitude.meters() * -1;

    printGPS();

    // Save data to SD card
    if (dataFile) {
        if (gps.location.isValid()) {
            dataFile.printf("Data: %02d/%02d/%04d, Hora: %02d:%02d:%02d, ",
                gps.date.day(), gps.date.month(), gps.date.year(),
                (gps.time.hour() - 3 + 24) % 24, gps.time.minute(), gps.time.second());
            dataFile.printf("Latitude: %.6f, Longitude: %.6f, Satélites: %d, Velocidade: %.2f km/h, Altitude GPS: %.2f m, ",
                gps.location.lat(), gps.location.lng(), gps.satellites.value(), gps.speed.kmph(), correctedAltitudeGPS);
            dataFile.printf("AccX: %.3f, AccY: %.3f, AccZ: %.3f, ", accX, accY, accZ);
            dataFile.printf("GyroX: %.3f, GyroY: %.3f, GyroZ: %.3f, ", gyroX, gyroY, gyroZ);
            dataFile.printf("Temp MPU: %.2f °C, Temp BMP: %.2f °C, Pressão: %.2f hPa, Altitude BMP: %.2f m\n",
                tempC, bmpTemp, pressure, altitude);
        } else {
            dataFile.println("Dados do GPS inválidos.");
        }
        dataFile.flush();
    }

    // Send data via LoRa
    if (gps.location.isValid()) {
        LoRa.beginPacket();
        LoRa.printf("Data: %02d/%02d/%04d, Hora: %02d:%02d:%02d, ",
            gps.date.day(), gps.date.month(), gps.date.year(),
            (gps.time.hour() - 3 + 24) % 24, gps.time.minute(), gps.time.second());
        LoRa.printf("Lat: %.6f, Lon: %.6f, Sat: %d, Vel: %.2f km/h, Alt: %.2f m, ",
            gps.location.lat(), gps.location.lng(), gps.satellites.value(), gps.speed.kmph(), correctedAltitudeGPS);
        LoRa.printf("AccX: %.3f, AccY: %.3f, AccZ: %.3f, ", accX, accY, accZ);
        LoRa.printf("GyroX: %.3f, GyroY: %.3f, GyroZ: %.3f, ", gyroX, gyroY, gyroZ);
        LoRa.printf("Temp MPU: %.2f °C, Temp BMP: %.2f °C, Pressão: %.2f hPa, Alt BMP: %.2f m",
            tempC, bmpTemp, pressure, altitude);
        LoRa.endPacket();
    }

    sep(60);
    delay(300);
}
