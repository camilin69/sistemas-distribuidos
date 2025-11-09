#include <TinyGPS++.h>
#include <LoRa.h>
#include <DHT.h>
#include <SoftwareSerial.h>

#define LORA_DI00 2
#define LORA_RST 9
#define LORA_NSS 10
#define BTN_START 3
#define BTN_STOP 4
#define DHT_PIN 6
#define GPS_TX A1
#define GPS_RX A0

DHT dht(DHT_PIN, DHT22);
TinyGPSPlus gps;
SoftwareSerial gpsSerial(GPS_RX, GPS_TX);

String ADMIN_KEY = "playboi";
String launch_id = "1"; 
bool transmitting = false;
unsigned long lastSendTime = 0;

unsigned long packetsSent = 0;
unsigned long packetsFailed = 0;
unsigned long lastDiagnostic = 0;

const byte XOR_KEYS[] = {0x2A, 0x4F, 0x31, 0x5C};
const int KEY_LENGTH = 4;

String xorEncrypt(const String& input) {
  String encrypted = "";
  for (int i = 0; i < input.length(); i++) {
    byte encryptedByte = input[i] ^ XOR_KEYS[i % KEY_LENGTH];
     // Convertir a hexadecimal para evitar caracteres problemáticos
    char hex[3];
    sprintf(hex, "%02X", encryptedByte);
    encrypted += hex;
  }
  return encrypted;
}

String xorDecrypt(const String& input) {
  String decrypted = "";
  for (int i = 0; i < input.length(); i += 2) {
    String hexByte = input.substring(i, i + 2);
    byte encryptedByte = strtol(hexByte.c_str(), NULL, 16);
    decrypted += (char)(encryptedByte ^ XOR_KEYS[(i/2) % KEY_LENGTH]);
  }
  return decrypted;
}

void readGPS() {
  while (gpsSerial.available() > 0) {
    gps.encode(gpsSerial.read());
  }
}

String getGPSData() {
  String gpsData = "";
  
  if (gps.location.isValid()) {
    gpsData += String(gps.location.lat(), 6) + ",";
    gpsData += String(gps.location.lng(), 6) + ",";
    gpsData += String(gps.altitude.meters(), 1) + ",";
    gpsData += String(gps.speed.kmph(), 1) + ",";
    gpsData += String(gps.satellites.value()) + ",";
    
    // Fecha y hora
    if (gps.date.isValid() && gps.time.isValid()) {
      gpsData += String(gps.date.year()) + "-";
      gpsData += String(gps.date.month()) + "-";
      gpsData += String(gps.date.day()) + " ";
      gpsData += String(gps.time.hour()) + ":";
      gpsData += String(gps.time.minute()) + ":";
      gpsData += String(gps.time.second());
    } else {
      gpsData += "NO_TIME";
    }
  } else {
    gpsData = "NO_GPS";
  }
  
  return gpsData;
}

void setup() {
  Serial.begin(9600);
  delay(3000); // Espera para inicializar serial
  
  LoRa.setPins(LORA_NSS, LORA_RST, LORA_DI00);
  testPins();
  if (!LoRa.begin(433E6)) {
    Serial.println("❌ FALLO CRITICO: LoRa no responde");
    return;
  }
  
  Serial.println("✅ LoRa iniciado en 433MHz");
}

void loop() {
  // Enviar paquete de prueba cada 2 segundos
  static unsigned long lastSend = 0;
  if (millis() - lastSend >= 2000) {
    sendTestPacket();
    lastSend = millis();
  }
  
  delay(100);
}

void sendTestPacket() {
  String packetID = String(packetsSent);
  String timestamp = String(millis());
  String message = "PKT#" + packetID + "|TIME:" + timestamp + "|CHECK:0x" + String(random(0x1000, 0xFFFF), HEX);
  
  Serial.print("📤 Enviando paquete #");
  Serial.print(packetID);
  Serial.print(": ");
  Serial.print(message);
  
  // Medir tiempo de transmisión
  unsigned long startTime = micros();
  
  LoRa.beginPacket();
  LoRa.print(message);
  bool success = LoRa.endPacket();
  
  unsigned long transmitTime = micros() - startTime;
  
  if (success) {
    packetsSent++;
    Serial.print(" ✅ (");
    Serial.print(transmitTime);
    Serial.println(" µs)");
    
    // Verificar si DIO0 se activa (paquete enviado)
    checkDIO0();
    
  } else {
    packetsFailed++;
    Serial.println(" ❌ FALLO EN ENVIO");
  }
}

void checkDIO0() {
  // Verificar si el pin DIO0 indica transmisión completada
  delay(10); // Pequeña espera para que se active
  bool dio0State = digitalRead(LORA_DI00);
  Serial.print("   DIO0 state: ");
  Serial.println(dio0State ? "HIGH" : "LOW");
}

void testPins() {
  Serial.println("🔌 TESTEO DE PINES:");
  
  // Verificar pines críticos
  int pins[] = {LORA_NSS, LORA_RST, LORA_DI00};
  String pinNames[] = {"NSS", "RST", "DIO0"};
  
  for (int i = 0; i < 3; i++) {
    pinMode(pins[i], OUTPUT);
    digitalWrite(pins[i], HIGH);
    delay(10);
    digitalWrite(pins[i], LOW);
    Serial.print("   Pin ");
    Serial.print(pinNames[i]);
    Serial.print(" (");
    Serial.print(pins[i]);
    Serial.println("): OK");
    delay(50);
  }
  Serial.println();
}

