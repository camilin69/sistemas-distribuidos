#include <LoRa.h>

// Pines LoRa
#define LORA_DI00 2
#define LORA_RST 9
#define LORA_NSS 10

// Variables de diagnóstico
unsigned long packetsSent = 0;
unsigned long packetsFailed = 0;
unsigned long lastDiagnostic = 0;

void setup() {
  Serial.begin(9600);
  delay(3000); // Espera para inicializar serial
  
  Serial.println();
  Serial.println("=== DIAGNOSTICO COMPLETO LoRa TX ===");
  Serial.println();
  
  // Test de pines primero
  testPins();
  
  // Inicializar LoRa
  Serial.println("🔄 INICIALIZANDO LoRa...");
  LoRa.setPins(LORA_NSS, LORA_RST, LORA_DI00);
  
  if (!LoRa.begin(433E6)) {
    Serial.println("❌ FALLO CRITICO: LoRa no responde");
    return;
  }
  
  Serial.println("✅ LoRa iniciado en 433MHz");
  
  Serial.println();
  Serial.println("📡 INICIANDO TRANSMISION DE PRUEBA...");
  Serial.println("   Cada 2 segundos se enviará un paquete");
  Serial.println("   Presione cualquier tecla para detener");
  Serial.println("======================================");
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

