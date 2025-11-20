#include <TinyGPS++.h>
#include <LoRa.h>
#include <DHT.h>
#include <SoftwareSerial.h>

#define LORA_DI00 2
#define LORA_RST 9
#define LORA_NSS 10
#define BTN_START 7
#define BTN_STOP 8
#define DHT_PIN 6
#define GPS_TX 3
#define GPS_RX 4

// Objetos
DHT dht(DHT_PIN, DHT22);
TinyGPSPlus gps;
SoftwareSerial GPS(GPS_RX, GPS_TX);

// Variables
String ADMIN_KEY = "playboi";
String launch_id = "4"; 
bool transmitting = false;
unsigned long lastSendTime = 0;
bool idAsked = false;

// XOR Encryption
const byte XOR_KEYS[] = {0x2A, 0x4F, 0x31, 0x5C};
const int KEY_LENGTH = 4;

void checkDIO0() {
  delay(10); 
  bool dio0State = digitalRead(LORA_DI00);
}

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

void testPins() {
  
  int pins[] = {LORA_NSS, LORA_RST, LORA_DI00};
  String pinNames[] = {"NSS", "RST", "DIO0"};
  
  for (int i = 0; i < 3; i++) {
    pinMode(pins[i], OUTPUT);
    digitalWrite(pins[i], HIGH);
    delay(10);
    digitalWrite(pins[i], LOW);
    delay(50);
  }
}

void readGPS() {
  while (GPS.available() > 0) {
    gps.encode(GPS.read());
  }
}

String getGPSData() {
  String gpsData = "";
  
  if (gps.location.isValid()) {
    gpsData += "*" + String(gps.location.lat(), 6);
    gpsData += "*" + String(gps.location.lng(), 6);
    gpsData += "*" + String(gps.altitude.meters(), 1); 
  } 
  
  return gpsData;
}


void hardResetLoRa() {
  LoRa.end();
  delay(100);
  
  pinMode(LORA_RST, OUTPUT);
  digitalWrite(LORA_RST, LOW);
  delay(10);
  digitalWrite(LORA_RST, HIGH);
  delay(50);
  
  LoRa.setPins(LORA_NSS, LORA_RST, LORA_DI00);
  
  if (!LoRa.begin(433E6)) {
    return;
  }

  LoRa.setSpreadingFactor(7);
  LoRa.setSignalBandwidth(125E3);
  LoRa.setCodingRate4(5);
  LoRa.setSyncWord(0x12);
}


void setup() {
  Serial.begin(9600);
  GPS.begin(9600);
  delay(3000);
  testPins();
  pinMode(BTN_START, INPUT_PULLUP);
  pinMode(BTN_STOP, INPUT_PULLUP);
  dht.begin();
  hardResetLoRa();
}

void loop() {
  start();
  stop();

  readGPS();
  
  if (transmitting && launch_id != "") {
    if (millis() - lastSendTime >= 1000) {
      sendLaunchData();
      lastSendTime = millis();
    }
  }
  delay(100);
}

void start() {
  if (digitalRead(BTN_START) == LOW) {
    delay(50);
    if (digitalRead(BTN_START) == LOW) {
      transmitting = true;
      lastSendTime = millis();
    }
  }
}

void stop() {
  if (digitalRead(BTN_STOP) == LOW) {
    delay(50);
    if (digitalRead(BTN_STOP) == LOW) {
      transmitting = false;
    }
  }
}

void sendLaunchData() {
  String timestamp = String(millis());
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();
  String gpsData = getGPSData();

  String message = ADMIN_KEY + "*" + launch_id + "*" + timestamp + "*";
  
  if (!isnan(temperature) && !isnan(humidity)) {
    message += String(temperature, 1) + "*";
    message += String(humidity, 1);
  }
  message += gpsData;

  String encryptedMessage = xorEncrypt(message);
  LoRa.beginPacket();
  LoRa.print(encryptedMessage);
  bool success = LoRa.endPacket();
  if (success) {
    checkDIO0();
  }  
}

void sendTestGPSData() {
  String timestamp = String(millis());
  String gpsData = getGPSData();

  // Solo enviamos datos del GPS
  String message = ADMIN_KEY + "*" + launch_id + "*" + timestamp + "*" + gpsData;

  String encryptedMessage = xorEncrypt(message);
  
  LoRa.beginPacket();
  LoRa.print(encryptedMessage);
  bool success = LoRa.endPacket();
  if (success) {
    checkDIO0();
  }  
}
