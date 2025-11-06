#include <TinyGPS++.h>
#include <LoRa.h>
#include <Servo.h>
#include <DHT.h>
#include <SoftwareSerial.h>

#define LORA_DI00 2
#define LORA_RST 9
#define LORA_NSS 10
#define BTN_START 3
#define BTN_STOP 4
#define SERVO_PIN 8
#define DHT_PIN 6
#define GPS_TX A0
#define GPS_RX A1

// Objetos
Servo servo;
DHT dht(DHT_PIN, DHT22);

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

const char BASE64_CHARS[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

// Base64
String base64Encode(const String& input) {
  String output = "";
  int i = 0;
  int len = input.length();
  
  while (i < len) {
    uint32_t octet_a = i < len ? (unsigned char)input[i++] : 0;
    uint32_t octet_b = i < len ? (unsigned char)input[i++] : 0;
    uint32_t octet_c = i < len ? (unsigned char)input[i++] : 0;
    
    uint32_t triple = (octet_a << 0x10) + (octet_b << 0x08) + octet_c;
    
    output += BASE64_CHARS[(triple >> 3 * 6) & 0x3F];
    output += BASE64_CHARS[(triple >> 2 * 6) & 0x3F];
    output += BASE64_CHARS[(triple >> 1 * 6) & 0x3F];
    output += BASE64_CHARS[(triple >> 0 * 6) & 0x3F];
  }
  
  if (len % 3 == 1) {
    output[output.length()-1] = '=';
    output[output.length()-2] = '=';
  } else if (len % 3 == 2) {
    output[output.length()-1] = '=';
  }
  
  return output;
}

String base64Decode(const String& input) {
  String output = "";
  int i = 0;
  int len = input.length();
  
  while (i < len) {
    uint32_t sextet_a = input[i] == '=' ? 0 & i++ : (strchr(BASE64_CHARS, input[i++]) - BASE64_CHARS);
    uint32_t sextet_b = input[i] == '=' ? 0 & i++ : (strchr(BASE64_CHARS, input[i++]) - BASE64_CHARS);
    uint32_t sextet_c = input[i] == '=' ? 0 & i++ : (strchr(BASE64_CHARS, input[i++]) - BASE64_CHARS);
    uint32_t sextet_d = input[i] == '=' ? 0 & i++ : (strchr(BASE64_CHARS, input[i++]) - BASE64_CHARS);
    
    uint32_t triple = (sextet_a << 3 * 6) + (sextet_b << 2 * 6) + (sextet_c << 1 * 6) + (sextet_d << 0 * 6);
    
    output += (char)((triple >> 2 * 8) & 0xFF);
    if (input[i-2] != '=') output += (char)((triple >> 1 * 8) & 0xFF);
    if (input[i-1] != '=') output += (char)((triple >> 0 * 8) & 0xFF);
  }
  
  return output;
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
  delay(1000);
  
  pinMode(BTN_START, INPUT_PULLUP);
  pinMode(BTN_STOP, INPUT_PULLUP);
  
  servo.attach(SERVO_PIN);
  servo.write(0);
  dht.begin();
  
  hardResetLoRa();
  Serial.println("Sistema listo - START para comenzar");
}

void loop() {
  start();
  stop();
  
  unsigned long currentTime = millis();
  
  if (transmitting) {
    if (requesting_id) {
      if (currentTime - lastSendTime >= 2000) { // 2 segundos entre intentos
        cansatReqId();
        Serial.println("TX...REQUESTING_ID");
        lastSendTime = currentTime;
        
        // Incrementar intentos y verificar máximo
        reqAttempts++;
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
  
  receive();
  delay(50);
}

void start() {
  if (digitalRead(BTN_START) == LOW) {
    delay(50);
    if (digitalRead(BTN_START) == LOW) {
      transmitting = true;
      requesting_id = true; 
      lastSendTime = millis();
      reqAttempts = 0; // Resetear contador de intentos
      moveServo(180);
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
        moveServo(0);
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
  
  // ✅ SIN CHECKSUM - Formato simple que espera el publisher
  String encryptedMessage = base64Encode(message);
  
  LoRa.beginPacket();
  LoRa.print(encryptedMessage);
  LoRa.endPacket();
}

void cansatReqId() {
  // ✅ SIN CHECKSUM - Solo el mensaje simple
  String message = ADMIN_KEY + "-CANSAT_REQ_ID";
  String encryptedMessage = base64Encode(message);
  
  Serial.print("TX REQ (intento ");
  Serial.print(reqAttempts);
  Serial.print("): ");
  Serial.println(encryptedMessage);
  
  LoRa.beginPacket();
  LoRa.print(encryptedMessage);
  LoRa.endPacket();
}

void receive() {
  if (LoRa.parsePacket() > 0) {
    String encryptedMessage = "";
    
    // Leer con filtrado básico
    while (LoRa.available()) {
      char c = (char)LoRa.read();
      // Solo aceptar caracteres Base64 válidos
      if (isalnum(c) || c == '+' || c == '/' || c == '=') {
        encryptedMessage += c;
      }
    }
    
    // Solo procesar si tiene longitud razonable
    if (encryptedMessage.length() >= 20 && encryptedMessage.length() <= 100) {
      Serial.print("RX: ");
      Serial.println(encryptedMessage);
      
      String decryptedMessage = base64Decode(encryptedMessage);
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
    } else {
      Serial.println("Mensaje descartado - longitud inválida");
    }
  }
}

void moveServo(int angle) {
  servo.write(angle);
  delay(300);
}