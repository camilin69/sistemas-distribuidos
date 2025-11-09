#include <LoRa.h>
#include <Servo.h>
#include <DHT.h>
#include <SoftwareSerial.h>


// Pines
#define LORA_NSS 10
#define LORA_RST 9
#define LORA_DI00 2
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
bool transmitting = false;
int packetCount = 0;
unsigned long lastSendTime = 0;

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
  LoRa.setPins(LORA_NSS, LORA_RST, LORA_DI00);
  LoRa.begin(433E6);
  LoRa.setSpreadingFactor(7);
  LoRa.setSignalBandwidth(125E3);
  
  Serial.println("Sistema listo - START para comenzar");
}

void loop() {
  // Leer botones
  if (digitalRead(BTN_START) == LOW) {
    delay(50);
    transmitting = true;
    packetCount = 0;
    lastSendTime = millis();
    moveServo(90);
    Serial.println("START - Transmitiendo...");
    while(digitalRead(BTN_START) == LOW);
  }
  
  if (digitalRead(BTN_STOP) == LOW) {
    delay(50);
    transmitting = false;
    moveServo(0);
    Serial.println("STOP - Detenido");
    while(digitalRead(BTN_STOP) == LOW);
  }
  
  
  // Transmitir cada 1 segundo
  if (transmitting && (millis() - lastSendTime >= 1000)) {
    sendSensorData();
    lastSendTime = millis();
  }
  
  delay(50);
}

void sendSensorData() {
  // Leer sensores
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();
  
  // Crear mensaje
  String message = String(packetCount) + "|";
  
  
  // Datos DHT
  if (!isnan(temperature) && !isnan(humidity)) {
    message += String(temperature, 1) + "|";
    message += String(humidity, 1);
  } else {
    message += "0|0";
  }
  
  // Enviar
  LoRa.beginPacket();
  LoRa.print(message);
  LoRa.endPacket();
  
  Serial.println("TX: " + message);
  packetCount++;
}

void moveServo(int angle) {
  servo.write(angle);
  delay(300);
}