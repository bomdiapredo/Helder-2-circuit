#include <Wire.h>
#include <SPI.h>
#include <LoRa.h>
#include <Adafruit_BMP280.h>
#include <TinyGPS++.h>
#include <HardwareSerial.h>
#include "board_def.h"

// MPU9250 defines e funções (i2cRead/i2cWrite) necessários para ler dados do MPU9250
// Definições do MPU9250
#define MPU 0x68
#define accX_H 0x3B
#define accY_H 0x3D
#define accZ_H 0x3F
#define gyroX_H 0x43
#define gyroY_H 0x45
#define gyroZ_H 0x47
#define TEMP_H 0x41

Adafruit_BMP280 bmp;
HardwareSerial gpsSerial(1);
TinyGPSPlus gps;

bool bmpAvailable = false;
bool mpuAvailable = false;

float accX, accY, accZ;
float gyroX, gyroY, gyroZ;
float tempC; // Temperatura MPU (esp. do giroscópio)
float bmpTemp, altitude;

int GPS_RX = 34;
int GPS_TX = 12;

void i2cWrite(uint8_t address, uint8_t reg, uint8_t val) {
  Wire.beginTransmission(address);
  Wire.write(reg);
  Wire.write(val);
  Wire.endTransmission();
}

int16_t i2cRead(uint8_t address, int8_t reg) {
  Wire.beginTransmission(address);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) {
    return 0;
  }
  Wire.requestFrom(address, 2, true);
  if (Wire.available() >= 2) {
    return (Wire.read() << 8) | Wire.read();
  }
  return 0;
}

void setup() {
  Serial.begin(115200);
  Wire.begin();

  bmpAvailable = bmp.begin(0x76);
  if (bmpAvailable) Serial.println("BMP280 inicializado");
  else Serial.println("BMP280 nao detectado");

  // Inicializa GPS UART
  gpsSerial.begin(9600, SERIAL_8N1, GPS_RX, GPS_TX);
  Serial.println("GPS iniciado");

  // Verifica MPU9250
  Wire.beginTransmission(MPU);
  if (Wire.endTransmission() == 0) {
    mpuAvailable = true;
    Serial.println("MPU9250 detectado");
  } else {
    Serial.println("MPU9250 nao detectado");
  }

  if (mpuAvailable) {
    // Liga sensor MPU9250
    i2cWrite(MPU, 0x6B, 0);
  }

  // Inicializa LoRa
  SPI.begin(CONFIG_CLK, CONFIG_MISO, CONFIG_MOSI, CONFIG_NSS);
  LoRa.setPins(CONFIG_NSS, CONFIG_RST, CONFIG_DIO0);
  if (!LoRa.begin(BAND)) {
    Serial.println("Falha na inicializacao do LoRa");
    while (1);
  }
  Serial.println("LoRa iniciado");
}

void loop() {
  // Lê GPS
  while (gpsSerial.available()) {
    gps.encode(gpsSerial.read());
  }

  // Lê temperatura MPU9250 (offset de +36.53 conforme datasheet)
  if (mpuAvailable) {
    int16_t rawTemp = i2cRead(MPU, TEMP_H);
    tempC = (rawTemp / 340.0) + 36.53;

    // Lê acelerômetro
    accX = i2cRead(MPU, accX_H) / 16384.0f;
    accY = i2cRead(MPU, accY_H) / 16384.0f;
    accZ = i2cRead(MPU, accZ_H) / 16384.0f;

    // Lê giroscópio
    gyroX = i2cRead(MPU, gyroX_H) / 131.0f;
    gyroY = i2cRead(MPU, gyroY_H) / 131.0f;
    gyroZ = i2cRead(MPU, gyroZ_H) / 131.0f;
  } else {
    tempC = accX = accY = accZ = gyroX = gyroY = gyroZ = 0;
  }

  // Lê BMP280 se disponível
  if (bmpAvailable) {
    bmpTemp = bmp.readTemperature();
    altitude = bmp.readAltitude(1013.25);
  } else {
    bmpTemp = 0;
    altitude = 0;
  }

  // Exibe no Monitor Serial do transmissor
  Serial.printf("Temperatura MPU: %.2f °C, Temperatura BMP: %.2f °C, Altitude: %.2f m\n", tempC, bmpTemp, altitude);
  Serial.printf("Accel (g): X=%.3f Y=%.3f Z=%.3f\n", accX, accY, accZ);
  Serial.printf("Gyro (deg/s): X=%.3f Y=%.3f Z=%.3f\n", gyroX, gyroY, gyroZ);
  if (gps.location.isValid()) {
    Serial.printf("GPS Lat: %.6f, Lon: %.6f, Data: %02d/%02d/%04d, Hora: %02d:%02d:%02d\n",
      gps.location.lat(), gps.location.lng(),
      gps.date.day(), gps.date.month(), gps.date.year(),
      gps.time.hour(), gps.time.minute(), gps.time.second());
  } else {
    Serial.println("GPS invalido");
  }

  // Envia todos os dados via LoRa
  if (gps.location.isValid()) {
    LoRa.beginPacket();
    LoRa.printf("TempMPU:%.2f,TempBMP:%.2f,Alt:%.2f,AccX:%.3f,AccY:%.3f,AccZ:%.3f,GyroX:%.3f,GyroY:%.3f,GyroZ:%.3f,Lat:%.6f,Lon:%.6f,Date:%02d/%02d/%04d,Time:%02d:%02d:%02d",
                 tempC, bmpTemp, altitude,
                 accX, accY, accZ,
                 gyroX, gyroY, gyroZ,
                 gps.location.lat(), gps.location.lng(),
                 gps.date.day(), gps.date.month(), gps.date.year(),
                 gps.time.hour(), gps.time.minute(), gps.time.second());
    LoRa.endPacket();
  }

  delay(2000);
}
