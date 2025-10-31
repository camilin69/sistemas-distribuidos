/**
* ADMIN - Sistema central de comunicación LoRa con cifrado XOR mejorado
*/

#define HELTEC_POWER_BUTTON
#include <heltec_unofficial.h>

// ========== CONFIGURACIÓN LORA ==========
#define FREQUENCY           866.3
#define BANDWIDTH           250.0
#define SPREADING_FACTOR    7
#define TRANSMIT_POWER      14

// ========== VARIABLES GLOBALES ==========
String mensaje;
volatile bool rxFlag = false;
String state = "1";
String llave_proteccion_rx = "6969";
String llave_proteccion_tx = "7777";

// ========== CIFRADO XOR MEJORADO ==========
const uint8_t XOR_KEYS[] = {0x2A, 0x4F, 0x31, 0x5C}; // *, O, 1, \ (clave de 4 bytes)

String cifrar_xor(const String& texto) {
  String resultado = texto;
  for (int i = 0; i < resultado.length(); i++) {
    resultado[i] = resultado[i] ^ XOR_KEYS[i % 4]; // Rota entre las 4 claves
  }
  return resultado;
}

String descifrar_xor(const String& texto_cifrado) {
  return cifrar_xor(texto_cifrado); // XOR es simétrico
}

// ========== DECLARACIÓN DE FUNCIONES ==========
String Start_Rx();
void Start_Tx(String data);

void setup() {
 heltec_setup();
 
 radio.begin();
 radio.setDio1Action(rx);
 radio.setFrequency(FREQUENCY);
 radio.setBandwidth(BANDWIDTH);
 radio.setSpreadingFactor(SPREADING_FACTOR);
 radio.setOutputPower(TRANSMIT_POWER);
 radio.startReceive(RADIOLIB_SX126X_RX_TIMEOUT_INF);
 
 both.println("Admin listo con cifrado XOR mejorado\n");
}

void loop() {
  heltec_loop();
  
  static bool lastButtonState = HIGH;
  static unsigned long lastButtonTime = 0;
  
  bool currentButton = digitalRead(0);

  if (currentButton == LOW && lastButtonState == HIGH && (millis() - lastButtonTime) > 300) {
    state = (state == "1") ? "2" : "1";
    both.println("CAMBIANDO A ESTADO " + state);
    lastButtonTime = millis();
  }
  lastButtonState = currentButton;
  
  if(Serial.available() > 0) {
    mensaje = Serial.readStringUntil('\n');  // CAMBIA ESTA LÍNEA
    mensaje.trim();
    
    if(mensaje == "estado") {
      Serial.println(state);
    } else {
      // Descifrar mensaje entrante
      String mensaje_descifrado = descifrar_xor(mensaje);
      both.println("Mensaje recibido: " + mensaje);
      both.println("Mensaje descifrado: " + mensaje_descifrado);
      
      if(mensaje_descifrado.startsWith(llave_proteccion_rx)) {
        String llave_tx;
        String operacion = (state == "1") ? "registrar-" : "recibir-";
        llave_tx = operacion + llave_proteccion_tx + "-" + mensaje_descifrado.substring(mensaje_descifrado.indexOf('-') + 1);
        both.println("LLAVE: " + llave_tx);
        // Cifrar antes de transmitir
        String llave_tx_cifrada = cifrar_xor(llave_tx);
        Start_Tx(llave_tx_cifrada);
        delay(1000);
      }
    }
  }
 
  if (rxFlag) {
    String mensaje_recibido = Start_Rx();
    if (mensaje_recibido.length() > 0) {
      // Descifrar mensaje LoRa
      String mensaje_descifrado = descifrar_xor(mensaje_recibido);
      Serial.println(mensaje_descifrado);
    }
  }
}

void rx() {
  rxFlag = true;
}

String Start_Rx() {
  String rxdata;
  rxFlag = false;
  
  int status = radio.readData(rxdata);
  if (status == RADIOLIB_ERR_NONE) {
    both.printf("Recibiendo [%s]\n", rxdata.c_str());
  }
  
  radio.startReceive(RADIOLIB_SX126X_RX_TIMEOUT_INF);
  return rxdata;
}

void Start_Tx(String data) {
  both.printf("Transmitiendo [%s]\n", data.c_str());
  radio.transmit(data.c_str());
  radio.startReceive(RADIOLIB_SX126X_RX_TIMEOUT_INF);
  rxFlag = false;
}