#include <SPI.h>
#include <LoRa.h>
#include <OLEDDisplay.h>
#include "board_def.h"

OLED_CLASS_OBJ display(OLED_ADDRESS, OLED_SDA, OLED_SCL);

void setup() {
  Serial.begin(115200);
  while(!Serial);

  display.init();
  display.clear();
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.drawString(0, 0, "LoRa Receptor");
  display.display();
  delay(2000);

  SPI.begin(CONFIG_CLK, CONFIG_MISO, CONFIG_MOSI, CONFIG_NSS);
  LoRa.setPins(CONFIG_NSS, CONFIG_RST, CONFIG_DIO0);
  if(!LoRa.begin(BAND)){
    Serial.println("Erro inicializacao LoRa");
    while(1);
  }
  Serial.println("LoRa iniciado");
}

void loop() {
  int packetSize = LoRa.parsePacket();
  if(packetSize){
    String recv = "";
    while(LoRa.available()){
      recv += (char)LoRa.read();
    }

    Serial.println("Dados recebidos:");
    Serial.println(recv);

    // Variáveis para os dados completos
    float tempMPU=0, tempBMP=0, altitude=0;
    float accX=0, accY=0, accZ=0;
    float gyroX=0, gyroY=0, gyroZ=0;
    float lat=0, lon=0;
    int day=0, month=0, year=0;
    int hour=0, minute=0, second=0;

    int parsed = sscanf(recv.c_str(),
      "TempMPU:%f,TempBMP:%f,Alt:%f,AccX:%f,AccY:%f,AccZ:%f,GyroX:%f,GyroY:%f,GyroZ:%f,Lat:%f,Lon:%f,Date:%d/%d/%d,Time:%d:%d:%d",
      &tempMPU, &tempBMP, &altitude,
      &accX, &accY, &accZ,
      &gyroX, &gyroY, &gyroZ,
      &lat, &lon,
      &day, &month, &year,
      &hour, &minute, &second);

    if(parsed == 18){
      // Exibe latitude e longitude no OLED
      display.clear();
      display.drawString(0, 10, "Latitude:");
      display.drawString(0, 24, String(lat, 6));
      display.drawString(0, 40, "Longitude:");
      display.drawString(0, 54, String(lon, 6));
      display.display();

      // Exibe no serial os dados completos
      Serial.printf("Temperatura MPU: %.2f C, Temperatura BMP: %.2f C, Altitude: %.2f m\n", tempMPU, tempBMP, altitude);
      Serial.printf("Acelerometro (g): X=%.3f Y=%.3f Z=%.3f\n", accX, accY, accZ);
      Serial.printf("Giroscopio (deg/s): X=%.3f Y=%.3f Z=%.3f\n", gyroX, gyroY, gyroZ);
      Serial.printf("Data: %02d/%02d/%04d  Hora: %02d:%02d:%02d\n", day, month, year, hour, minute, second);
    }
    else {
      Serial.println("Erro ao interpretar dados recebidos");
    }
  }
  delay(100);
}
