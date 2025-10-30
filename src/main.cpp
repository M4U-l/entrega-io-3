#include <ClosedCube_HDC1080.h>
#include <Wire.h>
#include <SoftwareSerial.h>
#include <TinyGPSPlus.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

// ==== CONFIG WiFi ====
const char* ssid = "UPBWiFi";          // <-- Poner nombre de tu red WiFi
const char* password = "";  // <-- Poner clave de tu red WiFi

// ==== CONFIG GPS ====
static const int RXPin = 2, TXPin = 0;    
static const uint32_t GPSBaud = 9600;
SoftwareSerial ss(RXPin, TXPin);
TinyGPSPlus gps;

// ==== SENSOR HDC1008 ====
ClosedCube_HDC1080 sensor;

// ==== Variables ====
float temperatura = 0;
float humedad = 0;
String gpsData = "";

// ==== SmartDelay que sigue procesando GPS ====
void smartDelay(unsigned long ms) {
  unsigned long start = millis();
  do {
    while (ss.available()) {
      gps.encode(ss.read());
    }
  } while (millis() - start < ms);
}

void setup() {
  Serial.begin(115200);
  Wire.begin();
  sensor.begin(0x40);
  ss.begin(GPSBaud);

  Serial.println("Iniciando sistema IoT...");

  // Conexión WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConectado a WiFi");
}

void loop() {
  // === PRUNING: Tomar 3 muestras y promediar ===
  float tempSum = 0;
  float humSum = 0;

  for (int i = 0; i < 3; i++) {
    float t = sensor.readTemperature();
    float h = sensor.readHumidity();

    tempSum += t;
    humSum += h;

    Serial.printf("Muestra %d -> T=%.2f°C H=%.2f%%\n", i+1, t, h);

    smartDelay(1000); // 1s entre cada muestra (procesa GPS mientras espera)
  }

  temperatura = tempSum / 3.0;
  humedad = humSum / 3.0;

  Serial.printf("Promedio -> T=%.2f°C  H=%.2f%%\n", temperatura, humedad);

  // === Leer GPS durante 2 segundos ===
  unsigned long start = millis();
  while (millis() - start < 2000) {
    while (ss.available() > 0) {
      gps.encode(ss.read());
    }
  }

  if (gps.location.isUpdated()) {
    gpsData = String(gps.location.lat(), 6) + "," + String(gps.location.lng(), 6);
  } else {
    gpsData = "NA,NA";
  }
  Serial.println("GPS: " + gpsData);

  // === Enviar datos al servidor ===
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    WiFiClient client;

    http.begin(client, "http://98.85.128.81/update_data");  
    http.addHeader("Content-Type", "application/json");

    String jsonData = "{";
    jsonData += "\"id\":\"point01\",";
    jsonData += "\"lat\":" + String(gps.location.lat(), 6) + ",";
    jsonData += "\"lon\":" + String(gps.location.lng(), 6) + ",";
    jsonData += "\"temperatura\":" + String(temperatura, 2) + ",";
    jsonData += "\"humedad\":" + String(humedad, 2);
    jsonData += "}";

    int httpResponseCode = http.POST(jsonData);

    Serial.print("POST -> "); Serial.println(jsonData);
    Serial.print("Respuesta: "); Serial.println(httpResponseCode);

    http.end();
  } else {
    Serial.println("WiFi desconectado, no se enviaron datos");
  }

  // === Esperar 10 segundos antes de la próxima actualización ===
  smartDelay(10000);
}
