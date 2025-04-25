#include <TinyGPS++.h>
#include <HardwareSerial.h>

// Cria uma instância do objeto TinyGPS++
TinyGPSPlus gps;

// Configura a porta serial para o GPS
HardwareSerial SerialGPS(1); // Usando a Serial 1 do ESP32

void setup() {
  // Inicia a comunicação serial com o computador
  Serial.begin(115200);
  
  // Inicia a comunicação serial com o GPS
  SerialGPS.begin(9600, SERIAL_8N1, 16, 17); // RX, TX

  Serial.println("Aguardando dados do GPS...");
}

void loop() {
  // Verifica se há dados disponíveis no GPS
  while (SerialGPS.available() > 0) {
    gps.encode(SerialGPS.read());
    
    // Se temos uma nova localização, exibe os dados
    if (gps.location.isUpdated()) {
      Serial.print("Latitude: ");
      Serial.print(gps.location.lat(), 6); // 6 casas decimais
      Serial.print(" | Longitude: ");
      Serial.print(gps.location.lng(), 6); // 6 casas decimais
      Serial.print(" | Altitude: ");
      Serial.print(gps.altitude.meters());
      Serial.println(" m");
    }
    
    // Exibe a velocidade, se disponível
    if (gps.speed.isUpdated()) {
      Serial.print("Velocidade: ");
      Serial.print(gps.speed.kmph());
      Serial.println(" km/h");
    }
    
    // Exibe a data e hora, se disponível
    if (gps.date.isUpdated() && gps.time.isUpdated()) {
      Serial.print("Data: ");
      Serial.print(gps.date.day());
      Serial.print("/");
      Serial.print(gps.date.month());
      Serial.print("/");
      Serial.print(gps.date.year());
      Serial.print(" Hora: ");
      Serial.print(gps.time.hour());
      Serial.print(":");
      Serial.print(gps.time.minute());
      Serial.print(":");
      Serial.print(gps.time.second());
      Serial.println();
    }
  }
}