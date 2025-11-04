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


// XOR Encryption
const byte XOR_KEYS[] = {0x2A, 0x4F, 0x31, 0x5C};
const int KEY_LENGTH = 4;


const char BASE64_CHARS[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

// Base64 robusto y probado
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
  
  // Padding
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
  
  LoRa.setPins(LORA_NSS, LORA_RST, LORA_DI00);
  // Inicializar LoRa
  LoRa.begin(433E6);
  LoRa.setSyncWord(0x12);

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
      if (currentTime - lastSendTime >= 1000) {
        cansatReqId();
        Serial.println("TX...REQUESTING_ID");
        lastSendTime = currentTime;
        delay(300);
        receive();

      }
    } else if (launch_id != "") {
      if (currentTime - lastSendTime >= 500 && startTransmittingCount > 0) {
        startTransmittingCount--;
      }else if(currentTime - lastSendTime >= 500 && startTransmittingCount == 0) {
        stateCurrent = STATE_LAUNCH;
        startTransmittingCount = -1;
      }

      if(currentTime - lastSendTime >= 500 && stopTransmittingCount > 0 && stopping) {
        stateCurrent = STATE_END;
        stopTransmittingCount--;
      } else if(currentTime - lastSendTime >= 500 && stopTransmittingCount == 0 && stopping) {
        transmitting = false;
        stopTransmittingCount = 5;
        startTransmittingCount = 5;
        stopping = false;
        Serial.println("STOP - Detenido");
      }

      sendLaunchData();
      lastSendTime = currentTime;
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
      if(launch_id == "") transmitting = false;
      requesting_id = false;
      moveServo(0);
      launch_id = "";
      Serial.println("STOP - Deteniendo");
      stopping = true;
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
  
  String encryptedMessage = base64Encode(message);
  
  LoRa.beginPacket();
  LoRa.print(encryptedMessage);
  LoRa.endPacket(true);
}

void cansatReqId() {
  String message = ADMIN_KEY + "-CANSAT_REQ_ID";
  String encryptedMessage = base64Encode(message);
  Serial.print(encryptedMessage);
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
    Serial.print("RX: ");
    Serial.println(encryptedMessage);
    String decryptedMessage = base64Decode(encryptedMessage);
    Serial.println(decryptedMessage);
    
    if(decryptedMessage.startsWith(ADMIN_KEY + "-CANSAT_REQ_ID-")) {
      int i1 = decryptedMessage.indexOf("-");
      int i2 = decryptedMessage.indexOf("-", i1 + 1);
      launch_id = decryptedMessage.substring(i2 + 1);
      requesting_id = false;
      Serial.println("LAUNCH ID: " + launch_id);
    }
  }
}

void moveServo(int angle) {
  servo.write(angle);
  delay(300);
}