#include <LoRa.h>
#include <Servo.h>
#include <DHT.h>
#include <SoftwareSerial.h>


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
unsigned long lastSendTime = 0;
bool requesting_id = false;

// Variables para el parpadeo del LED
unsigned long lastBlinkTime = 0;
bool ledState = false;

// XOR Encryption
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

void setup() {
  Serial.begin(9600);
  delay(1000);
  
  // Configurar pines
  pinMode(BTN_START, INPUT_PULLUP);
  pinMode(BTN_STOP, INPUT_PULLUP);
  
  // Inicializar componentes
  servo.attach(SERVO_PIN);
  servo.write(0);
  dht.begin();
  
  // Inicializar LoRa
  LoRa.begin(433E6);
  Serial.println("Sistema listo - START para comenzar");
}

void loop() {
  // Leer botones
  start();
  stop();
  
  
  // Lógica de transmisión con control de tiempo
  unsigned long currentTime = millis();
  
  if (transmitting) {
    if (requesting_id) {
      // Solicitar ID cada 2 segundos
      if (currentTime - lastSendTime >= 2000) {
        cansatReqId();
        Serial.println("TX...REQUESTING_ID");
        lastSendTime = currentTime;
        delay(100);
        receive();

      }
    } else if (launch_id != "") {
      // Transmitir datos cada 1 segundo
      if (currentTime - lastSendTime >= 1000) {
        sendLaunchData();
        Serial.println("TX...LAUNCH_DATA");
        lastSendTime = currentTime;
      }
    } else {
      // Si no tenemos ID pero no estamos solicitando, comenzar a solicitar
      requesting_id = true;
      lastSendTime = currentTime;
    }
  }
  
  delay(50);
}

void start() {
  if (digitalRead(BTN_START) == LOW) {
    delay(50); // Debounce
    if (digitalRead(BTN_START) == LOW) {
      transmitting = true;
      requesting_id = true; 
      lastSendTime = millis();
      moveServo(180);
      Serial.println("START - Transmitiendo...");
    }
  }
}

void stop() {
  if (digitalRead(BTN_STOP) == LOW) {
    delay(50); // Debounce
    if (digitalRead(BTN_STOP) == LOW) {
      transmitting = false;
      requesting_id = false;
      moveServo(0);
      launch_id = "";
      Serial.println("STOP - Detenido");
    }
  }
}

void sendLaunchData() {
  timestamp = millis();
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();
  
  String message = "[" + ADMIN_KEY + "|" + launch_id + "|" + STATES[stateCurrent] + "|" + timestamp + "|";
  
  
  if (!isnan(temperature) && !isnan(humidity)) {
    message += String(temperature, 1) + "|";
    message += String(humidity, 1) + "]";
  }
  
  String encryptedMessage = xorEncrypt(message);
  
  LoRa.beginPacket();
  LoRa.print(encryptedMessage);
  LoRa.endPacket(true);
}

void cansatReqId() {
  String message = "[" + ADMIN_KEY + "-CANSAT_REQ_ID]";
  String encryptedMessage = xorEncrypt(message);

  LoRa.beginPacket();
  LoRa.print(encryptedMessage);
  LoRa.endPacket(true);
}

void receive () {
  if(LoRa.parsePacket() > 0) {
    String encryptedMessage = "";

    while(LoRa.available() ) {
      encryptedMessage += (char)LoRa.read();
    }

    String decryptedMessage = xorDecrypt(encryptedMessage);
    Serial.print("RX: ");
    Serial.println(decryptedMessage);
    
    if(decryptedMessage.startsWith("[" + ADMIN_KEY + "-CANSAT_REQ_ID-")) {
      int i1 = decryptedMessage.indexOf("-");
      int i2 = decryptedMessage.indexOf("-", i1 + 1);
      int i3 = decryptedMessage.indexOf("]", i2 + 1);
      launch_id = decryptedMessage.substring(i2 + 1, i3);
      requesting_id = false;
      Serial.println("LAUNCH ID: " + launch_id);
    }
  }
}

void moveServo(int angle) {
  servo.write(angle);
  delay(300);
}