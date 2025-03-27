#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>
#include "FS.h"
#include "SD.h"
#include "SPI.h"

// Defina o pino CS (Chip Select) para o cartão SD
#define SD_CS_PIN 5  // Ajuste conforme o seu pino de CS

// Endereço I2C do MPU9250
#define MPU_ADDR 0x68

// Definições dos registros
#define ACCEL_XOUT_H 0x3B
#define ACCEL_YOUT_H 0x3D
#define ACCEL_ZOUT_H 0x3F
#define GYRO_XOUT_H 0x43
#define GYRO_YOUT_H 0x45
#define GYRO_ZOUT_H 0x47
#define TEMP_OUT_H 0x41

// Definindo pinos e outras configurações
#define GYRO_CONFIG_MODE 0  // 0: 250 dps
#define ACC_CONFIG_MODE 1   // 1: 4g

// Sensor BMP280
Adafruit_BMP280 bmp;

// Calibração
float accX, accY, accZ, gyroX, gyroY, gyroZ, tempC;

void setup() {
  // Inicializar Serial
  Serial.begin(115200);
  Wire.begin();

  // Inicializar o BMP280
  if (!bmp.begin(0x76)) {
    Serial.println("Erro ao inicializar o BMP280!");
    while (1);
  }

  // Inicializar o cartão SD
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("Falha ao montar o cartão SD!");
    return;
  }
  Serial.println("Cartão SD montado.");

  // Inicializar o MPU9250
  initMPU9250();
  
  // Espera para garantir que tudo esteja inicializado
  delay(2000);
  
  // Testar a escrita e leitura no cartão SD
  writeFile(SD, "/log.txt", "Iniciando a leitura dos sensores...\n");
}

void loop() {
  // Ler dados do MPU9250
  readAccel();
  readGyro();
  readTemperature();

  // Mostrar dados no Monitor Serial
  Serial.print("Acelerômetro X: ");
  Serial.print(accX);
  Serial.print(" Y: ");
  Serial.print(accY);
  Serial.print(" Z: ");
  Serial.println(accZ);

  Serial.print("Giroscópio X: ");
  Serial.print(gyroX);
  Serial.print(" Y: ");
  Serial.print(gyroY);
  Serial.print(" Z: ");
  Serial.println(gyroZ);

  Serial.print("Temperatura do MPU: ");
  Serial.println(tempC);

  // Escrever os dados no cartão SD
  String data = "Acelerômetro X: " + String(accX) + " Y: " + String(accY) + " Z: " + String(accZ) + "\n";
  data += "Giroscópio X: " + String(gyroX) + " Y: " + String(gyroY) + " Z: " + String(gyroZ) + "\n";
  data += "Temperatura do MPU: " + String(tempC) + "\n";

  writeFile(SD, "/log.txt", data.c_str());

  delay(1000);  // Pausa antes de ler novamente
}

void initMPU9250() {
  // Acorda o MPU9250
  i2cWrite(MPU_ADDR, 0x6B, 0x00);
  delay(100);

  // Configuração do giroscópio e acelerômetro
  i2cWrite(MPU_ADDR, 0x1B, GYRO_CONFIG_MODE);  // Giroscópio
  i2cWrite(MPU_ADDR, 0x1C, ACC_CONFIG_MODE);   // Acelerômetro
}

void i2cWrite(uint8_t address, uint8_t reg, uint8_t val) {
  Wire.beginTransmission(address);
  Wire.write(reg);
  Wire.write(val);
  Wire.endTransmission();
}

int16_t i2cRead(uint8_t address, uint8_t reg) {
  Wire.beginTransmission(address);
  Wire.write(reg);
  Wire.endTransmission(false);
  Wire.requestFrom(address, (uint8_t)2, true);
  return (Wire.read() << 8 | Wire.read());
}

void readAccel() {
  accX = i2cRead(MPU_ADDR, ACCEL_XOUT_H) / 16384.0;
  accY = i2cRead(MPU_ADDR, ACCEL_YOUT_H) / 16384.0;
  accZ = i2cRead(MPU_ADDR, ACCEL_ZOUT_H) / 16384.0;
}

void readGyro() {
  gyroX = i2cRead(MPU_ADDR, GYRO_XOUT_H) / 131.0;
  gyroY = i2cRead(MPU_ADDR, GYRO_YOUT_H) / 131.0;
  gyroZ = i2cRead(MPU_ADDR, GYRO_ZOUT_H) / 131.0;
}

void readTemperature() {
  int16_t rawTemp = i2cRead(MPU_ADDR, TEMP_OUT_H);
  tempC = (rawTemp / 340.0) + 16.53;
}

void writeFile(fs::FS &fs, const char * path, const char * message) {
  Serial.printf("Escrevendo arquivo: %s\n", path);
  File file = fs.open(path, FILE_APPEND); // Mudei para FILE_APPEND
  if (!file) {
    Serial.println("Falha ao abrir o arquivo para escrita");
    return;
  }
  if (file.print(message)) {
    Serial.println("Arquivo escrito");
  } else {
    Serial.println("Falha ao escrever no arquivo");
  }
}