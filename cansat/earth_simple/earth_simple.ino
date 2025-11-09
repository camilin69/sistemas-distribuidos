#include <LoRa.h>

#define LORA_DI00 2
#define LORA_RST 9
#define LORA_NSS 10

String ADMIN_KEY = "playboi";
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
  LoRa.onReceive(onReceive);
  LoRa.receive();
  
  Serial.println("Módulo LoRa reiniciado exitosamente");
}

void setup() {
  Serial.begin(9600);
  hardResetLoRa();
  Serial.println("RX listo - Esperando datos...");
}

void loop() {
  
}

void onReceive (int packetSize) {
  if (packetSize == 0) return;

  String encryptedMessage = "";
  while (LoRa.available()) {
    encryptedMessage += (char)LoRa.read();
  }
  
  String decryptedMessage = xorDecrypt(encryptedMessage);

  // LÓGICA ESPECÍFICA PARA TRABAJAR CON EL PUBLISHER
  if (decryptedMessage.startsWith(ADMIN_KEY)) Serial.println(decryptedMessage);
  
  
}