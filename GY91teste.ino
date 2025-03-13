#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>

// 0: 250 dps (131 LSB/dps); 1: 500 dps (65.5 LSB/dps) ; 2: 1000 dps ( 32.8 LSB/dps) ; 3: 2000 dps (16.4 LSB/dps)
#define GYRO_CONFIG_MODE 0

// 0: 2g (16384 LSB/g); 1: 4g (8192 LSB/g) ; 2: 8g (4096 LSB/g) ; 3: 16g (2048 LSB/g)
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
#define TEMP_H 0x41  // Registrador de temperatura (alta)
#define TEMP_L 0x42  // Registrador de temperatura (baixa)

// I2C Address of MPU9250
const uint8_t MPU = 0x68;

// Caliberation Time Assigned
unsigned long delay_cal = 3000;

float rawaccX = 0, rawaccY = 0, rawaccZ = 0, rawgyroX = 0, rawgyroY = 0, rawgyroZ = 0;
float tempC, gyroX, gyroY, gyroZ, accX, accY, accZ, calgyroX, calgyroY, calgyroZ, calaccX, calaccY, calaccZ;

// Create an instance of the BMP280 sensor
Adafruit_BMP280 bmp;

void i2cWrite(uint8_t address, uint8_t reg, uint8_t val) {
  Wire.beginTransmission(address);
  Wire.write(reg);
  Wire.write(val);
  Wire.endTransmission();
}

// Read 2 Bytes from a given register address i.e H and L
int16_t i2cRead(uint8_t address, int8_t reg) {
  int16_t temp = 0;
  Wire.beginTransmission(address);
  Wire.write(reg);
  Wire.endTransmission(false);
  Wire.requestFrom(address, 2, true);
  temp = (Wire.read() << 8 | Wire.read());
  return temp;
}

void sep(int a) {
  for (int i = 0; i < a; i++) {
    Serial.printf("_");
  }
  Serial.printf("\n");
}

// Função para ler a temperatura
void readTemperature() {
  int16_t rawTemp = i2cRead(MPU, TEMP_H); // Lê os dados de temperatura
  tempC = (rawTemp / 340.0) + 36.53; // Converte para Celsius
  Serial.printf("Temperatura: %.2f °C\n", tempC); // Imprime a temperatura
}

void printAcc(bool status) {
  sep(60);
  if (status) {
    Serial.printf("Caliberated AccX = ");
    Serial.println(calaccX);
    Serial.printf("Caliberated AccY = ");
    Serial.println(calaccY);
    Serial.printf("Caliberated AccZ = ");
    Serial.println(calaccZ);
  } else {
    Serial.printf("AccX = ");
    Serial.println(accX);
    Serial.printf("AccY = ");
    Serial.println(accY);
    Serial.printf("AccZ = ");
    Serial.println(accZ);
  }
}

void printGyro(bool status) {
  sep(60);
  if (status) {
    Serial.printf("Caliberated GyroX = ");
    Serial.println(calgyroX);
    Serial.printf("Caliberated GyroY = ");
    Serial.println(calgyroY);
    Serial.printf("Caliberated GyroZ = ");
    Serial.println(calgyroZ);
  } else {
    Serial.printf("GyroX = ");
    Serial.println(gyroX);
    Serial.printf("GyroY = ");
    Serial.println(gyroY);
    Serial.printf("GyroZ = ");
    Serial.println(gyroZ);
  }
}

void powMan() {
  // Access the Power Management Register 1 (107)
  // Configured at no sleep mode
  i2cWrite(MPU, MPU_POWER_REGISTER, 0);

  // Access the Power Management Register 2 (108)
  // Configured at all readings
  i2cWrite(MPU, MPU_MODE, 0);
}

// 0: 250 dps (131 LSB/dps); 1: 500 dps (65.5 LSB/dps) ; 2: 1000 dps ( 32.8 LSB/dps) ; 3: 2000 dps (16.4 LSB/dps)
void gyroConfig(uint8_t choice) {
  uint8_t val;
  switch (choice) {
    case 0:
      val = 0b00000001;
      break;
    case 1:
      val = 0b00001001;
      break;
    case 2:
      val = 0b00010001;
      break;
    case 3:
      val = 0b00011001;
      break;
    default:
      Serial.printf("Invalid input! By default Case 0 selected\n");
      val = 0b00000001;
      break;
  }

  // Accessing Register 26
  // Selecting Mode 0
  i2cWrite(MPU, GYRO_MODE, 0x00);

  // Accessing Register 27 (Gryo Config Register)
  // Setting the full scale range to 250dps
  i2cWrite(MPU, GYRO_CONFIG, val);
}

// 0: 2g (16384 LSB/g); 1: 4g (8192 LSB/g) ; 2: 8g (4096 LSB/g) ; 3: 16g (2048 LSB/g)
void accConfig(uint8_t choice) {
  uint8_t val;
  switch (choice) {
    case 0:
      val = 0b00000000;  // 0x00
      break;
    case 1:
      val = 0b00001000;  // 0x80
      break;
    case 2:
      val = 0b00010000;  // 0x10
      break;
    case 3:
      val = 0b00011000;  // 0x11
      break;
    default:
      Serial.printf("Invalid input! By default Case 0 selected\n");
      val = 0b00000000;  // 0x00
      break;
  }

  // Accessing Register 28 (Accel Config Initial)
  i2cWrite(MPU, ACCEL_INIT, val);

  // Accessing Register 29 (Accel Config Filter)
  // Selecting the mode ACCEL_CHOICE = 1 with BW = 1.13kHz
  i2cWrite(MPU, ACCEL_CONFIG, 0b00001000);
}

// Status will detect whether in caliberation mode or not -> true if in setup
void accData(bool status, int choice) {
  float rawVal;
  switch (choice) {
    case 0:
      rawVal = 16384.0;
      break;
    case 1:
      rawVal = 8192.0;
      break;
    case 2:
      rawVal = 4096.0;
      break;
    case 3:
      rawVal = 2048.0;
      break;
  }

  if (status) {
    // Registers 59 - 64: AccX_H, AccX_L, AccY_H, AccY_L, AccZ_H, AccZ_L {They provide raw value}
    unsigned int count = 0;
    unsigned long initTime = millis();
    Serial.printf("Do not move! Starting caliberation for %d seconds\n", (delay_cal / 1000));
    // In the loop for 3 seconds
    while (millis() - initTime <= delay_cal) {
      rawaccX += (i2cRead(MPU, accX_H)) / rawVal;
      rawaccY += (i2cRead(MPU, accY_H)) / rawVal;
      rawaccZ += (i2cRead(MPU, accZ_H)) / rawVal;
      count++;
    }
    Serial.printf("Count = %d\n", count);
    Serial.printf("Caliberation done!\n");
    calaccX = (float)rawaccX / count;
    calaccY = (float)rawaccY / count;
    calaccZ = (float)rawaccZ / count;
    printAcc(true);
  } else {
    // Registers 59 - 64: AccX_H, AccX_L, AccY_H, AccY_L, AccZ_H, AccZ_L {They provide raw value}
    accX = (i2cRead(MPU, accX_H) / rawVal) - calaccX;
    accY = (i2cRead(MPU, accY_H) / rawVal) - calaccY;
    accZ = (i2cRead(MPU, accZ_H) / rawVal) - calaccZ;
    printAcc(false);
  }
}

// 0: 250 dps (131 LSB/dps); 1: 500 dps (65.5 LSB/dps) ; 2: 1000 dps ( 32.8 LSB/dps) ; 3: 2000 dps (16.4 LSB/dps)
void gyroData(bool status, int choice) {
  float rawVal;
  switch (choice) {
    case 0:
      rawVal = 131.0;
      break;
    case 1:
      rawVal = 65.5;
      break;
    case 2:
      rawVal = 32.8;
      break;
    case 3:
      rawVal = 16.4;
      break;
  }
  // Registers 67-72: GyroX_H, GyroX_L, GyroY_H, GyroY_L, GyroZ_H, GyroZ_L {They provide raw value}
  if (status) {
    unsigned int count = 0;
    unsigned long initTime = millis();
    Serial.printf("Do not move! Starting caliberation for %d seconds\n", (delay_cal / 1000));
    // In the loop for 3 seconds
    while (millis() - initTime <= delay_cal) {
      rawgyroX += (i2cRead(MPU, gyroX_H)) / rawVal;
      rawgyroY += (i2cRead(MPU, gyroY_H)) / rawVal;
      rawgyroZ += (i2cRead(MPU, gyroZ_H)) / rawVal;
      count++;
    }
    Serial.printf("Count = %d\n", count);
    Serial.printf("Caliberation done!\n");
    calgyroX = (float)rawgyroX / count; 
    calgyroY = (float)rawgyroY / count;
    calgyroZ = (float)rawgyroZ / count;
    printGyro(true);
  } else {
    gyroX = (i2cRead(MPU, gyroX_H)) / rawVal - calgyroX;
    gyroY = (i2cRead(MPU, gyroY_H)) / rawVal - calgyroY;
    gyroZ = (i2cRead(MPU, gyroZ_H)) / rawVal - calgyroZ;
    printGyro(false);
  }
}

void setup() {
  Serial.begin(115200);
  // Join the bus as Controller
  Wire.begin();
  // Using the I2C communication at fast mode (400kHz)
  Wire.setClock(400000);
  // Initialize BMP280
  if (!bmp.begin(0x76)) { // Check the I2C address
    Serial.println("Could not find a valid BMP280 sensor, check wiring!");
    while (1);
  }
  // Calling the power management function
  powMan();
  sep(60);
  // Selection Mode 0: 250dps
  gyroConfig(GYRO_CONFIG_MODE);
  // Selection Mode 2: +-8g
  accConfig(ACC_CONFIG_MODE);
  // Caliberation for Acceleration
  accData(true, ACC_CONFIG_MODE);
  // Caliberation for Gyroscope
  gyroData(true, GYRO_CONFIG_MODE);
  delay(3000);
  sep(60);
}

void loop() {
  accData(false, ACC_CONFIG_MODE);
  gyroData(false, GYRO_CONFIG_MODE);
  readTemperature(); // Chama a função para ler e imprimir a temperatura

  // Read pressure and calculate altitude
  float pressure = bmp.readPressure() / 100.0F; // Convert to hPa
  float altitude = bmp.readAltitude(1013.25); // Assuming sea level pressure is 1013.25 hPa
  Serial.printf("Pressão: %.2f hPa\n", pressure);
  Serial.printf("Altura: %.2f m\n", altitude);
  
  sep(60);
  delay(300);
}