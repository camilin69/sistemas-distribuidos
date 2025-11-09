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

// Objetos
DHT dht(DHT_PIN, DHT22);
TinyGPSPlus gps;
SoftwareSerial gpsSerial(GPS_RX, GPS_TX);

// Variables
enum SystemState {
  STATE_START,
  STATE_LAUNCH, 
  STATE_END
};
const char* STATES[] = {
  "start",
  "launch", 
  "end"
};
SystemState stateCurrent = STATE_START;
unsigned long timestamp = 0;
String ADMIN_KEY = "playboi";
String launch_id = ""; 
bool transmitting = false;
int startTransmittingCount = 5;
int stopTransmittingCount = 5;
bool stopping = false;

unsigned long lastSendTime = 0;
bool requesting_id = false;
int reqAttempts = 0;
const int MAX_REQ_ATTEMPTS = 10;

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

void displayGPSInfo() {
  Serial.println("=== GPS INFO ===");
  
  if (gps.location.isValid()) {
    Serial.print("Latitude: ");
    Serial.println(gps.location.lat(), 6);
    Serial.print("Longitude: ");
    Serial.println(gps.location.lng(), 6);
    Serial.print("Altitude: ");
    Serial.print(gps.altitude.meters());
    Serial.println(" m");
    Serial.print("Speed: ");
    Serial.print(gps.speed.kmph());
    Serial.println(" km/h");
    Serial.print("Satellites: ");
    Serial.println(gps.satellites.value());
    
    if (gps.date.isValid()) {
      Serial.print("Date: ");
      Serial.print(gps.date.day());
      Serial.print("/");
      Serial.print(gps.date.month());
      Serial.print("/");
      Serial.println(gps.date.year());
    }
    
    if (gps.time.isValid()) {
      Serial.print("Time: ");
      Serial.print(gps.time.hour());
      Serial.print(":");
      Serial.print(gps.time.minute());
      Serial.print(":");
      Serial.println(gps.time.second());
    }
  } else {
    Serial.println("GPS: No data available");
    Serial.print("Satellites in view: ");
    Serial.println(gps.satellites.value());
  }
  
  Serial.println("================");
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


void hardResetLoRa() {
  Serial.println("Reiniciando módulo LoRa...");
  
  LoRa.end();
  delay(100);
  
  pinMode(LORA_RST, OUTPUT);
  digitalWrite(LORA_RST, LOW);
  delay(10);
  digitalWrite(LORA_RST, HIGH);
  delay(50);
  
  
  LoRa.setPins(LORA_NSS, LORA_RST, LORA_DI00);
  
  if (!LoRa.begin(433E6)) {
    Serial.println("Error iniciando LoRa después del reset!");
    return;
  }
  
  LoRa.setSpreadingFactor(7);
  LoRa.setSignalBandwidth(125E3);
  LoRa.setCodingRate4(5);
  LoRa.setSyncWord(0x12);
  Serial.println("Módulo LoRa reiniciado exitosamente");
}

void setup() {
  Serial.begin(9600);
  delay(3000);
  testPins();
  pinMode(BTN_START, INPUT_PULLUP);
  pinMode(BTN_STOP, INPUT_PULLUP);
  
  dht.begin();
  
  hardResetLoRa();
  Serial.println("Sistema listo - START para comenzar");
}

void loop() {
  start();
  stop();
  readGPS();
  unsigned long currentTime = millis();
  
  if (transmitting) {
    if (requesting_id) {
      if (currentTime - lastSendTime >= 2000) { // 2 segundos entre intentos
        cansatReqId();
        Serial.println("TX...REQUESTING_ID");
        lastSendTime = currentTime;
        
        // Incrementar intentos y verificar máximo
        reqAttempts++;
        transmitting = false;
        if (reqAttempts >= MAX_REQ_ATTEMPTS) {
          Serial.println("Máximo de intentos alcanzado - reiniciando LoRa");
          hardResetLoRa();
          reqAttempts = 0;
          requesting_id = false; 
          transmitting = false;
        }
      }
    } 
    else if (launch_id != "") {
      if (currentTime - lastSendTime >= 1000) {
        if (stopping) {
          stateCurrent = STATE_END;
          stopTransmittingCount--;
          Serial.print("STOP Transmitting Count: ");
          Serial.println(stopTransmittingCount);
          
          if (stopTransmittingCount <= 0) {
            transmitting = false;
            stopTransmittingCount = 5;
            startTransmittingCount = 5;
            stopping = false;
            stateCurrent = STATE_START;
            Serial.println("STOP - Transmisión finalizada");
          }
        } else if (startTransmittingCount > 0) {
          stateCurrent = STATE_START;
          startTransmittingCount--;
          Serial.print("START Transmitting Count: ");
          Serial.println(startTransmittingCount);
        } else {
          stateCurrent = STATE_LAUNCH;
        }
        
        sendLaunchData();
        lastSendTime = currentTime;
        Serial.print("Estado actual: ");
        Serial.println(STATES[stateCurrent]);
        
      }
    } else {
      requesting_id = true;
      lastSendTime = currentTime;
    }
  }
  
  delay(50);
}

void start() {
  if (digitalRead(BTN_START) == LOW) {
    delay(50);
    if (digitalRead(BTN_START) == LOW) {
      transmitting = true;
      requesting_id = true; 
      lastSendTime = millis();
      reqAttempts = 0; 
      Serial.println("START - Transmitiendo...");
    }
  }
}

void stop() {
  if (digitalRead(BTN_STOP) == LOW) {
    delay(50);
    if (digitalRead(BTN_STOP) == LOW) {
      if (launch_id == "") {
        transmitting = false;
        requesting_id = false;
        Serial.println("STOP - Transmisión cancelada (sin ID)");
      } else {
        stopping = true;
        stopTransmittingCount = 5;
        Serial.println("STOP - Iniciando secuencia de parada...");
      }
    }
  }
}

void sendLaunchData() {
  timestamp = millis();
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();
  
  String message = ADMIN_KEY + "-" + launch_id + "-" + STATES[stateCurrent] + "-" + timestamp + "-";
  
  if (!isnan(temperature) && !isnan(humidity)) {
    message += String(temperature, 1) + "-";
    message += String(humidity, 1) + "-";
  }
  
  String encryptedMessage = xorEncrypt(message);
  
  LoRa.beginPacket();
  LoRa.print(encryptedMessage);
  bool success = LoRa.endPacket();
  if (success) {
    Serial.print("SUCCESS TX, ");
    Serial.println("NOW RX...");
  }
}

void cansatReqId() {
  String message = ADMIN_KEY + "-CANSAT_REQ_ID";
  String encryptedMessage = xorEncrypt(message);
  
  Serial.print("TX REQ (intento ");
  Serial.print(reqAttempts);
  Serial.print("): ");
  Serial.println(encryptedMessage);
  
  LoRa.beginPacket();
  LoRa.print(encryptedMessage);
  LoRa.endPacket();
}

void onReceive() {
  Serial.println("Receiving after TX done...");
  if (LoRa.parsePacket() > 0) {
    String encryptedMessage = "";
    
    while (LoRa.available()) {
      encryptedMessage += (char)LoRa.read();
    }
    
    Serial.print("RX: ");
    Serial.println(encryptedMessage);
    
    String decryptedMessage = xorDecrypt(encryptedMessage);
    Serial.print("Decoded: ");
    Serial.println(decryptedMessage);
    
    if (decryptedMessage.startsWith(ADMIN_KEY + "-CANSAT_REQ_ID-")) {
      int i1 = decryptedMessage.indexOf("-");
      int i2 = decryptedMessage.indexOf("-", i1 + 1);
      launch_id = decryptedMessage.substring(i2 + 1);
      requesting_id = false;
      reqAttempts = 0; // Resetear intentos
      Serial.println("=== LAUNCH ID RECIBIDO: " + launch_id + " ===");
      
    }
  }
}