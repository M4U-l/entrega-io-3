#include <WiFi.h>
#include <Wire.h>
#include <ClosedCube_HDC1080.h>
#include <TinyGPSPlus.h>
#include <HardwareSerial.h>

// ==== CONFIG WiFi ====
const char* ssid = "UPBWiFi";        // <-- Nombre de tu red WiFi
const char* password = "";            // <-- Clave de tu red WiFi (si tiene)

// ==== CONFIG GPS ====
static const uint32_t GPSBaud = 9600;
TinyGPSPlus gps;

// Usamos el puerto serie hardware para GPS en T-Beam
HardwareSerial GPSSerial(1);

// ==== SENSOR HDC1080 ====
ClosedCube_HDC1080 sensor;

// ==== Variables ====
float temperatura = 0;
float humedad = 0;
String gpsData = "";

// ==== FUNCIONES ====

void smartDelay(unsigned long ms) {
  unsigned long start = millis();
  do {
    while (GPSSerial.available()) {
      gps.encode(GPSSerial.read());
    }
  } while (millis() - start < ms);
}

void setup() {
  Serial.begin(115200);
  Wire.begin();
  sensor.begin(0x40);

  // Inicializa GPS: RX=34, TX=12 (T-Beam estándar)
  GPSSerial.begin(GPSBaud, SERIAL_8N1, 34, 12);

  Serial.println("Iniciando sistema IoT...");

  // ==== Conexión WiFi ====
  WiFi.begin(ssid, password);
  Serial.print("Conectando a WiFi ");
  Serial.print(ssid);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n✅ Conectado a WiFi");
  Serial.print("IP local: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  Serial.println("\n===== CICLO DE MEDICIÓN =====");

  // === PRUNING: Tomar 3 muestras y promediar ===
  float tempSum = 0;
  float humSum = 0;

  for (int i = 0; i < 3; i++) {
    float t = sensor.readTemperature();
    float h = sensor.readHumidity();

    tempSum += t;
    humSum += h;

    Serial.printf("📍 Muestra %d -> T=%.2f°C | H=%.2f%%\n", i + 1, t, h);

    smartDelay(1000); // 1s entre cada muestra (procesa GPS mientras espera)
  }

  temperatura = tempSum / 3.0;
  humedad = humSum / 3.0;

  Serial.printf("🌡️  Promedio (pruning) -> T=%.2f°C | H=%.2f%%\n", temperatura, humedad);

  // === Leer GPS durante 2 segundos ===
  unsigned long start = millis();
  while (millis() - start < 2000) {
    while (GPSSerial.available() > 0) {
      gps.encode(GPSSerial.read());
    }
  }

  if (gps.location.isValid()) {
    gpsData = String(gps.location.lat(), 6) + "," + String(gps.location.lng(), 6);
  } else {
    gpsData = "NA,NA";
  }
  Serial.println("🛰️  GPS -> " + gpsData);

  // === Simular envío al servidor ===
  if (WiFi.status() == WL_CONNECTED) {
    String jsonData = "{";
    jsonData += "\"id\":\"point01\",";
    jsonData += "\"lat\":" + String(gps.location.lat(), 6) + ",";
    jsonData += "\"lon\":" + String(gps.location.lng(), 6) + ",";
    jsonData += "\"temperatura\":" + String(temperatura, 2) + ",";
    jsonData += "\"humedad\":" + String(humedad, 2);
    jsonData += "}";

    // Aquí podrías usar HTTPClient, MQTT o LoRa según el caso.
    Serial.println("📤 Datos enviados (simulado): " + jsonData);
  } else {
    Serial.println("⚠️  WiFi desconectado, no se enviaron datos");
  }

  // Esperar antes del siguiente ciclo
  smartDelay(10000);
}
