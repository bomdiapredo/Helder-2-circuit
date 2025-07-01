#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>
#include "FS.h"
#include "SD.h"
#include "SPI.h"

#define SD_CS_PIN 5
#define MPU_ADDR 0x68

// Definições dos registros
#define ACCEL_XOUT_H 0x3B
#define ACCEL_YOUT_H 0x3D
#define ACCEL_ZOUT_H 0x3F
#define GYRO_XOUT_H 0x43
#define GYRO_YOUT_H 0x45
#define GYRO_ZOUT_H 0x47
#define TEMP_OUT_H 0x41

#define GYRO_CONFIG_MODE 0
#define ACC_CONFIG_MODE 1

Adafruit_BMP280 bmp;

float accX, accY, accZ, gyroX, gyroY, gyroZ, tempC;
float referencePressure = 0;  // Vamos definir isso como marco zero
float referenceAltitude = 0;
bool firstReading = true;
float altitudeVariation;








void setup() {
  Serial.begin(115200);
  Wire.begin();
  
  pinMode(13, OUTPUT);          // Configura o pino 13 como saída

  if (!bmp.begin(0x76)) {
    Serial.println("Erro ao inicializar o BMP280!");
    while (1);
  }

  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("Falha ao montar o cartão SD!");
    return;
  }
  Serial.println("Cartão SD montado.");

  initMPU9250();
  delay(2000);
  writeFile(SD, "/log.txt", "Iniciando a leitura dos sensores...\n");
}

void loop() {
  readAccel();
  readGyro();
  readTemperature();
  readBMP280();

  if (altitudeVariation >= 1){
    digitalWrite(13, HIGH);
  }
  else{
    digitalWrite(13, LOW);
  }

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

  String data = "Acelerômetro X: " + String(accX) + " Y: " + String(accY) + " Z: " + String(accZ) + "\n";
  data += "Giroscópio X: " + String(gyroX) + " Y: " + String(gyroY) + " Z: " + String(gyroZ) + "\n";
  data += "Temperatura do MPU: " + String(tempC) + "\n";

  writeFile(SD, "/log.txt", data.c_str());
  delay(1000);
}

void initMPU9250() {
  i2cWrite(MPU_ADDR, 0x6B, 0x00);
  delay(100);
  i2cWrite(MPU_ADDR, 0x1B, GYRO_CONFIG_MODE);
  i2cWrite(MPU_ADDR, 0x1C, ACC_CONFIG_MODE);
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

void readBMP280() {
  float currentPressure = bmp.readPressure() / 100.0F; // Pressão em hPa
  float currentTemp = bmp.readTemperature();
  
  if(firstReading) {
    referencePressure = currentPressure;
    referenceAltitude = bmp.readAltitude(1013.25);
    firstReading = false;
    Serial.println("Ponto de referência definido!");
  }

  // Calcula variação de altitude usando fórmula simplificada
  // Altitude em metros = 44330 * [1 - (P/P0)^(1/5.255)]
  altitudeVariation = 44330.0 * (1.0 - pow(currentPressure/referencePressure, 0.1903));

  Serial.print("Pressão atual: ");
  Serial.print(currentPressure);
  Serial.print(" hPa, Variação: ");
  Serial.print(currentPressure - referencePressure);
  Serial.print(" hPa, Altitude relativa: ");
  Serial.print(altitudeVariation);
  Serial.print(" m, Temperatura: ");
  Serial.print(currentTemp);
  Serial.println(" °C");
}

void writeFile(fs::FS &fs, const char * path, const char * message) {
  Serial.printf("Escrevendo arquivo: %s\n", path);
  File file = fs.open(path, FILE_WRITE);
  if (!file) {
    Serial.println("Falha ao abrir o arquivo para escrita");
    return;
  }
  if (file.print(message)) {
    Serial.println("Arquivo escrito");
  } else {
    Serial.println("Falha ao escrever no arquivo");
  }
  file.close();
}
