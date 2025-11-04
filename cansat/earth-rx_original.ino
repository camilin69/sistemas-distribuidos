#include <LoRa.h>

const byte XOR_KEYS[] = {0x2A, 0x4F, 0x31, 0x5C};
const int KEY_LENGTH = 4;
String ADMIN_KEY = "playboi";
String decryptedMessage = "";
String cansat_req_id = "";


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
  
  LoRa.begin(433E6);
  
  Serial.println("RX listo - Esperando datos...");
}

void loop() {
  if (LoRa.parsePacket() > 0) {
    String encryptedMessage = "";
    while (LoRa.available()) {
      encryptedMessage += (char)LoRa.read();
    }

    decryptedMessage = xorDecrypt(encryptedMessage);

    if(decryptedMessage == ("[" + ADMIN_KEY + "-CANSAT_REQ_ID]")) {
      if (cansat_req_id == "") {
        Serial.println(decryptedMessage);
        delay(2000);
        cansat_req_id = "[" + ADMIN_KEY + "-CANSAT_REQ_ID-1]";
      }
      // if(Serial.available() && cansat_req_id == "") {
      //   cansat_req_id = Serial.readStringUntil('\n');
      //   cansat_req_id.trim();
      //   Serial.println("LORA GOT: " + cansat_req_id);

      // }
      if(cansat_req_id.startsWith("[" + ADMIN_KEY + "-CANSAT_REQ_ID-")) {
        Serial.println("LORA SEND: " + cansat_req_id);

        LoRa.beginPacket();
        LoRa.print(xorEncrypt(cansat_req_id));
        LoRa.endPacket();
      }

    } else if (decryptedMessage.startsWith("[" + ADMIN_KEY)){
      Serial.println(encryptedMessage);
    }
  } 
  delay(100);
}