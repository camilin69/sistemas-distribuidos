/**
* HTTP RECEIVER
*/

#define HELTEC_POWER_BUTTON
#include <heltec_unofficial.h>

// ========== CONFIGURACIÓN LORA ==========
#define FREQUENCY           866.3
#define BANDWIDTH           250.0
#define SPREADING_FACTOR    7
#define TRANSMIT_POWER      14

// ========== VARIABLES GLOBALES ==========
String mensaje_enviado;
volatile bool rxFlag = false;
String llave_proteccion = "7777";
String state = "2";


// ========== CIFRADO XOR MEJORADO ==========
const uint8_t XOR_KEYS[] = {0x2A, 0x4F, 0x31, 0x5C}; // *, O, 1, \ (clave de 4 bytes)

String cifrar_xor(const String& texto) {
  String resultado = texto;
  for (int i = 0; i < resultado.length(); i++) {
    resultado[i] = resultado[i] ^ XOR_KEYS[i % 4];
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
 
 both.println("Recibiendo Http");
}

void loop() {
  heltec_loop();

  static bool lastButtonState = HIGH;     // Estado anterior del botón
  static unsigned long lastButtonTime = 0; // Timestamp del último cambio
  
  bool currentButton = digitalRead(0);     // Leer estado actual del botón PRG (pin 0)

  if (currentButton == LOW && lastButtonState == HIGH && (millis() - lastButtonTime) > 300) {
    state = (state == "1") ? "2" : "1";
    Serial.print(llave_proteccion + "-state-" + state);

    lastButtonTime = millis();            // Guardar timestamp para debounce
  }
  lastButtonState = currentButton;
  if(Serial.available()) {
    mensaje_enviado = Serial.readStringUntil('\n');
    mensaje_enviado.trim();
    
    if (mensaje_enviado.length() > 0) {
      String mensaje_descifrado = descifrar_xor(mensaje_enviado);
      
      if(mensaje_descifrado.startsWith(llave_proteccion + "-")) {
        int i = mensaje_descifrado.indexOf("-");
        both.println(mensaje_descifrado.substring(i + 1));
        delay(2);
      }
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
    both.println("Mensaje cifrado recibido!");
    return rxdata;
  }
  
  radio.startReceive(RADIOLIB_SX126X_RX_TIMEOUT_INF);
  return ""; // Retornar string vacío si no hay datos
}

void Start_Tx(String data) {
  both.println("Transmitiendo mensaje cifrado\n" + data);
  radio.transmit(data.c_str());
  radio.startReceive(RADIOLIB_SX126X_RX_TIMEOUT_INF);
  rxFlag = false;
}