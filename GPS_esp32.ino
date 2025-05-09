#include <TinyGPS++.h>
#include <HardwareSerial.h>

// Instanciando o GPS
TinyGPSPlus gps;
HardwareSerial gpsSerial(1); // Usaremos UART1

// Define os pinos RX/TX do GPS no ESP32
#define GPS_RX 16
#define GPS_TX 17

void setup() {
  Serial.begin(115200);        // Monitor serial
  gpsSerial.begin(9600, SERIAL_8N1, GPS_RX, GPS_TX);  // Serial para GPS
  Serial.println("Iniciando GPS NEO-6M com ESP32...");
}

void loop() {
  // Lê dados do GPS
  while (gpsSerial.available() > 0) {
    gps.encode(gpsSerial.read());
  }

  // Se temos localização válida, mostra os dados
  if (gps.location.isUpdated()) {
    Serial.print("Latitude: ");
    Serial.println(gps.location.lat(), 6);

    Serial.print("Longitude: ");
    Serial.println(gps.location.lng(), 6);

    Serial.print("Satélites: ");
    Serial.println(gps.satellites.value());

    Serial.print("Altitude: ");
    Serial.println(gps.altitude.meters());

    Serial.print("Velocidade (km/h): ");
    Serial.println(gps.speed.kmph());

    Serial.print("Data: ");
    Serial.print(gps.date.day());
    Serial.print("/");
    Serial.print(gps.date.month());
    Serial.print("/");
    Serial.println(gps.date.year());

    Serial.print("Hora: ");
    Serial.print(gps.time.hour());
    Serial.print(":");
    Serial.print(gps.time.minute());
    Serial.print(":");
    Serial.println(gps.time.second());

    Serial.println("--------------------------");
    delay(1000); // Aguarda 1 segundo para nova leitura
  }
}
