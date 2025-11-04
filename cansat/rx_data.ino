#include <LoRa.h>

#define LORA_NSS 10
#define LORA_RST 9
#define LORA_DI00 2

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  LoRa.setPins(LORA_NSS, LORA_RST, LORA_DI00);
  LoRa.begin(433E6);
  LoRa.setSpreadingFactor(7);
  LoRa.setSignalBandwidth(125E3);
  
  Serial.println("RX listo - Esperando datos...");
}

void loop() {
  if (LoRa.parsePacket() > 0) {
    String message = "";
    while (LoRa.available()) {
      message += (char)LoRa.read();
    }
    Serial.print("RX: ");
    Serial.println(message);
  }
  delay(100);
}