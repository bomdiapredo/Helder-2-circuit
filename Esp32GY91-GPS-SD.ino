#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>
#include <TinyGPS++.h>
#include <HardwareSerial.h>
#include <SPI.h>
#include <SD.h>

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
    Serial.println("Acceleration (g):");
    Serial.printf("AccX = %.3f\n", mpuAvailable ? accX : 0.0);
    Serial.printf("AccY = %.3f\n", mpuAvailable ? accY : 0.0);
    Serial.printf("AccZ = %.3f\n", mpuAvailable ? accZ : 0.0);
}

void printGyro() {
    sep(30);
    Serial.println("Gyroscope (deg/s):");
    Serial.printf("GyroX = %.3f\n", mpuAvailable ? gyroX : 0.0);
    Serial.printf("GyroY = %.3f\n", mpuAvailable ? gyroY : 0.0);
    Serial.printf("GyroZ = %.3f\n", mpuAvailable ? gyroZ : 0.0);
}

void printBMP280() {
    sep(30);
    Serial.println("BMP280 Sensor:");
    if (bmpAvailable) {
        float pressure = bmp.readPressure() / 100.0F; // hPa
        float altitude = bmp.readAltitude(1013.25);
        float temperature = bmp.readTemperature();
        Serial.printf("Temperature: %.2f °C\n", temperature);
        Serial.printf("Pressure: %.2f hPa\n", pressure);
        Serial.printf("Altitude: %.2f m\n", altitude);
    } else {
        Serial.println("Temperature: 0.00 °C");
        Serial.println("Pressure: 0.00 hPa");
        Serial.println("Altitude: 0.00 m");
    }
}

void printGPS() {
    sep(30);
    Serial.println("GPS Data:");
    if (gps.location.isValid()) {
        Serial.printf("Latitude: %.6f\n", gps.location.lat());
        Serial.printf("Longitude: %.6f\n", gps.location.lng());
        Serial.printf("Satellites: %d\n", gps.satellites.value());
        Serial.printf("Altitude GPS: %.2f m\n", gps.altitude.meters());
        Serial.printf("Speed (km/h): %.2f\n", gps.speed.kmph());
        Serial.printf("Date: %02d/%02d/%04d\n", gps.date.day(), gps.date.month(), gps.date.year());
        Serial.printf("Time: %02d:%02d:%02d\n", gps.time.hour(), gps.time.minute(), gps.time.second());
    } else {
        Serial.println("Latitude: 0.000000");
        Serial.println("Longitude: 0.000000");
        Serial.println("Satellites: 0");
        Serial.println("Altitude GPS: 0.00 m");
        Serial.println("Speed (km/h): 0.00");
        Serial.println("Date: 00/00/0000");
        Serial.println("Time: 00:00:00");
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
            Serial.println("Invalid input! By default Case 0 selected");
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
            Serial.println("Invalid input! By default Case 0 selected");
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
        rawaccX = rawaccY = rawaccZ = 0; // reset before calibration
        Serial.printf("Do not move! Starting calibration for %d seconds\n", (delay_cal / 1000));
        while (millis() - initTime <= delay_cal) {
            rawaccX += (i2cRead(MPU, accX_H)) / rawVal;
            rawaccY += (i2cRead(MPU, accY_H)) / rawVal;
            rawaccZ += (i2cRead(MPU, accZ_H)) / rawVal;
            count++;
            delay(5);
        }
        Serial.printf("Count = %d\n", count);
        Serial.println("Calibration done!");
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
        Serial.printf("Do not move! Starting calibration for %d seconds\n", (delay_cal / 1000));
        while (millis() - initTime <= delay_cal) {
            rawgyroX += (i2cRead(MPU, gyroX_H)) / rawVal;
            rawgyroY += (i2cRead(MPU, gyroY_H)) / rawVal;
            rawgyroZ += (i2cRead(MPU, gyroZ_H)) / rawVal;
            count++;
            delay(5);
        }
        Serial.printf("Count = %d\n", count);
        Serial.println("Calibration done!");
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

    // Inicializa o SPI com os pinos definidos
    spiSD.begin(SPI_SCK, SPI_MISO, SPI_MOSI, SD_CS);

    // Inicializa o cartão SD
    if (!SD.begin(SD_CS, spiSD)) {
        Serial.println("Falha ao montar o cartão SD");
        return;
    }
    Serial.println("Cartão SD montado com sucesso");

    // Cria ou abre o arquivo para escrita
    dataFile = SD.open("/dados.txt", FILE_APPEND);
    if (!dataFile) {
        Serial.println("Erro ao abrir o arquivo");
        return;
    }

    // Check MPU availability (address 0x68)
    mpuAvailable = checkI2CDevice(MPU);
    if (mpuAvailable) {
        Serial.println("MPU9250 detected.");
    } else {
        Serial.println("MPU9250 not detected, sensor data will be zero.");
    }

    // Initialize BMP280, if present
    bmpAvailable = bmp.begin(0x76);
    if (bmpAvailable) {
        Serial.println("BMP280 sensor detected.");
    } else {
        Serial.println("BMP280 sensor not detected, pressure/altitude data will be zero.");
    }

    // Initialize GPS UART
    gpsSerial.begin(9600, SERIAL_8N1, GPS_RX, GPS_TX);
    Serial.println("GPS NEO-6M initialized.");

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

    // Print sensor data
    printGyro();
    Serial.printf("MPU Temperature (C): %.2f\n", mpuAvailable ? tempC : 0.0);
    printAcc();

    // BMP280 output
    sep(30);
    Serial.println("BMP280 Sensor:");
    if (bmpAvailable) {
        Serial.printf("Temperature BMP280: %.2f °C\n", bmpTemp);
        Serial.printf("Pressure: %.2f hPa\n", pressure);
        Serial.printf("Altitude: %.2f m\n", altitude);
    } else {
        Serial.println("Temperature BMP280: 0.00 °C");
        Serial.println("Pressure: 0.00 hPa");
        Serial.println("Altitude: 0.00 m");
    }

    // GPS output
    printGPS();

    // Grava os dados no cartão SD
    if (dataFile) {
        if (gps.location.isValid()) {
            dataFile.printf("Date: %02d/%02d/%04d, Time: %02d:%02d:%02d, ", 
                gps.date.day(), gps.date.month(), gps.date.year(),
                gps.time.hour(), gps.time.minute(), gps.time.second());
            dataFile.printf("Latitude: %.6f, Longitude: %.6f, Satellites: %d, Speed: %.2f km/h, Altitude GPS: %.2f m, ", 
                gps.location.lat(), gps.location.lng(), gps.satellites.value(), gps.speed.kmph(), gps.altitude.meters());
            dataFile.printf("AccX: %.3f, AccY: %.3f, AccZ: %.3f, ", accX, accY, accZ);
            dataFile.printf("GyroX: %.3f, GyroY: %.3f, GyroZ: %.3f, ", gyroX, gyroY, gyroZ);
            dataFile.printf("MPU Temp: %.2f C, BMP280 Temp: %.2f C, Pressure: %.2f hPa, Altitude BMP280: %.2f m\n", 
                tempC, bmpTemp, pressure, altitude);
        } else {
            dataFile.println("GPS data not valid.");
        }
        dataFile.flush(); // Garante que os dados sejam gravados no cartão SD
    }

    sep(60);
    delay(300);
}
