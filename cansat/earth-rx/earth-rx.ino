#include <LoRa.h>

#define LORA_DI00 2
#define LORA_RST 9
#define LORA_NSS 10

String ADMIN_KEY = "playboi";
String cansat_req_id = "";

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
  
  cansat_req_id = "";
  
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
  hardResetLoRa();
  Serial.println("RX listo - Esperando datos...");
}

void loop() {
  if (LoRa.parsePacket() > 0) {
    String encryptedMessage = "";
    
    // ✅ FILTRO CRÍTICO: Solo aceptar caracteres Base64 válidos
    while (LoRa.available()) {
      char c = (char)LoRa.read();
      // Solo aceptar caracteres Base64 válidos
      if (isalnum(c) || c == '+' || c == '/' || c == '=') {
        encryptedMessage += c;
      }
    }

    // Solo procesar si el mensaje tiene longitud razonable
    if (encryptedMessage.length() >= 20 && encryptedMessage.length() <= 100) {
      Serial.print("Packet received: ");
      Serial.println(encryptedMessage);
      
      String decryptedMessage = base64Decode(encryptedMessage);
      Serial.print("Decoded: ");
      Serial.println(decryptedMessage);

      // LÓGICA ESPECÍFICA PARA TRABAJAR CON EL PUBLISHER
      if(decryptedMessage == (ADMIN_KEY + "-CANSAT_REQ_ID")) {
        if (cansat_req_id == "") {
          Serial.println(decryptedMessage);
          delay(2000);
        }
        if(Serial.available() && cansat_req_id == "") {
          cansat_req_id = Serial.readStringUntil('\n');
          cansat_req_id.trim();
          Serial.println("LORA GOT: " + cansat_req_id);
        }
        if(cansat_req_id.startsWith(ADMIN_KEY + "-CANSAT_REQ_ID-")) {
          Serial.println("LORA SEND: " + cansat_req_id);
          encryptedMessage = base64Encode(cansat_req_id);
          LoRa.beginPacket();
          LoRa.print(encryptedMessage);
          LoRa.endPacket();
        }
      } else if (decryptedMessage.startsWith(ADMIN_KEY)){
        Serial.println(decryptedMessage);
        decryptedMessage = "";
      }
    } else {
      Serial.println("Mensaje descartado - longitud inválida");
    }
  } 
  delay(100);
}